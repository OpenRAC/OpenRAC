/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's host C, one copy for every disc (USA v1.01, PAL, USA v2.00; port/game/rac2/README.md).
 *
 * The files here are compiled into each release's program (hostgen.json, "host_shared") with that
 * release's translation and its table of names and addresses (rac2_version.h in
 * port/game/rac2-<release>/host). They are written with v1.01's names for the game's functions
 * (func_0027F538, func_L05_0041DF58, func_0027F640__game) and its addresses inside RAC2_ADDR() and
 * RAC2_LADDR(): each release's table turns them into its own, so a fix written here holds for every disc.
 */
#ifndef OPENRAC_RAC2_H
#define OPENRAC_RAC2_H

/* After a file's own includes (and the translation's prototypes, game_protos.h, where it reads them):
 * a release's table renames v1.01's names to its own. */
#include "rac2_version.h"

#if !defined(RAC2_ADDR) || !defined(RAC2_LADDR) || !defined(RAC2_LEVEL_BASE)
#error "rac2_version.h must define RAC2_ADDR, RAC2_LADDR and the release's layout"
#endif

/* A function's name in this release, as a string (for logs and tables that name functions). */
#define RAC2_NAME_(f) #f
#define RAC2_NAME(f) RAC2_NAME_(f)

#endif /* OPENRAC_RAC2_H */
