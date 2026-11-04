// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Existing water shader setup and dispatch recording; graph scheduling lives in water_simulation_graph.cpp.
#include <array>
#include <stdexcept>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/renderer/water_graph.h"
#include "enhanced/renderer/water_internal.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"
#include "scene/level_renderer.h"

namespace openrac::enhanced::water_detail {
using namespace gl;
namespace wgl = renderer::world::gl;

void Water::update_lighting(const renderer::FrameInput& input) {
    m_light.use();
    glUniform1i(m_light.uniform("probe_region_masks"), 15);
    m_scene.bind_surface_lighting(m_light, input.camera);
    glUniform1i(m_light.uniform("water_coast"), 12);
    glUniform1i(m_light.uniform("water_bodies"), 11);
    glUniform1i(m_light.uniform("water_body_count"), int(m_bodies.size() / 5));
    glUniform1i(m_light.uniform("water_light_count"), int(m_light_count));
    glActiveTexture(GL_TEXTURE0 + 12);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[kCoastTexture]);
    glActiveTexture(GL_TEXTURE0 + 11);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[kBodyTexture]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_buffers[kLighting]);
    const std::array uses{WaterBufferUse{
        m_buffers[kLighting], m_light_count * sizeof(Texel), renderer::graph::Access::Write
    }};
    // Inputs are captured after surface lighting has bound the current probe/shadow resources.
    const auto result = water_compute_pass("water lighting cache", uses, [&] {
        glDispatchCompute((m_light_count + 63) / 64, 1, 1);
    });

    // Incomplete water outputs cannot be published to later shader consumers.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    glActiveTexture(GL_TEXTURE0);
}

void Water::prepare_window(const SimulationUpdate& update) {
    m_window_prepare.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffers[kStateA]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffers[kStateB]);
    glUniform1i(m_window_prepare.uniform("initialize_history"), update.reset ? 1 : 0);
    glUniform2f(
        m_window_prepare.uniform("previous_window"),
        float(m_cached_origin[0]),
        float(m_cached_origin[1])
    );
    glUniform1f(m_window_prepare.uniform("water_current_scale"), m_debug.current_scale);
    glUniform1i(m_window_prepare.uniform("water_body_count"), int(m_bodies.size() / 5));
    glUniform1f(m_window_prepare.uniform("water_sim_height"), m_sim_height);
    glUniform1f(m_window_prepare.uniform("ocean_depth"), m_ocean_spectra[0] ? m_ocean_depth : 0.f);
    glUniform1i(m_window_prepare.uniform("ocean_dry_count"), m_ocean_dry_count);
    // The open ocean accepts optional exclusion rectangles from the level profile.
    if (m_ocean_dry_count) {
        glUniform4fv(
            m_window_prepare.uniform("ocean_dry[0]"),
            m_ocean_dry_count,
            m_ocean_dry_regions[0].data()
        );
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffers[kCoast]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffers[kBodies]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_buffers[kHydraulics]);
    glUniform4f(
        m_window_prepare.uniform("window_config"),
        float(update.window[0]),
        float(update.window[1]),
        float(kWindowSize),
        float(m_sim_body)
    );
    glDispatchCompute((kWindowCells + 63) / 64, 1, 1);
    m_cached_body = m_sim_body;
    m_cached_height = m_sim_height;
    m_cached_origin = update.window;
}

void Water::configure_wave_steps(const SimulationUpdate& update) {
    m_step.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffers[kCoast]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffers[kBodies]);
    glUniform4f(
        m_step.uniform("water_physics"),
        m_debug.current_scale,
        m_debug.friction,
        m_debug.outlet_damping,
        m_debug.interaction_radius * m_debug.interaction_radius
    );
    glUniform1f(m_step.uniform("displacement_limit"), m_debug.displacement_limit);
    glUniform1i(m_step.uniform("transport_reference"), 0);
    glUniform1f(m_step.uniform("wake_strength"), .9f);
    glUniform1i(m_step.uniform("cell_count"), int(kWindowCells));
    glUniform1i(m_step.uniform("local_window"), 1);
    glUniform1f(m_step.uniform("water_dt"), 1.f / 240.f);
    glUniform4f(
        m_step.uniform("window_config"),
        float(update.window[0]),
        float(update.window[1]),
        float(kWindowSize),
        float(m_sim_body)
    );
    glUniform2f(
        m_step.uniform("previous_window"), float(m_window_origin[0]), float(m_window_origin[1])
    );
    glUniform1i(m_step.uniform("atlas_width"), int(m_atlas_width));
    glUniform4fv(m_step.uniform("water_atlas[0]"), 64, m_atlas[0].data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_buffers[kRenderPixels]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_buffers[kHydraulics]);
    configure_wave_interactions(update);
}

