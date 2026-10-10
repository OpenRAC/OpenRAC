#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Extracted level geometry, instanced: one matrix per placed object, in the
// game's axes (Z up). The projection has reversed depth.

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 colour;
layout(location = 4) in mat4 model;  // locations 4 to 7
layout(location = 8) in vec4 tint;

uniform mat4 view_projection;

out vec2 v_uv;
out vec4 v_colour;
out vec3 v_normal;

void main() {
    gl_Position = view_projection * (model * vec4(position, 1.0));
    v_uv = uv;
    v_colour = colour * tint;
    v_normal = mat3(model) * normal;
}
