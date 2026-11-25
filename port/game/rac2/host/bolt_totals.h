/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's bolt totals past 9,999,999. The HUD's counter panel has room for seven digits
 * and the Load page clamps the total it lists; a mod that pays out more would overflow the one
 * and read as capped on the other. From 10,000,000 both show the total in millions, to a tenth:
 * "12.3M". Below that nothing changes. */
#ifndef OPENRAC_RAC2_BOLT_TOTALS_H
#define OPENRAC_RAC2_BOLT_TOTALS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define RAC2_BOLTS_STOCK_MAX 9999999

/* The longest text: "2147.4M" and its terminator. */
#define RAC2_BOLTS_COMPACT_SIZE 8

/* Writes the compact form of `bolts` and returns 1 when it is past what the stock display holds,
 * else writes nothing and returns 0. Truncates to the tenth, as a counter would. */
static inline int rac2_bolts_compact(char* out, size_t size, int32_t bolts) {
    if (bolts <= RAC2_BOLTS_STOCK_MAX || size < RAC2_BOLTS_COMPACT_SIZE) {
        return 0;
    }
    const int32_t tenths = bolts / 100000;
    snprintf(out, size, "%d.%dM", (int)(tenths / 10), (int)(tenths % 10));
    return 1;
}

#endif
