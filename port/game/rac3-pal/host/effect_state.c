/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal: the draw state its C sets for the world's effects, as native values at each set
 * (effect_state.h). The registers are the graphics chip's TEST_1, ZBUF_1, ALPHA_1, FBA_1 and FRAME_1
 * (A+D addresses 0x47, 0x4E, 0x42, 0x4A, 0x4C); each value is read once, where VU1_addGSregister is
 * given it, and kept in openrac_material's terms. Nothing here walks the display list: what else
 * the list holds is known only by where it ends, and a gap there forgets what was known.
 */
#include "effect_state.h"

#include <string.h>

enum {
    kTex0 = 0x06,
    kClamp1 = 0x08,
    kTex1 = 0x14,
    kAlpha1 = 0x42,
    kTest1 = 0x47,
    kFba1 = 0x4A,
    kFrame1 = 0x4C,
    kZbuf1 = 0x4E,
};

/* TEST_1's ATST and AFAIL, ZTST: openrac_material's values for each. */
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
/* TEX0_1's TFX: the texture functions. */
static const uint8_t kCombines[4] = {
    OPENRAC_MATERIAL_MODULATE,
    OPENRAC_MATERIAL_DECAL,
    OPENRAC_MATERIAL_HIGHLIGHT,
    OPENRAC_MATERIAL_HIGHLIGHT2,
};

/* TEST_1: ATE (bit 0), ATST (1-3), AREF (4-11), AFAIL (12-13), DATE (14), ZTE (16), ZTST (17-18).
 * Without ATE there is no alpha test; without ZTE no depth test. */
static void take_test(rac3_effect_state* s, uint64_t v) {
    const int ate = (int)(v & 1u);
    s->alpha_test = ate ? kAlphaTests[(v >> 1) & 7u] : OPENRAC_MATERIAL_TEST_ALWAYS;
    s->alpha_reference = ate ? (uint8_t)(v >> 4) : 0;
    s->alpha_fail = ate ? kAlphaFails[(v >> 12) & 3u] : OPENRAC_MATERIAL_FAIL_NOTHING;
    s->dest_alpha_test = (uint8_t)((v >> 14) & 1u);
    s->depth_test =
        ((v >> 16) & 1u) != 0 ? kDepthTests[(v >> 17) & 3u] : OPENRAC_MATERIAL_DEPTH_ALWAYS;
}

/* ALPHA_1's (A - B) C + D as the material's (source_k w + source_m) Cs + (dest_k w + dest_m) Cd:
 * A, B and D 0 Cs, 1 Cd, 2 zero; C 0 As, 1 Ad, 2 FIX (bits 32-39). 0 for a reserved operand (3). */
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

static void take_alpha(rac3_effect_state* s, uint64_t v) {
    openrac_material m;
    memset(&m, 0, sizeof(m));
    s->blend_valid = (uint8_t)blend_terms(v, &m);
    s->source_k = m.source_k;
    s->source_m = m.source_m;
    s->dest_k = m.dest_k;
    s->dest_m = m.dest_m;
    s->weight = m.weight;
    s->constant = m.constant;
}

/* FRAME_1: PSM (bits 24-29), FBMSK (32-63, a set bit keeps the target's). In 32-bit colour (PSM 0)
 * each byte of the mask is a channel, R first: a channel is written when its byte is clear, kept
 * when set; part of a byte, or another format, has no native form. */
static void take_frame(rac3_effect_state* s, uint64_t v) {
    if (((v >> 24) & 0x3Fu) != 0) {
        s->writes = RAC3_EFFECT_WRITES_PARTIAL;
        return;
    }
    uint8_t writes = 0;
    for (unsigned k = 0; k < 4; ++k) {
        const unsigned mask = (unsigned)((v >> (32 + 8 * k)) & 0xFFu);
        if (mask != 0 && mask != 0xFFu) {
            s->writes = RAC3_EFFECT_WRITES_PARTIAL;
            return;
        }
        writes |= mask == 0 ? (uint8_t)(1u << k) : 0;
    }
    s->writes = writes;
}

void rac3_effect_state_open(rac3_effect_state* s, uint32_t list, uint32_t at) {
    memset(s, 0, sizeof(*s));
    s->list = list;
    s->end = at;
}

void rac3_effect_state_close(rac3_effect_state* s) {
    memset(s, 0, sizeof(*s));
}

