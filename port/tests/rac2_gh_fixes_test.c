/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#include <stdio.h>
#include <stdlib.h>

#include "../game/rac2/host/gh_fixes.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

int main(void) {
    uint8_t flags[28][4] = {{0}};
    unsigned total = 0;
    for (unsigned i = 0; i <= 20; ++i) {
        total += rac2_planet_bolt_total(i);
        CHECK(rac2_planet_bolt_count(flags, i) == 0);
    }
    CHECK(total == 40);
    flags[2][0] = 1;
    flags[26][1] = 128;
    flags[1][0] = 1;
    CHECK(rac2_planet_bolt_count(flags, 2) == 2);
    CHECK(rac2_planet_bolt_total(2) == 2);
    CHECK(rac2_planet_bolt_count(flags, 1) == 1);
    CHECK(rac2_planet_bolt_count(flags, 26) == 1);
    CHECK(rac2_planet_bolt_count(flags, UINT32_MAX) == 0);
    CHECK(rac2_planet_bolt_total(21) == 0);
    CHECK(rac2_planet_bolt_total(UINT32_MAX) == 0);

    rac2_mining_thrust phase = {0x10C0, 0, 0};
    /* Backward stays backward after both scales; forward stays forward. */
    for (int direction = -1; direction <= 1; direction += 2) {
        const float acceleration = direction * 0.5f;
        const float first = rac2_mining_thrust_scale(&phase, 0x7000, 0x10C0, acceleration);
        const float second = rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, acceleration);
        CHECK(first * second == direction * 0.25f);
        CHECK(phase.pending == 0);
    }
    CHECK(rac2_mining_thrust_scale(0, 0x7000, 0x7000, -0.5f) == -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, -0.5f) == -0.5f);
    rac2_mining_thrust_scale(&phase, 0x7000, 0x10C0, -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7100, 0x10D0, -0.5f) == -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, -0.5f) == -0.5f);
    rac2_mining_thrust_scale(&phase, 0x7000, 0x10C0, -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, -0.25f) == -0.25f);
    CHECK(phase.pending == 0);
    puts("RAC2 per-planet counts and mining reverse controls passed");
    return 0;
}
