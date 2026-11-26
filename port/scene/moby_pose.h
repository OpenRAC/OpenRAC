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
#include <optional>
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
 * Guest memory past RAM that a class may live in: the port's own region the imported moby classes
 * are kept in (GUEST_PORT_LOW, rac1/host/foreign_class.c), its bytes and its game address. Poses
 * read a class there as in RAM. None until it is set.
 */
void set_moby_class_region(std::span<const std::uint8_t> bytes, std::uint32_t address);

/**
 * The port's own game memory: `memory` holds the game addresses from `base` on (runtime's
 * GUEST_PORT region), which the game reaches but game RAM does not hold. A mod's copy of an enemy
 * keeps pvars larger than a pool slot's there, and with them its joint modifiers and pose layers.
 * Said once, as above; until then such addresses read as RAM's.
 */
void set_port_memory(std::uint32_t base, std::span<const std::uint8_t> memory);

/**
 * Keys and blend to pose a moby with in place of its own animation fields: a pose the port gives a
 * moby the game leaves asleep (game/common/frontend/sleeping_mobys.h). `a` and `b` are game
 * addresses of two frames of its class, `t` the blend, `plain` whether they are consecutive keys
 * of one sequence (the game's plain lerp).
 */
struct AnimKeys {
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    float t = 0.0f;
    bool plain = true;
};

/**
 * The joint palette of the moby at `moby` (a game address): one matrix per joint of the class's
 * high LOD, F_j = P_j S_j in packed model units (rows: the images of the axes, then the
 * translation). Empty when the moby has no class, no joints or no readable key: draw it as stored.
 */
std::vector<assets::rac1::JointMatrix> moby_palette(
    std::span<const std::uint8_t> ram, std::uint32_t moby
);

/**
 * The same palette posed by `keys` instead of the moby's own keys and blend; its joint modifiers
 * and pose layers are still its own. Reads the game's memory only.
 */
std::vector<assets::rac1::JointMatrix> moby_palette(
    std::span<const std::uint8_t> ram, std::uint32_t moby, const AnimKeys& keys
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
 * How far apart two frames of a class pose its joints: the largest turn of any joint (radians) and
 * the largest move of any joint's translation (packed model units). A joint a frame does not
 * translate stands at its rest translation, as the evaluator leaves it.
 */
struct KeyGap {
    float turn = 0.0f;
    float move = 0.0f;
};

/**
 * The gap between the frames at game addresses `a` and `b` of the class at `cls`; none when either
 * is not a frame of it.
 */
std::optional<KeyGap> key_gap(
    std::span<const std::uint8_t> ram, std::uint32_t cls, std::uint32_t a, std::uint32_t b
);

/** The same pose matrices posed by `keys`, as moby_palette with keys. */
std::vector<assets::rac1::JointMatrix> moby_pose_matrices(
    std::span<const std::uint8_t> ram, std::uint32_t moby, const AnimKeys& keys
);

/**
 * Joint modifiers of the port's own for one moby (a mod's, openrac_api.hero_set_joints and
 * moby_set_joints), applied after the moby's own list wherever its pose is evaluated (drawing,
 * shadows, attachments). Replaces that moby's last set; an empty list clears it, moby 0 clears
 * every moby's. Safe from any thread.
 */
void set_extra_joint_modifiers(std::uint32_t moby, std::vector<assets::rac1::JointModifier> mods);

/**
 * The port's own joint modifiers of the moby at `moby` (set_extra_joint_modifiers): empty for any
 * other moby or when none are set. Safe from any thread.
 */
std::vector<assets::rac1::JointModifier> extra_joint_modifiers(std::uint32_t moby);

}  // namespace openrac::scene
