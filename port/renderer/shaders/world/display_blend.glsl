// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/assets/shaders/display_blend.wgsl
// and the GS_ATEST_* rule of crates/rc-engine/src/gs_state.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The GS's alpha test and blend at the end of a world fragment shader. The
// frame buffer holds the chip's display bytes, so GL's blend on it is the
// GS's arithmetic: every blended world draw uses ONE, ONE_MINUS_SRC_ALPHA
// (renderer/world/gs_pass.h) on the premultiplied term written here:
//   ALPHA_1 0x44, (Cs - Cd) * As >> 7 + Cd:  (Cs * As, As), As clamped to 1
//   ALPHA_1 0x48, Cs * As >> 7 + Cd:         (Cs * As, 0)
// `cs` is the fragment colour in bytes (after MODULATE it may reach 255 from
// a 0x80 texel and a 0xff vertex colour), `a_s` its alpha in the chip's units
// (0x80 = 1.0, up to 0xff).

uniform int u_alpha_test;  // 0 none, 1 keep As >= AREF (Z written), 2 keep As < AREF (colour only)
uniform float u_aref;
uniform int u_output;  // 0 the 0x44 term, 1 the 0x48 term

// Which half of the alpha-test split this draw is: the GS tests As after the
// texture function and before the blend; AFAIL RGB_ONLY keeps the colour
// of a failing pixel and drops only its Z, hence the two draws.
bool gs_alpha_discard(float a_s) {
    return (u_alpha_test == 1 && a_s < u_aref) || (u_alpha_test == 2 && a_s >= u_aref);
}

vec4 gs_mix(vec3 cs, float a_s) {
    float a = min(a_s / 128.0, 1.0);
    return vec4(cs / 255.0 * a, a);
}

vec4 gs_add(vec3 cs, float a_s) {
    return vec4(min(floor(cs * a_s / 128.0), vec3(255.0)) / 255.0, 0.0);
}

vec4 gs_output(vec3 cs, float a_s) {
    return u_output == 1 ? gs_add(cs, a_s) : gs_mix(cs, a_s);
}

// MODULATE (TFX 0, TCC 1) in the chip's units: Cv = Ct * Cf >> 7,
// Av = At * Af >> 7, clamped to 0xff. `t` is the sampled texel (bytes / 255,
// alpha in the chip's units), `cf` the vertex colour in bytes.
vec4 gs_modulate(vec4 t, vec4 cf) {
    vec4 ct = round(t * 255.0);
    return vec4(min(floor(ct.rgb * cf.rgb / 128.0), vec3(255.0)), min(floor(ct.a * cf.a / 128.0), 255.0));
}
