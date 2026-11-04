#version 410 core

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// One patch of heat haze: the picture behind it (a copy) read a little to the side, the offset
// waving with time and fading out to the patch's round edge.
uniform sampler2D tex_T0;
uniform vec4 patch_wave;  // its offset at most (pixels), the time (seconds), a seed, unused
in vec2 local;
out vec4 colour;
void main() {
    float r2 = dot(local, local);
    if (r2 >= 1.0) {
        discard;
    }
    float fade = (1.0 - r2) * (1.0 - r2);
    float t = patch_wave.y;
    float s = patch_wave.z;
    vec2 wave = vec2(
        sin(local.y * 7.0 + t * 9.0 + s) + 0.5 * sin(local.x * 13.0 - t * 15.0 + 2.0 * s),
        cos(local.x * 6.0 - t * 11.0 + 1.7 * s) + 0.5 * cos(local.y * 12.0 + t * 17.0 - s)
    ) / 1.5;
    ivec2 size = textureSize(tex_T0, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy + wave * patch_wave.x * fade);
    colour = texelFetch(tex_T0, clamp(pixel, ivec2(0), size - 1), 0);
}
