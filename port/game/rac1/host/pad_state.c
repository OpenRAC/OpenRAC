/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank's pad update, answered by the port above libpad2.
 *
 * On the console init_pads (func_00217EE8) started libdbc and libpad2 and opened a socket for
 * port 0, and each frame UpdatePad's poll_pad_device_state (func_00217F68; the levels' copy of
 * UpdatePad calls the same) asked libpad2 for the socket's state, read the button and vibration
 * profiles once (state 0 of the pad's +0x198), then read a report a frame (state 1) and handed it
 * to the game's reader, process_pad_input (Pad_ProcessReport, func_002181F0), which turns it into
 * the pad record the game plays from.
 *
 * Here both are the port's. The pad is always there, as the library replacements had it (a port
 * with no controller reads as one with nothing pressed); its profiles are what they answered
 * (every button and both sticks, two motors), and the report is the port's controller as
 * openrac_pad_report gives it, through the mods' pad callbacks. Pad_ProcessReport stays the
 * game's, and a hook point for mods (the lock-strafe mod takes it), so the dead zone, pressures,
 * the mirror option, edges, the flick detector and key repeat are still the game's own.
 *
 * Retail PAL v2.00 has no sceVibSetActParam (libvib's was dead-stripped): the game never drives
 * the motors.
 */
#include "game/common/lib/pad_report.h"
#include "rac1.h"

#define PAD RAC1_ADDR(0x0013CA40u) /* g_controller_state, the one pad record */

/* The pad record's fields this update writes (games/rac1 include/structs/pad.h). */
enum {
    PAD_PROFILE = 0x180,       /* the button profile, 4 bytes */
    PAD_ACT = 0x184,           /* the vibration profile, 4 bytes */
    PAD_MOTOR_SMALL = 0x188,   /* cleared each poll */
    PAD_MOTOR_LARGE = 0x189,   /* cleared each poll */
    PAD_QUIET = 0x18C,         /* polls left whose presses the reader ignores */
    PAD_SOCKET = 0x194,        /* libpad2's socket: the port */
    PAD_PROFILE_STATE = 0x198, /* 0 profiles to read, 1 reading */
    PAD_DEVICE_STATE = 0x19C,  /* scePad2GetState's answer: 1 connected */
    PAD_HELD = 0x1A0,
    PAD_PREV_HELD = 0x1AC,
    PAD_RAW_HELD = 0x1B0,
    PAD_PREV_RAW_HELD = 0x1BC,
    PAD_PRESSURES = 0x1DC, /* 0x79 when the button profile has every button */
    PAD_LATCHED_HELD = 0x348,
};

/* init_pads: the pad record's socket is its port, its profiles still to read. */
void func_00217EE8(void) {
    GREF(int32_t, PAD + PAD_SOCKET) = 0;
    GREF(int32_t, PAD + PAD_PROFILE_STATE) = 0;
    GREF(int32_t, PAD + PAD_DEVICE_STATE) = 0;
}

/* poll_pad_device_state(pad), as the game's steps it: the latches, then the profiles on the first
 * poll (with the reader's input cleared and two quiet polls), then a report a poll. */
void func_00217F68(gaddr pad) {
    GREF(int32_t, pad + PAD_HELD) = 0;
    GREF(int32_t, pad + PAD_PREV_HELD) = GREF(int32_t, pad + PAD_LATCHED_HELD);
    GREF(int32_t, pad + PAD_PREV_RAW_HELD) = GREF(int32_t, pad + PAD_RAW_HELD);
    GREF(int32_t, pad + PAD_RAW_HELD) = 0;
    GREF(int32_t, pad + PAD_DEVICE_STATE) = 1;

    if (GREF(int32_t, pad + PAD_PROFILE_STATE) == 0) {
        /* Every button and both sticks; then libvib's one byte, small and large motor, over the
         * same buffer, so the three bytes after it are the button profile's. */
        for (unsigned n = 0; n < 4; n++) {
            GREF(uint8_t, pad + PAD_PROFILE + n) = 0xFF;
            GREF(uint8_t, pad + PAD_ACT + n) = n == 0 ? 3 : 0xFF;
        }
        GREF(int32_t, pad + PAD_PRESSURES) = 0x79;
        GREF(int32_t, pad + PAD_PROFILE_STATE) = 1;
        func_00218188(pad); /* ClearPadInput */
        GREF(int16_t, pad + PAD_QUIET) = 2;
    } else {
        GFRAME(report, 0x40);
        openrac_pad_report(GREF(int32_t, pad + PAD_SOCKET), (uint8_t*)G(report));
        func_002181F0(pad, report, OPENRAC_PAD_REPORT); /* Pad_ProcessReport */
        if (GREF(int16_t, pad + PAD_QUIET) != 0) {
            GREF(int16_t, pad + PAD_QUIET)--;
        }
    }
    GREF(uint8_t, pad + PAD_MOTOR_SMALL) = 0;
    GREF(uint8_t, pad + PAD_MOTOR_LARGE) = 0;
}
