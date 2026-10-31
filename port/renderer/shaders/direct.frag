#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The chip's texture function, fog and alpha test, per pixel, in the chip's
// units (colour bytes 0..255, 0x80 = 1.0 when modulating; alpha 0x80 opaque).
// Blending and the depth test are GL's fixed-function state.

in vec3 v_stq;
in vec4 v_colour;
in float v_fog;
#ifdef OPENRAC_EFFECT_MATERIAL
#define OPENRAC_EFFECT_WORLD 1
in vec3 v_world;
#endif

uniform sampler2D tex_T0;
#ifndef OPENRAC_EFFECT_FIXED
uniform sampler2D tex_T1;      // with distance_field: R the glyphs' opacity, A the distance to their edge
#endif
uniform int textured;
uniform int tcc;              // take alpha from the texture
uniform int tfx;              // 0 modulate, 1 decal, 2 highlight, 3 highlight2
uniform float tex_alpha_scale;  // 1: texture alpha in the chip's units; 0.5: 0..255
uniform int fog_enable;
uniform vec3 fog_colour;      // 0..255
uniform int alpha_test;       // 0 off, else TEST.ATST (2 LESS ... 7 NOTEQUAL)
uniform float alpha_ref;      // TEST.AREF
uniform int alpha_keep_failing;  // the second pass of the double draw
uniform int distance_field;   // the texture's alpha is a distance field (text drawn sharp)
uniform int source_scale;     // the blend's weight of Cs, applied here (GlBlend): 1 As, 2 FIX
uniform float source_fix;     // FIX / 128, unclamped

layout(location = 0) out vec4 out_colour;

#ifndef OPENRAC_EFFECT_FIXED
bool alpha_passes(float a) {
    float value = floor(a + 0.5);
    switch (alpha_test) {
        case 2: return value < alpha_ref;
        case 3: return value <= alpha_ref;
        case 4: return value == alpha_ref;
        case 5: return value >= alpha_ref;
        case 6: return value > alpha_ref;
        case 7: return value != alpha_ref;
        default: return true;
    }
}

#endif

#ifdef OPENRAC_UPSCALED_TEXTURE_COLOURS
// OPENRAC_UPSCALED_TEXTURE_COLOURS_SOURCE
#endif

void main() {
    vec4 c = v_colour;
#ifndef OPENRAC_EFFECT_FIXED
    if (textured != 0)
#endif
    {
        // S/Q, T/Q per pixel, as the chip divides.
#ifdef OPENRAC_UPSCALED_TEXTURE_COLOURS
        vec4 t = sample_upscaled_texture_colours(v_stq.xy / v_stq.z) * 255.0;
#else
        vec4 t = texture(tex_T0, v_stq.xy / v_stq.z) * 255.0;
#endif
#ifndef OPENRAC_EFFECT_FIXED
        if (distance_field != 0) {
            // The edge from the distance field, about a pixel wide at any size; inside it, the
            // glyph's own opacity (the font fades its glyphs downwards), in the chip's units.
            vec4 f = texture(tex_T1, v_stq.xy / v_stq.z);
            float w = max(fwidth(f.a) * 0.75, 1e-4);
            t.a = smoothstep(0.5 - w, 0.5 + w, f.a) * f.r * 128.0;
        } else
#endif
        {
            t.a *= tex_alpha_scale;
        }
        // Byte RGB products are bounded by 255*255=65025 (<65504); alpha remains FP32.
        if (tfx == 0) {
            c.rgb = vec3(mp_vec3(c.rgb) * mp_vec3(t.rgb) / mp_float(128));
            c.a = tcc != 0 ? c.a * t.a / 128.0 : c.a;
        } else if (tfx == 1) {
            c.rgb = t.rgb;
            c.a = tcc != 0 ? t.a : c.a;
        } else if (tfx == 2) {
            c.rgb = vec3(mp_vec3(c.rgb) * mp_vec3(t.rgb) / mp_float(128) + mp_float(c.a));
            c.a = tcc != 0 ? c.a + t.a : c.a;
        } else {
            c.rgb = vec3(mp_vec3(c.rgb) * mp_vec3(t.rgb) / mp_float(128) + mp_float(c.a));
            c.a = tcc != 0 ? t.a : c.a;
        }
    }
    // COLCLAMP: the chip clamps to 0..255.
    c = clamp(c, 0.0, 255.0);
    // Active volumetric transport replaces authored strip fog rather than applying it twice.
    if (fog_enable != 0
#ifdef OPENRAC_EFFECT_ATMOSPHERE
        && atmosphere_enabled == 0
#endif
    ) {
        // Alpha and alpha-test arithmetic remain FP32.
        c.rgb = vec3(mix(mp_vec3(fog_colour), mp_vec3(c.rgb), mp_float(v_fog)));
    }
#ifndef OPENRAC_EFFECT_FIXED
    // A pixel that fails the test writes nothing.
    bool failed = alpha_test != 0 && alpha_passes(c.a) == (alpha_keep_failing != 0);
#ifdef OPENRAC_SAMPLE_MASK
    // On a multisampled frame its samples are masked out: with discard there, Intel Arc's driver
    // lost whole draws (menu text).
    gl_SampleMask[0] = failed ? 0 : -1;
#else
    // Elsewhere it is discarded and gl_SampleMask is never written: on one sample a pixel, Adreno
    // drew a flickering checkerboard over the menus' panels when it was.
    if (failed) {
        discard;
    }
#endif
#endif
    // Alpha 0x80 is 1.0 for blending.
#ifdef OPENRAC_EFFECT_MATERIAL
    c.rgb = effect_material(c.rgb / 255.0) * 255.0;
#endif
#ifdef OPENRAC_EFFECT_COMPOSITE
    if (effect_has_composite()) {
        vec3 medium_transmittance = vec3(1), medium_scatter = vec3(0);
#ifdef OPENRAC_EFFECT_ATMOSPHERE
        medium_transmittance = atmosphere_apply(vec3(1), v_world, false);
        medium_scatter = atmosphere_apply(vec3(0), v_world, atmosphere_inscatter != 0);
#endif
        out_colour = effect_composite(medium_transmittance, medium_scatter);
        return;
    }
#endif
#ifdef OPENRAC_EFFECT_ATMOSPHERE
    c.rgb = atmosphere_apply(c.rgb / 255.0, v_world, atmosphere_inscatter != 0) * 255.0;
#endif
    // Weights above 1 (As or FIX up to 0xFF) that GL's blend factors would clamp.
    if (source_scale == 1) {
        c.rgb = min(c.rgb * c.a / 128.0, 255.0);
    } else if (source_scale == 2) {
        c.rgb = min(c.rgb * source_fix, 255.0);
    }
    out_colour = vec4(c.rgb / 255.0, c.a / 128.0);
}
