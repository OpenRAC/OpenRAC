#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
uniform sampler2D tex_T0;
uniform vec2 display_size;
uniform vec2 jitter;
// The late GIF stream includes world-space effects as well as ordinary HUD.
// Restore unjittered display-space depth after the renderer's HUD depth clear.
void main() {
    ivec2 size = textureSize(tex_T0, 0);
    vec2 sample_position = gl_FragCoord.xy / display_size * vec2(size) + jitter;
    ivec2 at = clamp(ivec2(floor(sample_position)), ivec2(0), size - 1);
    gl_FragDepth = texelFetch(tex_T0, at, 0).r;
}
