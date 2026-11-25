// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captured scenery transforms, lighting and persistent visibility, independent of rendering.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "game/common/profile.h"
#include "scene/level_renderer.h"

namespace openrac::frontend {

// Owned scenery records and their resolved draw-distance caps.
struct CapturedScenery {
    scene::GameScenery scenery;  // Immutable after capture.
    std::uint32_t tie_cap = 0;   // World units; zero when unavailable.
    float shrub_cap = 0.0f;      // World units; zero when unavailable.
};

// Reads records on the capture thread; `words` names matrix and record-pointer globals from
// the game profile, `counts` matches the loaded ties and shrubs, and `scale` is zero for unlimited.
// `words_level`: the level whose program the words are named in (GameProfile::
// scenery_matrices_level), -1 the executable's; `tie_cap_after`: the ties' cap past the records'
// word (GameProfile::tie_cap_after_records).
CapturedScenery capture_scenery(
    std::span<const std::uint8_t> ram,
    const std::array<std::uint32_t, 4>& words,
    const host::GameState& state,
    float scale,
    const std::array<std::size_t, 2>& counts,
    int words_level = -1,
    std::uint32_t tie_cap_after = 0x48
);

}  // namespace openrac::frontend
