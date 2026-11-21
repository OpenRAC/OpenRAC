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
 * first two arguments are; until it is, a stream the player cannot open is skipped. The boot
 * program's movies are drawn with the game's own list over the picture, subtitles included
 * (below); frontbin's and the levels' copies show the picture alone for now.
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
#define MOVIE_BLANKS RAC3_ADDR(0x00225734u)
#define MOVIE_TITLE_LOOP RAC3_ADDR(0x00227400u)
#define LIST_CURSOR RAC3_ADDR(0x001DA050u)
#define LIST_BUFFERS RAC3_ADDR(0x001DA048u)
#define LIST_INDEX RAC3_ADDR(0x001DA05Cu)

int rac3_picture_record(gaddr cursor, const uint8_t* rgba, int width, int height); /* picture.c */
void func_0038A8F8(void);
void func_0038A798(void);
void func_003A9AF8(void);
void func_0012D938(gaddr channel, gaddr tag);

static void movie_overlay(const uint8_t* rgba, int width, int height) {
    if (width <= 0 || height <= 0 || width > 512 || height > 448) {
        return;
    }
    const gaddr kept = GREF(gaddr, LIST_CURSOR);
    const gaddr list = GREF(gaddr, LIST_BUFFERS + 4u * (uint32_t)GREF(int32_t, LIST_INDEX));
    GREF(gaddr, LIST_CURSOR) = list;
    func_003A9AF8();
    rac3_picture_record(LIST_CURSOR, rgba, width, height);
    GREF(int32_t, MOVIE_BLANKS) += 1;
    if (GREF(int32_t, MOVIE_TITLE_LOOP) != 0) {
        func_0038A798();
    } else {
        func_0038A8F8();
    }
    const gaddr end = GREF(gaddr, LIST_CURSOR);
    GREF(uint32_t, end) = 0x70000000u; /* END */
    GREF(uint32_t, end + 4) = 0;
    GREF(uint64_t, end + 8) = 0;
    GREF(gaddr, LIST_CURSOR) = kept;
    func_0012D938(0x10009000u, list & 0x0FFFFFFFu);
}

int func_003AFC48(int a, int b, gaddr alloc, int channel, int flag) {
    (void)alloc;
    (void)flag;
    return openrac_game_play_movie_over((uint32_t)a, (uint32_t)b, channel, 1, movie_overlay);
}

/* frontbin's copy of the entry (0x003AF890 in its program: the same instructions), which plays the
 * menus' background loop. */
int func_003AF890__frontbin(int a, int b, gaddr alloc, int channel, int flag) {
    return func_003AFC48(a, b, alloc, channel, flag);
}

/* Every level's copy of the entry (Veldin's place 0x004D28B0, the catalogue's frontbin row
 * func_003AF890 in 49 levels: the same instructions). The levels' story movies start it
 * (func_L01_004AFFF8, func_L01_004B88B0; the multiplayer maps' func_L39_005FB960); translated, it
 * ran the IPU decoder and its threads, which the port does not have. */
int func_L01_004D28B0(int a, int b, gaddr alloc, int channel, int flag) {
    return func_003AFC48(a, b, alloc, channel, flag);
}
