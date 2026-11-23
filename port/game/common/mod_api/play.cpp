// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "play": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_play_status(openrac_play_status* out) {
    if (out == nullptr || out->size < sizeof(openrac_play_status)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->phase = phase_now(&out->level);
    out->levels = g_play != nullptr ? g_play->levels : 0;
    out->loads = g_play != nullptr ? g_play->loads() : 0;
    return g_play != nullptr ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

const char* api_level_name(int level) {
    if (g_play == nullptr || g_play->level_names == nullptr || level < 0
        || level >= g_play->levels) {
        return nullptr;
    }
    return g_play->level_names[level];
}

int api_play_start(openrac_mod* mod, int level, const void* save, size_t size) {
    std::int32_t now = -1;
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_request.kind != Request::Kind::kNone) {
        return OPENRAC_HOOK_TAKEN;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(save);
    // The front end, a level the game has, a save it takes.
    if (phase_now(&now) != OPENRAC_PLAY_FRONT_END || level >= g_play->levels
        || (bytes != nullptr && !save_info(bytes, size, nullptr))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_request = {Request::Kind::kStart, mod, level, {}};
    if (bytes != nullptr) {
        g_request.save.assign(bytes, bytes + size);
    }
    return OPENRAC_HOOK_OK;
}

int api_play_travel(openrac_mod* mod, int level) {
    std::int32_t now = -1;
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_request.kind != Request::Kind::kNone) {
        return OPENRAC_HOOK_TAKEN;
    }
    if (!in_a_level(phase_now(&now)) || level < -1 || level >= g_play->levels) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_request = {Request::Kind::kTravel, mod, level, {}};
    return OPENRAC_HOOK_OK;
}

int api_play_overlay_pause(openrac_mod* mod, int on) {
    if (mod == nullptr || g_play == nullptr || g_play->keep_running == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->keep_running(on != 0 ? 1 : 0);
    return OPENRAC_HOOK_OK;
}

// A skip a mod asked for: the ticks it is pressed for still (play_skipping), and what it was asked
// in (the phase, the level's scene: -1 none), so it never presses into the next scene or movie.
constexpr int kSkipTicks = 40;

int api_play_skip(openrac_mod* mod) {
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::int32_t level = -1;
    g_skip_ticks = kSkipTicks;
    g_skip_phase = phase_now(&level);
    g_skip_scene = scene_now();
    return OPENRAC_HOOK_OK;
}

int api_bolts(int add) {
    return g_play != nullptr && g_play->bolts != nullptr ? g_play->bolts(add) : 0;
}

// The level's scenes: the one playing, and one to start as the level's own trigger does (the game
// starts it once the player is in control; -1 drops a request still waiting).
int api_scene_playing(int* frame) {
    std::int32_t played = 0;
    const std::int32_t scene =
        g_play != nullptr && g_play->scene_playing != nullptr ? g_play->scene_playing(&played) : -1;
    if (frame != nullptr) {
        *frame = scene >= 0 ? played : 0;
    }
    return scene >= 0 ? scene : -1;
}

int api_play_scene(openrac_mod* mod, int scene) {
    if (mod == nullptr || g_play == nullptr || g_play->scene_start == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->scene_start(scene < 0 ? -1 : scene) != 0 ? OPENRAC_HOOK_OK
                                                            : OPENRAC_HOOK_UNKNOWN;
}

int api_play_scene_from(openrac_mod*, int, int) {
    return OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
