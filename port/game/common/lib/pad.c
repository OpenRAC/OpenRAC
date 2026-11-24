/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The pad as the games' readers take it (pad_report.h), and the pad libraries (libdbc, libpad2,
 * libvib) for the games still answered at them. The pads come from the platform layer
 * (port/platform, through openrac_game_pad).
 *
 * Ratchet & Clank, Going Commando and single-player Up Your Arsenal use native pad updates
 * (rac1/host/pad_state.c, rac2/host/pad_state.c, rac3-pal/host/pad.c), which
 * hand the game's own reader openrac_pad_report's bytes. Deadlocked (rac4-ntsc/host/pad.c) still
 * runs its libpad state machine and reads through scePad2Read; rac3 multiplayer retains libpad too. */
#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "pad_report.h"

void (*openrac_game_pad_report_hook)(int port, uint8_t* report, int size);

int openrac_lib_sceDbcInit(void) {
    return 1;
} /* sceDbcInit */

int openrac_lib_scePad2Init(int mode) {
    (void)mode;
    return 1;
} /* scePad2Init */

/* The socket is the port the parameters name (their second word; rac1 writes {2, port}), so a
 * game that sets its pads up again (rac1 does after Quit Game) reads the same controller. Counting
 * sockets up gave the second setup socket 1, the empty second port, and every button stopped. */
int openrac_lib_scePad2CreateSocket(gaddr params, gaddr work) { /* scePad2CreateSocket */
    (void)work;
    const int port = params != 0 ? GREF(int, params + 4) : 0;
    return port >= 0 && port < 2 ? port : 0;
}

int openrac_pad_report(int port, uint8_t out[OPENRAC_PAD_REPORT]) {
    uint16_t buttons = 0xFFFF;
    uint8_t analog[4] = {0x80, 0x80, 0x80, 0x80};

    const int connected = openrac_game_pad(port, &buttons, analog);
    openrac_pad_report_from_state(port, buttons, analog, out);
    return connected;
}

void openrac_pad_report_from_state(
    int port, uint16_t buttons, const uint8_t analog[4], uint8_t out[OPENRAC_PAD_REPORT]
) {
    /* The twelve pressures follow the report contract in pad_report.h, each derived from its bit. */
    static const unsigned bit[12] = {5, 7, 4, 6, 12, 13, 14, 15, 10, 11, 8, 9};
    unsigned n;

    out[0] = (uint8_t)buttons;
    out[1] = (uint8_t)(buttons >> 8);
    out[2] = analog[0];
    out[3] = analog[1];
    out[4] = analog[2];
    out[5] = analog[3];
    for (n = 0; n < 12; n++) {
        out[6 + n] = (buttons >> bit[n]) & 1 ? 0 : 0xFF;
    }
    if (openrac_game_pad_report_hook != 0) {
        openrac_game_pad_report_hook(port, out, OPENRAC_PAD_REPORT);
        openrac_game_pad_after_mods(port, out, OPENRAC_PAD_REPORT);
    }
}

/* scePad2Read(socket, data): openrac_pad_report's bytes; the result is the number of bytes
 * written. The socket number is the port. */
int openrac_lib_scePad2Read(int socket, gaddr data) { /* scePad2Read */
    openrac_pad_report(socket, (uint8_t*)G(data));
    return OPENRAC_PAD_REPORT;
}

/* Which buttons exist: every button and both sticks. */
int openrac_lib_scePad2GetButtonProfile(int socket, gaddr profile) { /* scePad2GetButtonProfile */
    uint32_t all = 0xFFFFFFFFu;
    (void)socket;
    __builtin_memcpy(G(profile), &all, 4);
    return 4;
}

int openrac_lib_scePad2GetState(int socket) { /* scePad2GetState */
    (void)socket;
    return 1; /* connected and stable */
}

/* libvib. Profile byte: bit 0 small motor (on/off), bit 1 large motor (0..255). Motor data is
 * packed in that order: 1 bit, then 8 bits (rac2's pad poll sends (large << 1) | small). */
enum {
    VIB_SMALL = 1,
    VIB_LARGE = 2
};

int openrac_lib_sceVibGetProfile(int socket, gaddr profile) { /* sceVibGetProfile */
    (void)socket;
    *(uint8_t*)G(profile) = VIB_SMALL | VIB_LARGE;
    return 1;
}

/* Unpacks the motor data by the profile and passes it on. */
int openrac_lib_sceVibSetActParam(
    int socket, int profile_size, gaddr profile, int data_size, gaddr data
) { /* sceVibSetActParam */
    const uint8_t act = profile_size > 0 ? *(const uint8_t*)G(profile) : 0;
    uint32_t bits = 0;
    unsigned at = 0;
    int small = 0, large = 0, n;

    for (n = 0; n < data_size && n < 4; n++) {
        bits |= (uint32_t)((const uint8_t*)G(data))[n] << (8 * n);
    }
    if (act & VIB_SMALL) {
        small = (int)(bits & 1u);
        at = 1;
    }
    if (act & VIB_LARGE) {
        large = (int)((bits >> at) & 0xFFu);
    }
    openrac_game_set_motors(socket, small, large);
    return 1;
}
