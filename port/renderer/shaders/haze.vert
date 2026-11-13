#version 410 core

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// One patch of heat haze (renderer/effects/haze.h): a square around its centre, in clip space.
uniform vec4 patch_place;  // its centre (x, y) and half size (x, y), in normalised device units
out vec2 local;            // -1 to 1 across the square
void main() {
    const vec2 corners[6] =
        vec2[6](vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, -1), vec2(1, 1), vec2(-1, 1));
    local = corners[gl_VertexID];
    gl_Position = vec4(patch_place.xy + local * patch_place.zw, 0.0, 1.0);
}
