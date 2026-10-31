// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

/* A translated RAC1 loader calls this before it touches each expanded-arena
 * allocation. Vanilla RAM follows its retail path without entering this check. */
void game_rac1_check_level_heap(gaddr cursor, unsigned long long bytes) {
    if (openrac_guest_ram_size <= GUEST_RAM_SIZE) {
        return;
    }

    const gaddr heap_end = openrac_lib_EndOfHeap();
    if (cursor > heap_end || bytes > (unsigned long long)heap_end - cursor) {
        openrac_game_crash("RAC1 level allocations exceed expanded RAM heap");
    }
}
