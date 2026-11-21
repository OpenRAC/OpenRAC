// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "hero": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_hero_pose(openrac_pose* out) {
    if (out == nullptr || out->size < sizeof(openrac_pose) || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose pose{};
    if (g_play->hero_pose(&pose) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(&out->level, &pose, sizeof pose);
    return OPENRAC_HOOK_OK;
}

int api_hero_stand_in(openrac_mod* mod, const float position[3], int figure) {
    if (mod == nullptr || mod != g_gate.mod || g_before_slot < 0 || position == nullptr
        || g_play == nullptr || g_play->stand_in == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const Figure* f = figure >= 0 ? own_figure(mod, figure) : nullptr;
    g_play->stand_in(position, f != nullptr ? f->moby : 0);
    return OPENRAC_HOOK_OK;
}

int api_hero_held(openrac_held* out) {
    if (out == nullptr || out->size < sizeof(openrac_held) || g_play == nullptr
        || g_play->hero_held == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_held held{};
    if (g_play->hero_held(&held) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(&out->count, &held, sizeof held);
    return OPENRAC_HOOK_OK;
}

int api_hero_shots(openrac_shot* out, int max) {
    if (out == nullptr || max <= 0 || g_play == nullptr || g_play->hero_shots == nullptr) {
        return 0;
    }
    std::array<openrac_game_shot, OPENRAC_SHOTS_MAX> shots{};
    const int n = g_play->hero_shots(shots.data(), std::min(max, OPENRAC_SHOTS_MAX));
    for (int i = 0; i < n; ++i) {
        out[i].size = sizeof(openrac_shot);
        std::memcpy(&out[i].item, &shots[static_cast<std::size_t>(i)], sizeof(openrac_game_shot));
    }
    return n;
}

int api_hero_place(openrac_mod* mod, const float position[3], float yaw, std::uint32_t flags) {
    if (mod == nullptr || position == nullptr || g_play == nullptr || g_play->hero_place == nullptr
        || !std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2])
        || !std::isfinite(yaw)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_place(position, yaw, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_grab(openrac_mod* mod, int animation, std::uint32_t flags) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_grab == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_grab(animation, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_release(openrac_mod* mod) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_let_go == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_let_go() != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_hold_at(openrac_mod* mod, const float position[3], float yaw) {
    if (mod == nullptr || position == nullptr || g_play == nullptr
        || g_play->hero_hold_at == nullptr || !std::isfinite(position[0])
        || !std::isfinite(position[1]) || !std::isfinite(position[2]) || !std::isfinite(yaw)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_hold_at(position, yaw) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_stand_in_as(openrac_mod* mod, const openrac_stand_in* as, int figure) {
    if (mod == nullptr || mod != g_gate.mod || g_before_slot < 0 || as == nullptr
        || g_play == nullptr || g_play->stand_in_as == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const Figure* f = figure >= 0 ? own_figure(mod, figure) : nullptr;
    g_play->stand_in_as(as->position, f != nullptr ? f->moby : 0, as->state, as->health, as->flags);
    return OPENRAC_HOOK_OK;
}

int api_hero_stand_in_read(openrac_mod* mod, openrac_stand_in* out) {
    if (mod == nullptr || mod != g_gate.mod || out == nullptr || g_play == nullptr
        || g_play->stand_in_read == nullptr
        || g_play->stand_in_read(out->position, &out->yaw, &out->state, &out->health, &out->flags)
               == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

int api_hero_revive(openrac_mod* mod, const float position[3], std::uint32_t flags) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_revive == nullptr
        || (position != nullptr
            && (!std::isfinite(position[0]) || !std::isfinite(position[1])
                || !std::isfinite(position[2])))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_revive(position, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_shots2(openrac_shot2* out, int max) {
    if (out == nullptr || max <= 0 || g_play == nullptr || g_play->hero_shots2 == nullptr) {
        return 0;
    }
    std::array<openrac_game_shot2, OPENRAC_SHOTS_MAX> shots{};
    const int n = g_play->hero_shots2(shots.data(), std::min(max, OPENRAC_SHOTS_MAX));
    for (int i = 0; i < n; ++i) {
        out[i].size = sizeof(openrac_shot2);
        std::memcpy(&out[i].item, &shots[static_cast<std::size_t>(i)], sizeof(openrac_game_shot2));
    }
    return n;
}

int api_hero_set_pose(openrac_mod* mod, const openrac_pose* pose) {
    if (mod == nullptr || pose == nullptr || pose->size < sizeof(openrac_pose) || g_play == nullptr
        || g_play->hero_set_pose == nullptr || !holds_hook(mod, g_play->hero_update_hook)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose p{};
    std::memcpy(&p, &pose->level, sizeof p);
    return g_play->hero_set_pose(&p) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// Any mod may turn the hero's joints (ports before took them only from the holder of his update
// hook): its offsets replace its own last ones and are composed with the other mods' (g_joint_sets).
int api_hero_set_joints(openrac_mod* mod, const openrac_joint_offset* joints, int count) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_moby == nullptr
        || g_joint_offsets == nullptr || count < 0 || count > OPENRAC_JOINT_OFFSETS_MAX
        || (count > 0 && joints == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::uint32_t moby = g_play->hero_moby();
    if (moby == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    auto it = std::find_if(g_joint_sets.begin(), g_joint_sets.end(), [&](const JointSet& s) {
        return s.mod == mod;
    });
    if (count == 0) {
        if (it != g_joint_sets.end()) {
            g_joint_sets.erase(it);
        }
    } else {
        if (it == g_joint_sets.end()) {
            it = g_joint_sets.insert(g_joint_sets.end(), JointSet{mod, {}});
        }
        it->joints.assign(joints, joints + count);
    }
    std::size_t total = 0;
    for (const JointSet& s : g_joint_sets) {
        total += s.joints.size();
    }
    if (total > OPENRAC_JOINT_OFFSETS_MAX) {
        // More than the hero takes in all: this mod's last call is undone.
        std::erase_if(g_joint_sets, [&](const JointSet& s) { return s.mod == mod; });
        apply_joint_sets(moby);
        return OPENRAC_HOOK_UNKNOWN;
    }
    apply_joint_sets(moby);
    return OPENRAC_HOOK_OK;
}

int api_hero_hurt(openrac_mod* mod, int damage) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_hurt == nullptr) {
        return -1;
    }
    return g_play->hero_hurt(damage);
}

}  // namespace openrac::mods::detail
