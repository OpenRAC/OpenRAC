// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The window blurred, as the launcher's desktop header blurs the page under it (backdrop-filter):
// the game menu lies over this (debug/menu_window.cpp game_backdrop). The rows are read from the
// default framebuffer, halved a few times and blurred with a Gaussian there, with
// renderer/gl/gl.h's calls, so it runs on an OpenGL window and on the Vulkan renderer's alike.
//
// Its GL objects are made at the first blur and freed by release(), while the context is current.

#pragma once

namespace openrac::renderer::ui::backdrop_blur {

// A blurred picture of the window's top rows: its texture, the first row the bottom (GL's way).
struct Picture {
    unsigned int texture = 0;  // 0 when nothing could be made
    int rows = 0;              // the window's rows it shows, from its top
};

// A darkening laid over the blur as it is enlarged, with the same noise against banding: an
// overlay drawn as a gradient on top banded in eight bits (stripes down the dark edges).
struct Shade {
    float r = 0.0f, g = 0.0f, b = 0.0f;  // the colour it darkens toward
    float top = 0.0f;                    // how far at the top, then down to
    float bottom = 0.0f;                 // how far at the foot
    float side = 0.0f;                   // how far more at the left and right edges
    float side_width = 0.0f;             // over this share of the width, fading inward
};

// Blurs the window's top `rows` (framebuffer pixels) of the default framebuffer, `width` x
// `height`, with a Gaussian of `sigma` pixels. The picture stays good until the next call.
Picture blur(int width, int height, int rows, float sigma, const Shade& shade = {});

void release();

}  // namespace openrac::renderer::ui::backdrop_blur
