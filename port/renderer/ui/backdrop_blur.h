// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The window blurred, as the launcher's desktop header blurs the page under it (backdrop-filter):
// the game menu lies over this (debug/menu_window.cpp game_backdrop). The rows are read from the
// default framebuffer, halved a few times and blurred with a Gaussian there, with
// renderer/gl.h's calls, so it runs on an OpenGL window and on the Vulkan renderer's alike.
//
// Its GL objects are made at the first blur and freed by release(), while the context is current.

#pragma once

namespace openrac::renderer::ui::backdrop_blur {

// A blurred picture of the window's top rows: its texture, the first row the bottom (GL's way).
struct Picture {
    unsigned int texture = 0;  // 0 when nothing could be made
    int rows = 0;              // the window's rows it shows, from its top
};

// Blurs the window's top `rows` (framebuffer pixels) of the default framebuffer, `width` x
// `height`, with a Gaussian of `sigma` pixels. The picture stays good until the next call.
Picture blur(int width, int height, int rows, float sigma);

void release();

}  // namespace openrac::renderer::ui::backdrop_blur
