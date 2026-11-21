/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's movie player entry (0x003AFC48, C in the decompilation): (a, b, the allocator
 * it takes its buffers from, the audio channel (the language byte at 0x00143A07), a flag). It
 * takes its buffers, starts the decoder (func_003B0188 with a, b and the channel), runs the
 * playback loop (func_003AFE10: libmpeg's software decode through the IPU, fed from the disc) and
 * stops it (func_003B0360), as Going Commando's PlayFmvMovie does (rac2/host/movie.c). The IPU and
 * the threads the loop waits on are not in the port: the shared native player plays the stream
 * instead (or skips it with --skip-movies).
 *
 * Not checked yet: that a and b are the stream's first sector and its size, as Going Commando's
 * first two arguments are; until it is, a stream the player cannot open is skipped. Every program's
 * movies are drawn with the game's own list over the picture, subtitles included (below).
 */
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

/* The boot program's movies and their subtitles. On the console the hand-written vertical blank
 * handler (0x003B1118) sends a list each blank that func_003A1428 builds: the picture's sprites,
 * then the subtitles (func_0038A8F8: the table at the movie block 0x00225700 + 0x48, against the
 * blank count at +0x34, which func_003A1428 steps) or, in the title's loop, "Press START"
 * (func_0038A798). Here each blank's list is built in the boot program's own list buffer (the one
 * its loop, blocked in the movie, would build next: 0x001DA048[0x001DA05C]) as a 2D frame
 * (func_003A9AF8), the native player's picture as its full-screen record (picture.c) and the
 * subtitles printed through the wrapped font (ui2d.c), as Going Commando's port sends its movie
 * list (rac2/host/movie.c). */
/* Each program's copy of what the list needs: its list's cursor and two buffers (with the index of
 * the one being filled), the movie block's blank count, the title loop's flag, and its copies of
 * the 2D frame start, the subtitles and "Press START". frontbin's are at the boot program's
 * addresses (its routines at its own, paired with the boot program's instruction by instruction);
 * a level's at Veldin's, relocated to the loaded level's (OPENRAC_LDATA). */
typedef struct MovieList {
    gaddr cursor, buffers, index, blanks, title_loop;
    void (*frame_start)(void);
    void (*subtitles)(void);
    void (*press_start)(void);
} MovieList;

int rac3_picture_record(gaddr cursor, const uint8_t* rgba, int width, int height); /* picture.c */
void func_0012D938(gaddr channel, gaddr tag);
void func_003A9AF8(void);
void func_0038A8F8(void);
void func_0038A798(void);
/* The copies translated from their instructions return what they leave in $v0. */
int func_003A9740__frontbin(void);
int func_0038A518__frontbin(void);
void func_0038A3B8__frontbin(void);
int func_L01_004C5E90(void);
int func_L01_004050C0(void);
int func_L01_00404F60(void);

static void fb_frame_start(void) {
    func_003A9740__frontbin();
}

static void fb_subtitles(void) {
    func_0038A518__frontbin();
}

static void level_frame_start(void) {
    func_L01_004C5E90();
}

static void level_subtitles(void) {
    func_L01_004050C0();
}

static void level_press_start(void) {
    func_L01_00404F60();
}

static MovieList boot_list(void) {
    const MovieList m =
        {RAC3_ADDR(0x001DA050u),
         RAC3_ADDR(0x001DA048u),
         RAC3_ADDR(0x001DA05Cu),
         RAC3_ADDR(0x00225734u),
         RAC3_ADDR(0x00227400u),
         func_003A9AF8,
         func_0038A8F8,
         func_0038A798};
    return m;
}

static MovieList frontbin_list(void) {
    MovieList m = boot_list();
    m.frame_start = fb_frame_start;
    m.subtitles = fb_subtitles;
    m.press_start = func_0038A3B8__frontbin;
    return m;
}

/* Veldin's: the cursor 0x001E1068 after its two buffers 0x001E1060 (the index at 0x001E1074), the
 * movie block 0x001E6D80 and the title flag 0x001EA7C0 (its copy of func_003A1428, 0x0045E208);
 * the frame start 0x004C5E90, the subtitles 0x004050C0, "Press START" 0x00404F60. */
static MovieList level_list(void) {
    const MovieList m =
        {OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1068u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1060u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1074u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E6DB4u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001EA7C0u)),
         level_frame_start,
         level_subtitles,
         level_press_start};
    return m;
}

static MovieList s_list;

static void movie_overlay(const uint8_t* rgba, int width, int height) {
    const MovieList* m = &s_list;
    if (width <= 0 || height <= 0 || width > 512 || height > 448) {
        return;
    }
    const gaddr kept = GREF(gaddr, m->cursor);
    const gaddr list = GREF(gaddr, m->buffers + 4u * (uint32_t)GREF(int32_t, m->index));
    GREF(gaddr, m->cursor) = list;
    m->frame_start();
    rac3_picture_record(m->cursor, rgba, width, height);
    GREF(int32_t, m->blanks) += 1;
    if (GREF(int32_t, m->title_loop) != 0) {
        m->press_start();
    } else {
        m->subtitles();
    }
    const gaddr end = GREF(gaddr, m->cursor);
    GREF(uint32_t, end) = 0x70000000u; /* END */
    GREF(uint32_t, end + 4) = 0;
    GREF(uint64_t, end + 8) = 0;
    GREF(gaddr, m->cursor) = kept;
    func_0012D938(0x10009000u, list & 0x0FFFFFFFu);
}

static int play(const MovieList* m, int a, int b, int channel) {
    s_list = *m;
    return openrac_game_play_movie_over((uint32_t)a, (uint32_t)b, channel, 1, movie_overlay);
}

int func_003AFC48(int a, int b, gaddr alloc, int channel, int flag) {
    (void)alloc;
    (void)flag;
    const MovieList m = boot_list();
    return play(&m, a, b, channel);
}

/* frontbin's copy of the entry (0x003AF890 in its program: the same instructions), which plays the
 * menus' background loop and the movies after a level. */
int func_003AF890__frontbin(int a, int b, gaddr alloc, int channel, int flag) {
    (void)alloc;
    (void)flag;
    const MovieList m = frontbin_list();
    return play(&m, a, b, channel);
}

/* Every level's copy of the entry (Veldin's place 0x004D28B0, the catalogue's frontbin row
 * func_003AF890 in 49 levels: the same instructions). The levels' story movies start it
 * (func_L01_004AFFF8, func_L01_004B88B0; the multiplayer maps' func_L39_005FB960); translated, it
 * ran the IPU decoder and its threads, which the port does not have. */
int func_L01_004D28B0(int a, int b, gaddr alloc, int channel, int flag) {
    (void)alloc;
    (void)flag;
    const MovieList m = level_list();
    return play(&m, a, b, channel);
}
