/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's rooms past 8 players (mp_wide.c): the room's count and the settings object's
 * second half, for mp_net.c; the slots past 8 of the settings and results, for mp_ui.c. */
#ifndef OPENRAC_RAC3_MP_WIDE_H
#define OPENRAC_RAC3_MP_WIDE_H

#include <stdint.h>

#include "openrac/guest.h"

/* Whether this disc's tables can grow (both discs' sites are known: mp/wide.json). */
int rac3_mp_wide_available(void);
/* The room's count of players (8-24; below 8 is 8): the game's tables hold that many. */
void rac3_mp_wide_set_slots(int slots);
int rac3_mp_wide_slots(void);
/* A new session: the port's entries 8-23 cleared. */
void rac3_mp_wide_reset(void);
/* OBJECT_GAME_SETTINGS_WIDE: its size, a new one's empty slots, and where this console keeps it. */
uint32_t rac3_mp_wide_object_size(void);
void rac3_mp_wide_object_clear(gaddr data);
void rac3_mp_wide_object(gaddr data);
/* Slot `slot`'s entry of the settings object's array at `offset` (+0x000 names, +0x0E0 clients,
 * ...), and of the results block's array at `offset` (+0x000 names, +0x120 teams, +0x140 kills and
 * deaths, +0x1B8 suicides, ...), wherever the port keeps it (0: no settings object). */
gaddr rac3_mp_wide_settings(uint32_t offset, int slot);
gaddr rac3_mp_wide_results(uint32_t offset, int slot);
/* The game-mode struct's array at `offset`, slots first..first+count-1, copied into `bytes` (out)
 * or from it; the bytes copied (0: no such array in this program). */
int rac3_mp_wide_mode_copy(uint32_t offset, int first, int count, uint8_t* bytes, int out);

#endif /* OPENRAC_RAC3_MP_WIDE_H */
