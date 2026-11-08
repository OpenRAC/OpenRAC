// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compact multi-resolution Gaussian bloom. Compute builds and reconstructs an RGBA8 pyramid;
// one additive fullscreen draw composites the final half-resolution result under the HUD.
#include "enhanced/renderer/bloom.h"

#include <algorithm>
#include <cstdint>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif
#include "renderer_shaders.h"

namespace openrac::enhanced {
using namespace openrac::gl;

namespace {
constexpr float kThreshold = 0.82f;
constexpr float kKnee = 0.18f;
constexpr float kScatter = 0.62f;
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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
    return m_buffer != 0;
}

bool Bloom::copy_output(unsigned texture, int width, int height, std::string& error) {
#if defined(OPENRAC_VULKAN)
    if (vkgl::copy_buffer_to_texture(m_buffer, texture, width, height, true)) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return true;
    }
#else
    (void)texture;
    (void)width;
    (void)height;
#endif
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    error = "bloom compute output copy failed";
    return false;
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

bool Bloom::downsample(
    unsigned source,
    int source_width,
    int source_height,
    Level& destination,
    bool first,
    std::string& error
) {
    if (!ensure_buffer(output_bytes(destination.width, destination.height))) {
        error = "bloom compute buffer allocation failed";
        return false;
    }

    m_downsample.use();
    texture_input(0, source);
    glUniform1i(m_downsample.uniform("tex_T0"), 0);
    glUniform2f(
        m_downsample.uniform("source_size"),
        static_cast<float>(source_width),
        static_cast<float>(source_height)
    );
    glUniform1i(m_downsample.uniform("first_pass"), first ? 1 : 0);
    glUniform1f(m_downsample.uniform("threshold"), kThreshold);
    glUniform1f(m_downsample.uniform("knee"), kKnee);
    glDispatchCompute(
        static_cast<unsigned>((destination.width + kWorkgroup - 1) / kWorkgroup),
        static_cast<unsigned>((destination.height + kWorkgroup - 1) / kWorkgroup),
        1
    );
    return copy_output(destination.colour, destination.width, destination.height, error);
}

bool Bloom::upsample(
    unsigned high, unsigned low, unsigned destination, int width, int height, std::string& error
) {
    if (!ensure_buffer(output_bytes(width, height))) {
        error = "bloom compute buffer allocation failed";
        return false;
    }

    m_upsample.use();
    texture_input(0, high);
    texture_input(1, low);
    glUniform1i(m_upsample.uniform("tex_T0"), 0);
    glUniform1i(m_upsample.uniform("tex_T1"), 1);
    glUniform2f(
        m_upsample.uniform("output_size"), static_cast<float>(width), static_cast<float>(height)
    );
    glUniform1f(m_upsample.uniform("scatter"), kScatter);
    glDispatchCompute(
        static_cast<unsigned>((width + kWorkgroup - 1) / kWorkgroup),
        static_cast<unsigned>((height + kWorkgroup - 1) / kWorkgroup),
        1
    );
    glActiveTexture(GL_TEXTURE0);
    return copy_output(destination, width, height, error);
}

void Bloom::composite(renderer::FrameBuffer& output, float strength) {
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
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
}

void Bloom::apply(renderer::FrameBuffer& output, float strength) {
    if (m_failed || strength <= 0.0f || output.width() <= 1 || output.height() <= 1
        || !enhanced_rendering_available()) {
        return;
    }
#if !defined(OPENRAC_VULKAN)
    (void)output;
    (void)strength;
    return;
#else
    if (!is_vulkan()) {
        return;
    }

    const stats::Scope timing("Bloom", stats::kGpu);
    strength = std::clamp(strength, 0.0f, 1.0f);

    std::string error;
    if (!prepare(output.width(), output.height(), error)) {
        log::error("Bloom disabled: {}", error);
        m_failed = true;
        return;
    }

    // Resolve multisampling once before the compute chain samples the completed world colour.
    output.resolved();
    unsigned source = output.colour_texture();
    int source_width = output.width();
    int source_height = output.height();

    for (int i = 0; i < m_count; ++i) {
        auto& level = m_levels[i];
        if (!downsample(source, source_width, source_height, level, i == 0, error)) {
            log::error("Bloom disabled: {}", error);
            m_failed = true;
            return;
        }
        source = level.colour;
        source_width = level.width;
        source_height = level.height;
    }

    for (int i = m_count - 2; i >= 0; --i) {
        auto& high = m_levels[i];
        const auto& low = m_levels[i + 1];
        if (!upsample(high.colour, low.colour, high.colour, high.width, high.height, error)) {
            log::error("Bloom disabled: {}", error);
            m_failed = true;
            return;
        }
    }

    composite(output, strength);

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
