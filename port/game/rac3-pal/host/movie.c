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
 * first two arguments are; until it is, a stream the player cannot open is skipped. The picture is
 * shown alone, without the game's own list over it.
 */
#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

int func_003AFC48(int a, int b, gaddr alloc, int channel, int flag) {
    (void)alloc;
    (void)flag;
    return openrac_game_play_movie((uint32_t)a, (uint32_t)b, channel, 1);
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
