#ifndef ASM_FUNCTIONS_LEVEL_03_H
#define ASM_FUNCTIONS_LEVEL_03_H

/* Functions with no C definition: level_03. */

#include "common.h"

extern void is_value_within_interpolated_window_cf(void *);
extern void rebuild_hero_relative_camera_transform(void *);
extern void spawn_emitter_child_effect(void *, void *, int, int, float, float, int);
extern void *spawn_path_mover(void *, float);
extern void update_kerwan_path_enemy_hit_reaction(void *);

#endif /* ASM_FUNCTIONS_LEVEL_03_H */
