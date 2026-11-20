/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Tests the RAC2 hero shadow rig with synthetic body and equipment records. */
#include <stdio.h>

#include "hero_attachments.h"

/* Writes a synthetic pointer to the hero's equipment or body field. */
static void write_address(uint8_t* ram, size_t field, uint32_t address) {
    memcpy(ram + field, &address, sizeof(address));
}

/* Keeps body-first ordering, rejects detached, duplicate and freed mobys, and bounds output. */
int main(void) {
    /* The hero state precedes the synthetic moby records. */
    uint8_t ram[0x3000] = {0};
    const uint32_t hero = 0x100;
    const uint32_t body = 0x2800;
    const uint32_t weapon = 0x2900;
    const uint32_t helmet = 0x2A00;
    const uint32_t pack = 0x2B00;
    const uint32_t freed = 0x2C00;
    const size_t slots = hero + 0x1220u;
    const size_t stride = 0x50;
    /* One spare output entry lets the scan reach invalid slots after the valid ones. */
    uint32_t casters[5] = {0};

    /* Fixtures mirror the body, single weapon, paired armour and pack slots. */
    write_address(ram, hero + 0x2290u, body);
    write_address(ram, slots, weapon);
    write_address(ram, slots + stride, body);
    write_address(ram, slots + stride + 4, helmet);
    write_address(ram, slots + stride + 8, freed);
    write_address(ram, slots + 3 * stride, pack);
    write_address(ram, slots + 4 * stride, 0xFFFFFFFFu);
    ram[freed + 0x20u] = 0xFF;

    /* Invalid slot pointers and duplicates leave exactly the four live casters. */
    if (rac2_hero_attachments(ram, sizeof(ram), hero, casters, 5) != 4 || casters[0] != body
        || casters[1] != weapon || casters[2] != helmet || casters[3] != pack) {
        return 1;
    }

    /* A thrown weapon remains in its slot but casts independently until it returns. */
    ram[slots + 0x1Au] = 1;
    if (rac2_hero_attachments(ram, sizeof(ram), hero, casters, 2) != 2 || casters[0] != body
        || casters[1] != helmet) {
        return 1;
    }

    /* A missing body or truncated hero state must not promote equipment to the anchor. */
    write_address(ram, hero + 0x2290u, 0);
    if (rac2_hero_attachments(ram, sizeof(ram), hero, casters, 5) != 0
        || rac2_hero_attachments(ram, slots, hero, casters, 5) != 0) {
        return 1;
    }

    puts("RAC2 hero shadow attachments: OK");
    return 0;
}
