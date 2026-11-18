#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// The caller uses additive RGB blending and keeps the world's alpha unchanged.
layout(location = 0) out vec4 result;
uniform sampler2D tex_T0;
uniform vec2 output_size;
uniform float strength;

void main() {
    vec2 uv = gl_FragCoord.xy / output_size;
    result = vec4(texture(tex_T0, uv).rgb * strength, 0.0);
}
