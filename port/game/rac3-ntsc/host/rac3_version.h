/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * NTSC-U's (SCUS-97353) addresses for the shared host C (port/game/rac3-pal/host, written with PAL's,
 * rac3-pal/host/rac3_version.h): this folder is first on the NTSC-U program's include path, so the
 * shared C reads this one. Each RAC3_ADDR(0x...u) is NTSC-U's value from rac3_addresses.h, written by
 * port/tools/hostgen/release_carry_rac3.py. */
#ifndef OPENRAC_RAC3_VERSION_H
#define OPENRAC_RAC3_VERSION_H

#define RAC3_NTSC 1

#define RAC3_ADDR(a) RAC3_ADDR_##a

#include "rac3_addresses.h"

#endif /* OPENRAC_RAC3_VERSION_H */
