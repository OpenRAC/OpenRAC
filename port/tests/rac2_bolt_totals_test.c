/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac2/host/bolt_totals.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static void expect(int32_t bolts, const char* text) {
    char out[RAC2_BOLTS_COMPACT_SIZE] = "";
    CHECK(rac2_bolts_compact(out, sizeof out, bolts) == 1);
    if (strcmp(out, text) != 0) {
        fprintf(stderr, "%d: got %s, want %s\n", (int)bolts, out, text);
        exit(1);
    }
}

int main(void) {
    char out[RAC2_BOLTS_COMPACT_SIZE] = "same";
    /* Everything the stock display holds is left alone. */
    CHECK(!rac2_bolts_compact(out, sizeof out, 0));
    CHECK(!rac2_bolts_compact(out, sizeof out, 123456));
    CHECK(!rac2_bolts_compact(out, sizeof out, 9999999));
    CHECK(!rac2_bolts_compact(out, sizeof out, -5));
    CHECK(strcmp(out, "same") == 0);
    expect(10000000, "10.0M");
    expect(12345678, "12.3M");
    expect(99999999, "99.9M");
    expect(123456789, "123.4M");
    expect(2147483647, "2147.4M");
    /* A buffer too small for the longest text is refused rather than overrun. */
    char small[4];
    CHECK(!rac2_bolts_compact(small, sizeof small, 12345678));
    return 0;
}
