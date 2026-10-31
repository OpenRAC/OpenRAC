/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Expanded RAC1 collision grid. The retail grid remains untouched for the 32 MiB path. */
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "rac1.h"

enum {
    GRID_HEADER = 0x10,
    GRID_CELLS = GRID_HEADER,
    GRID_CELL_SIZE = 12,
    GRID_CELL_COUNT = 64 * 64,
    GRID_CELLS_SIZE = GRID_CELL_SIZE * GRID_CELL_COUNT,
    GRID_POOL = GRID_CELLS + GRID_CELLS_SIZE,
    GRID_POOL_BYTES = 0x01000000,
    GRID_BLOCK_SIZE = 32,
    GRID_BLOCKS = GRID_POOL_BYTES / GRID_BLOCK_SIZE,
    GRID_BITMAP = GRID_POOL + GRID_POOL_BYTES,
    GRID_BITMAP_WORDS = GRID_BLOCKS / 32,
    GRID_USED_BYTES = GRID_BITMAP + GRID_BITMAP_WORDS * 4
};

_Static_assert(GRID_USED_BYTES <= OPENRAC_EXPANDED_MOBY_GRID_RESERVE,
               "expanded moby grid exceeds its guest RAM reservation");
_Static_assert(GUEST_RAM_EXPANDED_SIZE > OPENRAC_EXPANDED_RAC1_RESERVE,
               "expanded RAC1 reservation leaves no heap");

static uint32_t grid_program_load = UINT32_MAX;
/* Non-authoritative next-fit hints, one for each power-of-two block size. */
static uint32_t grid_allocator_cursor[20];

/* Host collision queries and the translated updater use the same expanded grid. A new
 * ParseBin load starts with empty cells and an empty pool. Guest RAM snapshots include the grid. */
gaddr rac1_expanded_grid_base(void) {
    if (openrac_guest_ram_size <= GUEST_RAM_SIZE) {
        return 0;
    }
    const gaddr base = openrac_guest_ram_size - OPENRAC_EXPANDED_RAC1_RESERVE;
    const uint32_t load = rac1_program_loads();
    if (grid_program_load != load) {
        memset(G(base), 0, GRID_USED_BYTES);
        GREF(uint32_t, base + GRID_BITMAP) = 1; /* block 0 is the null block index */
        memset(grid_allocator_cursor, 0, sizeof grid_allocator_cursor);
        grid_program_load = load;
    }
    return base;
}

static gaddr grid_pool(gaddr base) {
    return base + GRID_POOL;
}

static gaddr grid_bitmap(gaddr base) {
    return base + GRID_BITMAP;
}

static int grid_range_free(gaddr bitmap, uint32_t first, uint32_t blocks) {
    const uint32_t end = first + blocks;
    for (uint32_t word = first >> 5; word <= (end - 1) >> 5; ++word) {
        const uint32_t word_first = word << 5;
        const uint32_t lo = first > word_first ? first - word_first : 0;
        const uint32_t hi = end < word_first + 32 ? end - word_first : 32;
        const uint32_t width = hi - lo;
        const uint32_t mask = width == 32 ? UINT32_MAX : ((1u << width) - 1u) << lo;
        if (GREF(uint32_t, bitmap + word * 4) & mask) {
            return 0;
        }
    }
    return 1;
}

static void grid_range_mark(gaddr bitmap, uint32_t first, uint32_t blocks, int used) {
    const uint32_t end = first + blocks;
    for (uint32_t word = first >> 5; word <= (end - 1) >> 5; ++word) {
        const uint32_t word_first = word << 5;
        const uint32_t lo = first > word_first ? first - word_first : 0;
        const uint32_t hi = end < word_first + 32 ? end - word_first : 32;
        const uint32_t width = hi - lo;
        const uint32_t mask = width == 32 ? UINT32_MAX : ((1u << width) - 1u) << lo;
        if (used) {
            GREF(uint32_t, bitmap + word * 4) |= mask;
        } else {
            GREF(uint32_t, bitmap + word * 4) &= ~mask;
        }
    }
}

