#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The effect renderer's vertices: world positions through the game's camera; the rest as the
// direct renderer's (direct.frag shades both).

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 st;
layout(location = 2) in vec4 colour;  // normalised bytes

uniform mat4 view_projection;

out vec3 v_stq;
out vec4 v_colour;  // the chip's units: 0..255, 0x80 = 1.0 when modulating
out float v_fog;

void main() {
    gl_Position = view_projection * vec4(position, 1.0);
    v_stq = vec3(st, 1.0);
    v_colour = colour * 255.0;
    v_fog = 1.0;
}
