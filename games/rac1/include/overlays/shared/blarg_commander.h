#ifndef OVERLAYS_SHARED_BLARG_COMMANDER_H
#define OVERLAYS_SHARED_BLARG_COMMANDER_H

/* Functions defined in src/overlays/shared/blarg_commander.c. */

#include "common.h"

extern void apply_npc_big_head_cheat(char *);
extern void *create_blarg_commander_moby_from_source(void *);
extern void draw_level_12_effects(void);
extern void draw_textured_effect_list(void *, int, int, int);
extern void start_blarg_commander_knockback(void *, void *);

#endif /* OVERLAYS_SHARED_BLARG_COMMANDER_H */
