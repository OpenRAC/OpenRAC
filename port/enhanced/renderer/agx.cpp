// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements AgxPreview with an in-place compute transform and a separate-source raster fallback.
#include "enhanced/renderer/agx.h"

#include <algorithm>
#include <array>

#include "common/log.h"
#include "common/result.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/temporal_graph.h"
#endif
#include "renderer_shaders.h"

namespace openrac::enhanced {
using namespace openrac::gl;

namespace {
// Exposure is in EV; both paths retain the shader's existing supported interval.
constexpr float kExposureLimit = 4.8f;

// Restores fullscreen state shared by tone mapping and the following native-resolution HUD.
void fullscreen_target(const renderer::FrameBuffer& output) {
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    glViewport(0, 0, output.width(), output.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

#if defined(OPENRAC_VULKAN)
// Selects compute only when the existing output already supports storage, without reallocating it.
bool direct_transform(renderer::FrameBuffer& output, float exposure) {
    // Multisampled or compatibility targets retain their separate-source raster path.
    if (!is_vulkan() || output.samples() != 1
        || !renderer::render_opt::enabled(renderer::render_opt::Opt::AgxDirectOutput)
        || !vkgl::storage_texture(output.colour_texture())) {
        return false;
    }

    // The context uses this immutable shader variant for every exposure update.
    static const std::string compute = [] {
        std::string source(renderer::shaders::agx_frag);
        source.replace(0, source.find('\n'), "#version 450\n#define OPENRAC_AGX_COMPUTE 1");
        return source;
    }();
    return vkgl::transform_colour(output.colour_texture(), compute, exposure);
}
#endif

// Copies complete world colour before the fallback samples it, including an MSAA resolve.
Result<void> snapshot(const renderer::FrameBuffer& output, unsigned destination) {
    // The copy records synchronously on the render thread and retains its original filter.
    const auto copy = [&output, destination] {
        output.blit_to(destination, output.width(), output.height());
    };
#if defined(OPENRAC_VULKAN)
    // Compatibility mode never imports Vulkan images or changes its original copy behavior.
    if (is_vulkan()) {
        return vkgl::temporal_blit(
            "AgX colour snapshot", output.id(), destination, vkgl::TemporalConsumer::Shader, copy
        );
    }
#endif
    copy();
    return {};
}

// Declares the complete tone-mapped output while keeping source colour separate from its attachment.
Result<void> raster_transform(
    [[maybe_unused]] const renderer::FrameBuffer& output, [[maybe_unused]] unsigned source
) {
#if defined(OPENRAC_VULKAN)
    // The original OpenGL draw remains outside the optional Vulkan graph.
    if (is_vulkan()) {
        const std::array inputs{source};
        const vkgl::TemporalTarget target{
            output.id(),
            vkgl::TemporalContents::Overwrite,
            renderer::graph::Aspect::Colour,
            false,
            vkgl::TemporalConsumer::Attachment
        };
        // The fullscreen shader writes every texel, including alpha, before HUD drawing resumes.
        return vkgl::temporal_raster("AgX tone mapping", target, inputs, [] {
            glDrawArrays(GL_TRIANGLES, 0, 3);
        });
    }
#endif
    glDrawArrays(GL_TRIANGLES, 0, 3);
    return {};
}
}  // namespace

AgxPreview::~AgxPreview() {
    release();
}

void AgxPreview::release() {
    m_copy.release();
    m_shader.release();
    // The VAO belongs to this preview's graphics context.
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_vao = 0;
    m_failed = false;
}

bool AgxPreview::prepare(const renderer::FrameBuffer& output, std::string& error) {
    // The fallback samples an independent image, recreated before any graph captures its allocation.
    if ((!m_shader.id()
         && !m_shader
                 .build("AgX", renderer::shaders::gtao_vert, renderer::shaders::agx_frag, error))
        || ((m_copy.width() != output.width() || m_copy.height() != output.height())
            && !m_copy.create_colour(output.width(), output.height(), error))) {
        return false;
    }

    // Fullscreen raster work needs no vertex attributes.
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
    }
    return true;
}

void AgxPreview::apply(renderer::FrameBuffer& output, float exposure) {
    // Missing targets and disabled enhanced rendering retain their unmodified world colour.
    if (m_failed || !enhanced_rendering_available() || output.width() <= 0
        || output.height() <= 0) {
        return;
    }
    const stats::Scope timing("AgX", stats::kGpu);
    exposure = std::clamp(exposure, -kExposureLimit, kExposureLimit);
#if defined(OPENRAC_VULKAN)
    // The in-place path avoids a snapshot but restores the raster fallback's HUD postconditions.
    if (direct_transform(output, exposure)) {
        fullscreen_target(output);
        stats::gauge("AgX image copies", 0);
        return;
    }
#endif
    std::string error;
    // Resource preparation must finish before importing any fallback attachments.
    if (!prepare(output, error)) {
        log::error("AgX disabled: {}", error);
        m_failed = true;
        return;
    }
    stats::gauge("AgX image copies", 1);
    const auto copied = snapshot(output, m_copy.id());
    // Rejected source contents cannot feed the tone-mapping shader.
    if (!copied) {
        log::error("AgX disabled: {}", copied.error().message);
        m_failed = true;
        return;
    }
    fullscreen_target(output);
    glDepthMask(GL_FALSE);
    glBindVertexArray(m_vao);
    m_shader.use();
    glUniform1f(m_shader.uniform("exposure_ev"), exposure);
    glBindTexture(GL_TEXTURE_2D, m_copy.colour_texture());
    const auto drawn = raster_transform(output, m_copy.colour_texture());
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    // A failed recording cannot be retried as a completed tone-mapped frame.
    if (!drawn) {
        log::error("AgX disabled: {}", drawn.error().message);
        m_failed = true;
    }
}
}  // namespace openrac::enhanced
