/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Expanded RAC1 collision grid. The retail grid remains untouched for the 32 MiB path. */
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "openrac/game_host.h"
#include "rac1.h"

#define F32(a) GREF(float, (a))

enum {
    GRID_HEADER = 0x10,
    GRID_CELLS = GRID_HEADER,
    GRID_CELL_SIZE = 12,
    GRID_CELL_COUNT = 64 * 64,
    GRID_CELLS_SIZE = GRID_CELL_SIZE * GRID_CELL_COUNT,
    GRID_POOL = GRID_CELLS + GRID_CELLS_SIZE,
    GRID_POOL_BYTES = 0x00E80000,
    GRID_BLOCK_SIZE = 32,
    GRID_BLOCKS = GRID_POOL_BYTES / GRID_BLOCK_SIZE,
    GRID_BITMAP = GRID_POOL + GRID_POOL_BYTES,
    GRID_BITMAP_WORDS = GRID_BLOCKS / 32,
    GRID_HASH = GRID_BITMAP + GRID_BITMAP_WORDS * 4,
    GRID_HASH_COUNT = 262144,
    GRID_HASH_CELL_SIZE = 16,
    GRID_BOUNDS = GRID_HASH + GRID_HASH_COUNT * GRID_HASH_CELL_SIZE,
    GRID_BOUNDS_COUNT = 32768,
    GRID_USED_BYTES = GRID_BOUNDS + GRID_BOUNDS_COUNT * 8
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
        memset(G(base + GRID_BOUNDS), 0xFF, GRID_BOUNDS_COUNT * 8u);
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

/* Cells in the original 64x64 area retain their compact direct table. The rest of the
 * expanded 8192x8192 world uses a fixed open-addressed hash table stored in guest RAM. Hash
 * slots are never deleted during a level load, so an empty key remains a valid probe terminator. */
static gaddr grid_cell_address(gaddr base, unsigned x, unsigned y, int create) {
    if (x < 64 && y < 64) {
        return base + GRID_CELLS + (x + y * 64u) * GRID_CELL_SIZE;
    }
    if (x >= 8192 || y >= 8192) {
        return 0;
    }
    const uint32_t key = 1u + x + y * 8192u;
    uint32_t slot = (key * 2654435761u) & (GRID_HASH_COUNT - 1u);
    for (uint32_t probe = 0; probe < GRID_HASH_COUNT; ++probe) {
        const gaddr cell = base + GRID_HASH + slot * GRID_HASH_CELL_SIZE;
        const uint32_t found = GREF(uint32_t, cell + 12u);
        if (found == key) {
            return cell;
        }
        if (found == 0) {
            if (!create) {
                return 0;
            }
            GREF(uint32_t, cell + 12u) = key;
            return cell;
        }
        slot = (slot + 1u) & (GRID_HASH_COUNT - 1u);
    }
    return 0;
}

static int grid_add(gaddr base, unsigned x, unsigned y, uint16_t id) {
    const gaddr cell = grid_cell_address(base, x, y, 1);
    if (cell == 0) {
        return 0;
    }
    uint32_t first = GREF(uint32_t, cell);
    uint32_t count = GREF(uint32_t, cell + 4);
    uint32_t capacity = GREF(uint32_t, cell + 8);
    if (count == capacity * 16u) {
        const uint32_t grown_capacity = capacity ? capacity * 2u : 1u;
        uint32_t grown_first;
        if (!grid_allocate(base, grown_capacity, &grown_first)) {
            if (count == 0 && cell >= base + GRID_HASH && cell < base + GRID_BOUNDS) {
                GREF(uint32_t, cell + 12u) = 0;
            }
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

static int grid_remove(gaddr base, unsigned x, unsigned y, uint16_t id) {
    const gaddr cell = grid_cell_address(base, x, y, 0);
    if (cell == 0) {
        return 0;
    }
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

typedef struct {
    unsigned x0, y0, x1, y1;
    int valid;
} GridRect;

static int floor_cell(float x1024, int64_t* cell) {
    if (!isfinite(x1024)) {
        return 0;
    }
    const double q = floor((double)x1024 / 16384.0);
    if (q < INT32_MIN || q > INT32_MAX) {
        return 0;
    }
    *cell = (int64_t)q;
    return 1;
}

static GridRect moby_rect(gaddr moby) {
    GridRect rect = {0, 0, 0, 0, 0};
    int64_t x0, y0, x1, y1;
    const float radius = F32(moby + 0xCu);
    if (!(radius >= 0.0f) || !floor_cell(F32(moby) - radius, &x0)
        || !floor_cell(F32(moby + 4u) - radius, &y0)
        || !floor_cell(F32(moby) + radius, &x1)
        || !floor_cell(F32(moby + 4u) + radius, &y1)
        || x1 < 0 || y1 < 0 || x0 >= 8192 || y0 >= 8192) {
        return rect;
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= 8192) x1 = 8191;
    if (y1 >= 8192) y1 = 8191;
    rect.x0 = (unsigned)x0;
    rect.y0 = (unsigned)y0;
    rect.x1 = (unsigned)x1;
    rect.y1 = (unsigned)y1;
    rect.valid = rect.x0 <= rect.x1 && rect.y0 <= rect.y1;
    return rect;
}

static int grid_rect_contains(GridRect rect, unsigned x, unsigned y) {
    return rect.valid && x >= rect.x0 && x <= rect.x1 && y >= rect.y0 && y <= rect.y1;
}

static uint64_t grid_rect_area(GridRect rect) {
    return rect.valid ? ((uint64_t)rect.x1 - rect.x0 + 1u)
                          * ((uint64_t)rect.y1 - rect.y0 + 1u)
                      : 0;
}

static uint32_t grid_rect_packed(GridRect rect) {
    return rect.valid ? ((rect.x0 & 255u) | ((rect.y0 & 255u) << 8)
                         | ((rect.x1 & 255u) << 16) | ((rect.y1 & 255u) << 24))
                      : 0;
}

static GridRect grid_rect_load(gaddr base, uint16_t id) {
    const gaddr at = base + GRID_BOUNDS + (gaddr)id * 8u;
    const unsigned x0 = GREF(uint16_t, at);
    GridRect rect = {x0, GREF(uint16_t, at + 2u), GREF(uint16_t, at + 4u),
                     GREF(uint16_t, at + 6u), x0 != 0xFFFFu};
    return rect;
}

static void grid_rect_store(gaddr base, uint16_t id, GridRect rect) {
    const gaddr at = base + GRID_BOUNDS + (gaddr)id * 8u;
    GREF(uint16_t, at) = rect.valid ? (uint16_t)rect.x0 : 0xFFFFu;
    GREF(uint16_t, at + 2u) = rect.valid ? (uint16_t)rect.y0 : 0;
    GREF(uint16_t, at + 4u) = rect.valid ? (uint16_t)rect.x1 : 0;
    GREF(uint16_t, at + 6u) = rect.valid ? (uint16_t)rect.y1 : 0;
}

static int grid_add_wide_rect(gaddr base, GridRect rect, GridRect keep, uint16_t id) {
    if (!rect.valid) return 1;
    for (unsigned y = rect.y0; y <= rect.y1; ++y) {
        for (unsigned x = rect.x0; x <= rect.x1; ++x) {
            if (!grid_rect_contains(keep, x, y) && !grid_add(base, x, y, id)) return 0;
        }
    }
    return 1;
}

static void grid_remove_wide_rect(gaddr base, GridRect rect, GridRect keep, uint16_t id,
                                  int require_all) {
    if (!rect.valid) return;
    for (unsigned y = rect.y0; y <= rect.y1; ++y) {
        for (unsigned x = rect.x0; x <= rect.x1; ++x) {
            if (!grid_rect_contains(keep, x, y) && !grid_remove(base, x, y, id) && require_all) {
                openrac_game_crash("expanded moby collision grid lost a moby index");
                return;
            }
        }
    }
}

static void grid_rollback_wide_rect(gaddr base, GridRect rect, GridRect keep, uint16_t id) {
    if (!rect.valid) return;
    for (unsigned y = rect.y0; y <= rect.y1; ++y) {
        for (unsigned x = rect.x0; x <= rect.x1; ++x) {
            if (!grid_rect_contains(keep, x, y)) grid_remove(base, x, y, id);
        }
    }
}

/* Hostgen calls this only in expanded RAM. Moby sphere coordinates use x1024 units; 16-unit
 * cells therefore map to 16384 in the stored sphere representation. */
void game_rac1_update_expanded_moby_grid(gaddr moby, uint32_t next) {
    const uint32_t raw_id = GREF(uint32_t, moby + 0xACu);
    if (raw_id >= GRID_BOUNDS_COUNT) {
        game_rac1_warn_moby_grid_full();
        return;
    }
    const uint16_t id = (uint16_t)raw_id;
    const gaddr base = rac1_expanded_grid_base();
    const GridRect old = grid_rect_load(base, id);
    /* The native updater uses -1 to remove a moby from the grid. The retail packed rectangle can
     * also equal -1 at a valid expanded coordinate, so distinguish it by recomputing the sphere's
     * low-byte rectangle before treating it as removal. */
    GridRect fresh = moby_rect(moby);
    if (next == UINT32_MAX && grid_rect_packed(fresh) != next) {
        fresh.valid = 0;
    }
    if (old.valid == fresh.valid && (!old.valid || (old.x0 == fresh.x0 && old.y0 == fresh.y0
        && old.x1 == fresh.x1 && old.y1 == fresh.y1))) {
        return;
    }
    /* A bounding sphere wider than the complete addressable cell table cannot be represented.
     * Reject it before walking the rectangle, keeping malformed or intentionally enormous bounds
     * from stalling a frame while retaining the moby's previous valid coverage. */
    if (grid_rect_area(fresh) > GRID_HASH_COUNT + GRID_CELL_COUNT) {
        game_rac1_warn_moby_grid_full();
        return;
    }
    if (!grid_add_wide_rect(base, fresh, old, id)) {
        grid_rollback_wide_rect(base, fresh, old, id);
        game_rac1_warn_moby_grid_full();
        return;
    }
    grid_remove_wide_rect(base, old, fresh, id, 1);
    grid_rect_store(base, id, fresh);
    /* Native readers never consume this field in expanded mode; mark it outside the retail grid. */
    GREF(uint32_t, moby + 0xA0u) = 0x80000000u;
}

gaddr rac1_expanded_grid_cell(unsigned x, unsigned y, int* count) {
    const gaddr base = rac1_expanded_grid_base();
    if (x >= 8192 || y >= 8192) {
        *count = 0;
        return 0;
    }
    const gaddr cell = grid_cell_address(base, x, y, 0);
    if (cell == 0) {
        *count = 0;
        return 0;
    }
    *count = (int)GREF(uint32_t, cell + 4u);
    return base + GRID_POOL + GREF(uint32_t, cell) * GRID_BLOCK_SIZE;
}
