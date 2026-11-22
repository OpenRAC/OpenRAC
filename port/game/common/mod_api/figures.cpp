// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "figures": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_figure_create(openrac_mod* mod) {
    std::int32_t level = -1;
    if (mod == nullptr || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // Figures gone with an earlier level make room.
    for (Figure& f : g_figures) {
        if (f.mod != nullptr && !live(f)) {
            f = Figure{};
        }
    }
    for (std::size_t i = 0; i < g_figures.size(); ++i) {
        if (g_figures[i].mod != nullptr) {
            continue;
        }
        const gaddr moby = g_play->figure_spawn();
        // The level's pool is full.
        if (moby == 0) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        g_figures[i] = {mod, moby, g_play->loads()};
        return static_cast<int>(i);
    }
    return OPENRAC_HOOK_UNKNOWN;
}

int api_figure_pose(openrac_mod* mod, int id, const openrac_pose* pose) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || pose == nullptr || pose->size < sizeof(openrac_pose)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose p{};
    std::memcpy(&p, &pose->level, sizeof p);
    g_play->figure_pose(f->moby, &p);
    return OPENRAC_HOOK_OK;
}

int api_figure_remove(openrac_mod* mod, int id) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->figure_remove(f->moby);
    *f = Figure{};
    return OPENRAC_HOOK_OK;
}

int api_figure_slot(openrac_mod* mod, int id) {
    if (g_play == nullptr || g_play->hero_slot == nullptr) {
        return -1;
    }
    if (id < 0) {
        return g_play->hero_slot();
    }
    const Figure* f = own_figure(mod, id);
    return f != nullptr ? g_play->slot_of(f->moby) : -1;
}

int api_figure_held(openrac_mod* mod, int id, const openrac_held* held) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || held == nullptr || held->size < sizeof(openrac_held)
        || g_play->figure_held == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_held h{};
    std::memcpy(&h, &held->count, sizeof h);
    h.count = std::clamp<std::int32_t>(h.count, 0, OPENRAC_GAME_HELD_MAX);
    g_play->figure_held(f->moby, &h);
    return OPENRAC_HOOK_OK;
}

int api_figure_shot(openrac_mod* mod, int id, const openrac_shot* shot) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || shot == nullptr || shot->size < sizeof(openrac_shot)
        || g_play->figure_shot == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_shot s{};
    std::memcpy(&s, &shot->item, sizeof s);
    return g_play->figure_shot(f->moby, &s) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_figure_shot2(openrac_mod* mod, int id, const openrac_shot2* shot) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || shot == nullptr || shot->size < sizeof(openrac_shot2)
        || g_play->figure_shot2 == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_shot2 s{};
    std::memcpy(&s, &shot->item, sizeof s);
    return g_play->figure_shot2(f->moby, &s) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