static int grid_allocate(gaddr base, uint32_t blocks, uint32_t* first_out) {
    if (blocks == 0 || (blocks & (blocks - 1)) != 0 || blocks > GRID_BLOCKS) {
        return 0;
    }
    const gaddr bitmap = grid_bitmap(base);
    const uint32_t aligned_end = GRID_BLOCKS & ~(blocks - 1u);
    unsigned order = 0;
    for (uint32_t size = blocks; size > 1; size >>= 1) {
        ++order;
    }
    const uint32_t start = grid_allocator_cursor[order] < aligned_end
                               ? grid_allocator_cursor[order]
                               : 0;
    for (unsigned pass = 0; pass < 2; ++pass) {
        const uint32_t from = pass == 0 ? start : 0;
        const uint32_t to = pass == 0 ? aligned_end : start;
        for (uint32_t first = from; first < to; first += blocks) {
            if (grid_range_free(bitmap, first, blocks)) {
                grid_range_mark(bitmap, first, blocks, 1);
                *first_out = first;
                grid_allocator_cursor[order] = first + blocks;
                if (grid_allocator_cursor[order] >= aligned_end) {
                    grid_allocator_cursor[order] = 0;
                }
                return 1;
            }
        }
    }
    return 0;
}

static void grid_free(gaddr base, uint32_t first, uint32_t blocks) {
    if (blocks != 0 && first + blocks <= GRID_BLOCKS) {
        grid_range_mark(grid_bitmap(base), first, blocks, 0);
    }
}

static gaddr grid_cell_address(gaddr base, unsigned x, unsigned y) {
    return base + GRID_CELLS + (x + y * 64u) * GRID_CELL_SIZE;
}

static int rect_contains(uint32_t rect, int x, int y) {
    return !(rect & 0x80000000u) && x >= (int)(rect & 255u)
        && y >= (int)((rect >> 8) & 255u) && x <= (int)((rect >> 16) & 255u)
        && y <= (int)(rect >> 24);
}

static int rect_valid(uint32_t rect) {
    return (rect & 0x80000000u)
        || ((rect & 255u) <= ((rect >> 16) & 255u)
            && ((rect >> 8) & 255u) <= (rect >> 24)
            && ((rect >> 16) & 255u) <= 64u
            && (rect >> 24) * 64u + ((rect >> 16) & 255u) < GRID_CELL_COUNT);
}

static int grid_add(gaddr base, unsigned x, unsigned y, uint16_t id) {
    const gaddr cell = grid_cell_address(base, x, y);
    uint32_t first = GREF(uint32_t, cell);
    uint32_t count = GREF(uint32_t, cell + 4);
    uint32_t capacity = GREF(uint32_t, cell + 8);
    if (count == capacity * 16u) {
        const uint32_t grown_capacity = capacity ? capacity * 2u : 1u;
        uint32_t grown_first;
        if (!grid_allocate(base, grown_capacity, &grown_first)) {
            return 0;
        }
        if (count != 0) {
            memcpy(G(grid_pool(base) + grown_first * GRID_BLOCK_SIZE),
                   G(grid_pool(base) + first * GRID_BLOCK_SIZE), count * sizeof(uint16_t));
            grid_free(base, first, capacity);
        }
        first = grown_first;
        capacity = grown_capacity;
        GREF(uint32_t, cell) = first;
        GREF(uint32_t, cell + 8) = capacity;
    }
    GREF(uint16_t, grid_pool(base) + first * GRID_BLOCK_SIZE + count * 2u) = id;
    GREF(uint32_t, cell + 4) = count + 1;
    return 1;
}

static int grid_add_rect(gaddr base, uint32_t rect, uint32_t keep, uint16_t id) {
    if (rect & 0x80000000u) {
        return 1;
    }
    for (unsigned y = (rect >> 8) & 255u; y <= rect >> 24; ++y) {
        for (unsigned x = rect & 255u; x <= ((rect >> 16) & 255u); ++x) {
            if (!rect_contains(keep, (int)x, (int)y) && !grid_add(base, x, y, id)) {
                return 0;
            }
        }
    }
    return 1;
}

