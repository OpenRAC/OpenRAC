// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A mod's energy material (OPENRAC_MOBY_MATERIAL_ENERGY, interface 1.25) on the effects a moby
// makes: the world quads its draw callbacks draw (EffectQuad::parent) and the particles tied to it
// (EffectQuad::attachment, GameProfile::particle_owners). Each is drawn in the material's colour at
// its own colour's brightness, brighter with the material's level, and a particle a little larger.
// Render-only: the quads are the window's copies; the game's particles and quads keep their colours.
// mesh.frag draws the moby's own body and glow packets.
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

#include "openrac/openrac_mod.h"

namespace openrac::frontend::energy {

// What energy draws an effect with: the mod's colour (0xRRGGBB) and the material's level.
struct Look {
    std::uint32_t rgb = 0xFFFFFF;
    int level = 0;
};

// The effects' brightening at level 0, and what each level adds: about half again at the top.
inline constexpr float kGain = 1.4f;
inline constexpr float kGainPerLevel = 0.04f;

// What each level adds to a particle's size: a third at the top (OPENRAC_MOBY_MATERIAL_ENERGY_TOP).
inline constexpr float kSizePerLevel = 1.0f / 45.0f;

// Returns the energy look of a moby with material `material` (as moby_material took it) and `rgb`,
// or none when the material is not energy.
inline std::optional<Look> look_of(std::uint32_t material, std::uint32_t rgb) {
    // Only energy recolours what a moby makes.
    if (OPENRAC_MOBY_MATERIAL_KIND(material) != OPENRAC_MOBY_MATERIAL_ENERGY) {
        return std::nullopt;
    }

    return Look{rgb & 0xFFFFFFu, static_cast<int>((material >> 8) & 0xFu)};
}

// Returns the brightening at `level`.
inline float gain(int level) {
    return kGain + kGainPerLevel * static_cast<float>(std::clamp(level, 0, OPENRAC_MOBY_MATERIAL_ENERGY_TOP));
}

// Returns a particle's size at `level`, times its own.
inline float size(int level) {
    return 1.0f + kSizePerLevel * static_cast<float>(std::clamp(level, 0, OPENRAC_MOBY_MATERIAL_ENERGY_TOP));
}

// Returns the chip's colour word `abgr` (R in the low byte, 0x80 = 1.0, alpha the high byte) in
// `look`'s colour at its luminance, brightened by the level's gain, at most 0xFF a channel; its
// alpha as it is.
inline std::uint32_t colour(std::uint32_t abgr, const Look& look) {
    const float red = static_cast<float>(abgr & 0xFFu);
    const float green = static_cast<float>((abgr >> 8) & 0xFFu);
    const float blue = static_cast<float>((abgr >> 16) & 0xFFu);
    const float grey = (0.299f * red + 0.587f * green + 0.114f * blue) * gain(look.level);
    const auto channel = [grey](std::uint32_t mod) {
        const float value = grey * static_cast<float>(mod & 0xFFu) / 255.0f + 0.5f;
        return static_cast<std::uint32_t>(std::min(value, 255.0f));
    };
    return (abgr & 0xFF000000u) | channel(look.rgb) << 16 | channel(look.rgb >> 8) << 8
           | channel(look.rgb >> 16);
}

// Draws `quad` (a renderer::EffectQuad: four rgba words, four corners) in `look`: its four colours,
// and, for a particle (`grow`), its corners spread from its centre by the level's size.
template <class Quad>
void apply(Quad& quad, const Look& look, bool grow) {
    for (std::uint32_t& c : quad.rgba) {
        c = colour(c, look);
    }

    // A callback's quads join one another (a ribbon's segments): only a particle grows.
    if (!grow) {
        return;
    }

    const float scale = size(look.level);
    for (int axis = 0; axis < 3; ++axis) {
        const float centre =
            (quad.corner[0][axis] + quad.corner[1][axis] + quad.corner[2][axis] + quad.corner[3][axis])
            / 4.0f;
        for (auto& corner : quad.corner) {
            corner[axis] = centre + (corner[axis] - centre) * scale;
        }
    }
}

}  // namespace openrac::frontend::energy
