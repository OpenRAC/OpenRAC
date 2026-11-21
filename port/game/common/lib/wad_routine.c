/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A game's WAD decompressor routine (wad_routine.h): the checks the retail
 * routine makes on its two game addresses and the header, then the shared
 * decoder in the routine's own way (wad_decompress_as_game) on the stream
 * where it lies in game memory. */
#include "game/common/lib/wad_routine.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"

/* The EE's segments map the same RAM; the low 29 bits are its physical
 * address (documented). */
static const uint32_t kPhysicalMask = 0x1FFFFFFFu;

/* A WAD stream's 16-byte header: "WAD", then the compressed size, header
 * included, as a little-endian word at byte 3 (documented: assets/disc/wad.h,
 * after ReRAC). */
static const char kMagic[] = "WAD";
static const size_t kSizeOffset = 3;
static const uint32_t kHeaderBytes = 16u;

/* Decodes the WAD stream at `source` as the game's routine does (wad.cpp): `readable` bytes may be
 * read from it, `before` lie before `destination` and `capacity` after it. Sets *made to the
 * routine's result and returns 1; 0 when the routine traps or the output does not fit. */
int openrac_lib_wad_decompress_as_game(
    const uint8_t* source,
    uint32_t readable,
    uint8_t* destination,
    uint32_t before,
    uint32_t capacity,
    uint32_t* made
);

/* Ends the program with the crash message "<game> WAD <problem>". */
static _Noreturn void crash(const char* game, const char* problem) {
    /* Room for the game's name and the longest problem after it. */
    char message[160];

    snprintf(message, sizeof message, "%s WAD %s", game, problem);
    openrac_game_crash(message);
}

int openrac_wad_routine(int source, int destination, const char* game) {
    const uint32_t from = (uint32_t)source & kPhysicalMask;
    const uint32_t to = (uint32_t)destination & kPhysicalMask;

    /* The whole header is read below, and the output has to start inside RAM. */
    if (from > openrac_guest_ram_size - kHeaderBytes || to >= openrac_guest_ram_size) {
        crash(game, "buffer lies outside game RAM");
    }

    const uint8_t* data = (const uint8_t*)G(from);
    uint32_t compressed;
    memcpy(&compressed, data + kSizeOffset, sizeof compressed);

    /* The retail routine traps on a bad header; a size past RAM's end cannot be read. */
    if (memcmp(data, kMagic, sizeof kMagic - 1) != 0 || compressed < kHeaderBytes
        || compressed > openrac_guest_ram_size - from) {
        crash(game, "header or compressed size is invalid");
    }

    /* The routine checks the compressed size only between packets, so a stream whose header and
     * packets disagree (the map's cache can make one) is read on past it, as on the console. */
    uint32_t made = 0;
    const int decoded = openrac_lib_wad_decompress_as_game(
        data, openrac_guest_ram_size - from, G(to), to, openrac_guest_ram_size - to, &made
    );

    /* The routine traps on the stream's packets, or they would not fit in RAM after `to`. */
    if (!decoded) {
        crash(game, "decompression failed");
    }

    /* OPENRAC_TRACE_WAD (docs/port/DEBUGGING.md). */
    if (openrac_env_string("OPENRAC_TRACE_WAD")) {
        fprintf(
            stderr,
            "[debug] %s WAD at %08x: %u -> %u bytes, destination %08x\n",
            game,
            from,
            compressed,
            made,
            to
        );
    }

    return (int)made;
}
