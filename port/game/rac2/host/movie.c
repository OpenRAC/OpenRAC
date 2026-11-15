/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's movie player entry, PlayFmvMovie (0x0034EF58, matched C in
 * the decompilation): (first sector, size, an EE buffer pool, the player's
 * state block, a mode, the volume). It starts the IPU decoder and its threads
 * (InitFmvPlaybackEngine), feeds the stream from the disc (sceCdRead at the
 * sector in the stream object's +4, from the first argument) and tears it all
 * down again. Threads, the IPU and the DMA it waits on are not in the port:
 * the shared native player plays the same stream in the window and owns its
 * buffers, and the game keeps its own movie transitions around the call. Its
 * result is the native player's: 1 when skipped, 0 at the end, as Deadlocked's
 * port answers its own (rac4-ntsc/host/movie.c).
 *
 * The picture is drawn as the console draws it, with what goes over it: the
 * decoder's frame is uploaded to the GS at g_fmvFrameTbp (D_001A72DC, the
 * BITBLTBUF 0x0034FE18 builds: 8 pages wide, PSMCT32), and each blank the
 * vertical blank handler (0x0034FFC0) sends the list 0x002B6480 builds at
 * g_pFmvGsBase + 0x1C81C0: the picture's sprites, then the title movie's
 * pulsing "Press START" (0x0027D4B8, while the title's stream plays) or the
 * movie's subtitles (0x0027D5C0). The port uploads the native player's picture
 * there and builds that list with the game's own code on each blank, sent as
 * the tick's display list.
 */
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

/* g_pFmvGsBase: the movie's buffers; the list after the picture is built 0x1C81C0 in. The
 * picture's place in the GS's memory, in bytes (its block is this over 256). */
#define FMV_GS_BASE GREF(int, OPENRAC_DATA(RAC2_ADDR(0x001B23C8u)))
#define FMV_FRAME_TBP GREF(int, OPENRAC_DATA(RAC2_ADDR(0x001A72DCu)))

void func_002B6480(gaddr packet, int mode);
void func_00127220(gaddr channel, gaddr tag);

/* A blank of the movie: the picture to where the decoder's chain puts it, then the list the
 * console's handler sends (0x0034FFC0's func_002B6480 call, its first buffer's), sent to VIF1 as
 * the tick's list. */
static void movie_overlay(const uint8_t* rgba, int width, int height) {
    const gaddr list = (gaddr)(FMV_GS_BASE + 0x1C81C0);
    /* The texture the list's sprites read is 512 x 512 (TEX0's TW and TH): the rows under the
     * picture are black, as nothing is drawn there. */
    static uint8_t pixels[512 * 512 * 4];
    const openrac_game_image picture = {FMV_FRAME_TBP >> 8, 8, 0, 0, 0, 512, 512, 0};

    if (width > 512 || height > 512) {
        return;
    }
    for (int y = 0; y < height; ++y) {
        memcpy(pixels + (size_t)y * 512 * 4, rgba + (size_t)y * width * 4, (size_t)width * 4);
    }
    openrac_game_load_picture(&picture, pixels);

    func_002B6480(list, 0);
    func_00127220(0x10009000u, list & 0x0FFFFFFFu);
}

int func_0034EF58(int first, int second, int pool, int state, int mode, int volume) {
    (void)pool;
    (void)state;
    (void)mode;
    (void)volume;
    const int result =
        openrac_game_play_movie_over((uint32_t)first, (uint32_t)second, 0, 1, movie_overlay);

    openrac_game_forget_picture(FMV_FRAME_TBP >> 8);
    return result;
}
