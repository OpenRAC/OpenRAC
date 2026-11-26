#version 410 core

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// A screen effect's pass (renderer/effects/screen_effect.h, docs/port/SCREEN_EFFECTS.md). Colours
// are the target's (0 to 1); the game's 0x80 = 1.0 units are taken out of the uniforms already.
uniform sampler2D tex_T0;  // what the pass reads: the frame's copy, or a smaller copy
uniform sampler2D tex_T1;  // putting back: the blurred half-size copy
uniform int mode;          // 0 copy, 1 blur, 2 resample, 3 add back, 4 mix back
uniform vec4 tint;         // the copy's colour; its alpha scales the frame's alpha
uniform float threshold;   // the copy keeps a pixel whose alpha times the tint's is above this
uniform float strength;    // how much of the blurred copy goes back
uniform vec2 texel;        // one texel of tex_T0, in uv
in vec2 uv;
out vec4 colour;
void main() {
    if (mode == 0) {
        // The game's alpha test on the frame's alpha (the world writes the game's alpha there,
        // 0x80 = 1.0 clamped to 1): what fails stays black.
        vec4 c = texture(tex_T0, uv);
        colour = c.a * tint.a > threshold ? vec4(min(c.rgb * tint.rgb, vec3(1.0)), 1.0) : vec4(0.0);
    } else if (mode == 1) {
        // Four reads half a texel off each corner: the 3 x 3 binomial (1 2 1) each way.
        vec2 h = 0.5 * texel;
        colour = 0.25 * (texture(tex_T0, uv + vec2(-h.x, -h.y)) + texture(tex_T0, uv + vec2(h.x, -h.y))
                         + texture(tex_T0, uv + vec2(-h.x, h.y)) + texture(tex_T0, uv + vec2(h.x, h.y)));
    } else if (mode == 2) {
        colour = texture(tex_T0, uv);
    } else {
        // Put back: added (the glow's add of the source times FIX), or mixed by the strength.
        vec4 c = texture(tex_T0, uv);
        vec3 b = texture(tex_T1, uv).rgb;
        colour = vec4(mode == 3 ? c.rgb + b * strength : mix(c.rgb, b, strength), c.a);
    }
}
