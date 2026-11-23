// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "graphics": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

const openrac_graphics* api_graphics(void) {
    static openrac_graphics none = [] {
        openrac_graphics n{};
        n.size = sizeof(openrac_graphics);
        return n;
    }();
    const openrac_graphics* graphics =
        g_host != nullptr && g_host->graphics != nullptr ? g_host->graphics() : nullptr;
    return graphics != nullptr ? graphics : &none;
}

// A structure the mod gives with its own size first, copied into `out`: what a mod built against
// a smaller one does not have stays zero. False when it is not even the size and the user pointer.
template <class T>
bool copy_sized(const T* from, T& out) {
    if (from == nullptr || from->size < offsetof(T, user) + sizeof(void*)) {
        return false;
    }
    out = T{};
    std::memcpy(&out, from, std::min<std::size_t>(from->size, sizeof(T)));
    out.size = sizeof(T);
    return true;
}

int api_on_views(openrac_mod* mod, const openrac_view_source* source) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_views_mod != nullptr) {
        log::warn("mod {} asks for views, which mod {} has", mod->id, g_views_mod->id);
        return OPENRAC_HOOK_TAKEN;
    }
    openrac_view_source s;
    if (!copy_sized(source, s) || s.begin == nullptr || s.end == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_views = s;
    g_views_mod = mod;
    return OPENRAC_HOOK_OK;
}

int api_setup_vulkan(openrac_mod* mod, const openrac_vulkan_setup* setup) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (!native_vulkan_allowed(mod->manifest)) {
        log::warn("mod {} must declare \"vulkan_queue\": true to use native Vulkan", mod->id);
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_vulkan_mod != nullptr) {
        log::warn(
            "mod {} asks to make the Vulkan device, which mod {} makes", mod->id, g_vulkan_mod->id
        );
        return OPENRAC_HOOK_TAKEN;
    }
    openrac_vulkan_setup s;
    if (!copy_sized(setup, s)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (!mod->manifest.vulkan_device) {
        log::warn(
            "mod {} makes the Vulkan device, but its manifest has no \"vulkan_device\": true; "
            "the window may open before it loads",
            mod->id
        );
    }
    g_vulkan = s;
    g_vulkan_mod = mod;
    return OPENRAC_HOOK_OK;
}

// The phases an overlay is drawn over (interface 1.31).
constexpr std::uint32_t kOverlayPhases =
    OPENRAC_OVERLAY_PLAYING | OPENRAC_OVERLAY_MENU | OPENRAC_OVERLAY_SCENE;

// Interface 1.31: the same over the phases the mod asks for (on_overlay is play's alone).
int api_on_overlay_phases(
    openrac_mod* mod, openrac_overlay_fn fn, void* user, std::uint32_t phases
) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (fn == nullptr || phases == 0 || (phases & ~kOverlayPhases) != 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // One a mod: the last it gave.
    for (OverlayCallback& c : g_overlays) {
        if (c.mod == mod) {
            c = {mod, fn, user, phases};
            return OPENRAC_HOOK_OK;
        }
    }
    g_overlays.push_back({mod, fn, user, phases});
    return OPENRAC_HOOK_OK;
}

int api_on_overlay(openrac_mod* mod, openrac_overlay_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    return api_on_overlay_phases(mod, fn, user, OPENRAC_OVERLAY_PLAYING);
}

int api_play_live_picture(openrac_mod* mod, int on) {
    if (mod == nullptr || g_play == nullptr || g_play->live_picture == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->live_picture(on != 0 ? 1 : 0);
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail
