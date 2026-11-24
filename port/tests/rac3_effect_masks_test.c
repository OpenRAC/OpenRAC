/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's alpha-fail masks through the levels' own host C (port/game/rac3-pal/host:
 * render.c, textures.c, effect_state.c) on a synthetic level in test memory: DrawFrame's wrap
 * opens the capture, EffectTex0's wrap keeps the image of the mask's texture, VU1_addGSregister's
 * wrap appends the caller's TEST_1 0x31001 and DrawWorldQuad draws the record. Without the switch
 * the quad goes to openrac_game_material_effect_quad; with OPENRAC_UI_SKIP naming
 * rac3_effect_masks, through openrac_game_effect_quad and its marker, while the capture runs on.
 * The game's own routines (the __game bodies) are the test's: VU1_addGSregister's appends its
 * 48-byte packet, EffectTex0's hands out a handle. No disc data is used.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/frame.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

/* Veldin's places the host C reads (render.c, textures.c; the level loaded is Veldin, so its
 * addresses are its own): the list pointer, the effect table, its count and base. */
#define LIST_POINTER 0x001E1068u
#define EFFECTS 0x001E7C80u
#define EFFECT_COUNT 0x001DF980u
#define LIST 0x00100000u
#define RECORD 0x00200000u
#define TEXELS 0x00210000u

/* The mask's effect texture (EffectTex0's n) and the handle the game's EffectTex0 hands out. */
#define MASK_TEXTURE 0x3B
#define MASK_HANDLE 0x0000001400004321ull

/* The refraction mask's TEST_1, the draw callbacks' (DrawFrame's 0x5360B), the record's words. */
#define MASK_TEST 0x31001ull
#define FRAME_TEST 0x5360Bull
#define MASK_ALPHA 0x800000004Aull
#define LINEAR 0xFF9000000260ull

#define TEST_1 0x47

uint8_t* openrac_guest_base;

/* What reached the window: native materials, older quads, their markers and anchors. */
static int s_materials, s_quads, s_marked;
static openrac_game_material_quad s_material;
static openrac_game_quad s_quad;
static gaddr s_anchor;
static const char* s_skip;

/* The level's draw callbacks: what DrawFrame's body (the test's) runs, and the TEST each sets. */
static uint64_t s_test;
static int s_quads_per_frame;
static const char* s_skip_after_first;

gaddr rac3_draw_owner;
gaddr rac3_draw_callback;
unsigned rac3_draw_ordinal;

gaddr openrac_relocate_level_data(int from, gaddr address) {
    (void)from;
    return address;
}

const char* openrac_env_string(const char* name) {
    return strcmp(name, "OPENRAC_UI_SKIP") == 0 ? s_skip : NULL;
}

int openrac_game_material_effect_quad(const openrac_game_material_quad* quad) {
    s_material = *quad;
    ++s_materials;
    return 1;
}

void openrac_game_effect_quad(const openrac_game_quad* quad) {
    s_quad = *quad;
    ++s_quads;
}

uint32_t openrac_game_effect_quad_marker(void) {
    return 0x5u;
}

void openrac_frame_anchor(gaddr list_cursor) {
    s_anchor = list_cursor;
}

void openrac_frame_effect_quad(uint32_t marker) {
    s_marked += marker == 0x5u;
}

/* The wrapped host C's entries the test calls (render.c, textures.c). */
void func_L01_004C5B90(int reg, unsigned long long value);
void func_L01_00403440(int view);
long long func_L01_00404040(int n);
int func_L01_00407E28(int a0, int a1);

/* The game's VU1_addGSregister: its packet (CNT, DIRECT, GIF tag, the A+D pair) at the list
 * pointer, which moves past it. */
void func_L01_004C5B90__game(int reg, unsigned long long value) {
    const gaddr at = GREF(gaddr, LIST_POINTER);
    memset(G(at), 0, 48);
    memcpy(G(at + 32), &value, 8);
    GREF(uint32_t, at + 40) = (uint32_t)reg;
    GREF(gaddr, LIST_POINTER) = at + 48;
}

/* The game's EffectTex0: the handle of effect texture n (the frame buffer's for -1). */
long long func_L01_00404040__game(int n) {
    return n == MASK_TEXTURE ? (long long)MASK_HANDLE : 0x0000001400005000ll;
}

/* A mask's draw callback as func_L01_00332160 draws one: its record from EffectTex0's handle (as
 * func_003D93B0 fills it), its own TEST_1 set (when `own_test`), then DrawWorldQuad with nothing
 * between. */
static void draw_mask(int own_test) {
    uint8_t* r = G(RECORD);
    memset(r, 0, 0x90);
    for (int k = 0; k < 4; ++k) {
        float corner[4] = {(float)(k & 1), (float)(k >> 1), 0.0f, 1.0f};
        memcpy(r + 16 * k, corner, sizeof corner);
        GREF(uint32_t, RECORD + 0x40u + 4u * (gaddr)k) = 0x80808080u;
    }
    const unsigned long long tex0 = (unsigned long long)func_L01_00404040(MASK_TEXTURE);
    const unsigned long long tex1 = LINEAR, alpha = MASK_ALPHA, clamp = 0;
    memcpy(r + 0x70, &clamp, 8);
    memcpy(r + 0x78, &tex0, 8);
    memcpy(r + 0x80, &tex1, 8);
    memcpy(r + 0x88, &alpha, 8);
    if (own_test) {
        func_L01_004C5B90(TEST_1, s_test);
    }
    func_L01_00407E28((int)RECORD, 0);
}

/* The game's DrawFrame: the frame's first packet (the capture does not see it), then the draw
 * callback's quads, the first after its TEST_1 set, the others under the same TEST. */
void func_L01_00403440__game(int view) {
    (void)view;
    GREF(gaddr, LIST_POINTER) += 0x130;
    for (int i = 0; i < s_quads_per_frame; ++i) {
        draw_mask(i == 0);

        /* From the second quad on, the switch as the test says. */
        if (s_skip_after_first != NULL) {
            s_skip = s_skip_after_first;
        }
    }
}

/* What the host C reaches that this test does not run: the other renderers' window calls and the
 * other getters' game bodies. */
void openrac_frame_world(unsigned layers) {
    (void)layers;
}

void openrac_frame_world_mobys(void) {}

int openrac_frame_mobys(gaddr list, int count) {
    (void)list;
    (void)count;
    return 0;
}

void openrac_game_draw(unsigned layers) {
    (void)layers;
}

const unsigned char* openrac_game_fix(const char* key) {
    static const unsigned char off = 0;
    (void)key;
    return &off;
}

void openrac_game_image_keep(gaddr data, uint32_t bytes) {
    (void)data;
    (void)bytes;
}

void openrac_game_moby_chain(gaddr moby, gaddr marks) {
    (void)moby;
    (void)marks;
}

void openrac_game_particle(
    const uint8_t* record, uint32_t pixels, uint32_t clut, int log2_side, uint32_t id
) {
    (void)record;
    (void)pixels;
    (void)clut;
    (void)log2_side;
    (void)id;
}

float openrac_game_picture_wider(void) {
    return 1.0f;
}

void openrac_game_sky_shell(int shell, const float* matrix) {
    (void)shell;
    (void)matrix;
}

float openrac_game_view_opened(void) {
    return 1.0f;
}

void openrac_game_world_view(void) {}

void func_00397D60__game(
    int data, int address, int format, int log2_width, int log2_height, int now
) {
    (void)data, (void)address, (void)format, (void)log2_width, (void)log2_height, (void)now;
}

void func_00397980__frontbin__game(
    int data, int address, int format, int log2_width, int log2_height, int now
) {
    (void)data, (void)address, (void)format, (void)log2_width, (void)log2_height, (void)now;
}

int func_L01_004215C0__game(
    int data, int address, int format, int log2_width, int log2_height, int now
) {
    (void)data, (void)address, (void)format, (void)log2_width, (void)log2_height, (void)now;
    return 0;
}

unsigned long long func_00396E18__game(int n) {
    return (unsigned long long)n;
}

unsigned long long func_00396A38__frontbin__game(int n) {
    return (unsigned long long)n;
}

unsigned long long func_L01_00420080__game(int n) {
    return (unsigned long long)n;
}

long long func_00389978__game(int n) {
    return n;
}

long long func_00389598__frontbin__game(int n) {
    return n;
}

unsigned long long func_0039B088__game(int header) {
    return (unsigned long long)header;
}

long long func_0039ACA8__frontbin__game(gaddr header) {
    return (long long)header;
}

unsigned long long func_L01_0042D898__game(int header) {
    return (unsigned long long)header;
}

