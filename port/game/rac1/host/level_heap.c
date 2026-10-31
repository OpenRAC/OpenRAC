// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <limits.h>
#include <stdint.h>
#include <stdio.h>

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
        const unsigned long long available = cursor > heap_end ? 0 : heap_end - cursor;
        const unsigned long long cursor_over = cursor > heap_end ? cursor - heap_end : 0;
        const unsigned long long size_over = bytes > available ? bytes - available : 0;
        const unsigned long long shortfall =
            ULLONG_MAX - size_over < cursor_over ? ULLONG_MAX : size_over + cursor_over;
        char reason[192];
        snprintf(
            reason,
            sizeof(reason),
            "RAC1 level %d needs %llu more heap bytes than remain (%llu requested, %llu free)",
            openrac_guest_overlay(),
            shortfall,
            bytes,
            available
        );
        openrac_game_crash(reason);
    }
}