void Water::configure_wave_interactions(const SimulationUpdate& update) {
    glUniform3f(m_step.uniform("previous_hero"), m_hero[0], m_hero[1], m_hero[2]);

    // Preserve the original hand ordering for both previous and current contact parameters.
    for (unsigned hand = 0; hand < update.hand_impulses.size(); ++hand) {
        const auto point = m_hands[hand].value_or(renderer::Vec3{});
        glUniform3f(
            m_step.uniform(hand == 0 ? "previous_hands[0]" : "previous_hands[1]"),
            point[0],
            point[1],
            point[2]
        );
    }

    // Empty impulse spans must not supply an invalid first-element upload pointer.
    if (!update.impulse_starts.empty()) {
        glUniform4fv(
            m_step.uniform("impulse_starts[0]"),
            int(update.impulse_starts.size()),
            update.impulse_starts[0].data()
        );
        glUniform4fv(
            m_step.uniform("impulse_ends[0]"),
            int(update.impulse_starts.size()),
            update.impulse_ends[0].data()
        );
    }
}

void Water::record_wave_step(const SimulationUpdate& update, unsigned step) {
    m_step.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffers[m_current]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffers[1 - m_current]);
    glUniform1i(m_step.uniform("reset_waves"), update.reset && step == 0 ? 1 : 0);
    // A fresh window clears in substep zero; kick it after that clear, exactly once.
    glUniform1i(
        m_step.uniform("impulse_count"),
        step == (update.reset ? 1u : 0u) ? int(update.impulse_starts.size()) : 0
    );
    glUniform4f(
        m_step.uniform("hero"),
        update.hero[0],
        update.hero[1],
        update.hero[2],
        step == 0 ? update.impulse : 0.f
    );
    glUniform1i(
        m_step.uniform("hand_count"),
        step == 0 && (update.hand_impulses[0] > 0 || update.hand_impulses[1] > 0) ? 2 : 0
    );

    // Preserve the original hand ordering for both previous and current contact parameters.
    for (unsigned hand = 0; hand < update.hand_impulses.size(); ++hand) {
        const auto point = update.input.water_hands[hand].value_or(renderer::Vec3{});
        glUniform4f(
            m_step.uniform(hand == 0 ? "hands[0]" : "hands[1]"),
            point[0],
            point[1],
            point[2],
            update.hand_impulses[hand]
        );
    }

    glDispatchCompute((kWindowCells + 63) / 64, 1, 1);
    glUniform2f(
        m_step.uniform("previous_window"), float(update.window[0]), float(update.window[1])
    );
    m_current = 1 - m_current;
}

bool Water::simulate(const SimulationUpdate& update) {
    const stats::Scope timer("water simulation", stats::kGpu);
    constexpr std::uint64_t kWindowBytes = kWindowCells * sizeof(Texel);
    const std::array<std::uint64_t, kBufferCount> bytes{
        kWindowBytes,
        kWindowBytes,
        m_coast.size() * sizeof(Texel),
        m_bodies.size() * sizeof(Texel),
        0,
        kWindowBytes,
        m_hydraulics.size() * sizeof(Texel),
        kWindowBytes,
        kWindowBytes
    };

    // Importable storage must be stable before graph compilation, not allocated lazily inside callbacks.
    for (unsigned index = 0; index < kBufferCount; ++index) {
        // Unused lighting storage belongs to its separate compute pass.
        if (bytes[index]) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_buffers[index]);
        }
    }

    const bool window_changed = m_cached_body != m_sim_body || m_cached_origin != update.window
                                || m_cached_height != m_sim_height;
    configure_wave_steps(update);
    const SimulationResources resources{m_buffers, bytes, m_maps[0], kWindowSize};
    const SimulationPlan plan{m_current, update.steps * 4, window_changed, update.reset};
    // All callbacks finish before the borrowed frame input and impulse spans leave prepare_frame().
    const auto result =
        water_simulation_passes(resources, plan, [&](SimulationPhase phase, unsigned index) {
            // Window preparation must precede the first shader read of its new metadata.
            if (phase == SimulationPhase::Window) {
                prepare_window(update);
            } else {
                record_wave_step(update, index);
            }
        });

    // Incomplete water outputs cannot be published to later shader consumers.
    if (!result) {
        log::warn("water simulation unavailable: {}", result.error().message);
        return false;
    }

    return true;
}
}  // namespace openrac::enhanced::water_detail
