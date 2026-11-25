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
 * (func_003A9AF8), the native player's picture given to the frame recorder (openrac/frame.h: a port
 * picture drawn over the picture's area, where the list's cursor is) and the subtitles printed
 * through the wrapped font (ui2d.c), as Going Commando's port sends its movie list
 * (rac2/host/movie.c). */
/* Each program's copy of what the list needs: its list's cursor and two buffers (with the index of
 * the one being filled), the movie block's blank count, the title loop's flag, and its copies of
 * the 2D frame start, the subtitles and "Press START". frontbin's are at the boot program's
 * addresses (its routines at its own, paired with the boot program's instruction by instruction);
 * a level's at Veldin's, relocated to the loaded level's (OPENRAC_LDATA). */
typedef struct MovieList {
    gaddr cursor, buffers, index, blanks, title_loop;
    gaddr font_uploaded, font_frame; /* a level's FontUpload words (0: none to force) */
    void (*frame_start)(void);
    void (*subtitles)(void);
    void (*press_start)(void);
} MovieList;

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
         0,
         0,
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
 * the frame start 0x004C5E90, the subtitles 0x004050C0, "Press START" 0x00404F60. A level's font
 * pages go up with its frame (its FontUpload, 0x0040F8F0, the boot program's func_0038E2B0, once
 * for each value of the frame count 0x001DF948, the last kept at 0x001D64EC), into the 2D layer's
 * GS area (the allocator 0x001D4BB0); the level's loop does not run while a movie plays, so each
 * blank asks for the upload again in the blank's own list and puts the allocator back where it
 * was, else the subtitles' glyphs were drawn from pages no longer there. */
static MovieList level_list(void) {
    const MovieList m =
        {OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1068u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1060u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1074u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E6DB4u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001EA7C0u)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001D64ECu)),
         OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DF948u)),
         level_frame_start,
         level_subtitles,
         level_press_start};
    return m;
}

/* The port pictures the movie is drawn from (openrac_frame_picture_set): its frame, and on a
 * window wider than the game's picture the backdrop the window's sides show, the frame an eighth
 * the size each way, blurred and darkened (BACKDROP_LIGHT of 256), as picture.c's main menu. */
#define MOVIE_PICTURE 0x52334D31u
#define MOVIE_BACKDROP 0x52334D32u
#define BACKDROP_SHRINK 8
#define BACKDROP_WIDTH (512 / BACKDROP_SHRINK)
#define BACKDROP_HEIGHT (448 / BACKDROP_SHRINK)
#define BACKDROP_LIGHT 224

static uint8_t s_backdrop[BACKDROP_WIDTH * BACKDROP_HEIGHT * 4];

/* The backdrop of a 512-wide frame of `lines` rows: each 8 x 8 square averaged, then three passes
 * of a 3-tap box blur each way, darkened. Its rows. */
static int make_backdrop(const uint8_t* from, int lines) {
    static int sums[BACKDROP_HEIGHT][BACKDROP_WIDTH][3];
    static int pass[BACKDROP_HEIGHT][BACKDROP_WIDTH][3];
    const int rows = (lines + BACKDROP_SHRINK - 1) / BACKDROP_SHRINK;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < BACKDROP_WIDTH; ++x) {
            int c[3] = {0, 0, 0}, n = 0;
            for (int py = y * BACKDROP_SHRINK; py < (y + 1) * BACKDROP_SHRINK && py < lines; ++py) {
                const uint8_t* p = from + ((size_t)py * 512u + (size_t)x * BACKDROP_SHRINK) * 4u;
                for (int px = 0; px < BACKDROP_SHRINK; ++px, p += 4, ++n) {
                    c[0] += p[0], c[1] += p[1], c[2] += p[2];
                }
            }
            for (int k = 0; k < 3; ++k) {
                sums[y][x][k] = c[k] / n;
            }
        }
    }
    for (int round = 0; round < 3; ++round) {
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < BACKDROP_WIDTH; ++x) {
                const int l = x > 0 ? x - 1 : x, r = x + 1 < BACKDROP_WIDTH ? x + 1 : x;
                for (int k = 0; k < 3; ++k) {
                    pass[y][x][k] = (sums[y][l][k] + sums[y][x][k] * 2 + sums[y][r][k]) / 4;
                }
            }
        }
        for (int y = 0; y < rows; ++y) {
            const int u = y > 0 ? y - 1 : y, d = y + 1 < rows ? y + 1 : y;
            for (int x = 0; x < BACKDROP_WIDTH; ++x) {
                for (int k = 0; k < 3; ++k) {
                    sums[y][x][k] = (pass[u][x][k] + pass[y][x][k] * 2 + pass[d][x][k]) / 4;
                }
            }
        }
    }
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < BACKDROP_WIDTH; ++x) {
            uint8_t* p = s_backdrop + ((size_t)y * BACKDROP_WIDTH + (size_t)x) * 4u;
            for (int k = 0; k < 3; ++k) {
                p[k] = (uint8_t)(sums[y][x][k] * BACKDROP_LIGHT / 256);
            }
            p[3] = 0x80;
        }
    }
    return rows;
}

/* The movie's frame where the list's cursor (the word at `cursor`) is: over the picture's area (the
 * frame stretched over the frame's lines, as the main menu's 416-line background is on PAL), and on
 * a wider window its backdrop under it, across the whole window. */
static void record_frame(gaddr cursor, const uint8_t* rgba, int width, int height) {
    openrac_frame_anchor(GREF(gaddr, cursor));
    if (width == 512 && openrac_game_picture_wider() > 1.001f) {
        const int rows = make_backdrop(rgba, height < 448 ? height : 448);
        openrac_frame_picture_set(MOVIE_BACKDROP, s_backdrop, BACKDROP_WIDTH, rows);
        openrac_frame_picture(
            MOVIE_BACKDROP, NULL, 0x80808080u, OPENRAC_BLEND_OPAQUE | OPENRAC_PICTURE_WHOLE
        );
    }
    openrac_frame_picture_set(MOVIE_PICTURE, rgba, width, height);
    openrac_frame_picture(MOVIE_PICTURE, NULL, 0x80808080u, OPENRAC_BLEND_OPAQUE);
}

static MovieList s_list;
static uint32_t s_allocator;
#define GS_ALLOCATOR RAC3_ADDR(0x001D4BB0u)

static void movie_overlay(const uint8_t* rgba, int width, int height) {
    const MovieList* m = &s_list;
    if (width <= 0 || height <= 0 || width > 512 || height > 448) {
        return;
    }
    const gaddr kept = GREF(gaddr, m->cursor);
    const gaddr list = GREF(gaddr, m->buffers + 4u * (uint32_t)GREF(int32_t, m->index));
    GREF(gaddr, m->cursor) = list;
    if (m->font_uploaded != 0) {
        GREF(uint32_t, GS_ALLOCATOR) = s_allocator;
        GREF(int32_t, m->font_uploaded) = GREF(int32_t, m->font_frame) + 1;
    }
    m->frame_start();
    record_frame(m->cursor, rgba, width, height);
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
    s_allocator = GREF(uint32_t, GS_ALLOCATOR);
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
