/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The disc library (libcdvd). The game reads its data by absolute sector
 * from the disc (docs/port/RAC1_PAL_SURVEY.md, section 6), so the port reads
 * the same sectors from the player's disc image that the extractor kept
 * (iso_data/<game>/disc.iso). A read completes before the call returns, so the
 * game's polling (sceCdSync) finds it done. */
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <time.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

#define SECTOR 2048

static FILE* disc;
static int last_error;

static FILE* open_disc(void) {
    if (disc == NULL && openrac_game_disc_image != NULL) {
        disc = fopen(openrac_game_disc_image, "rb");
        if (disc == NULL) {
            fprintf(stderr, "[error] cannot open the disc image %s\n", openrac_game_disc_image);
        }
    }
    return disc;
}

/* sceCdInit */
int openrac_lib_sceCdInit(int mode) {
    (void)mode;
    return open_disc() != NULL;
}

/* sceCdDiskReady: 2 is "ready". */
int openrac_lib_sceCdDiskReady(int mode) {
    (void)mode;
    return open_disc() != NULL ? 2 : 6;
}

/* sceCdMmode: the media type (DVD); nothing to set. */
int openrac_lib_sceCdMmode(int media) {
    (void)media;
    return 1;
}

/* sceCdRead(sector, count, buffer, mode) */
int openrac_lib_sceCdRead(unsigned int sector, unsigned int count, gaddr buffer, gaddr mode) {
    (void)mode;
    FILE* f = open_disc();
    if (f == NULL) {
        last_error = 0x01; /* any non-zero error: the game retries */
        return 0;
    }
    if (fseeko(f, (off_t)sector * SECTOR, SEEK_SET) != 0
        || fread(G(buffer), SECTOR, count, f) != count) {
        fprintf(stderr, "[error] disc read of %u sectors at %u failed\n", count, sector);
        last_error = 0x32;
        return 0;
    }
    last_error = 0;
    return 1;
}

/* sceCdSync: 0 is "done" (every read is). */
int openrac_lib_sceCdSync(int mode) {
    (void)mode;
    return 0;
}

/* sceCdGetError */
int openrac_lib_sceCdGetError(void) {
    return last_error;
}

/* sceCdBreak */
int openrac_lib_sceCdBreak(void) {
    return 1;
}

static uint8_t bcd(int v) {
    return (uint8_t)(((v / 10) << 4) | (v % 10));
}

/* sceCdReadClock: the console's clock, in BCD: status, second, minute, hour,
 * a pad byte, day, month, year (two digits). */
int openrac_lib_sceCdReadClock(gaddr clock) {
    time_t now = time(NULL);
    struct tm local;
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    uint8_t* c = (uint8_t*)G(clock);
    c[0] = 0;
    c[1] = bcd(local.tm_sec);
    c[2] = bcd(local.tm_min);
    c[3] = bcd(local.tm_hour);
    c[4] = 0;
    c[5] = bcd(local.tm_mday);
    c[6] = bcd(local.tm_mon + 1);
    c[7] = bcd(local.tm_year % 100);
    return 1;
}
