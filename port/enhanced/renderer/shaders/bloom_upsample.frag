#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Reconstructs the Gaussian pyramid with a small bilinear tent contribution from the next level.
layout(location = 0) out vec4 result;
uniform sampler2D tex_T0;
uniform vec2 output_size;
uniform float scatter;

void main() {
    vec2 uv = gl_FragCoord.xy / output_size;
    vec2 texel = 0.5 / vec2(textureSize(tex_T0, 0));
    vec3 low = texture(tex_T0, uv + vec2(-texel.x, -texel.y)).rgb;
    low += texture(tex_T0, uv + vec2(texel.x, -texel.y)).rgb;
    low += texture(tex_T0, uv + vec2(-texel.x, texel.y)).rgb;
    low += texture(tex_T0, uv + vec2(texel.x, texel.y)).rgb;
    result = vec4(low * (0.25 * scatter), 0.0);
}
