#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(location=3) in vec4 colour;
uniform mat4 clip;
out vec3 v_position,v_normal;
out vec2 v_uv;
out vec4 v_colour;
void main() {
    v_position=position;v_normal=normal;v_uv=uv;v_colour=colour;
    gl_Position=clip*vec4(position,1);
}
