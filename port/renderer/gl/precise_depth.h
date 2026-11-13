// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Precise world depth: reversed Z in a 32-bit float depth buffer, with clip-space z sent to the
// window as it is (glClipControl GL_ZERO_TO_ONE), for a game that asks (GameProfile::precise_depth).
//
// The camera's projection keeps GL's form (perspective_reversed, z in -w..w): everything that reads
// it or the depth buffer back (culling, shadows, ambient occlusion, temporal upscaling, the GS-Z
// passes) is unchanged. The world's own draws (scene::LevelScene::draw_layer) use its
// zero_to_one() form instead, which puts the same depth at every distance in the window, but
// without the sum (z + w) / 2 that GL's form ends in: that sum keeps 2^-24 of the range, so a
// window depth of n / d resolved only about 1.2e-6 d^2 units at the window's near 0.05 (2.7 cm at
// 150 units), and coplanar scenery a few centimetres apart fought. In a float buffer n / d keeps
// 24 bits of its own, about 6e-8 of the distance at any distance.
#pragma once

#include "renderer/math.h"

namespace openrac::renderer {

// Asks for precise depth from the next frame buffer made on (before the first: the frame buffers
// keep the depth format they were made with).
void request_precise_depth(bool wanted);

// Whether the world's depth is precise: it was asked for and the context has glClipControl. The
// frame buffers then have a DEPTH32F_STENCIL8 depth.
bool precise_depth();

// `projection` (GL's form, z in -w..w) for glClipControl's GL_ZERO_TO_ONE: its z row is half the
// sum of its z and w rows, so that the window gets the same depth as before. For
// perspective_reversed's rows (0, 0, (f + n) / (f - n), 2 f n / (f - n)) and (0, 0, -1, 0) that is
// (0, 0, n / (f - n), f n / (f - n)), each exact in float: (f + n) / (f - n) - 1 loses nothing
// between 1 and 2.
inline Mat4 zero_to_one(const Mat4& projection) {
    Mat4 out = projection;
    for (int column = 0; column < 4; ++column) {
        out[column * 4 + 2] = 0.5f * (projection[column * 4 + 2] + projection[column * 4 + 3]);
    }
    return out;
}

}  // namespace openrac::renderer