/* One DrawFrame drawing `quads` quads under `test`, OPENRAC_UI_SKIP `skip` (from the second quad
 * on `later`, when given). */
static void frame(uint64_t test, int quads, const char* skip, const char* later) {
    s_materials = s_quads = s_marked = 0;
    s_anchor = 0;
    s_test = test;
    s_quads_per_frame = quads;
    s_skip = skip;
    s_skip_after_first = later;
    GREF(gaddr, LIST_POINTER) = LIST;
    func_L01_00403440(0);
}

int main(void) {
    openrac_guest_base = calloc(1, 0x00400000u);
    CHECK(openrac_guest_base != NULL);

    /* Veldin's effect table: entry 0x3B an 8-bit 64 x 64 image, its pixels and palette 0x10 and
     * 0x50 64-byte units past the table's base. */
    GREF(int32_t, EFFECT_COUNT) = 0x60;
    GREF(gaddr, EFFECT_COUNT + 4u) = TEXELS;
    const gaddr entry = EFFECTS + 16u * MASK_TEXTURE;
    GREF(uint16_t, entry + 8u) = 0x10;
    GREF(uint16_t, entry + 0xAu) = 0x50;
    GREF(uint8_t, entry + 0xCu) = 6;
    GREF(uint8_t, entry + 0xDu) = 6;
    GREF(int16_t, entry + 0xEu) = 0x13;

    /* Without the switch: the native material, no older quad, no marker. */
    frame(MASK_TEST, 1, NULL, NULL);
    CHECK(s_materials == 1 && s_quads == 0 && s_marked == 0);
    const openrac_material* m = &s_material.material;
    CHECK(
        m->alpha_test == OPENRAC_MATERIAL_TEST_NEVER
        && m->alpha_fail == OPENRAC_MATERIAL_FAIL_COLOUR
    );
    CHECK(
        m->depth_test == OPENRAC_MATERIAL_DEPTH_ALWAYS
        && (m->flags & OPENRAC_MATERIAL_DEPTH_WRITE) == 0
    );
    CHECK(m->source_k == 0 && m->source_m == 0 && m->dest_k == 0 && m->dest_m == 1);
    CHECK(
        s_material.image.pixels == TEXELS + 64u * 0x10
        && s_material.image.palette == TEXELS + 64u * 0x50
    );
    CHECK(
        s_material.image.width == 64 && s_material.image.height == 64 && s_material.image.bits == 8
    );
    CHECK(s_material.rgba[0] == 0x80808080u && s_material.corner[3][1] == 1.0f);

    /* With the switch: the older call with the record's words, and its marker anchored where the
     * mask's TEST packet ends. */
    frame(MASK_TEST, 1, "rac3_effect_masks", NULL);
    CHECK(s_materials == 0 && s_quads == 1 && s_marked == 1);
    CHECK(s_anchor == LIST + 0x130u + 48u);
    CHECK(s_quad.tex0 == MASK_HANDLE && s_quad.alpha == MASK_ALPHA && s_quad.tex1 == LINEAR);

    /* Among other names, as the 2D routines' are listed; another name alone leaves the masks. */
    frame(MASK_TEST, 1, "func_001F5800,rac3_effect_masks", NULL);
    CHECK(s_materials == 0 && s_quads == 1 && s_marked == 1);
    frame(MASK_TEST, 1, "func_001F5800", NULL);
    CHECK(s_materials == 1 && s_quads == 0 && s_marked == 0);

    /* The capture runs on under the switch: the first quad the older way, the switch then off,
     * and the next quad under the same TEST is native. */
    frame(MASK_TEST, 2, "rac3_effect_masks", "");
    CHECK(s_quads == 1 && s_marked == 1 && s_materials == 1);

    /* A quad that is not a mask (the draw callbacks' TEST) keeps its older way without the switch,
     * and so does a mask drawn outside DrawFrame. */
    frame(FRAME_TEST, 1, NULL, NULL);
    CHECK(s_materials == 0 && s_quads == 1 && s_marked == 1);
    s_materials = s_quads = s_marked = 0;
    s_skip = NULL;
    s_test = MASK_TEST;
    draw_mask(1);
    CHECK(s_materials == 0 && s_quads == 1 && s_marked == 1);

    free(openrac_guest_base);
    puts("rac3_effect_masks: ok");
    return 0;
}
