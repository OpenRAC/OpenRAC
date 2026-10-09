/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The pad libraries (libdbc, libpad2, libvib). The pads come from the
 * platform layer (port/platform, through openrac_rac1_pad).
 *
 * Not done yet: the layout of what scePad2Read fills in. The game's own
 * reader (func_00217F68, matched C) and its post-processing (func_002181F0)
 * show which bytes it takes; until that is written down, a read reports no
 * data, which the game takes for a pad that is not connected. */
#include "game_protos.h"
#include "rac1_host.h"

int func_00124650(void) {
    return 1;
} /* sceDbcInit */

int func_00124B88(int mode) {
    (void)mode;
    return 1;
} /* scePad2Init */

static int next_socket;

int func_00124BC8(gaddr params, gaddr work) { /* scePad2CreateSocket */
    (void)params;
    (void)work;
    return next_socket++;
}

int func_00124D18(int socket, gaddr data) { /* scePad2Read */
    (void)socket;
    (void)data;
    return 0;
}

int func_00124DF0(int socket, gaddr profile) { /* scePad2GetButtonProfile */
    (void)socket;
    (void)profile;
    return 0;
}

int func_00124EE0(int socket) { /* scePad2GetState */
    (void)socket;
    return 0; /* disconnected, until scePad2Read fills its data */
}

int func_00125218(int socket, gaddr profile) { /* sceVibGetProfile */
    (void)socket;
    (void)profile;
    return 0;
}
