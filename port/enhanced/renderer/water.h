// SPDX-License-Identifier: GPL-3.0-or-later
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
    float displacement_limit = .25f;
    float fine_normal_strength = 1;
    int view = 0;  // Lit, height, velocity, banks, flow detail.
};

WaterDebugSettings water_debug_settings();
void set_water_debug_settings(WaterDebugSettings settings);
void reset_water_simulation();

// True only after a valid water material has initialized its GPU resources.
bool water_available();

// Render-thread-only binding of GPU water surfaces for wetness contact tests.
bool bind_water_contact(const renderer::Shader& shader);

// Loads the separately baked, local water asset. Missing/invalid bakes retain original rendering.
std::unique_ptr<renderer::EffectMaterial> make_water(
    scene::LevelScene& scene, const std::filesystem::path& directory
);
}  // namespace openrac::enhanced
