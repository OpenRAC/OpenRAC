/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The game's hand-written assembly routines that only drive the console's
 * hardware, answered by the port (hostgen.json, "host_functions"). Each is
 * written from what the retail routine does, never from its bytes. */
#include "game_protos.h"
#include "openrac/game_host.h"

#include <math.h>

/* InitDma: sets the DMA controller's priority bit, enables it and clears the
 * tag address of the VIF0, VIF1, GIF, IPU and SPR channels. The port has no
 * DMA controller: the game's display lists are read at sceDmaSend. */
void func_0020C268(void) {}

/* The square root and the reciprocal square root the hand-written vector
 * routines (hand/) use, as the FPU's sqrt.s and rsqrt.s; translated code calls
 * a named function of its own under the game_ prefix. */
float game_openrac_sqrt(float x) { return sqrtf(x); }
float game_openrac_rsqrt(float a, float b) { return a / sqrtf(b); }
