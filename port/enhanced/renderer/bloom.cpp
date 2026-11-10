// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compact multi-resolution Gaussian bloom. Compute builds and reconstructs an RGBA8 pyramid;
// one additive fullscreen draw composites the final half-resolution result under the HUD.
#include "enhanced/renderer/bloom.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/surface_chain.h"
#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/temporal_graph.h"
#endif
#include "renderer_shaders.h"

namespace openrac::enhanced {
using namespace openrac::gl;

namespace {
constexpr int kWorkgroup = 8;

std::size_t output_bytes(int width, int height) {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height)
           * sizeof(std::uint32_t);
}

void texture_input(unsigned unit, unsigned texture) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}
}  // namespace

Bloom::~Bloom() {
    release();
}

void Bloom::release_level(Level& level) {
    if (level.colour) {
        glDeleteTextures(1, &level.colour);
    }
    level = {};
}

void Bloom::release() {
    for (auto& level : m_levels) {
        release_level(level);
    }
    if (m_buffer) {
        glDeleteBuffers(1, &m_buffer);
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_buffer = 0;
    m_buffer_capacity = 0;
    m_vao = 0;
    m_count = 0;
    m_downsample.release();
    m_upsample.release();
    m_composite.release();
    m_failed = false;
}

void Bloom::allocate_texture(Level& level, int width, int height) {
    if (!level.colour) {
        glGenTextures(1, &level.colour);
    }
    glBindTexture(GL_TEXTURE_2D, level.colour);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    level.width = width;
    level.height = height;
}

bool Bloom::ensure_buffer(std::size_t bytes) {
    if (!m_buffer) {
        glGenBuffers(1, &m_buffer);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
    if (bytes > m_buffer_capacity) {
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(bytes), nullptr, GL_DYNAMIC_DRAW
        );
        m_buffer_capacity = bytes;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return m_buffer != 0;
}

bool Bloom::prepare(int width, int height, std::string& error) {
#if !defined(OPENRAC_VULKAN)
    (void)width;
    (void)height;
    error = "bloom requires the Vulkan enhanced renderer";
    return false;
#else
    if (!is_vulkan() || !enhanced_rendering_available()) {
        error = "bloom requires the Vulkan enhanced renderer";
        return false;
    }

    if (!m_downsample.id()
        && (!m_downsample.build_compute(
                "bloom gaussian downsample", renderer::shaders::bloom_downsample_comp, error
            )
            || !m_upsample
                    .build_compute("bloom upsample", renderer::shaders::bloom_upsample_comp, error)
            || !m_composite.build(
                "bloom composite",
                renderer::shaders::gtao_vert,
                renderer::shaders::bloom_composite_frag,
                error
            ))) {
        return false;
    }

    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
    }

    int next_width = width;
    int next_height = height;
    int count = 0;
    for (; count < kMaxLevels; ++count) {
        next_width = std::max(1, (next_width + 1) / 2);
        next_height = std::max(1, (next_height + 1) / 2);
        auto& level = m_levels[count];

        if (level.width != next_width || level.height != next_height || !level.colour) {
            allocate_texture(level, next_width, next_height);
        }

        if (next_width == 1 && next_height == 1) {
            ++count;
            break;
        }
    }

    for (int i = count; i < kMaxLevels; ++i) {
        release_level(m_levels[i]);
    }

    m_count = count;
    glActiveTexture(GL_TEXTURE0);
    return m_count > 0 && m_vao != 0;
#endif
}

void Bloom::downsample(
    const Level& source,
    const Level& destination,
    bool first,
    const renderer::BloomSettings& settings
) {
    m_downsample.use();
    texture_input(0, source.colour);
    glUniform1i(m_downsample.uniform("tex_T0"), 0);
    glUniform2f(
        m_downsample.uniform("source_size"),
        static_cast<float>(source.width),
        static_cast<float>(source.height)
    );
    glUniform1i(m_downsample.uniform("first_pass"), first ? 1 : 0);
    glUniform1f(m_downsample.uniform("threshold"), settings.threshold);
    glUniform1f(m_downsample.uniform("knee"), settings.knee);
    glDispatchCompute(
        static_cast<unsigned>((destination.width + kWorkgroup - 1) / kWorkgroup),
        static_cast<unsigned>((destination.height + kWorkgroup - 1) / kWorkgroup),
        1
    );
}

void Bloom::upsample(const Level& high, const Level& low, float scatter) {
    m_upsample.use();
    texture_input(0, high.colour);
    texture_input(1, low.colour);
    glUniform1i(m_upsample.uniform("tex_T0"), 0);
    glUniform1i(m_upsample.uniform("tex_T1"), 1);
    glUniform2f(m_upsample.uniform("output_size"), float(high.width), float(high.height));
    glUniform1f(m_upsample.uniform("scatter"), scatter);
    glDispatchCompute(
        static_cast<unsigned>((high.width + kWorkgroup - 1) / kWorkgroup),
        static_cast<unsigned>((high.height + kWorkgroup - 1) / kWorkgroup),
        1
    );
    glActiveTexture(GL_TEXTURE0);
}

#if defined(OPENRAC_VULKAN)
Result<void> Bloom::build_pyramid(const Level& source, const renderer::BloomSettings& settings) {
    namespace graph = renderer::graph;
    vkgl::GraphExecution execution;
    const auto world = execution.import_image("bloom world colour", source.colour);
    if (!world) {
        return unexpected(world.error());
    }

    // Mark the buffer as storage before importing its stable Vulkan allocation.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
    const auto scratch =
        execution.import_buffer("bloom scratch", m_buffer, vkgl::InitialContents::Discard);
    if (!scratch) {
        return unexpected(scratch.error());
    }

    std::array<graph::ImageHandle, kMaxLevels> levels;
    std::array<graph::BufferUse, kMaxLevels> copies;
    for (int i = 0; i < m_count; ++i) {
        const auto image = execution.import_image(
            "bloom pyramid level", m_levels[i].colour, vkgl::InitialContents::Discard
        );
        if (!image) {
            return unexpected(image.error());
        }
        levels[i] = *image;
        copies[i] =
            {*scratch,
             {0, output_bytes(m_levels[i].width, m_levels[i].height)},
             {graph::Stage::Transfer, graph::Access::Read, graph::Layout::Buffer}};
    }

    const auto sampled = [](graph::ImageHandle image) -> graph::ImageUse {
        return {
            image, {}, {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Sampled}
        };
    };
    const auto add_step = [&](graph::Pass pass, int level) {
        const auto& copy = copies[level];
        pass.buffers.push_back(
            {*scratch,
             copy.range,
             {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}
        );
        execution.add_pass(std::move(pass));
        execution.add_pass(
            {"bloom copy",
             {{levels[level],
               {},
               {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}},
             {copy}}
        );
    };

    for (int i = 0; i < m_count; ++i) {
        add_step({"bloom downsample", {sampled(i == 0 ? *world : levels[i - 1])}}, i);
    }
    for (int i = m_count - 2; i >= 0; --i) {
        add_step({"bloom upsample", {sampled(levels[i]), sampled(levels[i + 1])}}, i);
    }

    // Each step dispatches first, then copies the scratch output before another dispatch reuses it.
    return execution.run([&](unsigned pass) {
        const auto step = static_cast<int>(pass / 2);
        const bool reduce = step < m_count;
        const int level = reduce ? step : 2 * m_count - 2 - step;
        if (pass % 2 != 0) {
            const auto copied = execution.copy_buffer_to_image(copies[level], levels[level]);
            if (!copied) {
                throw std::runtime_error(copied.error().message);
            }
        } else if (reduce) {
            downsample(
                level == 0 ? source : m_levels[level - 1], m_levels[level], level == 0, settings
            );
        } else {
            upsample(m_levels[level], m_levels[level + 1], settings.scatter);
        }
    });
}

Result<void> Bloom::composite(renderer::FrameBuffer& output, float strength) {
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    glViewport(0, 0, output.width(), output.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    m_composite.use();
    glUniform1f(m_composite.uniform("strength"), strength);
    glUniform2f(
        m_composite.uniform("output_size"),
        static_cast<float>(output.width()),
        static_cast<float>(output.height())
    );
    texture_input(0, m_levels[0].colour);
    glUniform1i(m_composite.uniform("tex_T0"), 0);
    glBindVertexArray(m_vao);
    const std::array inputs{m_levels[0].colour};
    const vkgl::TemporalTarget target{
        output.id(),
        vkgl::TemporalContents::Preserve,
        renderer::graph::Aspect::Colour,
        false,
        vkgl::TemporalConsumer::Attachment
    };
    const auto result = vkgl::temporal_raster("bloom composite", target, inputs, [] {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    });

    glBindVertexArray(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    return result;
}
#endif

void Bloom::apply(renderer::FrameBuffer& output, const renderer::BloomSettings& settings) {
    if (m_failed || !settings.enabled || settings.strength <= 0.0f || output.width() <= 1
        || output.height() <= 1 || !enhanced_rendering_available()) {
        return;
    }
#if !defined(OPENRAC_VULKAN)
    (void)output;
    return;
#else
    if (!is_vulkan()) {
        return;
    }

    // Complete the world producers before running the graph's synchronous buffer-to-image copies.
    Result<void> rendered;
    const auto setup = renderer::run_surface_setup([&] {
        const stats::Scope timing("Bloom", stats::kGpu);
        std::string error;
        if (!prepare(output.width(), output.height(), error)) {
            rendered = unexpected(Error{std::move(error)});
            return;
        }
        if (!ensure_buffer(output_bytes(m_levels[0].width, m_levels[0].height))) {
            rendered = unexpected(Error{"bloom compute buffer allocation failed"});
            return;
        }

        output.resolved();
        rendered =
            build_pyramid({output.colour_texture(), output.width(), output.height()}, settings);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        if (rendered) {
            rendered = composite(output, settings.strength);
        }
    });
    if (!setup || !rendered) {
        log::error("Bloom disabled: {}", setup ? rendered.error().message : setup.error().message);
        m_failed = true;
        return;
    }

    std::size_t texture_bytes = 0;
    for (int i = 0; i < m_count; ++i) {
        texture_bytes += output_bytes(m_levels[i].width, m_levels[i].height);
    }
    stats::gauge("Bloom levels", m_count);
    stats::gauge(
        "Bloom working MiB",
        static_cast<double>(texture_bytes + m_buffer_capacity) / (1024.0 * 1024.0)
    );
#endif
}

}  // namespace openrac::enhanced
