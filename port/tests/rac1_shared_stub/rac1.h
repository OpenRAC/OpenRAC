/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * What port/game/rac1/host/shared.c needs of rac1.h, for rac1_shared_test.c: game memory is the
 * test's array, and an address is an offset in it.
 */
#ifndef RAC1_SHARED_STUB_H
#define RAC1_SHARED_STUB_H
/* The real rac1.h, which shared.c names beside itself, stays out. */
#define OPENRAC_RAC1_H

#include <stdint.h>

typedef uint32_t gaddr;

#define OPENRAC_GAME_SHARED_GOLD_WEAPONS 1
#define OPENRAC_GAME_SHARED_SKILL_POINTS 2
#define OPENRAC_GAME_SHARED_LEVEL_FLAGS 3
#define OPENRAC_GAME_SHARED_SPECIAL_ITEMS 4

extern uint8_t test_memory[0x200000];
extern int test_level_running;

#define RAC1_ADDR(a) (a)
#define OPENRAC_DATA(a) (a)
#define G(a) ((void*)(test_memory + (a)))

static inline int rac1_level_running(void) {
    return test_level_running;
}

int rac1_shared_read(int tag, void* out, uint32_t size);
int rac1_shared_give(int tag, const void* data, uint32_t size);
int rac1_mission_state_read(int level, uint8_t* out);
int rac1_mission_state_write(int level, const uint8_t* in);

#endif
