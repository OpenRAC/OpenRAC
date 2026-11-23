// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "camera": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_on_camera_script(openrac_mod* mod, openrac_camera_script_fn fn, void* user) {
    if (!has_camera_scripts()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_camera_scripts.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_camera_scripts = {mod, fn, user};
    return OPENRAC_HOOK_OK;
}

int api_camera_script_play(openrac_mod* mod, int id) {
    if (mod == nullptr || !has_camera_scripts() || id < OPENRAC_CAMERA_SCRIPT_CUT
        || (g_camera_shower != nullptr && g_camera_shower != mod)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_play->camera_script_play(id) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_camera_shower = id >= 0 ? mod : nullptr;
    return OPENRAC_HOOK_OK;
}

int api_camera_script_view(float eye[3], float rotation[3], float where[3], int* stand_in) {
    if (eye == nullptr || rotation == nullptr || where == nullptr || !has_camera_scripts()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::int32_t standing = 0;
    if (g_play->camera_script_view(eye, rotation, where, &standing) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (stand_in != nullptr) {
        *stand_in = standing;
    }
    return OPENRAC_HOOK_OK;
}

int api_camera_script_show(openrac_mod* mod, const float eye[3], const float rotation[3]) {
    if (mod == nullptr || mod != g_camera_shower || eye == nullptr || rotation == nullptr
        || !has_camera_scripts() || !std::isfinite(eye[0] + eye[1] + eye[2])
        || !std::isfinite(rotation[0] + rotation[1] + rotation[2])) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->camera_script_show(eye, rotation) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_camera_position(float out[3]) {
    if (out == nullptr || g_play == nullptr || g_play->camera_position == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->camera_position(out) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

std::uint32_t g_camera_loads = 0;
std::mutex g_camera_visibility_lock;
std::map<const openrac_mod*, std::uint32_t> g_camera_visibility;

std::map<int, AmmoPanelOffset> g_ammo_panel_offsets;

int api_camera_view(openrac_camera* out) {
    if (out == nullptr || out->size < sizeof(openrac_camera) || g_play == nullptr
        || g_play->camera_view == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    float eye[3], rows[9];
    if (g_play->camera_view(eye, rows) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::copy(eye, eye + 3, out->eye);
    std::copy(rows, rows + 3, out->forward);
    std::copy(rows + 3, rows + 6, out->left);
    std::copy(rows + 6, rows + 9, out->up);
    return OPENRAC_HOOK_OK;
}

int api_camera_visibility_bypass(openrac_mod* mod, int enabled) {
    if (mod == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_camera_visibility_lock);
    if (enabled == 0) {
        g_camera_visibility.erase(mod);
        return OPENRAC_HOOK_OK;
    }
    std::int32_t level = -1;
    if (enabled != 1 || phase_now(&level) != OPENRAC_PLAY_PLAYING || level < 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_camera_visibility[mod] = loads_now();
    return OPENRAC_HOOK_OK;
}

int api_camera_set(openrac_mod* mod, const openrac_camera* camera) {
    if (mod == nullptr || g_play == nullptr || g_play->camera_set == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_camera_mod != nullptr && g_camera_loads != loads_now()) {
        g_camera_mod = nullptr;  // its level went, and the camera with it
    }
    if (g_camera_mod != nullptr && g_camera_mod != mod) {
        return OPENRAC_HOOK_TAKEN;
    }
    if (camera == nullptr) {
        if (g_camera_mod == mod) {
            g_play->camera_set(nullptr, nullptr);
            g_camera_mod = nullptr;
        }
        return OPENRAC_HOOK_OK;
    }
    const auto square = [](const float a[3], const float b[3]) {
        return std::fabs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) < 0.01f;
    };
    if (camera->size < sizeof(openrac_camera) || !unit(camera->forward) || !unit(camera->left)
        || !unit(camera->up) || !square(camera->forward, camera->left)
        || !square(camera->forward, camera->up) || !square(camera->left, camera->up)
        || !std::all_of(camera->eye, camera->eye + 3, [](float v) { return std::isfinite(v); })) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    float rows[9];
    std::copy(camera->forward, camera->forward + 3, rows);
    std::copy(camera->left, camera->left + 3, rows + 3);
    std::copy(camera->up, camera->up + 3, rows + 6);
    if (g_play->camera_set(camera->eye, rows) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_camera_mod = mod;
    g_camera_loads = loads_now();
    return OPENRAC_HOOK_OK;
}

// Interface 1.33: the game's own camera shake on the level camera, channel 0 or 1. The game's routine
// decays it; a new call replaces the channel's shake. Nothing is kept here: the shake lives in the
// game's camera, so it ends with the level (the game's camera is made again).
int api_camera_shake(openrac_mod* mod, int channel, float amplitude, int frames) {
    if (mod == nullptr || g_play == nullptr || g_play->camera_shake == nullptr || channel < 0
        || channel > 1 || !std::isfinite(amplitude) || amplitude < 0.0f || amplitude > 1.0f
        || frames < 0 || frames > OPENRAC_CAMERA_SHAKE_FRAMES_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->camera_shake(channel, amplitude, frames) != 0 ? OPENRAC_HOOK_OK
                                                                 : OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