static int grid_remove(gaddr base, unsigned x, unsigned y, uint16_t id) {
    const gaddr cell = grid_cell_address(base, x, y);
    const uint32_t first = GREF(uint32_t, cell);
    const uint32_t count = GREF(uint32_t, cell + 4);
    uint32_t capacity = GREF(uint32_t, cell + 8);
    if (count == 0 || capacity == 0) {
        return 0;
    }
    const gaddr entries = grid_pool(base) + first * GRID_BLOCK_SIZE;
    uint32_t found = 0;
    while (found < count && GREF(uint16_t, entries + found * 2u) != id) {
        ++found;
    }
    if (found == count) {
        return 0;
    }
    const uint32_t new_count = count - 1;
    if (found != new_count) {
        GREF(uint16_t, entries + found * 2u) = GREF(uint16_t, entries + new_count * 2u);
    }
    GREF(uint16_t, entries + new_count * 2u) = 0;
    GREF(uint32_t, cell + 4) = new_count;
    if (new_count == 0) {
        grid_free(base, first, capacity);
        GREF(uint32_t, cell) = 0;
        GREF(uint32_t, cell + 8) = 0;
    } else {
        /* Return half of an underused list as it shrinks. Keep four spare entries, as the
         * retail grid does, to avoid reallocating on small count fluctuations. */
        while (capacity > 1 && new_count < capacity * 8u - 4u) {
            capacity >>= 1;
            grid_free(base, first + capacity, capacity);
            GREF(uint32_t, cell + 8) = capacity;
        }
    }
    return 1;
}

/* Undo an insertion attempt, including any cell-list growth performed before a later cell ran
 * out of blocks. Allocations are aligned power-of-two ranges, so the unused upper half of each
 * grown range can be returned without copying the retained entries. */
static void grid_rollback_add_rect(gaddr base, uint32_t rect, uint32_t keep, uint16_t id) {
    if (rect & 0x80000000u) {
        return;
    }
    for (unsigned y = (rect >> 8) & 255u; y <= rect >> 24; ++y) {
        for (unsigned x = rect & 255u; x <= ((rect >> 16) & 255u); ++x) {
            if (rect_contains(keep, (int)x, (int)y) || !grid_remove(base, x, y, id)) {
                continue;
            }
            const gaddr cell = grid_cell_address(base, x, y);
            const uint32_t first = GREF(uint32_t, cell);
            uint32_t capacity = GREF(uint32_t, cell + 8);
            const uint32_t count = GREF(uint32_t, cell + 4);
            while (capacity > 1 && count <= capacity * 8u) {
                capacity >>= 1;
                grid_free(base, first + capacity, capacity);
                GREF(uint32_t, cell + 8) = capacity;
            }
        }
    }
}

static void grid_remove_rect(gaddr base, uint32_t rect, uint32_t keep, uint16_t id,
                             int require_all) {
    if (rect & 0x80000000u) {
        return;
    }
    for (unsigned y = (rect >> 8) & 255u; y <= rect >> 24; ++y) {
        for (unsigned x = rect & 255u; x <= ((rect >> 16) & 255u); ++x) {
            if (!rect_contains(keep, (int)x, (int)y) && !grid_remove(base, x, y, id)
                && require_all) {
                openrac_game_crash("expanded moby collision grid lost a moby index");
                return;
            }
        }
    }
}

/* Hostgen calls this only when expanded RAM is selected. Vanilla keeps the original updater and
 * the original four-byte cell layout. */
void game_rac1_update_expanded_moby_grid(gaddr moby, uint32_t next) {
    const uint32_t old = GREF(uint32_t, moby + 0xA0u);
    const uint16_t id = (uint16_t)GREF(uint32_t, moby + 0xACu);
    if (!rect_valid(old) || !rect_valid(next)) {
        openrac_game_crash("expanded moby collision grid received an invalid rectangle");
        return;
    }
    const gaddr base = rac1_expanded_grid_base();
    const int same_moby = old == next;
    if (same_moby) {
        return;
    }

    /* Add newly covered cells before removing old cells. If pool allocation fails, roll back
     * those additions and retain the previous grid rectangle and its collision coverage. */
    if (!grid_add_rect(base, next, old, id)) {
        grid_rollback_add_rect(base, next, old, id);
        game_rac1_warn_moby_grid_full();
        return;
    }
    grid_remove_rect(base, old, next, id, 1);
    GREF(uint32_t, moby + 0xA0u) = next;
}
