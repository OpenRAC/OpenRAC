// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "graphics": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"
#include "renderer/ui/overlay_images.h"

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
// The layer bit a callback may add to its phases (interface 1.41).
constexpr std::uint32_t kOverlayLayers = OPENRAC_OVERLAY_UNDER_HUD;

// Interface 1.31: the same over the phases the mod asks for (on_overlay is play's alone).
int api_on_overlay_phases(
    openrac_mod* mod, openrac_overlay_fn fn, void* user, std::uint32_t phases
) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (fn == nullptr || (phases & kOverlayPhases) == 0
        || (phases & ~(kOverlayPhases | kOverlayLayers)) != 0) {
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

// openrac_api.overlay_image_create. Since 1.41.
// Interface 1.41: a picture of the mod's for its overlay (renderer/ui/overlay_images.h).
int api_overlay_image_create(openrac_mod* mod, const openrac_overlay_image* image) {
    if (mod == nullptr || image == nullptr
        || image->size < offsetof(openrac_overlay_image, flags) + sizeof(image->flags)
        || image->rgba == nullptr || image->width < 1 || image->height < 1
        || image->width > OPENRAC_OVERLAY_IMAGE_SIDE_MAX
        || image->height > OPENRAC_OVERLAY_IMAGE_SIDE_MAX
        || (image->flags & ~OPENRAC_OVERLAY_IMAGE_SMOOTH) != 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::size_t bytes =
        static_cast<std::size_t>(image->width) * static_cast<std::size_t>(image->height) * 4u;
    renderer::ui::overlay_images::Refusal why = renderer::ui::overlay_images::Refusal::None;
    const int id = renderer::ui::overlay_images::create(
        mod,
        image->width,
        image->height,
        {image->rgba, bytes},
        (image->flags & OPENRAC_OVERLAY_IMAGE_SMOOTH) != 0,
        &why
    );
    if (id > 0) {
        return id;
    }
    if (why == renderer::ui::overlay_images::Refusal::Full) {
        return OPENRAC_HOOK_FULL;
    }
    return OPENRAC_HOOK_UNKNOWN;
}

int api_overlay_image_destroy(openrac_mod* mod, int image) {
    return renderer::ui::overlay_images::destroy(mod, image) ? OPENRAC_HOOK_OK
                                                             : OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
