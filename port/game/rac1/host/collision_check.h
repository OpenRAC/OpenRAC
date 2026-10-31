/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the collision queries (collision.c) checked against the kernels' own
 * instructions (collision_check.c).
 */
#ifndef OPENRAC_RAC1_COLLISION_CHECK_H
#define OPENRAC_RAC1_COLLISION_CHECK_H

#include <stddef.h>
#include <stdint.h>

#include "openrac/guest.h"

enum {
    RAC_COLL_LINE,
    RAC_COLL_SPHERE,
    RAC_COLL_CAPSULE,
    RAC_COLL_SPHERE_MOBYS,
    RAC_COLL_HERO_GROUPS,
    RAC_COLL_LINE_BOOT,
    RAC_COLL_KINDS
};

/* A query's arguments: its floats, then its words, each in the order the kernel takes them. */
typedef struct {
    float f[2];
    uint32_t u[5];
} RacCollArgs;

/* collision.c: a query as this port runs it; its state between calls (copied out to `to`, in from
 * `from`, either may be NULL; *size its bytes); the hit registers it keeps; the CollOutput. */
int rac1_coll_host(int kind, const RacCollArgs* a);
void rac1_coll_state(void* to, const void* from, size_t* size);
const uint32_t (*rac1_coll_vf(void))[4];
gaddr rac1_coll_output(int boot);

/* collision_check.c: whether OPENRAC_COLL_CHECK is on, and a query run both ways and compared
 * (the result is this port's). */
int rac1_coll_check_on(void);
int rac1_coll_check(int kind, const RacCollArgs* a);

static inline int rac1_coll_checking(void) {
    static int on = -1;
    if (on < 0) {
        on = rac1_coll_check_on();
    }
    return on;
}

#endif
