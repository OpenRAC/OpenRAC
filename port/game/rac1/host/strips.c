/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * Hoven's and Oltanis' reflective effect lists on the native strip record (openrac_frame_strip,
 * docs/port/FRAME_RECORDER.md 3.2): draw_textured_effect_list's entries (0x20 bytes: a bounding
 * sphere, then at +0x10 the positions, +0x14 the ST, +0x18 the normals and +0x1C the vertex count)
 * each draw one strip twice, the second time with the environment ST that
 * compute_reflection_env_uvs makes and the list's second texture.
 *
 * The state the routine gives the GS is the static block it sends first (its two contexts, read
 * from the discs' data, the same in both levels): TEST depth-tested nearer-or-equal with no alpha
 * test, CLAMP repeating, TEX1 linear; context 1's ALPHA (Cs - Cd) FIX + Cd with FIX 0x80 (the
 * texture's own colour), context 2's the same with FIX 0x40 (an even mix over the first). The
 * texture function every effect texture has (GetEffectTex): modulate, with the alpha. The strip
 * is the font program's (font_queue_vu_state): GS fog on, the F line the view context's.
 *
 * The effect textures are the callers' (Hoven's 0x2C and 0x2D, Oltanis' 0x2E and 0x2F), found in
 * the effect texture table by their index; the game's getter, which pages them into the list for
 * the chip, is not called when the window draws natively. Without a window (or under
 * OPENRAC_UI_REPLAY) the game's routine runs as it was.
 */
#include <string.h>

#include "openrac/frame.h"
#include "openrac/material.h"
#include "rac1.h"

#define EFFECT_TABLE OPENRAC_DATA(RAC1_ADDR(0x0018D540u))
#define EFFECT_BASE OPENRAC_DATA(RAC1_ADDR(0x0015F560u))
#define VIEW_CONTEXT OPENRAC_DATA(RAC1_ADDR(0x0018CE00u))
#define SCRATCH_COLOURS 0x70000000u
#define SCRATCH_ST 0x70001000u
#define MOST_VERTICES 0xB3u

/* The effect texture n of the table (16 bytes each: the pixels' and the palette's offsets in units
 * of 16 at +8 and +0xA, the log2 sides as halfwords at +0xC and +0xE; always 8 bits, swizzled). */
static int effect_image(int n, openrac_game_ui_image* out) {
    const gaddr base = GREF(gaddr, EFFECT_BASE);
    if (n < 0 || n > 4095 || base == 0) {
        return 0;
    }
    const gaddr e = EFFECT_TABLE + 16u * (gaddr)n;
    const unsigned log2_width = GREF(uint16_t, e + 0xCu);
    const unsigned log2_height = GREF(uint16_t, e + 0xEu);
    if (log2_width > 10 || log2_height > 10) {
        return 0;
    }
    out->pixels = base + 16u * (gaddr)GREF(uint16_t, e + 8u);
    out->palette = base + 16u * (gaddr)GREF(uint16_t, e + 0xAu);
    out->width = (uint16_t)(1u << log2_width);
    out->height = (uint16_t)(1u << log2_height);
    out->bits = 8;
    out->swizzled = 1;
    out->reserved[0] = out->reserved[1] = 0;
    return 1;
}

/* A pass's material: (Cs - Cd) FIX / 0x80 + Cd, tested nearer-or-equal against the world and
 * written (the shadows drawn after the ice test against it), bilinear, repeated, the texture's
 * colour and alpha modulated by the vertex. */
static void pass_material(openrac_material* m, uint8_t fix) {
    memset(m, 0, sizeof *m);
    m->source_k = 1;
    m->source_m = 0;
    m->dest_k = -1;
    m->dest_m = 1;
    m->weight = OPENRAC_MATERIAL_WEIGHT_CONSTANT;
    m->constant = fix;
    m->alpha_test = OPENRAC_MATERIAL_TEST_ALWAYS;
    m->depth_test = OPENRAC_MATERIAL_DEPTH_NEARER_OR_EQUAL;
    m->combine = OPENRAC_MATERIAL_MODULATE;
    m->writes = OPENRAC_MATERIAL_WRITE_COLOUR;
    m->flags = OPENRAC_MATERIAL_DEPTH_WRITE | OPENRAC_MATERIAL_BLEND
               | OPENRAC_MATERIAL_TEXTURE_ALPHA | OPENRAC_MATERIAL_LINEAR_MAG
               | OPENRAC_MATERIAL_LINEAR_MIN | OPENRAC_MATERIAL_REPEAT_S
               | OPENRAC_MATERIAL_REPEAT_T;
}

/* The font program's fog: the F line font_queue_vu_state copies out of the view context (+0x210
 * the multiplier, +0x1AC the offset, +0x22C and +0x228 the far and near F), towards the view's fog
 * colour (+0x23C, +0x240, +0x244 as the background's 0..255). */
static void fog_line(openrac_frame_strip_desc* d) {
    float f[4];
    memcpy(&f[0], G(VIEW_CONTEXT + 0x210u), 4);
    memcpy(&f[1], G(VIEW_CONTEXT + 0x1ACu), 4);
    memcpy(&f[2], G(VIEW_CONTEXT + 0x22Cu), 4);
    memcpy(&f[3], G(VIEW_CONTEXT + 0x228u), 4);
    memcpy(d->fog, f, sizeof f);
    d->fog_colour = (GREF(uint32_t, VIEW_CONTEXT + 0x23Cu) & 0xFFu)
                    | (GREF(uint32_t, VIEW_CONTEXT + 0x240u) & 0xFFu) << 8
                    | (GREF(uint32_t, VIEW_CONTEXT + 0x244u) & 0xFFu) << 16;
    d->flags |= OPENRAC_STRIP_FOG;
}

/* The list's entries as draw_textured_effect_list draws them. 0 when the window does not take the
 * textures or the call (nothing was recorded), so the caller runs the game's. */
static int draw_effect_list(
    gaddr list, int count, int first_texture, int second_texture, gaddr cursor
) {
    openrac_game_ui_image first, second;
    if (!openrac_frame_native() || count <= 0 || !effect_image(first_texture, &first)
        || !effect_image(second_texture, &second)) {
        return 0;
    }
    uint32_t* white = (uint32_t*)G(SCRATCH_COLOURS);
    for (unsigned i = 0; i < MOST_VERTICES; ++i) {
        white[i] = 0x80808080u;
    }
    openrac_frame_strip_desc d;
    memset(&d, 0, sizeof d);
    fog_line(&d);
    d.rgba = white;
    openrac_frame_anchor_at(cursor);
    for (int i = 0; i < count; ++i, list += 0x20u) {
        if (func_L00_00200290(list, 256.0f) == -1) {
            continue;
        }
        const int n = GREF(int, list + 0x1Cu);
        if (n <= 0 || (unsigned)n > MOST_VERTICES) {
            continue;
        }
        func_L12_002677B4(
            n, (int)GREF(gaddr, list + 0x10u), (int)GREF(gaddr, list + 0x18u), (int)SCRATCH_ST
        );
        d.count = (uint32_t)n;
        d.position = (const float*)G(GREF(gaddr, list + 0x10u));
        d.st = (const float*)G(GREF(gaddr, list + 0x14u));
        pass_material(&d.material, 0x80);
        d.image = first;
        (void)openrac_frame_strip(&d);
        d.st = (const float*)G(SCRATCH_ST);
        pass_material(&d.material, 0x40);
        d.image = second;
        (void)openrac_frame_strip(&d);
    }
    return 1;
}

/* draw_level_12_effects (Hoven): 0x2F entries, textures 0x2C and 0x2D. */
void func_L12_002E8530(void) {
    if (!draw_effect_list(
            OPENRAC_LDATA(12, RAC1_LADDR(12, 0x001F5740u)),
            0x2F,
            0x2C,
            0x2D,
            OPENRAC_LDATA(12, RAC1_LADDR(12, 0x00161240u))
        )) {
        func_L12_002E8530__game();
    }
}

/* draw_level14_textured_effect_list (Oltanis): 0x13 entries, textures 0x2E and 0x2F. The effect list
 * routine is one function with a place in each level, so its render packet cursor is level 12's
 * address, relocated to the level that runs. */
void func_L14_00308EB0(void) {
    if (!draw_effect_list(
            OPENRAC_LDATA(14, RAC1_LADDR(14, 0x001F62C0u)),
            0x13,
            0x2E,
            0x2F,
            OPENRAC_LDATA(12, RAC1_LADDR(12, 0x00161240u))
        )) {
        func_L14_00308EB0__game();
    }
}
