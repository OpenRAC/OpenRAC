/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Up Your Arsenal's multiplayer images and 2D screen (rac3-pal/host/textures.c, ui_adapter.c) on
 * memory laid out as the multiplayer executable leaves it: each copy of SendImage (the core's,
 * level 39's, every map's of host/mp_maps.h) keeps what it hands over and still runs the game's;
 * the kept images and the handles' images go when another executable starts over memory; the 2D
 * screen, the whole screen's offset and a split-screen view's corner come from the frame block,
 * not from the offset a view moves. Made-up addresses and pixels: no data of the game's. */
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

#include "../game/rac3-pal/host/textures.c"
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

/* What openrac_game_image_keep was given last, how often, and how often everything went. */
static gaddr g_kept_data;
static uint32_t g_kept_bytes;
static int g_keeps;
static int g_forgets;

void openrac_game_image_keep(gaddr data, uint32_t bytes) {
    g_kept_data = data;
    g_kept_bytes = bytes;
    ++g_keeps;
}

void openrac_game_images_forget(void) {
    ++g_forgets;
}

/* The game's own SendImage bodies: which copy ran, with what, and what it returned. */
static int g_sent_by;
static int g_sent[6];

static int game_sent(int copy, int a0, int a1, int a2, int a3, int a4, int a5) {
    const int args[6] = {a0, a1, a2, a3, a4, a5};
    g_sent_by = copy;
    memcpy(g_sent, args, sizeof g_sent);
    return 0x40 + copy;
}

#define CORE 1
#define MENUS 39

void func_00397D60__game(int a0, int a1, int a2, int a3, int a4, int a5) {
    game_sent(0, a0, a1, a2, a3, a4, a5);
}

void func_00397980__frontbin__game(int a0, int a1, int a2, int a3, int a4, int a5) {
    game_sent(0, a0, a1, a2, a3, a4, a5);
}

int func_L01_004215C0__game(int a0, int a1, int a2, int a3, int a4, int a5) {
    return game_sent(0, a0, a1, a2, a3, a4, a5);
}

int mp_func_00285660__game(int a0, int a1, int a2, int a3, int a4, int a5) {
    return game_sent(CORE, a0, a1, a2, a3, a4, a5);
}

int mp_func_L39_0059D6B0__game(int a0, int a1, int a2, int a3, int a4, int a5) {
    return game_sent(MENUS, a0, a1, a2, a3, a4, a5);
}

/* Each map's SendImage body, and each map's wrapper to call, from the same generated header. */
#define MP_MAP(...)
#define MP_MAP_UI(...)
#define MP_MAP_HUD(m, sprite, gradient, tex0, send_image)                                          \
    int mp_func_L##m##_##send_image##__game(int a0, int a1, int a2, int a3, int a4, int a5) {      \
        return game_sent(m, a0, a1, a2, a3, a4, a5);                                               \
    }
#define MP_MAP_VU0(...)
#include "../game/rac3-pal/host/mp_maps.h"
#undef MP_MAP_HUD

typedef int (*SendImage)(int, int, int, int, int, int);

#define MP_MAP_HUD(m, sprite, gradient, tex0, send_image) {m, mp_func_L##m##_##send_image},

static const struct {
    int map;
    SendImage send;
} kMapSends[] = {

#include "../game/rac3-pal/host/mp_maps.h"
};

/* The texture getters' game bodies: HUD texture 3's handle (its image's place 0x2A40, palette's
 * 0x3FC0, 64 x 32 PSMT8), the others none. */
#define HUD_HANDLE                                                                                 \
    (0x2A40ull | (1ull << 14) | (0x13ull << 20) | (6ull << 26) | (5ull << 30) | (0x3FC0ull << 37))

unsigned long long func_00396E18__game(int n) {
    return n == 3 ? HUD_HANDLE : 0;
}

unsigned long long func_00396A38__frontbin__game(int n) {
    (void)n;
    return 0;
}

unsigned long long func_L01_00420080__game(int n) {
    (void)n;
    return 0;
}

long long func_00389978__game(int n) {
    (void)n;
    return 0;
}

long long func_00389598__frontbin__game(int n) {
    (void)n;
    return 0;
}

long long func_L01_00404040__game(int n) {
    (void)n;
    return 0;
}

unsigned long long func_0039B088__game(int header) {
    (void)header;
    return 0;
}

long long func_0039ACA8__frontbin__game(gaddr header) {
    (void)header;
    return 0;
}

unsigned long long func_L01_0042D898__game(int header) {
    (void)header;
    return 0;
}

/* ---- The game's memory ---- */

static void put_u32(gaddr at, uint32_t v) {
    memcpy(G(at), &v, sizeof v);
}

static void put_u16(gaddr at, uint16_t v) {
    memcpy(G(at), &v, sizeof v);
}

/* A HUD bank at `tables` with texture 3: an entry naming palette 1 and image 2, the image's pixels
 * at `pixels` (64 x 32, its place 0x2A40), the palette's at `colours` (its place 0x3FC0). */
