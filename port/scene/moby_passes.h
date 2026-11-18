// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The order the mobys' colour passes draw in (LevelScene::draw_layer) and which of them write depth.
// The GS draws every moby face with TEST 0x5360b (renderer/world/gs_pass.h): a fragment whose alpha
// reaches 0x60 writes Z, the others colour only. The port draws the solid faces first, then the
// glow faces (MobyProc's glow list, mode bit 0x10) in two draws split at that reference, so that a
// near glow face hides the far ones behind it. The blended mobys are split the same way at their
// own reference (0x60, or 0x08 for one fading at its draw distance; an added one writes no
// depth). Every draw that writes depth comes first; then the rest of the glow, the translucent
// texels, the rest of the blended mobys and the metal packets, which write no depth. Drawn in one
// blended draw without depth, the parked ship's front thrusters' glow showed through its rear
// thrusters' light bars on Pokitaru.

#pragma once

#include <array>

namespace openrac::scene {

// The mobys' colour passes, by the number draw_layer and mesh.frag give them.
enum MobyPass : int {
    kMobySolid = 0,
    kMobyGlow = 1,
    kMobyTranslucent = 2,
    kMobyEffects = 3,
    kMobyMetal = 4
};

// One draw of the mobys' colour passes.
struct MobyPassStep {
    int pass = kMobySolid;  // the MobyPass drawn
    // mesh.frag's glow_split, for the glow and the blended mobys' passes: 1 the texels at or above
    // the reference, 2 the rest; 0 all.
    int glow_split = 0;
    bool depth_write = false;
    // 1 keeps only fully opaque fragments. -1 leaves those fragments to the opaque pass.
    int opacity_split = 0;
};

// The draws in order.
inline constexpr std::array<MobyPassStep, 7> kMobyPassSteps{{
    {kMobySolid, 0, true},
    {kMobyGlow, 1, true},
    {kMobyEffects, 1, true},
    {kMobyGlow, 2, false},
    {kMobyTranslucent, 0, false},
    {kMobyEffects, 2, false},
    {kMobyMetal, 0, false},
}};

inline constexpr std::array<MobyPassStep, 9> kMobyOpaqueSplitSteps{{
    {kMobySolid, 0, true},
    {kMobyGlow, 1, true, 1},
    {kMobyEffects, 1, true, 1},
    {kMobyGlow, 1, true, -1},
    {kMobyEffects, 1, true, -1},
    {kMobyGlow, 2, false},
    {kMobyTranslucent, 0, false},
    {kMobyEffects, 2, false},
    {kMobyMetal, 0, false},
}};

}  // namespace openrac::scene
