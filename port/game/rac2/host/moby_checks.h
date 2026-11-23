/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: the checks the port's own C makes before it follows a pointer from a moby
 * (weapon_culling.c, weapon_shots.c, sheep_once.c, play.c's traces).
 *
 * A moby's words are not always what the game's code that owns it reads them as: a free or reused
 * slot keeps the last owner's words, a scene or a program's load (g_nGameState 6) rewrites the table
 * under the port's feet, and NG++'s copies of enemies keep their pvars in the port's own memory
 * (GUEST_PORT). Yeedil's scene before the last boss had a live enemy whose pvars word (+0x68) held
 * a float (179.7, 0x4333AC8F), read as a pointer for its target record (#797); a new game started
 * from Yeedil had a dead shot's pvars word hold another (0x4C58xxxx), read for its timer. So a
 * pointer taken from a moby is followed only when it is a word of the game's RAM (or, where the
 * code may, the port's), the moby a slot of the table, and the port's per-tick work runs only in
 * play.
 */
#ifndef OPENRAC_RAC2_MOBY_CHECKS_H
#define OPENRAC_RAC2_MOBY_CHECKS_H

#include <stdint.h>

#include "game_protos.h"
#include "openrac/guest.h"
#include "rac2.h"

/* The first of the game's RAM where a moby, a class or a record may be: past the console kernel's. */
#define RAC2_RAM_LOW 0x00080000u

/* Whether the `bytes` from `a` are the game's RAM, `a` a word's address. */
static inline int rac2_ram_at(gaddr a, uint32_t bytes) {
    return (a & 3u) == 0 && a >= RAC2_RAM_LOW && a < openrac_guest_ram_size
           && bytes <= openrac_guest_ram_size - a;
}

/* Whether they are the game's RAM or the port's own memory (GUEST_PORT: the mods' copies' pvars). */
static inline int rac2_memory_at(gaddr a, uint32_t bytes) {
    return rac2_ram_at(a, bytes)
           || ((a & 3u) == 0 && a >= GUEST_PORT && a - GUEST_PORT < GUEST_PORT_SIZE
               && bytes <= GUEST_PORT_SIZE - (a - GUEST_PORT));
}

/* Whether `m` is a slot of the moby table (g_mobyTableBase to g_mobyTableEnd, 0x100 bytes each). */
static inline int rac2_moby_slot(gaddr m) {
    const gaddr base = GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableBase));
    const gaddr end = GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableEnd));
    return rac2_ram_at(base, 0x100u) && end > base && end <= openrac_guest_ram_size && m >= base
           && m < end && (m - base) % 0x100u == 0;
}

/* Whether `m` is a live moby of the table: its state (+0x20) under 0x80, not dying, dead or free. */
static inline int rac2_moby_live(gaddr m) {
    return rac2_moby_slot(m) && GREF(uint8_t, m + 0x20u) < 0x80u;
}

/* A moby's pvars (+0x68), when 0x80 bytes of them are memory: the game's RAM, or the port's too for
 * `port`; else 0. */
static inline gaddr rac2_moby_pvars(gaddr m, int port) {
    const gaddr pvars = GREF(gaddr, m + 0x68u);
    return (port ? rac2_memory_at(pvars, 0x80u) : rac2_ram_at(pvars, 0x80u)) ? pvars : 0;
}

/* The including C names the function, `#define RAC2_MOBY_TARGET_RECORD func_002AB1F0` before the
 * include: hostgen renames a release's functions in the .c files only, not in a header. */
#ifndef RAC2_MOBY_TARGET_RECORD
#define RAC2_MOBY_TARGET_RECORD func_002AB1F0
#endif

/* A moby's target record (moby_get_private_value_0, func_002AB1F0: its pvars' first word, with
 * +0x34's 0x20), read only from pvars in RAM and taken only when it is a record of RAM itself (its
 * health at +0, its target height at +0x10); else 0. */
static inline gaddr rac2_target_record(gaddr m) {
    if ((GREF(uint16_t, m + 0x34u) & 0x20u) == 0 || rac2_moby_pvars(m, 0) == 0) {
        return 0;
    }
    const gaddr record = RAC2_MOBY_TARGET_RECORD(m);
    return rac2_ram_at(record, 0x14u) ? record : 0;
}

/* Whether the game is in play: g_nGameState 0 (not a scene, a menu, a dialog, a vendor or a
 * program's load), with the hero's moby a live slot of the table. */
static inline int rac2_in_play(void) {
    const gaddr hero = GREF(gaddr, OPENRAC_DATA(RAC2_g_pHeroMoby));
    return GREF(int32_t, OPENRAC_DATA(RAC2_g_nGameState)) == 0 && rac2_moby_live(hero);
}

#endif
