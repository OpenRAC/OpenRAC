// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mods draw over the game (mod interface 1.12: openrac_api.on_overlay, docs/port/MODS.md):
// each displayed frame, after the world and the game's own 2D and before the port's menus and
// overlay, the mods' callbacks fill a list of triangles (renderer/ui/overlay_list.h) with the
// frame's own camera, and it is blended over the frame. With no mod asking, nothing runs.

#pragma once

#include "renderer/renderer.h"

namespace openrac::frontend::mod_overlay {

// Over the finished frame (`framebuffer`, `width` x `height` pixels, shown `aspect` wide to 1 tall),
// on the window thread with the graphics in hand. `has_camera`: the frame drew a world with `camera`.
void draw(
    const renderer::Camera& camera,
    bool has_camera,
    unsigned framebuffer,
    int width,
    int height,
    float aspect
);

// GL objects, before the context goes.
void release();

}  // namespace openrac::frontend::mod_overlay
