// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/gtao.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "common/breadcrumbs.h"
#include "common/log.h"
#include "common/stats.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer/settings/render_opt.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"

namespace openrac::renderer {
using namespace openrac::gl;

bool Gtao::prepare_program(int scale, bool normals, bool performance, bool hi_z) {
    std::string error;
    m_program = unsigned(scale - 1) + (normals ? 2u : 0u);
    // Hi-Z is specialised independently, so the no-Hi-Z shader retains its original sampling cost.
    const unsigned evaluation = m_program + (hi_z ? kProgramVariants : 0u);
    Shader& kernel = performance ? m_evaluate_performance[evaluation] : m_evaluate[evaluation];
    if (!kernel.id() || !m_composite[m_program].id()) {
        std::string evaluate(shaders::gtao_evaluate_comp);
        evaluate.insert(
            evaluate.find('\n') + 1,
            std::string("#define OPENRAC_GTAO_HIZ ") + (hi_z ? "1\n" : "0\n")
        );
        std::string composite(shaders::gtao_composite_frag);
        evaluate.insert(evaluate.find('\n') + 1, shaders::surface_normal_glsl);
        composite.insert(composite.find('\n') + 1, shaders::surface_normal_glsl);
        evaluate.insert(evaluate.find('\n') + 1, shaders::gtao_common_glsl);
        composite.insert(composite.find('\n') + 1, shaders::gtao_common_glsl);
        const auto definitions = "#define OPENRAC_GTAO_SCALE " + std::to_string(scale)
                                 + "\n#define OPENRAC_SURFACE_NORMALS " + (normals ? "1\n" : "0\n");
        evaluate.insert(evaluate.find('\n') + 1, definitions);
        composite.insert(composite.find('\n') + 1, definitions);
        const auto constants =
            {ShaderIntConstant{"evaluation_scale", scale},
             ShaderIntConstant{"surface_normals_enabled", normals ? 1 : 0}};
        evaluate = shader_constants(std::move(evaluate), constants);
        composite = shader_constants(std::move(composite), constants);
        if (performance) {
            evaluate.insert(evaluate.find('\n') + 1, "#define GTAO_REFINEMENT_STEPS 3\n");
        }
        if ((!kernel.id()
             && !kernel.build_compute(
                 performance ? "GTAO performance horizons" : "GTAO horizons", evaluate, error
             ))
            || (!m_composite[m_program].id()
                && !m_composite[m_program]
                        .build("GTAO filter", shaders::gtao_vert, composite, error))) {
            log::error("GTAO: {}", error);
            m_failed = true;
            return false;
        }
        if (!m_vao) {
            glGenVertexArrays(1, &m_vao);
        }
    }
    return true;
}

bool Gtao::prepare(int width, int height, int scale, bool visibility, bool copy_depth) {
    if (!enhanced_rendering_available() || m_failed || !glDispatchCompute || !glMemoryBarrier) {
        return false;
    }
    std::string error;
    if (copy_depth && (m_depth.width() != width || m_depth.height() != height)
        && !m_depth.create(width, height, error, 1, true)) {
        log::error("GTAO: {}", error);
        m_failed = true;
        return false;
    }
    const int ao_width = (width + scale - 1) / scale;
    const int ao_height = (height + scale - 1) / scale;
    if (m_ao_width != ao_width || m_ao_height != ao_height) {
        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
        const std::size_t cells = std::size_t(ao_width) * ao_height;
        if (cells > std::size_t(limit)) {
            return false;
        }
        if (!m_ao_buffer) {
            glGenBuffers(1, &m_ao_buffer);
            glGenTextures(1, &m_ao_texture);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ao_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, cells * 4, nullptr, GL_DYNAMIC_DRAW);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(world::gl::GL_TEXTURE_BUFFER, m_ao_texture);
        glTexBuffer(world::gl::GL_TEXTURE_BUFFER, GL_RGBA8, m_ao_buffer);
        m_ao_width = ao_width;
        m_ao_height = ao_height;
    }
    if (visibility && (m_visibility.width() != width || m_visibility.height() != height)
        && !m_visibility.create(width, height, error)) {
        log::error("GTAO: {}", error);
        m_failed = true;
        return false;
    }
    return true;
}

bool Gtao::render(
    const FrameInput& input, const RenderTarget& target, bool apply_to_colour, bool evaluate_only
) {
    m_sampled_depth = 0;
    if (!enhanced_rendering_available()) {
        return false;
    }
    // Both game and viewer use this perspective form, including off-centre
    // projections. Menus and orthographic targets have no world AO.
    const Mat4& p = input.camera.projection;
    const auto& options = input.gtao_settings;
    const int scale = options.half_resolution ? 2 : 1;
    if (target.width < 2 || target.height < 2 || p[11] != -1.0f || p[0] == 0 || p[5] == 0
        || options.strength <= 0.0f) {
        return false;
    }
    // Sampling an attachment while drawing back into it would be framebuffer
    // feedback. target.depth_texture is the prepass's own depth, never the colour
    // target's attachment, so the colour composite may sample it too (gtao-direct-depth).
    crash::breadcrumb("GTAO");
    const bool direct_depth =
        target.depth_texture != 0
        && (!apply_to_colour || render_opt::enabled(render_opt::Opt::GtaoDirectDepth));
    const auto scratch =
        direct_depth ? SurfaceDepthScratch{}
                     : lease_surface_depth(target.framebuffer, {target.width, target.height});
    if (!prepare(
            target.width,
            target.height,
            scale,
            !apply_to_colour && !evaluate_only,
            !direct_depth && !scratch
        )
        || !prepare_program(
            scale, target.normal_texture != 0, options.quality != GtaoQuality::Quality, options.hi_z
        )) {
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        glViewport(0, 0, target.width, target.height);
        return false;
    }
    const stats::Scope pass("gtao");
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    m_sampled_depth = direct_depth ? target.depth_texture
                      : scratch    ? scratch.texture
                                   : m_depth.depth_texture();
    if (!direct_depth) {
        const unsigned destination = scratch ? scratch.framebuffer : m_depth.id();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination);
        const stats::Scope depth("GTAO depth copy", stats::kGpu);
        const auto queued = queue_surface_copy(
            target.framebuffer,
            destination,
            {target.width, target.height},
            SurfaceCopy::DepthStencil
        );
        // A failed resolve must reject the chain before horizon or lighting consumers execute.
        if (!queued) {
            throw std::runtime_error(queued.error().message);
        }
        // Immediate recording retains the exact original copy or sample-zero resolve.
        if (!*queued) {
            glBlitFramebuffer(
                0,
                0,
                target.width,
                target.height,
                0,
                0,
                target.width,
                target.height,
                GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                GL_NEAREST
            );
        }
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sampled_depth);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, target.normal_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, m_ao_buffer);
    // Restore shared render state even when graph validation or optional pass recording fails.
    const bool rendered = execute(input, target, apply_to_colour, evaluate_only);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(world::gl::GL_TEXTURE_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return rendered;
}

void Gtao::release() {
    m_sampled_depth = 0;
    m_hi_z.release();
    m_depth.release();
    if (m_ao_texture) {
        glDeleteTextures(1, &m_ao_texture);
    }
    if (m_ao_buffer) {
        glDeleteBuffers(1, &m_ao_buffer);
    }
    m_ao_texture = m_ao_buffer = 0;
    m_ao_width = m_ao_height = 0;
    m_visibility.release();
    for (auto* programs : {&m_evaluate, &m_evaluate_performance}) {
        for (auto& program : *programs) {
            program.release();
        }
    }
    // The filter is shared by both Hi-Z variants and keeps only the scale/normal variants.
    for (auto& program : m_composite) {
        program.release();
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_vao = 0;
    m_failed = false;
}
}  // namespace openrac::renderer
