/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: OPENRAC_RAC2_DEATH_TRACE=1, a log of how Siberius's cart riders die (death_trace.c;
 * docs/port/DEBUGGING.md). */
#ifndef OPENRAC_RAC2_DEATH_TRACE_H
#define OPENRAC_RAC2_DEATH_TRACE_H

#include <stdint.h>

#include "openrac/guest.h"

/* Whether the trace is on (OPENRAC_RAC2_DEATH_TRACE set to anything but "" and "0"). */
int rac2_death_trace_on(void);

/* Once a tick, before the moby loop: the watched mobys' changes since the last tick (their state,
 * hidden bits, death path, claws, slots freed and given out again) and the hero's sinking count. */
void rac2_death_trace_tick(void);

/* SetDeathBits (func_002A6C58) is about to record the death of `moby` (`flags`, `arg` its
 * arguments): one line for a watched class. */
void rac2_death_trace_death(gaddr moby, int flags, int arg);

#endif
