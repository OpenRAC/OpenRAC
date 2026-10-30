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
uniform mat4 view;
// A fogged strip's F line (EffectStrip::fog: slope per integer unit of depth, offset, lower and
// upper clamp), as the world programs compute F (world_common.glsl, fog_coefficient).
uniform vec4 fog_line;

out vec3 v_world;
out vec3 v_stq;
out vec4 v_colour;  // the chip's units: 0..255, 0x80 = 1.0 when modulating
out float v_fog;

void main() {
    v_world = position;
    vec2 uv = st;
    v_colour = colour * 255.0;
#ifdef OPENRAC_EFFECT_POSITION
    effect_vertex(v_world, uv, v_colour);
#endif
    gl_Position = view_projection * vec4(v_world, 1.0);
    v_stq = vec3(uv, 1.0);
    float depth = -(view * vec4(v_world, 1.0)).z * 1024.0;
    // Between the two intensities whichever is lower: GLSL leaves clamp undefined when the lower
    // bound is above the upper one.
    float lower = min(fog_line.z, fog_line.w), upper = max(fog_line.z, fog_line.w);
    v_fog = trunc(clamp(depth * fog_line.x + fog_line.y, lower, upper)) / 255.0;
}
