/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Going Commando's current body and equipped mobys for the hero shadow rig. */
#ifndef OPENRAC_RAC2_HERO_ATTACHMENTS_H
#define OPENRAC_RAC2_HERO_ATTACHMENTS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Appends one live moby from a validated pointer field when the caller has room. */
static inline unsigned rac2_append_hero_moby(
    const uint8_t* ram, size_t size, size_t field, uint32_t* addresses, unsigned count
) {
    uint32_t address;
    memcpy(&address, ram + field, sizeof(address));

    /* A moby's record is 0x100 bytes and its signed state at +0x20 marks freed entries. */
    if (!address || address > size || size - address < 0x100u || ram[address + 0x20u] >= 0x80u) {
        return count;
    }

    for (unsigned i = 0; i < count; ++i) {
        /* Shared pointers in the item slots describe the same caster. */
        if (addresses[i] == address) {
            return count;
        }
    }

    addresses[count] = address;
    return count + 1;
}

/* Writes the live body first and then its attached equipment, up to capacity.
 * Layout comes from SpawnHeroItemMobies and update_hero_item_mobys (seen in game code). */
static inline unsigned rac2_hero_attachments(
    const uint8_t* ram, size_t size, uint32_t hero, uint32_t* addresses, unsigned capacity
) {
    /* The body pointer at +0x2290 ends after all of the item slots. */
    const size_t body_offset = 0x2290u;
    if (!ram || !addresses || !capacity || !hero || hero > size
        || size - hero < body_offset + sizeof(uint32_t)) {
        return 0;
    }

    unsigned count = rac2_append_hero_moby(ram, size, hero + body_offset, addresses, 0);
    /* Without a live body, an item must not become the projection's anchor. */
    if (!count) {
        return 0;
    }

    /* Seven 0x50-byte slots start at +0x1220; +0x1A marks a detached item. */
    for (unsigned slot = 0; slot < 7 && count < capacity; ++slot) {
        const size_t at = hero + 0x1220u + slot * 0x50u;
        /* A thrown item stays in its slot until it returns to the hero's joint. */
        if (ram[at + 0x1Au]) {
            continue;
        }

        /* Only the armour and pack/accessory slots attach secondary mobys. */
        const unsigned parts = slot == 1 || slot == 3 || slot == 4 ? 3 : 1;
        for (unsigned part = 0; part < parts && count < capacity; ++part) {
            count =
                rac2_append_hero_moby(ram, size, at + part * sizeof(uint32_t), addresses, count);
        }
    }

    return count;
}

#endif
