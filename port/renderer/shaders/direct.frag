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
#ifdef OPENRAC_EFFECT_PAIR
// Effect quads that take turns with two textures are one draw: v_pair picks the second one.
flat in float v_pair;
uniform sampler2D tex_T2;
#endif
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
uniform int sharp_scale;      // a picture scaled up: cubic (Catmull-Rom) instead of bilinear
uniform int source_scale;     // the blend's weight of Cs, applied here (GlBlend): 1 As, 2 FIX; 3, 4 the weight itself
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

// A picture drawn larger than it is: Catmull-Rom through five bilinear taps, which keeps edges and
// small text crisp where bilinear blurs them. Where it is not larger (a texel is at least a pixel),
// the plain sample.
vec4 sample_scaled_picture(vec2 uv) {
    vec2 size = vec2(textureSize(tex_T0, 0));
    vec2 texel = uv * size;
    if (max(length(dFdx(texel)), length(dFdy(texel))) >= 1.0) {
        return texture(tex_T0, uv);
    }
    vec2 centre = floor(texel - 0.5) + 0.5;
    vec2 f = texel - centre;
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12 = w1 + w2;
    vec2 near = (centre + w2 / w12) / size;
    vec2 before = (centre - 1.0) / size;
    vec2 after = (centre + 2.0) / size;
    vec4 sum = texture(tex_T0, vec2(near.x, before.y)) * (w12.x * w0.y)
             + texture(tex_T0, vec2(before.x, near.y)) * (w0.x * w12.y)
             + texture(tex_T0, near) * (w12.x * w12.y)
             + texture(tex_T0, vec2(after.x, near.y)) * (w3.x * w12.y)
             + texture(tex_T0, vec2(near.x, after.y)) * (w12.x * w3.y);
    float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return clamp(sum / weight, 0.0, 1.0);
}

void main() {
    vec4 c = v_colour;
    // Active volumetric transport replaces the strip's authored fog.
    bool apply_fog = fog_enable != 0;
#ifdef OPENRAC_EFFECT_ATMOSPHERE
    apply_fog = apply_fog && atmosphere_enabled == 0;
#endif
#ifdef OPENRAC_EFFECT_FOG
    bool gs_colour = apply_fog && textured != 0 && tfx == 0;
#endif
#ifndef OPENRAC_EFFECT_FIXED
    if (textured != 0)
#endif
    {
        // S/Q, T/Q per pixel, as the chip divides.
#if defined(OPENRAC_EFFECT_PAIR)
        // v_pair is flat, the same over a whole triangle, so the derivatives stay valid.
        vec4 t = (v_pair > 0.5 ? texture(tex_T2, v_stq.xy / v_stq.z)
                               : sample_upscaled_texture_colours(v_stq.xy / v_stq.z))
                 * 255.0;
#elif defined(OPENRAC_UPSCALED_TEXTURE_COLOURS)
        vec4 t = sample_upscaled_texture_colours(v_stq.xy / v_stq.z) * 255.0;
#else
        vec4 t = (sharp_scale != 0 ? sample_scaled_picture(v_stq.xy / v_stq.z)
                                   : texture(tex_T0, v_stq.xy / v_stq.z))
                 * 255.0;
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
#ifdef OPENRAC_EFFECT_FOG
            if (gs_colour) {
                // Keep the GS's byte truncation between sampling, modulation and fog.
                // Allow a small tolerance for sampler precision at integer texel values.
                c.rgb = trunc(trunc(c.rgb) * trunc(t.rgb + 0.05) / 128.0);
            } else
#endif
            {
                c.rgb = vec3(mp_vec3(c.rgb) * mp_vec3(t.rgb) / mp_float(128));
            }
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
    if (apply_fog) {
#ifdef OPENRAC_EFFECT_FOG
        if (gs_colour) {
            // v_fog normalises the byte to 255, while the GS uses a divisor of 256.
            c.rgb = trunc(mix(fog_colour, c.rgb, v_fog * (255.0 / 256.0)));
        } else
#endif
        {
            c.rgb = vec3(mix(mp_vec3(fog_colour), mp_vec3(c.rgb), mp_float(v_fog)));
        }
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
    } else if (source_scale == 3) {
        c.rgb = vec3(min(c.a / 128.0, 1.0) * 255.0);  // Cd (As + 1): As as the colour, DST_COLOR
    } else if (source_scale == 4) {
        c.rgb = vec3(min(source_fix, 1.0) * 255.0);  // Cd (FIX + 1)
    }
    out_colour = vec4(c.rgb / 255.0, c.a / 128.0);
}
