/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal: the draw state the game's C sets for its world effects, kept as native values
 * where it sets it (effect_state.c, docs/port/TEXTURES.md section 4). The capture follows one
 * display list through one DrawFrame: each VU1_addGSregister call is a set of one register, and the
 * capture knows a register only while nothing it did not see was appended to the list since that
 * register's set. Plain C over values: render.c reads the list pointer and calls these. */
#ifndef RAC3_EFFECT_STATE_H
#define RAC3_EFFECT_STATE_H

#include <stdint.h>

#include "openrac/material.h"

#ifdef __cplusplus
extern "C" {
#endif

/* rac3_effect_state::known: the registers set since the last append the capture did not see. */
#define RAC3_EFFECT_TEST 0x01u
#define RAC3_EFFECT_ZBUF 0x02u
#define RAC3_EFFECT_ALPHA 0x04u
#define RAC3_EFFECT_FBA 0x08u
#define RAC3_EFFECT_FRAME 0x10u
#define RAC3_EFFECT_CLAMP 0x20u
#define RAC3_EFFECT_TEX0 0x40u
#define RAC3_EFFECT_TEX1 0x80u

/* rac3_effect_state::writes when FRAME_1 masks part of a channel, or its format is not 32-bit
 * colour: no native write mask says that. */
#define RAC3_EFFECT_WRITES_PARTIAL 0xFFu

typedef struct rac3_effect_state {
    /* The address of the list pointer the open DrawFrame appends through (0: none open), and where
     * in the list the last set the capture saw ends. */
    uint32_t list;
    uint32_t end;
    uint32_t known;
    /* TEST_1: the alpha test, its reference and what a failing pixel writes, in openrac_material's
     * terms; 1 when the destination alpha test is on (no native form); the depth test. */
    uint8_t alpha_test;
    uint8_t alpha_reference;
    uint8_t alpha_fail;
    uint8_t dest_alpha_test;
    uint8_t depth_test;
    /* ZBUF_1: 1 when depth is written where a pixel passes. */
    uint8_t depth_write;
    /* ALPHA_1 as openrac_material's blend terms; 0 in blend_valid when it uses a reserved operand. */
    uint8_t blend_valid;
    int8_t source_k;
    uint8_t source_m;
    int8_t dest_k;
    uint8_t dest_m;
    uint8_t weight;
    uint8_t constant;
    /* FBA_1: 1 when every written alpha has its top bit set. */
    uint8_t alpha_top;
    /* FRAME_1: the channels written (OPENRAC_MATERIAL_WRITE_*), or RAC3_EFFECT_WRITES_PARTIAL. */
    uint8_t writes;
    /* CLAMP_1, TEX0_1 (the handle EffectTex0 gave) and TEX1_1 as set. */
    uint64_t clamp, tex0, tex1;
} rac3_effect_state;

/* What a DrawWorldQuad record owns: its CLAMP_1, TEX0_1, TEX1_1 and ALPHA_1 (+0x70 to +0x88). */
typedef struct rac3_quad_words {
    uint64_t clamp, tex0, tex1, alpha;
} rac3_quad_words;

/* DrawFrame starts with the list pointer at `list` reading `at`: nothing is known, whatever the
 * last frame set (its first packets, the frame's draw environment, are data the capture does not
 * read). */
void rac3_effect_state_open(rac3_effect_state* s, uint32_t list, uint32_t at);
/* DrawFrame has returned: nothing is known until the next opens. */
void rac3_effect_state_close(rac3_effect_state* s);
/* VU1_addGSregister(reg, value) appended its packet to the list at `list`, from `before` to
 * `after`. A packet that does not start where the last one the capture saw ended follows something
 * it did not see: every register is unknown again before this one is kept. */
void rac3_effect_state_set(
    rac3_effect_state* s, uint32_t list, uint32_t before, uint32_t after, int reg, uint64_t value
);
/* A DrawWorldQuad was drawn: the console's routine sends its record's ALPHA_1 with it. */
void rac3_effect_state_quad(rac3_effect_state* s);

/*
 * The material of the alpha-fail masks: a DrawWorldQuad drawn with the list pointer at `list`
 * reading `at`, right after its caller's own TEST_1 set with nothing appended between, where that
 * TEST fails every pixel's alpha test and keeps its colour and alpha (ATE, ATST NEVER, AFAIL
 * FB_ONLY, no destination alpha test: the refraction masks' 0x31001). Such a draw never writes
 * depth, so ZBUF_1 does not matter; its blend, texture function, filters and wrap are the record's.
 * 1 and `out` filled when all of that holds and the words have a native form; 0 otherwise (the
 * caller draws the quad its older way).
 */
int rac3_effect_mask_material(
    const rac3_effect_state* s,
    uint32_t list,
    uint32_t at,
    const rac3_quad_words* words,
    openrac_material* out
);

/* The strip routine (the levels' func_003D3A24) or its setup appended `before` to `after` through
 * `list`: neither sets a register, so what was known stays known when the append starts where the
 * last one the capture saw ended. */
void rac3_effect_state_advance(
    rac3_effect_state* s, uint32_t list, uint32_t before, uint32_t after
);

/*
 * The material of a world strip the strip routine would draw now, from the registers the caller set
 * just before it: TEST_1, ALPHA_1, CLAMP_1, TEX0_1 and TEX1_1 must all be known with nothing
 * appended since (`at` is the list pointer's word and must be where the last append ended). The
 * strip is blended (the routine's GIF tag has ABE and FGE on). `texture` gets TEX0_1's handle.
 * Depth is written only by the place in the list (OPENRAC_STRIP_CHAIN_DEPTH): `out->flags` has
 * OPENRAC_MATERIAL_DEPTH_WRITE when the chain's rule for a strip drawn with the liquids says so.
 * 1 when all of that holds and the words have a native form; 0 otherwise.
 */
int rac3_effect_strip_material(
    const rac3_effect_state* s, uint32_t list, uint32_t at, openrac_material* out, uint64_t* texture
);

#ifdef __cplusplus
}
#endif

#endif
