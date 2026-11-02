// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A live moby's pose, evaluated from the running game's memory.
//
// The game animates its mobys itself (MobyAnimAdvance runs in the port as C:
// it steps each moby's keys and blend factor every frame), but the evaluator
// that turns those fields into joint matrices, MobyAnimEval, is hand-written
// VU0 code that fed the game's VU1 renderer and does not run in the port. So
// the renderer evaluates the pose itself, from exactly what MobyAnimEval
// reads (ReRAC docs/plan/moby_animation.md sections 2, 5 and 6):
//
//   moby +0x24   the class: +0x08 joint count, +0x14 the skeleton (inverse
//                bind matrices), +0x18 common_trans (rest translations and
//                parent words)
//   moby +0x50 / +0x51 / +0x52 / +0x53   frame and sequence of keys A and B
//   moby +0x54   t, the blend from A to B
//   moby +0x68 / +0x6c   the two keys' frame headers
//
// The keys are read where the moby points, so whatever set them (the class's
// own sequences, Ratchet's table of sequences, a blend's snapshot) is drawn
// as the game would draw it. The arithmetic is assets/geometry's
// moby_animation.cpp (the game's own, to the bit).

#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "assets/geometry/moby_animation.h"

namespace openrac::scene {

/**
 * Where a moby's animation fields start: the frames here, the keys at +0x18, as above. The game's
 * profile says it once, before any pose is asked for; 0x50 until then, Ratchet & Clank's.
 */
void set_moby_anim_fields(std::uint32_t first);

/**
 * Whether the game's pose layers keep records instead of keys (rac1::PoseRecord); Going Commando's
 * do. Said once, as above.
 */
void set_moby_layer_records(bool records);

/**
 * The joint palette of the moby at `moby` (a game address): one matrix per joint of the class's
 * high LOD, F_j = P_j S_j in packed model units (rows: the images of the axes, then the
 * translation). Empty when the moby has no class, no joints or no readable key: draw it as stored.
 */
std::vector<assets::rac1::JointMatrix> moby_palette(
    std::span<const std::uint8_t> ram, std::uint32_t moby
);

/**
 * The pose matrices P_j of the same moby (each joint's frame in the model, before the inverse bind
 * matrix): what MobyAnimEvalChain leaves in the scratchpad for the game's own use (bone points,
 * attachments). Empty as for moby_palette.
 */
std::vector<assets::rac1::JointMatrix> moby_pose_matrices(
    std::span<const std::uint8_t> ram, std::uint32_t moby
);

/**
 * Joint modifiers of the port's own for one moby (a mod's, openrac_api.hero_set_joints), applied
 * after the moby's own list wherever its pose is evaluated (drawing, shadows, attachments).
 * Replaces the last set; an empty list or moby 0 clears it. Safe from any thread.
 */
void set_extra_joint_modifiers(std::uint32_t moby, std::vector<assets::rac1::JointModifier> mods);

/**
 * The port's own joint modifiers of the moby at `moby` (set_extra_joint_modifiers): empty for any
 * other moby or when none are set. Safe from any thread.
 */
std::vector<assets::rac1::JointModifier> extra_joint_modifiers(std::uint32_t moby);

}  // namespace openrac::scene
