// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include "renderer/shadow_volume.h"

namespace openrac::scene {
struct MobyShadowState {
    bool enabled = false;
    renderer::Vec3 extrusion{};
    std::array<float, 2> heights{};
    float scale = 0;
    std::uint8_t direction_index = 0;
};

// MobyProc (NTSC 0x212738..0x2128bc): selected direction, ground slab,
// mode gates and distance fade. The game updates the table and moby fields.
MobyShadowState moby_shadow_state(
    std::span<const std::uint8_t> ram,
    std::uint32_t moby,
    std::uint32_t directions,
    const renderer::Mat4& view
);

// Going Commando's MobyProc (0x2A6010..0x2A6188) and shadow list (func_002DFD88): the slab lies
// across the moby's up axis (matrix row +0xE0), +0x70 along it from the moby's position and +0x74
// either side; +0xBD below 0 casts along that axis reversed; the fade distance is byte +0x6F. The
// heights returned are the slab's world z where its centre is cast.
MobyShadowState moby_shadow_state_on_axis(
    std::span<const std::uint8_t> ram,
    std::uint32_t moby,
    std::uint32_t directions,
    const renderer::Mat4& view
);

// The relocated class block, posed with P_j (not the mesh's P_j S_j).
std::vector<renderer::ShadowCapsule> moby_shadows(
    std::span<const std::uint8_t> ram,
    std::uint32_t moby,
    const renderer::Mat4& model,
    float scale = 1.0f
);
}  // namespace openrac::scene
