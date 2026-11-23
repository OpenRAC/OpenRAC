/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Copyright (c) 2026 the OpenRAC contributors */
#ifndef OPENRAC_RAC2_GH_FIXES_H
#define OPENRAC_RAC2_GH_FIXES_H

#include <math.h>
#include <stdint.h>

/* Greatest Hits' planet totals (func_US2_002B1D18). Planet 26's bolts belong to
 * Maktar's counter, as in func_US2_002B1C90. These are counts, not asset data. */
static inline unsigned rac2_planet_bolt_total(unsigned planet) {
    static const uint8_t totals[] = {0, 2, 2, 2, 2, 1, 3, 2, 3, 2, 1, 2, 3, 3, 1, 1, 1, 2, 2, 3, 2};
    return planet < sizeof(totals) ? totals[planet] : 0;
}

static inline unsigned rac2_planet_bolt_count(const uint8_t flags[28][4], unsigned planet) {
    if (planet >= 28) {
        return 0;
    }

    unsigned count = 0;

    for (unsigned i = 0; i < 4; ++i) {
        count += flags[planet][i] != 0;

        if (planet == 2) {
            count += flags[26][i] != 0;
        }
    }

    return count;
}

/* The mining update first scales its forward basis into thrust, then scales
 * that same temporary in place after limiting its vertical component. GH uses
 * |acceleration| for the second scale, so reverse thrust keeps its sign.
 * Track that pair, not every negative vector scale in the vehicle's helpers. */
typedef struct rac2_mining_thrust {
    uint32_t forward;
    uint32_t pending;
    float acceleration;
} rac2_mining_thrust;

static inline float rac2_mining_thrust_scale(
    rac2_mining_thrust* phase, uint32_t destination, uint32_t source, float scale
) {
    if (phase == 0) {
        return scale;
    }

    const uint32_t pending = phase->pending;
    const float acceleration = phase->acceleration;
    phase->pending = 0;

    if (source == phase->forward && destination != source) {
        phase->pending = destination;
        phase->acceleration = scale;
    } else if (pending != 0 && source == pending && destination == source
               && scale == acceleration) {
        return fabsf(scale);
    }

    return scale;
}

#endif
