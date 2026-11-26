/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: a level's water and its sea sheets as native strip records (openrac/frame.h,
 * openrac_frame_strip; docs/port/TEXTURES.md section 4, FRAME_RECORDER.md 3.2) instead of VU1 strip
 * packets the window decodes.
 *
 * Every strip of the game's draw callbacks goes through one hand-written routine, part_draw_glint_effect
 * (func_00281810): its arguments are the vertex count, the positions (3 floats each), the colours
 * (one word each, 0x80 = 1.0), the ST pairs and a flag that picks the VU1 program. It appends a GIF
 * tag for a gouraud, textured, fogged, alpha-blended triangle strip and three UNPACKs; the GS state
 * it draws with is the callback's own, set a moment before with VU1_addGSregister (func_002FC560):
 * TEX0_1, TEX1_1, CLAMP_1, ALPHA_1 and TEST_1 (DrawOceanWaterSheet, DrawWaterSheet), and the fog line
 * is the one font_queue_vu_state (func_00280328) put into VU1 memory from the view context (+0x210
 * the slope, +0x1AC the offset, +0x22C and +0x228 the far and near F), the fog colour the view
 * context's words at +0x230.
 *
 * Those values are read where the game sets them, as the native values of openrac_material, and kept
 * only while nothing else was appended to the display list since the last of them: a set or a strip
 * that does not begin where the last one the capture saw ended follows something it did not see, and
 * everything is unknown again (as rac3-pal/host/effect_state.c). Each draw callback starts from
 * nothing (rac2_strips_forget), so a strip never takes state from another callback's. A strip whose
 * state is all known, whose texture is an effect texture the window can read and whose callback is of
 * the lists run before the mobys (the level's water: wide_view.c) is handed to openrac_frame_strip at
 * the place of the call; anything else goes the game's way.
 */
#include <stdint.h>
#include <string.h>

#include "draw_owner.h"
#include "game_protos.h"
#include "rac2.h"
#include "game/common/lib/effect_texture.h"
#include "openrac/game_host.h"

/* The effect textures' uploads this frame: their count and the records GetEffectTex writes
 * (render.c's). */
#define TEXTURE_UPLOADS RAC2_ADDR(0x001B15FCu)
#define TEXTURE_RECORDS RAC2_ADDR(0x001B9340u)
/* The view context (UpdateViewContext's): the fog the strip program reads. */
#define VIEW_CONTEXT RAC2_ADDR(0x001B9040u)
#define VIEW_FOG_SLOPE 0x210u
#define VIEW_FOG_OFFSET 0x1ACu
#define VIEW_FOG_FAR 0x22Cu
#define VIEW_FOG_NEAR 0x228u
#define VIEW_FOG_COLOUR 0x230u

/* VU1_addGSregister's registers (A+D addresses). */
enum {
    kTex0 = 6,
    kClamp = 8,
    kTex1 = 0x14,
    kAlpha = 0x42,
    kTest = 0x47,
};

enum {
    kKnownTex0 = 1u,
    kKnownTex1 = 2u,
    kKnownClamp = 4u,
    kKnownAlpha = 8u,
    kKnownTest = 16u,
    kKnownAll = 31u,
};

/* The lists of wide_view.c's runners that run before the world's mobys: 3 and 4 (func_0027CEE0 and
 * func_0027CF58, which render.c's MobyProc follows). */
#define LIST_BEFORE_MOBYS(list) ((list) == 3u || (list) == 4u)

static struct {
    gaddr end;       /* where the last append the capture saw ended */
    uint32_t known;  /* kKnown*: the registers set since the last append it did not see */
    int fog_known;   /* the view's fog was put into VU1 memory since then */
    uint64_t tex0, tex1, clamp, alpha, test;
    uint32_t pixels, clut; /* tex0's effect texture, 0 when it has none */
    float fog[4];
    uint32_t fog_colour;
} s;

void rac2_strips_forget(void) {
    memset(&s, 0, sizeof s);
}

static gaddr cursor(void) {
    return GREF(gaddr, RAC2_LIST_CURSOR);
}

/* An append that starts where the last one ended is the capture's own; any other is unseen. */
static void saw(gaddr before) {
    if (before != s.end) {
        s.known = 0;
        s.fog_known = 0;
    }
}

void func_002FC560(unsigned int reg, unsigned long long value) {
    const gaddr before = cursor();
    func_002FC560__game(reg, value);
    saw(before);
    s.end = cursor();
    switch (reg) {
        case kTex0:
            s.tex0 = value;
            s.pixels = s.clut = 0;
            if (openrac_effect_texture(
                    value,
                    GREF(int, OPENRAC_DATA(TEXTURE_UPLOADS)),
                    OPENRAC_DATA(TEXTURE_RECORDS),
                    &s.pixels,
                    &s.clut
                )
                == 0) {
                s.pixels = s.clut = 0;
            }
            s.known |= kKnownTex0;
            break;
        case kTex1:
            s.tex1 = value;
            s.known |= kKnownTex1;
            break;
        case kClamp:
            s.clamp = value;
            s.known |= kKnownClamp;
            break;
        case kAlpha:
            s.alpha = value;
            s.known |= kKnownAlpha;
            break;
        case kTest:
            s.test = value;
            s.known |= kKnownTest;
            break;
        default:
            break;
    }
}

/* font_queue_vu_state: the view's matrices and its fog line go into VU1 memory. */
void func_00280328(void) {
    const gaddr before = cursor();
    func_00280328__game();
    saw(before);
    s.end = cursor();
    const gaddr view = OPENRAC_DATA(VIEW_CONTEXT);
    s.fog[0] = GREF(float, view + VIEW_FOG_SLOPE);
    s.fog[1] = GREF(float, view + VIEW_FOG_OFFSET);
    s.fog[2] = GREF(float, view + VIEW_FOG_FAR);
    s.fog[3] = GREF(float, view + VIEW_FOG_NEAR);
    s.fog_colour = (GREF(uint32_t, view + VIEW_FOG_COLOUR) & 0xFFu)
                   | (GREF(uint32_t, view + VIEW_FOG_COLOUR + 4u) & 0xFFu) << 8
                   | (GREF(uint32_t, view + VIEW_FOG_COLOUR + 8u) & 0xFFu) << 16;
    s.fog_known = 1;
}

/* TEST_1: ATE (bit 0), ATST (1-3), AREF (4-11), AFAIL (12-13), DATE (14), ZTE (16), ZTST (17-18). */
static const uint8_t kAlphaTests[8] = {
    OPENRAC_MATERIAL_TEST_NEVER,
    OPENRAC_MATERIAL_TEST_ALWAYS,
    OPENRAC_MATERIAL_TEST_LESS,
    OPENRAC_MATERIAL_TEST_LESS_EQUAL,
    OPENRAC_MATERIAL_TEST_EQUAL,
    OPENRAC_MATERIAL_TEST_GREATER_EQUAL,
    OPENRAC_MATERIAL_TEST_GREATER,
    OPENRAC_MATERIAL_TEST_NOT_EQUAL,
};
static const uint8_t kAlphaFails[4] = {
    OPENRAC_MATERIAL_FAIL_NOTHING,
    OPENRAC_MATERIAL_FAIL_COLOUR,
    OPENRAC_MATERIAL_FAIL_DEPTH,
    OPENRAC_MATERIAL_FAIL_RGB,
};
static const uint8_t kDepthTests[4] = {
    OPENRAC_MATERIAL_DEPTH_NEVER,
    OPENRAC_MATERIAL_DEPTH_ALWAYS,
    OPENRAC_MATERIAL_DEPTH_NEARER_OR_EQUAL,
    OPENRAC_MATERIAL_DEPTH_NEARER,
};
static const uint8_t kCombines[4] = {
    OPENRAC_MATERIAL_MODULATE,
    OPENRAC_MATERIAL_DECAL,
    OPENRAC_MATERIAL_HIGHLIGHT,
    OPENRAC_MATERIAL_HIGHLIGHT2,
};

/* ALPHA_1's (A - B) C + D as the material's terms: A, B and D 0 Cs, 1 Cd, 2 zero; C 0 As, 1 Ad, 2 FIX
 * (bits 32-39). 0 for a reserved operand. */
static int blend_terms(uint64_t v, openrac_material* m) {
    const unsigned a = (unsigned)(v & 3u), b = (unsigned)((v >> 2) & 3u);
    const unsigned c = (unsigned)((v >> 4) & 3u), d = (unsigned)((v >> 6) & 3u);
    if (a == 3 || b == 3 || c == 3 || d == 3) {
        return 0;
    }
    m->source_k = (int8_t)((a == 0) - (b == 0));
    m->dest_k = (int8_t)((a == 1) - (b == 1));
    m->source_m = d == 0;
    m->dest_m = d == 1;
    m->weight = c == 0   ? OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA
                : c == 1 ? OPENRAC_MATERIAL_WEIGHT_DEST_ALPHA
                         : OPENRAC_MATERIAL_WEIGHT_CONSTANT;
    m->constant = c == 2 ? (uint8_t)(v >> 32) : 0;
    return 1;
}

/* The strip's material from the known registers: 1 when they all have a native form. The depth is
 * written where the alpha test keeps some pixels (a surface drawn where its texture is), or by a
 * liquid's blend (ALPHA 0x64 with a FIX above 0x60), and not by a test that fails every pixel but
 * keeps the colour (a layer over the picture): what the window's chain reading decided for these
 * strips (renderer/effects/effects.cpp before_mobys_writes_z). ZBUF_1 is the draw
 * environment's, which the callbacks do not set. */
static int material_of_state(openrac_material* m) {
    memset(m, 0, sizeof *m);
    if (!blend_terms(s.alpha, m)) {
        return 0;
    }
    const uint64_t test = s.test;
    const int ate = (int)(test & 1u);
    if (((test >> 14) & 1u) != 0) {
        return 0; /* the destination alpha test has no native form */
    }
    const unsigned wms = (unsigned)(s.clamp & 3u), wmt = (unsigned)((s.clamp >> 2) & 3u);
    if (wms > 1 || wmt > 1) {
        return 0;
    }
    m->alpha_test = ate ? kAlphaTests[(test >> 1) & 7u] : OPENRAC_MATERIAL_TEST_ALWAYS;
    m->alpha_reference = ate ? (uint8_t)(test >> 4) : 0;
    m->alpha_fail = ate ? kAlphaFails[(test >> 12) & 3u] : OPENRAC_MATERIAL_FAIL_NOTHING;
    m->depth_test = ((test >> 16) & 1u) != 0 ? kDepthTests[(test >> 17) & 3u]
                                             : OPENRAC_MATERIAL_DEPTH_ALWAYS;
    m->combine = kCombines[(s.tex0 >> 35) & 3u];
    m->writes = OPENRAC_MATERIAL_WRITE_COLOUR;
    const unsigned mmin = (unsigned)((s.tex1 >> 6) & 7u);
    const unsigned mxl = (unsigned)((s.tex1 >> 2) & 7u);
    m->flags = OPENRAC_MATERIAL_BLEND;
    m->flags |= ((s.tex0 >> 34) & 1u) != 0 ? OPENRAC_MATERIAL_TEXTURE_ALPHA : 0u;
    m->flags |= ((s.tex1 >> 5) & 1u) != 0 ? OPENRAC_MATERIAL_LINEAR_MAG : 0u;
    m->flags |= mmin == 1 || mmin == 4 || mmin == 5 ? OPENRAC_MATERIAL_LINEAR_MIN : 0u;
    m->flags |= mxl > 0 && mmin >= 2 ? OPENRAC_MATERIAL_MIPMAP : 0u;
    m->flags |= wms == 0 ? OPENRAC_MATERIAL_REPEAT_S : 0u;
    m->flags |= wmt == 0 ? OPENRAC_MATERIAL_REPEAT_T : 0u;

    const int never = m->alpha_test == OPENRAC_MATERIAL_TEST_NEVER;
    const int fails_keep_colour =
        m->alpha_fail == OPENRAC_MATERIAL_FAIL_COLOUR || m->alpha_fail == OPENRAC_MATERIAL_FAIL_RGB;
    const int writes_z = !(ate && never && fails_keep_colour);
    const int liquid = (s.alpha & 0xFF) == 0x64 && ((s.alpha >> 32) & 0xFF) > 0x60;
    const int keeps_some = ate && !never && m->alpha_test != OPENRAC_MATERIAL_TEST_ALWAYS;
    if (writes_z && (liquid || keeps_some)) {
        m->flags |= OPENRAC_MATERIAL_DEPTH_WRITE;
    }
    return 1;
}

/* TEX0_1's image: the effect texture's pixels and CLUT (8 or 4 bits, a 32-bit CLUT). */
static int image_of_state(openrac_game_ui_image* image) {
    const uint64_t tex0 = s.tex0;
    const unsigned psm = (unsigned)((tex0 >> 20) & 0x3Fu);
    const unsigned log2_width = (unsigned)((tex0 >> 26) & 0xFu);
    const unsigned log2_height = (unsigned)((tex0 >> 30) & 0xFu);
    const unsigned clut_psm = (unsigned)((tex0 >> 51) & 0xFu);
    if (s.pixels == 0 || s.clut == 0 || (psm != 0x13 && psm != 0x14) || clut_psm != 0
        || log2_width > 10 || log2_height > 10) {
        return 0;
    }
    memset(image, 0, sizeof *image);
    image->pixels = s.pixels;
    image->palette = s.clut;
    image->width = (uint16_t)(1u << log2_width);
    image->height = (uint16_t)(1u << log2_height);
    image->bits = psm == 0x14 ? 4 : 8;
    image->swizzled = psm == 0x14 ? 0 : 1;
    return 1;
}

/* part_draw_glint_effect: the strip's vertices, drawn with the state the callback set. */
void func_00281810(int count, int positions, int colours, gaddr uvs, int flag) {
    const gaddr before = cursor();
    saw(before);
    openrac_material material;
    openrac_game_ui_image image;
    if (LIST_BEFORE_MOBYS(rac2_draw_list) && s.known == kKnownAll && s.fog_known && count >= 3
        && count <= (int)OPENRAC_STRIP_MOST_VERTICES && openrac_frame_native()
        && material_of_state(&material) && image_of_state(&image)) {
        openrac_frame_strip_desc desc;
        memset(&desc, 0, sizeof desc);
        desc.count = (uint32_t)count;
        desc.position = (const float*)G((gaddr)positions);
        desc.st = (const float*)G(uvs);
        desc.rgba = (const uint32_t*)G((gaddr)colours);
        desc.material = material;
        desc.image = image;
        desc.flags = OPENRAC_STRIP_LIQUID | OPENRAC_STRIP_FOG;
        memcpy(desc.fog, s.fog, sizeof desc.fog);
        desc.fog_colour = s.fog_colour;
        rac2_anchor();
        if (openrac_frame_strip(&desc) != 0) {
            return; /* nothing was appended to the list: the capture's end stays */
        }
    }
    func_00281810__game(count, positions, colours, uvs, flag);
    s.end = cursor();
}
