/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Bind a moby class to its level's update and callback records. The executable's lvl.vtbl is an
 * empty sentinel, while every loaded level has its own table at a different address. Relocating
 * the executable address (0x1E8F00) into level data does not find that section: the loaded level
 * has unrelated bytes there. Use the vtbl location from that disc's program manifest instead.
 */
#include <stdint.h>

#include "openrac/guest.h"
#include "rac1.h"

typedef struct {
    int32_t oclass;
    uint32_t update;
    uint32_t callbacks;
} MobyVtableEntry;

#define MOBY_SLOT_COUNT 256u
#define MOBY_VTBL_MAX_ENTRIES 196u

void func_00213BB8(int oclass) {
    const int level = openrac_guest_overlay();
    const gaddr vtbl = level >= 0 ? RAC1_MOBY_VTBL(level) : 0;
    const gaddr resources = OPENRAC_DATA(RAC1_ADDR(0x001B3580u));
    const gaddr updates = OPENRAC_DATA(RAC1_ADDR(0x001B3900u));
    const gaddr slot_count = OPENRAC_DATA(RAC1_ADDR(0x00160000u));
    const uint32_t slot = GREF(uint32_t, slot_count);
    uint32_t update = 0;
    uint32_t callbacks = 0;

    if (vtbl != 0) {
        for (uint32_t i = 0; i < MOBY_VTBL_MAX_ENTRIES; ++i) {
            const gaddr record = vtbl + i * sizeof(MobyVtableEntry);
            const int32_t key = GREF(int32_t, record);
            if (key == -1) {
                break;
            }
            if (key == oclass) {
                update = GREF(uint32_t, record + 4u);
                callbacks = GREF(uint32_t, record + 8u);
                break;
            }
        }
    } else {
        /* Preserve the executable's retail empty-table behavior outside a level. */
        const gaddr empty = OPENRAC_DATA(RAC1_ADDR(0x001E8F00u));
        if (GREF(int32_t, empty) == oclass) {
            update = GREF(uint32_t, empty + 4u);
            callbacks = GREF(uint32_t, empty + 8u);
        }
    }

    if (slot >= MOBY_SLOT_COUNT) {
        return;
    }
    GREF(uint32_t, updates + slot * 4u) = update;
    const gaddr class_resource = GREF(gaddr, resources + slot * 4u);
    if (class_resource != 0) {
        GREF(uint32_t, class_resource + 0x2Cu) = callbacks;
    }
}
