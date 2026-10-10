#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/assets/shaders/tfrag.wgsl:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Terrain pixels: GS TFX = MODULATE, then the GS fog (PRIM FGE), no lighting.
// MODULATE (TCC 1): Cv = Ct * Cf >> 7, Av = At * Af >> 7, clamped to 0xff; the
// texture holds the raw texel alpha (0..0x80). The tfrag pass runs with
// ALPHA_1 = (Cs - Cd) * As >> 7 + Cd and TEST_1 0x5360b (alpha GEQUAL 0x60,
// AFAIL RGB_ONLY): renderer/world/gs_pass.h draws a batch once or twice and
// tells this shader which half it is.

in vec2 v_uv;
noperspective in vec4 v_colour;
noperspective in float v_fog;
in float v_depth;
flat in uint v_tint;
flat in float v_k;

uniform sampler2D tex_T0;
uniform float u_max_level;  // MXL

layout(location = 0) out vec4 out_colour;

void main() {
    float level = gs_mip_level(v_depth, v_k, u_max_level);
    vec4 c = gs_modulate(textureLod(tex_T0, v_uv, level), v_colour);
    if (gs_alpha_discard(c.a)) {
        discard;
    }
    vec3 rgb = apply_fog(c.rgb / 255.0, v_fog) * 255.0;
    if (v_tint == 1u) {
        rgb = mix(rgb, vec3(255.0, 0.0, 0.0), 0.5);
    } else if (v_tint == 2u) {
        rgb = mix(rgb, vec3(0.0, 51.0, 255.0), 0.5);
    } else if (v_tint == 3u) {
        rgb = mix(rgb, vec3(0.0, 255.0, 0.0), 0.5);
    }
    out_colour = gs_output(rgb, c.a);
}
