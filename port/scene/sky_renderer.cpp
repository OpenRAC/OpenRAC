// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// LevelScene's sky phase: current shadows, atmosphere transport and preserved sky colour in one scope.
#include <stdexcept>

#include "common/stats.h"
#include "renderer/colour_graph.h"
#include "renderer/gl.h"
#include "renderer/surface_chain.h"
#include "scene/level_lighting.h"
#include "scene/level_renderer.h"
#include "scene/level_renderer_internal.h"

namespace openrac::scene {
using namespace gl;
using namespace render_detail;

namespace {
// Returns the authored affine shell transform, or identity without a supplied override.
renderer::Mat4 sky_shell_matrix(const renderer::FrameInput& input, int shell) {
    renderer::Mat4 result = renderer::identity();
    // Negative and absent slots retain exported geometry without a live transform.
    if (shell < 0 || shell >= static_cast<int>(input.sky_shells.size())
        || (input.sky_shell_mask & (1u << shell)) == 0) {
        return result;
    }
    // The shell's matrix as the game drew it this tick; its translation is in the game's
    // sky units, the vertices were exported in 1/1024ths of them (editor/sky.py).
    result = input.sky_shells[static_cast<std::size_t>(shell)];
    result[3] = result[7] = result[11] = 0.0f;
    result[12] /= 1024.0f;
    result[13] /= 1024.0f;
    result[14] /= 1024.0f;
    result[15] = 1.0f;
    return result;
}
}  // namespace

bool LevelScene::can_prepare_atmosphere(const renderer::FrameInput& input) const {
    return m_lighting && m_lighting->prepares_atmosphere_chain()
           && m_lighting->atmosphere_wanted(input) && !input.keep_colour
           && renderer::surface_chain_enabled() && renderer::colour_graph_enabled();
}

void LevelScene::prepare_atmosphere_inputs(
    const renderer::FrameInput& input, renderer::RenderState& state
) {
    // The first sky or terrain consumer prepares transport once for this displayed frame.
    if (!m_lighting || state.depth_prepass || state.atmosphere_prepared) {
        return;
    }
    // Atmosphere samples the same current-frame shadow maps as later early lighting.
    if (m_lighting->atmosphere_wanted(input) && !state.world_shadows_prepared) {
        prepare_shadows(input, state);
        state.world_shadows_prepared = true;
    }
    m_lighting->prepare_atmosphere(*this, input, state.target, m_fog_colour, m_fog_params);
    state.atmosphere_prepared = true;
}

void LevelScene::prepare_atmosphere(
    const renderer::FrameInput& input, renderer::RenderState& state
) {
    // Missing-sky levels still compose shadow and transport preparation before terrain colour.
    if (can_prepare_atmosphere(input) && !renderer::collecting_surface_chain()) {
        const auto result = renderer::run_surface_chain([this, &input, &state] {
            prepare_atmosphere_inputs(input, state);
        });
        // Terrain cannot sample a transport buffer whose shared plan was rejected.
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
        return;
    }
    prepare_atmosphere_inputs(input, state);
}

void LevelScene::record_sky(const renderer::FrameInput& input, renderer::RenderState& state) {
    prepare_atmosphere_inputs(input, state);
    const stats::Scope timing("sky colour", stats::kGpu);
    // Allocation/preparation callbacks may leave a shadow target bound instead of world colour.
    glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
    glViewport(0, 0, state.target.width, state.target.height);
    // Captures remain valid until the enclosing scoped plan records all sky consumers.
    if (renderer::collecting_surface_chain()) {
        const auto result = renderer::colour_pass(
            "authored sky",
            state.target,
            [this, &input, &state] { draw_sky(input, state); },
            renderer::ColourAttachments::Colour
        );
        // A rejected colour phase cannot silently leave part of the sky unrendered.
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
    } else {
        draw_sky(input, state);
    }
    // The fog overlay reads transport after the authored sky has preserved the background.
    if (m_lighting) {
        m_lighting->draw_atmosphere_sky(state.target);
    }
}

void LevelScene::render_sky(const renderer::FrameInput& input, renderer::RenderState& state) {
    // Unknown providers and reference options retain immediate preparation and draw ordering.
    if (!can_prepare_atmosphere(input) || renderer::collecting_surface_chain()) {
        record_sky(input, state);
        return;
    }
    const auto result =
        renderer::run_surface_chain([this, &input, &state] { record_sky(input, state); });
    // Compilation precedes every deferred shadow, transport and sky command.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

void LevelScene::draw_sky(const renderer::FrameInput& input, renderer::RenderState& state) {
    // The shells sit around the camera and never move with it: the view's
    // rotation only. Each is blended over the last, without depth.
    renderer::Mat4 rotation = input.camera.view;
    rotation[12] = rotation[13] = rotation[14] = 0.0f;
    const renderer::Mat4 view_projection = renderer::multiply(input.camera.projection, rotation);
    m_sky.use();
    glUniformMatrix4fv(m_sky.uniform("view_projection"), 1, GL_FALSE, view_projection.data());
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glActiveTexture(GL_TEXTURE0);
    const int tint_uniform = m_sky.uniform("sky_tint");
    // Shell models retain their exported instance and blend order.
    for (const Group& group : m_groups[static_cast<std::size_t>(Layer::Sky)]) {
        glBindVertexArray(group.vao);
        // Each primitive can select its own texture and live shell transform.
        for (const Primitive& primitive : m_models[group.model].primitives) {
            const Material& material = m_materials[static_cast<std::size_t>(primitive.material)];
            // Untextured shells use the existing white texture.
            const renderer::TextureHandle texture =
                material.image >= 0 ? m_textures[static_cast<std::size_t>(material.image)]
                                    : m_white;
            state.textures.bind_upscaled_texture_colours(
                texture, m_sky, input.texture_filter, !material.clamp_s, !material.clamp_t
            );
            // Material tint changes only the RGB of this shell, preserving its alpha.
            const auto tint = m_lighting ? m_lighting->sky_tint(group.model, primitive.material)
                                         : std::array<float, 3>{1, 1, 1};
            glUniform3f(tint_uniform, tint[0], tint[1], tint[2]);
            const auto shell = sky_shell_matrix(input, primitive.shell);
            glUniformMatrix4fv(m_sky.uniform("shell"), 1, GL_FALSE, shell.data());
            glDrawElementsInstanced(
                GL_TRIANGLES,
                static_cast<GLsizei>(primitive.index_count),
                GL_UNSIGNED_INT,
                offset(primitive.first_index * sizeof(std::uint32_t)),
                group.count
            );
            ++state.stats.draw_calls;
            state.stats.triangles += primitive.index_count / 3;
        }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

}  // namespace openrac::scene
