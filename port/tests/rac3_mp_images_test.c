/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Up Your Arsenal's multiplayer 2D screen (rac3-pal/host/ui_adapter.c) on memory laid out as the
 * multiplayer executable leaves it: the 2D screen, the whole screen's offset and a split-screen
 * view's corner come from the frame block, not from the offset a view moves. Made-up addresses: no
 * data of the game's. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

#include "../game/rac3-pal/host/ui_adapter.c"

uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = 0x02000000u;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;
gaddr openrac_guest_sp = 0x01F00000u;

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

/* ---- What the host C calls ---- */

static int g_program = RAC3_EXEC_BOOT;

int rac3_exec_program(void) {
    return g_program;
}

static const openrac_ui_adapter* g_adapter;

void openrac_ui_set_adapter(const openrac_ui_adapter* adapter) {
    g_adapter = adapter;
}

/* ---- The game's memory ---- */

static void put_u32(gaddr at, uint32_t v) {
    memcpy(G(at), &v, sizeof v);
}

static void put_u16(gaddr at, uint16_t v) {
    memcpy(G(at), &v, sizeof v);
}

/* The multiplayer core's frame block's screen and its 2D offset words. */
static void mp_screen_set(int width, int height, int offset_x, int offset_y) {
    put_u16(0x00228580u + 0x150u, (uint16_t)width);
    put_u16(0x00228580u + 0x152u, (uint16_t)height);
    put_u32(0x00240310u, (uint32_t)offset_x);
    put_u32(0x00240314u, (uint32_t)offset_y);
}

/* ---- Cases ---- */

static void screens(void) {
    float w = 0.0f, h = 0.0f, x = -1.0f, y = -1.0f;
    int ox = 0, oy = 0;
    rac3_ui_adapter_install();
    CHECK(g_adapter != NULL && g_adapter->screen != NULL);

    /* The boot program: its offset is its whole screen's, centred on 2048 (512 x 448). */
    g_program = RAC3_EXEC_BOOT;
    put_u32(0x001D4BD0u, 0x7000u);
    put_u32(0x001D4BD4u, 0x7200u);
    CHECK(rac3_ui_offset() == 0x001D4BD0u && rac3_ui_frame() == 0x001CFEC0u);
    g_adapter->screen(&w, &h);
    CHECK(w == 512.0f && h == 448.0f);
    rac3_ui_origin(&ox, &oy);
    CHECK(ox == 0x7000 && oy == 0x7200);
    rac3_ui_view_corner(&x, &y);
    CHECK(x == 0.0f && y == 0.0f);

    /* The multiplayer executable before its screen is set up: the offset alone, as before. */
    g_program = RAC3_EXEC_MULTIPLAYER;
    CHECK(rac3_ui_offset() == 0x00240310u && rac3_ui_frame() == 0x00228580u);
    mp_screen_set(0, 0, 0x7000, 0x7200);
    g_adapter->screen(&w, &h);
    CHECK(w == 512.0f && h == 448.0f);
    rac3_ui_origin(&ox, &oy);
    CHECK(ox == 0x7000 && oy == 0x7200);

    /* A split screen's lower view (its top left at 0, 224): the view's offset is the screen's
     * moved there; the 2D still spans the whole 512 x 448 screen, from its own top left. */
    mp_screen_set(512, 448, (2048 - 256 + 0) << 4, (2048 - 224 + 224) << 4);
    g_adapter->screen(&w, &h);
    CHECK(w == 512.0f && h == 448.0f);
    rac3_ui_origin(&ox, &oy);
    CHECK(ox == (2048 - 256) << 4 && oy == (2048 - 224) << 4);
    rac3_ui_view_corner(&x, &y);
    CHECK(x == 0.0f && y == 224.0f);

    /* A four-way split's right view of a 640 x 448 screen (its top left at 320, 0). */
    mp_screen_set(640, 448, (2048 - 320 + 320) << 4, (2048 - 224) << 4);
    g_adapter->screen(&w, &h);
    CHECK(w == 640.0f && h == 448.0f);
    rac3_ui_view_corner(&x, &y);
    CHECK(x == 320.0f && y == 0.0f);
}

int main(void) {
    openrac_guest_base = calloc(1, openrac_guest_ram_size);
    CHECK(openrac_guest_base != NULL);
    screens();
    free(openrac_guest_base);
    printf("rac3_mp_images: ok\n");
    return 0;
}
