/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's rooms past 8 players (mp_wide.c): the room's count and the settings object's
 * second half, for mp_net.c. */
#ifndef OPENRAC_RAC3_MP_WIDE_H
#define OPENRAC_RAC3_MP_WIDE_H

#include <stdint.h>

#include "openrac/guest.h"

/* Whether this disc's tables can grow (PAL: yes; NTSC-U keeps 8). */
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

#endif /* OPENRAC_RAC3_MP_WIDE_H */
