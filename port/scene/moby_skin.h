// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Skins: a moby of the level drawn as another model (from another level or game) while the game
// runs its own class's logic. The editor's pack writes them beside the level's placements
// (OpenRAC Editor's openrac/tools/skins.py): skins.json names each moby by its spawn id, the model (a skinned .glb with its
// own animations, read as a prop's is: load_model and load_prop_rig), the scale its own is
// multiplied by, and which of the model's animations plays for each of the class's sequences.
// Only what is drawn changes: the game, its collision and its logic never see a skin.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "scene/level.h"
#include "scene/prop_rig.h"

namespace openrac::scene {

struct MobySkin {
    std::uint32_t model = 0;  // in the level's models
    PropRig rig;
    float scale = 1.0f;
    // The rig's animation for each of the class's sequences; -1 (or none): the same number when
    // the rig has it, else its first.
    std::vector<int> animations;
};

struct MobySkins {
    std::unordered_map<int, MobySkin> by_spawn_id;

    // How long each drawn moby has been in its sequence, by its address: the skin's animation
    // plays from the start when the class's sequence changes.
    struct Clock {
        int sequence = -1;
        std::uint32_t ticks = 0;
    };

    std::unordered_map<std::uint32_t, Clock> clocks;

    bool empty() const { return by_spawn_id.empty(); }
};

// Reads `file` (skins.json; its models by paths relative to its folder, inside it) into `out`,
// the models into `level`. False with the reason in `error` when the file is broken; a model that
// cannot be read or has no skeleton leaves its moby drawn as its class, with the reason in
// `warnings`.
bool load_moby_skins(
    const std::filesystem::path& file,
    LevelData& level,
    MobySkins& out,
    std::string& error,
    std::vector<std::string>& warnings
);

// The rig's animation that plays while the class plays `sequence`.
int moby_skin_animation(const MobySkin& skin, int sequence);

// Seconds into that animation for the moby at `address`, at the game's 60 ticks a second: one tick
// later than the last call for the same moby, or 0 when its sequence changed. Called once a tick.
float moby_skin_seconds(MobySkins& skins, std::uint32_t address, int sequence);

}  // namespace openrac::scene
