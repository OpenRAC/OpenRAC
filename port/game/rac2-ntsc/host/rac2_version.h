/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * USA v1.01's (SCUS-97268) names and addresses for the shared host C (port/game/rac2/host/rac2.h):
 * the shared C is written with them, so nothing is renamed here. */
#ifndef OPENRAC_RAC2_VERSION_H
#define OPENRAC_RAC2_VERSION_H

#define RAC2_ADDR(a) a
#define RAC2_LADDR(n, a) a
/* A constant of the game's code, v1.01's as the shared C writes it (`where` it is read). */
#define RAC2_K(name, value, where) (value)

#define RAC2_LEVEL_BASE 0x001A7C80u /* a level's program replaces the executable's memory from here */
#define RAC2_GP 0x001AEFF0u         /* the executable's $gp */
#define RAC2_EXE_SIZE 0x00252000u   /* the executable's loaded bytes from 0x00100000, to a page */

#include "rac2_addresses.h"

#endif /* OPENRAC_RAC2_VERSION_H */