static void hud_bank(gaddr tables, gaddr pixels, gaddr colours) {
    const gaddr entries = 0x00A00000u, images = 0x00A01000u, palettes = 0x00A02000u;
    put_u32(tables, entries);
    put_u32(tables + 4u, images);
    put_u32(tables + 8u, palettes);
    put_u16(entries + 4u * 3u, 1);
    put_u16(entries + 4u * 3u + 2u, 2);
    put_u32(images + 8u * 2u, pixels);
    put_u16(images + 8u * 2u + 4u, 0x2A40);
    GREF(uint8_t, images + 8u * 2u + 6u) = 6;
    GREF(uint8_t, images + 8u * 2u + 7u) = 5;
    put_u32(palettes + 8u * 1u, colours);
    put_u16(palettes + 8u * 1u + 4u, 0x3FC0);
}

/* The multiplayer core's frame block's screen and its 2D offset words. */
static void mp_screen_set(int width, int height, int offset_x, int offset_y) {
    put_u16(0x00228580u + 0x150u, (uint16_t)width);
    put_u16(0x00228580u + 0x152u, (uint16_t)height);
    put_u32(0x00240310u, (uint32_t)offset_x);
    put_u32(0x00240314u, (uint32_t)offset_y);
}

/* ---- Cases ---- */

/* SendImage's arguments: the data, the chip's place, the format, the log2 width and height, and
 * whether it is sent at once. A copy keeps (1 << w) x (1 << h) pixels of its format's bits, then
 * runs the game's own body with the same arguments and returns what it returned. */
static void check_sent(
    SendImage send, int copy, int format, int log2_w, int log2_h, uint32_t bytes
) {
    const int keeps = g_keeps;
    const int r = send(0x00510000, 0x1A40, format, log2_w, log2_h, 1);
    CHECK(g_keeps == keeps + 1);
    CHECK(g_kept_data == 0x00510000u && g_kept_bytes == bytes);
    CHECK(g_sent_by == copy && r == 0x40 + copy);
    CHECK(g_sent[0] == 0x00510000 && g_sent[1] == 0x1A40 && g_sent[2] == format);
    CHECK(g_sent[3] == log2_w && g_sent[4] == log2_h && g_sent[5] == 1);
}

static void multiplayer_send_image(void) {
    g_program = RAC3_EXEC_MULTIPLAYER;
    /* The core's and level 39's: 8-bit indices, 4-bit indices, a 256-colour palette. */
    check_sent(mp_func_00285660, CORE, 0x13, 7, 6, 128u * 64u);
    check_sent(mp_func_L39_0059D6B0, MENUS, 0x14, 6, 5, 64u * 32u / 2u);
    check_sent(mp_func_L39_0059D6B0, MENUS, 0x00, 4, 4, 16u * 16u * 4u);
    /* Every map's own copy runs its own body. */
    const int maps = (int)(sizeof kMapSends / sizeof kMapSends[0]);
    CHECK(maps >= 14);
    for (int i = 0; i < maps; ++i) {
        CHECK(kMapSends[i].map >= 40 && kMapSends[i].map <= 55);
        check_sent(kMapSends[i].send, kMapSends[i].map, 0x1B, 8, 8, 256u * 256u);
    }
    /* A format with no pixel size, or a size past the chip's, is sent but not kept. */
    const int keeps = g_keeps;
    CHECK(mp_func_00285660(0x00510000, 0x1A40, 0x30, 7, 6, 1) == 0x40 + CORE);
    CHECK(mp_func_L39_0059D6B0(0x00510000, 0x1A40, 0x13, 11, 6, 1) == 0x40 + MENUS);
    CHECK(g_keeps == keeps);
}

static void another_executable_forgets(void) {
    g_program = RAC3_EXEC_BOOT;
    hud_bank(0x001D9EA8u, 0x00B00000u, 0x00B10000u);
    openrac_game_ui_image image;
    /* The boot program names HUD texture 3 by its handle: the image its bank holds is kept. */
    CHECK(func_00396E18(3) == HUD_HANDLE);
    CHECK(rac3_texture_image(HUD_HANDLE, 0, &image));
    CHECK(image.pixels == 0x00B00000u && image.palette == 0x00B10000u);
    CHECK(image.width == 64 && image.height == 32 && image.bits == 8 && image.swizzled == 1);
    /* The multiplayer executable starts over memory: the handle names nothing until it is named
     * again, and the images the boot program handed over are let go. */
    const int forgets = g_forgets;
    rac3_textures_forget();
    CHECK(g_forgets == forgets + 1);
    CHECK(!rac3_texture_image(HUD_HANDLE, 0, &image));
    CHECK(func_00396E18(3) == HUD_HANDLE);
    CHECK(rac3_texture_image(HUD_HANDLE, 0, &image) && image.pixels == 0x00B00000u);
}

/* The multiplayer core's HUD bank, and level 39's, are read as the boot program's is. */
static void multiplayer_hud_banks(void) {
    openrac_game_ui_image image;
    hud_bank(0x00244428u, 0x00C00000u, 0x00C10000u);
    CHECK(rac3_hud_image(0x00244428u, 3, &image));
    CHECK(image.pixels == 0x00C00000u && image.palette == 0x00C10000u && image.width == 64);
    hud_bank(0x0024BD00u, 0x00D00000u, 0x00D10000u);
    CHECK(rac3_hud_image(0x0024BD00u, 3, &image));
    CHECK(image.pixels == 0x00D00000u && image.palette == 0x00D10000u && image.height == 32);
}

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
    multiplayer_send_image();
    another_executable_forgets();
    multiplayer_hud_banks();
    screens();
    free(openrac_guest_base);
    printf("rac3_mp_images: ok\n");
    return 0;
}
