#ifndef WEAPONS_H
#define WEAPONS_H

#include "common.h"

/*
 * Weapon Mod flags for Ratchet & Clank: Going Commando.
 *
 * In RaC2/RaC3, weapon mods are stored as bit flags per byte
 * in a global table indexed by weapon ID (verified by @imnot_im, 2026-09-06).
 */
#define WEAPON_MOD_LOCK_ON   (1 << 0)  /* 0x01: Lock-on mod */
#define WEAPON_MOD_SHOCK     (1 << 1)  /* 0x02: Shock mod */
#define WEAPON_MOD_ACID      (1 << 2)  /* 0x04: Acid mod */

typedef struct WeaponEntry {
    u8 weaponId;
    u8 modFlags;
    u8 level;          /* Upgrade level (v1 .. v5) */
    u8 pad;
    s32 currentAmmo;
    s32 maxAmmo;
} WeaponEntry;

#endif /* WEAPONS_H */
