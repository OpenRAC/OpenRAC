/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: more room for spawned mobys than on a PS2, for a mod that asks (openrac_api
 * moby_pool_raise and moby_pool_status, interface 1.27; moby_pool.c says how the pool is sized). */
#ifndef OPENRAC_RAC2_MOBY_POOL_H
#define OPENRAC_RAC2_MOBY_POOL_H

#include <stdint.h>

#include "openrac/guest.h"

/* The most spare slots a request adds to a level (each takes 0x180 bytes of the level's heap). */
#define RAC2_POOL_EXTRA_MAX 1024

/* Asks for `extra` more spare slots in every level loaded from now on (the largest request of any
 * mod counts, at most RAC2_POOL_EXTRA_MAX). Returns what is granted, 0 for none. */
int rac2_pool_raise(int extra);

/* The pool as the level in memory has it: [0] its spare slots in all (the pool the game's
 * CreateMoby gives from), [1] the ones alive, [2] the free ones, [3] the extra slots this load
 * added, [4] the extra slots asked for. 0 when there is no level, -1 when the table is unreadable. */
int rac2_pool_status(int32_t out[5]);

/* The level loader (LoadLevelData) begins: the gameplay file it will decompress is the load
 * context's. */
void rac2_pool_loading(void);

/* The loader has returned. */
void rac2_pool_loaded(void);

/* A WAD decompressed to `to` (wad.c): the gameplay file of a loader that named it gets its extra
 * spare slots. */
void rac2_wad_decompressed(gaddr to);

#endif /* OPENRAC_RAC2_MOBY_POOL_H */
