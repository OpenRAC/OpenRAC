// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Gaussian reduction into RGBA8 targets, then additive reconstruction and composition.
#include "enhanced/renderer/bloom.h"

#include <algorithm>
#include <utility>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/surface_chain.h"
#include "renderer/vulkan/temporal_graph.h"
#endif
#include "renderer_shaders.h"

namespace openrac::enhanced {
using namespace openrac::gl;

Bloom::~Bloom() {
    release();
}

void Bloom::release() {
    for (auto& level : m_levels) {
        level.release();
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_vao = 0;
    m_count = 0;
    m_downsample.release();
    m_upsample.release();
    m_composite.release();
    m_failed = false;
}

#if defined(OPENRAC_VULKAN)
bool Bloom::prepare(int width, int height, std::string& error) {
    if (!m_downsample.id()
        && (!m_downsample.build(
                "bloom gaussian downsample",
                renderer::shaders::gtao_vert,
                renderer::shaders::bloom_downsample_frag,
                error
            )
            || !m_upsample.build(
                "bloom upsample",
                renderer::shaders::gtao_vert,
                renderer::shaders::bloom_upsample_frag,
                error
            )
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
    if (!m_vao) {
        error = "bloom vertex array allocation failed";
        return false;
    }

    m_count = 0;
    for (auto& level : m_levels) {
        width = std::max(1, (width + 1) / 2);
        height = std::max(1, (height + 1) / 2);
        if ((level.width() != width || level.height() != height)
            && !level.create_colour(width, height, error)) {
            return false;
        }
        ++m_count;
        if (width == 1 && height == 1) {
            break;
        }
    }

    for (int i = m_count; i < kMaxLevels; ++i) {
        m_levels[i].release();
    }
    return true;
}

Result<void> Bloom::draw(
    std::string_view name, renderer::FrameBuffer& output, unsigned source, bool additive
) {
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    glViewport(0, 0, output.width(), output.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    if (additive) {
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, additive ? GL_FALSE : GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    glBindVertexArray(m_vao);

    const vkgl::TemporalTarget target{
        output.id(),
        additive ? vkgl::TemporalContents::Preserve : vkgl::TemporalContents::Overwrite,
        renderer::graph::Aspect::Colour,
        false,
        vkgl::TemporalConsumer::Attachment
    };
    const std::array inputs{source};
    return vkgl::temporal_raster(name, target, inputs, [] { glDrawArrays(GL_TRIANGLES, 0, 3); });
}

Result<void> Bloom::draw_pyramid(
    renderer::FrameBuffer& output, const renderer::BloomSettings& settings
) {
    const auto* source = &output;
    for (int i = 0; i < m_count; ++i) {
        auto& level = m_levels[i];
        m_downsample.use();
        glUniform1i(m_downsample.uniform("tex_T0"), 0);
        glUniform1i(m_downsample.uniform("first_pass"), i == 0 ? 1 : 0);
        glUniform1f(m_downsample.uniform("threshold"), settings.threshold);
        glUniform1f(m_downsample.uniform("knee"), settings.knee);
        const auto reduced = draw("bloom downsample", level, source->colour_texture(), false);
        if (!reduced) {
            return unexpected(reduced.error());
        }
        source = &level;
    }

    // Add the low level directly to the higher attachment through blending.
    for (int i = m_count - 2; i >= 0; --i) {
        auto& high = m_levels[i];
        m_upsample.use();
        glUniform1i(m_upsample.uniform("tex_T0"), 0);
        glUniform2f(m_upsample.uniform("output_size"), float(high.width()), float(high.height()));
        glUniform1f(m_upsample.uniform("scatter"), settings.scatter);
        const auto spread = draw("bloom upsample", high, m_levels[i + 1].colour_texture(), true);
        if (!spread) {
            return unexpected(spread.error());
        }
    }

    m_composite.use();
    glUniform1i(m_composite.uniform("tex_T0"), 0);
    glUniform1f(m_composite.uniform("strength"), settings.strength);
    glUniform2f(m_composite.uniform("output_size"), float(output.width()), float(output.height()));
    return draw("bloom composite", output, m_levels[0].colour_texture(), true);
}
#endif

void Bloom::apply(renderer::FrameBuffer& output, const renderer::BloomSettings& settings) {
#if defined(OPENRAC_VULKAN)
    if (m_failed || !settings.enabled || settings.strength <= 0.0f || output.width() <= 1
        || output.height() <= 1 || !enhanced_rendering_available() || !is_vulkan()) {
        return;
    }

    // Complete world colour before resolving MSAA and running the bloom passes as one graph.
    Result<void> rendered;
    const auto setup = renderer::run_surface_setup([&] {
        const stats::Scope timing("Bloom", stats::kGpu);
        std::string error;
        if (!prepare(output.width(), output.height(), error)) {
            rendered = unexpected(Error{std::move(error)});
            return;
        }

        output.resolved();
        const auto recorded =
            renderer::run_surface_chain([&] { rendered = draw_pyramid(output, settings); });
        glBindVertexArray(0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glActiveTexture(GL_TEXTURE0);
        if (!recorded) {
            rendered = unexpected(recorded.error());
        }
    });
    if (!setup || !rendered) {
        log::error("Bloom disabled: {}", setup ? rendered.error().message : setup.error().message);
        m_failed = true;
        return;
    }

    double texels = 0;
    for (int i = 0; i < m_count; ++i) {
        texels += double(m_levels[i].width()) * m_levels[i].height();
    }
    stats::gauge("Bloom levels", m_count);
    stats::gauge("Bloom working MiB", texels * 4 / (1024 * 1024));
#else
    (void)output;
    (void)settings;
#endif
}

}  // namespace openrac::enhanced
