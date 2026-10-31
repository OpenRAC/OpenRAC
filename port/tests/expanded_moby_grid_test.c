/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#include <stdio.h>
#include <stdlib.h>

#include "../game/rac1/host/moby_grid.c"

OPENRAC_GUEST_GLOBAL uint8_t* openrac_guest_base;
OPENRAC_GUEST_GLOBAL uint32_t openrac_guest_ram_size;

uint32_t rac1_program_loads(void) {
    return 1;
}

void openrac_game_crash(const char* reason) {
    fprintf(stderr, "unexpected game crash: %s\n", reason);
    abort();
}

void game_rac1_warn_moby_grid_full(void) {
    fprintf(stderr, "unexpected expanded moby grid exhaustion\n");
    abort();
}

#define CHECK(condition)                                                                          \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%d: %s\n", __LINE__, #condition);                                   \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int cell_has(unsigned x, unsigned y, uint16_t id) {
    int count = 0;
    const gaddr entries = rac1_expanded_grid_cell(x, y, &count);
    return count == 1 && GREF(uint16_t, entries) == id;
}

int main(void) {
    openrac_guest_base = calloc(1, GUEST_RAM_EXPANDED_SIZE);
    CHECK(openrac_guest_base != NULL);
    openrac_guest_ram_size = GUEST_RAM_EXPANDED_SIZE;
    const gaddr base = rac1_expanded_grid_base();

    /* These three keys differ by one hash-table length, so they start at the same slot. */
    CHECK(grid_add(base, 64, 64, 41));
    CHECK(grid_add(base, 64, 96, 42));
    const gaddr first = grid_cell_address(base, 64, 64, 0);
    CHECK(first != 0);
    CHECK(cell_has(64, 64, 41));
    CHECK(cell_has(64, 96, 42));

    /* Removing the first cell leaves a tombstone before the still-live second key. */
    CHECK(grid_remove(base, 64, 64, 41));
    CHECK(grid_cell_address(base, 64, 64, 0) == 0);
    CHECK(cell_has(64, 96, 42));

    /* A new colliding key reuses the tombstone without hiding the key after it. */
    CHECK(grid_add(base, 64, 128, 43));
    CHECK(grid_cell_address(base, 64, 128, 0) == first);
    CHECK(cell_has(64, 128, 43));
    CHECK(cell_has(64, 96, 42));
    CHECK(!cell_has(64, 64, 41));

    /* The former 65,536-slot ceiling must not return as the table fills. */
    for (uint32_t i = 0; i < 65537; ++i) {
        const unsigned x = 64u + i % 8128u;
        const unsigned y = 64u + i / 8128u;
        CHECK(grid_add(base, x, y, (uint16_t)i));
    }
    CHECK(cell_has(64u + 65536u % 8128u, 64u + 65536u / 8128u, (uint16_t)65536u));

    free(openrac_guest_base);
    return 0;
}
