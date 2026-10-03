#ifndef FLAGS_H
#define FLAGS_H

#include "common.h"

/*
 * Global Game and Event Flag conventions for Ratchet & Clank: Going Commando.
 *
 * Recovered from internal debug symbols and assertions in the Aug 8 2002 prototype
 * (@creepnt, @chaoticgd 2026):
 *
 * Format:
 *   GLOBAL_RC2FLAG_L<level_id>_evt_<event_id>
 *
 * Examples:
 *   GLOBAL_RC2FLAG_L00_evt_01 (Veldin / Prologue event 1)
 *   GLOBAL_RC2FLAG_L01_evt_02 (Oozla event 2)
 */

#define RC2_FLAG_NAME(level, event) GLOBAL_RC2FLAG_L##level##_evt_##event

/*
 * Gameplay state flags
 */
typedef struct GameFlags {
    /* Challenge mode counter: retail code evaluates `challenge_mode > 0` */
    u8 challengeMode;
    u8 pad[3];
} GameFlags;

#endif /* FLAGS_H */
