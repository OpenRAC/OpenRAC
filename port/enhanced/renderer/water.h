// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <filesystem>
#include <memory>

#include "renderer/effects.h"

namespace openrac::scene {
class LevelScene;
}

namespace openrac::enhanced {
// Session-only controls, copied safely between the debug UI and render worker.
struct WaterDebugSettings {
    bool paused = false;
    float current_scale = 1;
    float friction = .12f;
    float outlet_damping = 12;
    float interaction_strength = 1.4f;
    float interaction_radius = .54772256f;
    float projectile_strength = 1;  // Multipliers preserve each weapon's base response.
    float projectile_radius = 1;
    float explosion_strength = 1;
    float explosion_radius = 1;
    float displacement_limit = .25f;
    float fine_normal_strength = 1;
    // Lit (0), simulation (1-4), lighting (5-11), reference reflections (12),
    // foam (13-15), shoreline (16-17).
    int view = 0;
};

WaterDebugSettings water_debug_settings();
void set_water_debug_settings(WaterDebugSettings settings);
void reset_water_simulation();

// True only after a valid water material has initialized its GPU resources.
bool water_available();

// Render-thread-only binding of GPU water surfaces for wetness contact tests.
bool bind_water_contact(const renderer::Shader& shader);

// Render-thread attachment bindings use reserved texture units 18-20.
void prepare_water_deformation(const renderer::FrameInput& input);
bool bind_water_deformation(const renderer::Shader& shader);

// Loads the separately baked, local water asset. Missing/invalid bakes retain original rendering.
std::unique_ptr<renderer::EffectMaterial> make_water(
    scene::LevelScene& scene, const std::filesystem::path& directory
);
}  // namespace openrac::enhanced