void rac3_effect_state_set(
    rac3_effect_state* s, uint32_t list, uint32_t before, uint32_t after, int reg, uint64_t value
) {
    if (s->list == 0 || list != s->list) {
        return;
    }
    if (before != s->end) {
        s->known = 0;
    }
    s->end = after;
    switch (reg) {
        case kTest1:
            take_test(s, value);
            s->known |= RAC3_EFFECT_TEST;
            break;
        case kZbuf1:
            /* ZMSK (bit 32) keeps the depth there. */
            s->depth_write = ((value >> 32) & 1u) == 0;
            s->known |= RAC3_EFFECT_ZBUF;
            break;
        case kAlpha1:
            take_alpha(s, value);
            s->known |= RAC3_EFFECT_ALPHA;
            break;
        case kFba1:
            s->alpha_top = (uint8_t)(value & 1u);
            s->known |= RAC3_EFFECT_FBA;
            break;
        case kFrame1:
            take_frame(s, value);
            s->known |= RAC3_EFFECT_FRAME;
            break;
        case kTex0:
            s->tex0 = value;
            s->known |= RAC3_EFFECT_TEX0;
            break;
        case kClamp1:
            s->clamp = value;
            s->known |= RAC3_EFFECT_CLAMP;
            break;
        case kTex1:
            s->tex1 = value;
            s->known |= RAC3_EFFECT_TEX1;
            break;
        default:
            /* Another register, alone in its packet: what is known stays known. */
            break;
    }
}

void rac3_effect_state_quad(rac3_effect_state* s) {
    s->known &= ~RAC3_EFFECT_ALPHA;
}

int rac3_effect_mask_material(
    const rac3_effect_state* s,
    uint32_t list,
    uint32_t at,
    const rac3_quad_words* words,
    openrac_material* out
) {
    /* The caller's TEST_1 set, the last thing in the open frame's list. */
    if (s->list == 0 || list != s->list || at != s->end || (s->known & RAC3_EFFECT_TEST) == 0) {
        return 0;
    }
    if (s->alpha_test != OPENRAC_MATERIAL_TEST_NEVER
        || s->alpha_fail != OPENRAC_MATERIAL_FAIL_COLOUR || s->dest_alpha_test != 0) {
        return 0;
    }
    /* A known FBA_1 that forces the written alpha's top bit, a known FRAME_1 that masks part of a
     * channel: neither has a native form. */
    if (((s->known & RAC3_EFFECT_FBA) != 0 && s->alpha_top != 0)
        || ((s->known & RAC3_EFFECT_FRAME) != 0 && s->writes == RAC3_EFFECT_WRITES_PARTIAL)) {
        return 0;
    }
    openrac_material m;
    memset(&m, 0, sizeof(m));
    if (!blend_terms(words->alpha, &m)) {
        return 0;
    }
    /* CLAMP_1's WMS and WMT: 0 repeats, 1 holds at the edge; the region modes have no native form. */
    const unsigned wms = (unsigned)(words->clamp & 3u);
    const unsigned wmt = (unsigned)((words->clamp >> 2) & 3u);
    if (wms > 1 || wmt > 1) {
        return 0;
    }
    m.alpha_test = OPENRAC_MATERIAL_TEST_NEVER;
    m.alpha_reference = s->alpha_reference;
    m.alpha_fail = OPENRAC_MATERIAL_FAIL_COLOUR;
    m.depth_test = s->depth_test;
    /* TEX0_1's TFX (bits 35-36) and TCC (bit 34); TEX1_1's MMAG (bit 5) and MMIN (6-8), whose 1, 4
     * and 5 minify linearly. */
    m.combine = kCombines[(words->tex0 >> 35) & 3u];
    const unsigned mmin = (unsigned)((words->tex1 >> 6) & 7u);
    m.writes = (s->known & RAC3_EFFECT_FRAME) != 0 ? s->writes : OPENRAC_MATERIAL_WRITE_COLOUR;
    m.flags = OPENRAC_MATERIAL_BLEND;
    m.flags |= ((words->tex0 >> 34) & 1u) != 0 ? OPENRAC_MATERIAL_TEXTURE_ALPHA : 0u;
    m.flags |= ((words->tex1 >> 5) & 1u) != 0 ? OPENRAC_MATERIAL_LINEAR_MAG : 0u;
    m.flags |= mmin == 1 || mmin == 4 || mmin == 5 ? OPENRAC_MATERIAL_LINEAR_MIN : 0u;
    m.flags |= wms == 0 ? OPENRAC_MATERIAL_REPEAT_S : 0u;
    m.flags |= wmt == 0 ? OPENRAC_MATERIAL_REPEAT_T : 0u;
    *out = m;
    return 1;
}

