/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The pad as the games' own readers take it, for the host C that answers their pad updates
 * (rac1/host/pad_state.c, rac2/host/pad_state.c, rac3-pal/host/pad.c).
 *
 * On the console each game asked libpad or libpad2 for the controller's state, its profiles and a
 * report, over several frames, and handed the report to its own reader, which turns it into the
 * pad record the game plays from (sticks with a dead zone, pressures, presses and releases, key
 * repeat). The port answers the pad update itself: it fills the reader's input from the port's
 * controllers (openrac_game_pad) and runs the game's reader on it, so everything the game
 * computes from the pad is still the game's. */
#ifndef OPENRAC_PAD_REPORT_H
#define OPENRAC_PAD_REPORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The size of the input the readers take: two bytes of buttons (a clear bit is a pressed button,
 * the low byte first), the right stick's x and y, the left stick's x and y (0x80 at rest), then
 * twelve pressures in the order right, left, up, down, triangle, circle, cross, square, L1, R1,
 * L2, R2 (0xFF pressed, 0 released). */
#define OPENRAC_PAD_REPORT 18

/* `port`'s pad as those 18 bytes at `out`, after the mods' pad callbacks
 * (openrac_game_pad_report_hook) and what the port does after them (openrac_game_pad_after_mods).
 * The result is openrac_game_pad's: nonzero while a controller or the keyboard drives the port. */
int openrac_pad_report(int port, uint8_t out[OPENRAC_PAD_REPORT]);

#ifdef __cplusplus
}
#endif

#endif
