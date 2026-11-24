// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mods draw over the game (mod interface 1.14: openrac_api.on_overlay, and 1.31's
// on_overlay_phases over the game's menus and scenes too, docs/port/MODS.md):
// each displayed frame, after the world and the game's own 2D and before the port's menus and
// overlay, the mods' callbacks fill a list of triangles (renderer/ui/overlay_list.h) with the
// frame's own camera, and it is blended over the frame. With no mod asking, nothing runs. 1.40 adds
// the mods' pictures (renderer/ui/overlay_images.h) and a call whose drawing goes under the game's
// 2D (OPENRAC_OVERLAY_UNDER_HUD).

#pragma once

#include <cstdint>

#include "renderer/renderer.h"
#include "renderer/ui/overlay_list.h"

namespace openrac::frontend::mod_overlay {

// Over the finished frame (`framebuffer`, `width` x `height` pixels, shown `aspect` wide to 1 tall),
// on the window thread with the graphics in hand. `has_camera`: the frame drew a world with `camera`;
// `world_under_menu`: it drew it live under a menu. `phase`: the OPENRAC_OVERLAY_* bit of the tick
// the frame shows (mods::overlay_phase), 0 for none.
void draw(
    const renderer::Camera& camera,
    bool has_camera,
    bool world_under_menu,
    std::uint32_t phase,
    unsigned framebuffer,
    int width,
    int height,
    float aspect
);

// The mods' drawing under the game's 2D (OPENRAC_OVERLAY_UNDER_HUD, interface 1.40) for a frame of
// `phase` shown `aspect` wide to 1 tall, into `out` (left empty with no mod asking). On the window
// thread, before the frame is handed to the renderer: `project` answers 0 in it.
void record_under_hud(std::uint32_t phase, float aspect, renderer::ui::OverlayList& out);

// What record_under_hud gathered, blended over `framebuffer` (`width` x `height`), on the thread
// drawing the frame with the graphics in hand, before the game's 2D.
void draw_under_hud(
    const renderer::ui::OverlayList& list, unsigned framebuffer, int width, int height
);

// GL objects, before the context goes.
void release();

}  // namespace openrac::frontend::mod_overlay
