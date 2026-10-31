// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstdint>
#include <optional>

#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "scene/level.h"

namespace openrac::enhanced {
// Session-only material tuning, shared safely between the UI and render worker.
struct WetnessMaterialSettings {
    float diffuse_scale = .34f;
    float specular_strength = 1.07f;
    float dry_roughness = .51f;
    float wet_roughness = .35f;
    float reflectivity = .0633f;
};

WetnessMaterialSettings wetness_material_settings();
void set_wetness_material_settings(WetnessMaterialSettings settings);

bool wetness_debug_view();
void set_wetness_debug_view(bool enabled);

// Persistent surface samples: only the GPU computes contact and wet/dry history.
class HeroWetness {
public:
    ~HeroWetness();
    void load(const scene::LevelData& level);
    void release();
    void prepare(
        const renderer::FrameInput& input,
        unsigned palette,
        const std::optional<scene::Instance>& hero
    );
    void bind(const renderer::Shader& shader) const;

private:
    renderer::Shader m_update;
    unsigned m_vertices = 0, m_state = 0, m_dummy = 0;
    unsigned m_first = 0, m_count = 0, m_model = ~0u;
    int m_palette = -1;
    std::uint64_t m_frame = ~0ull, m_identity = 0;
    bool m_active = false, m_reset = true;
};
}  // namespace openrac::enhanced
