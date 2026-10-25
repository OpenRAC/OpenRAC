#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Exact world-space source edges, transformed with the displayed camera.
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
uniform mat4 view_projection;
out vec3 marker_colour;
void main() {
    gl_Position = view_projection * vec4(position, 1.0);
    marker_colour = colour;
}