void rac3_effect_state_advance(
    rac3_effect_state* s, uint32_t list, uint32_t before, uint32_t after
) {
    if (s->list != 0 && list == s->list && before == s->end) {
        s->end = after;
    }
}

int rac3_effect_strip_material(
    const rac3_effect_state* s, uint32_t list, uint32_t at, openrac_material* out, uint64_t* texture
) {
    const uint32_t need = RAC3_EFFECT_TEST | RAC3_EFFECT_ALPHA | RAC3_EFFECT_CLAMP
                          | RAC3_EFFECT_TEX0 | RAC3_EFFECT_TEX1;
    if (s->list == 0 || list != s->list || at != s->end || (s->known & need) != need
        || !s->blend_valid || s->dest_alpha_test != 0) {
        return 0;
    }
    if (((s->known & RAC3_EFFECT_FBA) != 0 && s->alpha_top != 0)
        || ((s->known & RAC3_EFFECT_FRAME) != 0 && s->writes == RAC3_EFFECT_WRITES_PARTIAL)) {
        return 0;
    }
    /* CLAMP_1's WMS and WMT: 0 repeats, 1 holds at the edge; the region modes have no native form. */
    const unsigned wms = (unsigned)(s->clamp & 3u);
    const unsigned wmt = (unsigned)((s->clamp >> 2) & 3u);
    if (wms > 1 || wmt > 1) {
        return 0;
    }
    openrac_material m;
    memset(&m, 0, sizeof(m));
    m.source_k = s->source_k;
    m.source_m = s->source_m;
    m.dest_k = s->dest_k;
    m.dest_m = s->dest_m;
    m.weight = s->weight;
    m.constant = s->constant;
    m.alpha_test = s->alpha_test;
    m.alpha_reference = s->alpha_reference;
    m.alpha_fail = s->alpha_fail;
    m.depth_test = s->depth_test;
    m.combine = kCombines[(s->tex0 >> 35) & 3u];
    m.writes = (s->known & RAC3_EFFECT_FRAME) != 0 ? s->writes : OPENRAC_MATERIAL_WRITE_COLOUR;
    const unsigned mmin = (unsigned)((s->tex1 >> 6) & 7u);
    m.flags = OPENRAC_MATERIAL_BLEND;
    m.flags |= ((s->tex0 >> 34) & 1u) != 0 ? OPENRAC_MATERIAL_TEXTURE_ALPHA : 0u;
    m.flags |= ((s->tex1 >> 5) & 1u) != 0 ? OPENRAC_MATERIAL_LINEAR_MAG : 0u;
    m.flags |= mmin == 1 || mmin == 4 || mmin == 5 ? OPENRAC_MATERIAL_LINEAR_MIN : 0u;
    m.flags |= wms == 0 ? OPENRAC_MATERIAL_REPEAT_S : 0u;
    m.flags |= wmt == 0 ? OPENRAC_MATERIAL_REPEAT_T : 0u;
    /* The chain's rule for a strip drawn before the mobys (before_mobys_writes_z over what the
     * registers set): not a layer whose alpha test never passes and keeps only the colour; a liquid
     * (ALPHA (Cs - Cd) FIX + Cd, FIX over 0x60) or an alpha test that keeps some pixels writes it. */
    const int layer_only = s->alpha_test == OPENRAC_MATERIAL_TEST_NEVER
                           && (s->alpha_fail == OPENRAC_MATERIAL_FAIL_COLOUR
                               || s->alpha_fail == OPENRAC_MATERIAL_FAIL_RGB);
    const int liquid = s->source_k == 1 && s->source_m == 0 && s->dest_k == -1 && s->dest_m == 1
                       && s->weight == OPENRAC_MATERIAL_WEIGHT_CONSTANT && s->constant > 0x60;
    const int kept_test = s->alpha_test != OPENRAC_MATERIAL_TEST_NEVER
                          && s->alpha_test != OPENRAC_MATERIAL_TEST_ALWAYS;
    if (!layer_only && (liquid || kept_test)) {
        m.flags |= OPENRAC_MATERIAL_DEPTH_WRITE;
    }
    *out = m;
    *texture = s->tex0;
    return 1;
}
