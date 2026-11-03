// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The active body's shadow rig for frame.cpp, shared by mesh maps and original stencil volumes.
// Ownership comes from the game's callback; renderer availability must not change the body.

#pragma once

#include <algorithm>
#include <array>

#include "openrac/game_host.h"

namespace openrac::frontend {

// A bounded frame-local ownership list, with the controlled body before its attached equipment.
struct HeroShadowRig {
    // Room for the body and equipment without allocating on each frame.
    static constexpr unsigned kCapacity = 32;
    std::array<gaddr, kCapacity> addresses{};  // Guest addresses supplied by the game.
    unsigned count = 0;                        // Valid entries, never more than kCapacity.

    // Tests whether the game includes this body or attachment in the hero rig.
    [[nodiscard]] bool owns(gaddr address) const {
        return std::find(addresses.begin(), addresses.begin() + count, address)
               != addresses.begin() + count;
    }

    // Selects the controlled body; `fallback` preserves games without a readable ownership list.
    [[nodiscard]] bool anchors(gaddr address, bool fallback) const {
        // A supplied body overrides class-based fallbacks, including an inactive Ratchet.
        if (count) {
            return address == addresses[0];
        }

        return fallback;
    }
};

// Reads ownership for either shadow path in a supported loaded level.
[[nodiscard]] inline HeroShadowRig read_hero_shadow_rig(
    const openrac_front_end* game, int level, bool mesh_shadow, bool stencil_shadow
) {
    HeroShadowRig rig;

    // Original volumes need the active body even when the enhanced shadow map is disabled.
    if ((!mesh_shadow && !stencil_shadow) || !game || level < 0 || level >= game->levels
        || !game->hero_shadow_casters) {
        return rig;
    }

    rig.count = std::min(
        game->hero_shadow_casters(rig.addresses.data(), HeroShadowRig::kCapacity),
        HeroShadowRig::kCapacity
    );
    return rig;
}

}  // namespace openrac::frontend
