/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * A native material: how a draw of the world's effects blends, tests alpha and depth, writes, and
 * samples its image, in the window's own terms (docs/port/TEXTURES.md, section 4). No field is a
 * word of the console's graphics chip. Host C fills one where it makes the record, from state the
 * record's own code sets, or that a capture of the game's own register sets proves (Up Your
 * Arsenal's alpha-fail masks: rac3-pal/host/effect_state.h); state a draw only inherits from the
 * display list around it is not a material's to guess (keep such a draw on openrac_game_effect_quad
 * and its marker).
 *
 * Colours, alphas and weights are in the games' units: 0x80 = 1.0.
 */
#ifndef OPENRAC_MATERIAL_H
#define OPENRAC_MATERIAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* openrac_material::weight, the w of the blend: the drawn pixel's alpha, the alpha already in the
 * target there, or `constant`. */
#define OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA 0
#define OPENRAC_MATERIAL_WEIGHT_DEST_ALPHA 1
#define OPENRAC_MATERIAL_WEIGHT_CONSTANT 2

/* openrac_material::alpha_test: how the drawn pixel's alpha compares with alpha_reference for the
 * pixel to pass. ALWAYS is no test; NEVER fails every pixel (only alpha_fail's writes are made). */
#define OPENRAC_MATERIAL_TEST_NEVER 0
#define OPENRAC_MATERIAL_TEST_ALWAYS 1
#define OPENRAC_MATERIAL_TEST_LESS 2
#define OPENRAC_MATERIAL_TEST_LESS_EQUAL 3
#define OPENRAC_MATERIAL_TEST_EQUAL 4
#define OPENRAC_MATERIAL_TEST_GREATER_EQUAL 5
#define OPENRAC_MATERIAL_TEST_GREATER 6
#define OPENRAC_MATERIAL_TEST_NOT_EQUAL 7

/* openrac_material::alpha_fail: what a pixel that fails the alpha test still writes: nothing; its
 * colour and alpha; its depth (with OPENRAC_MATERIAL_DEPTH_WRITE); its colour without alpha. */
#define OPENRAC_MATERIAL_FAIL_NOTHING 0
#define OPENRAC_MATERIAL_FAIL_COLOUR 1
#define OPENRAC_MATERIAL_FAIL_DEPTH 2
#define OPENRAC_MATERIAL_FAIL_RGB 3

/* openrac_material::depth_test, against the depth the world left: not tested, never passes,
 * passes nearer the camera or as near, passes nearer only. */
#define OPENRAC_MATERIAL_DEPTH_ALWAYS 0
#define OPENRAC_MATERIAL_DEPTH_NEVER 1
#define OPENRAC_MATERIAL_DEPTH_NEARER_OR_EQUAL 2
#define OPENRAC_MATERIAL_DEPTH_NEARER 3

/* openrac_material::combine, how a texel and the vertex colour make the drawn colour: modulated
 * (texel times colour, 0x80 = 1.0), the texel alone (decal), modulated plus the vertex alpha
 * (highlight), or that with the texel's alpha alone (highlight 2). */
#define OPENRAC_MATERIAL_MODULATE 0
#define OPENRAC_MATERIAL_DECAL 1
#define OPENRAC_MATERIAL_HIGHLIGHT 2
#define OPENRAC_MATERIAL_HIGHLIGHT2 3

/* openrac_material::writes, the target's channels a draw writes. */
#define OPENRAC_MATERIAL_WRITE_RED 1u
#define OPENRAC_MATERIAL_WRITE_GREEN 2u
#define OPENRAC_MATERIAL_WRITE_BLUE 4u
#define OPENRAC_MATERIAL_WRITE_ALPHA 8u
#define OPENRAC_MATERIAL_WRITE_COLOUR 15u

/* openrac_material::flags: blended by the coefficients (else the drawn colour replaces the
 * target's); depth written where drawn; the image's alpha counts (else the vertex alpha alone);
 * magnified and minified bilinearly (else texel by texel); the image repeated past its edges
 * across (S) and down (T), else held at them; no image at all, the vertex colours alone. */
#define OPENRAC_MATERIAL_BLEND 0x01u
#define OPENRAC_MATERIAL_DEPTH_WRITE 0x02u
#define OPENRAC_MATERIAL_TEXTURE_ALPHA 0x04u
#define OPENRAC_MATERIAL_LINEAR_MAG 0x08u
#define OPENRAC_MATERIAL_LINEAR_MIN 0x10u
#define OPENRAC_MATERIAL_REPEAT_S 0x20u
#define OPENRAC_MATERIAL_REPEAT_T 0x40u
#define OPENRAC_MATERIAL_UNTEXTURED 0x80u
/* The image is minified through a mip chain the window makes from it (a tiled surface seen far
 * away, as water is); with OPENRAC_MATERIAL_LINEAR_MIN the levels blend. */
#define OPENRAC_MATERIAL_MIPMAP 0x100u

/*
 * With OPENRAC_MATERIAL_BLEND the drawn colour is (source_k w + source_m) Cs + (dest_k w + dest_m)
 * Cd, each k -1, 0 or 1 and each m 0 or 1, w as `weight` says: mix is 1, 0, -1, 1 (Cs w + Cd (1 -
 * w)); add 1, 0, 0, 1; subtract -1, 0, 0, 1; multiply 0, 0, -1, 1; keep 0, 0, 0, 1 (the target's
 * colour stays; a mask layer that writes only its alpha). The alpha written is the drawn pixel's
 * own (the image's with OPENRAC_MATERIAL_TEXTURE_ALPHA, times the vertex alpha), never blended.
 * `constant` is w with OPENRAC_MATERIAL_WEIGHT_CONSTANT, 0x80 = 1.0, up to 0xFF.
 */
typedef struct openrac_material {
    int8_t source_k;
    uint8_t source_m;
    int8_t dest_k;
    uint8_t dest_m;
    uint8_t weight;
    uint8_t constant;
    uint8_t alpha_test;
    uint8_t alpha_reference;
    uint8_t alpha_fail;
    uint8_t depth_test;
    uint8_t combine;
    uint8_t writes;
    uint32_t flags;
} openrac_material;

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_MATERIAL_H */
