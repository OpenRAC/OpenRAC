#ifndef OVERLAYS_SHARED_HERO_HIGH_JUMP_INPUT_H
#define OVERLAYS_SHARED_HERO_HIGH_JUMP_INPUT_H

/* Functions defined in src/overlays/shared/hero_high_jump_input.c. */

#include "common.h"

extern int check_hero_forward_collision_height(void);
extern int hero_handle_jump_press(void);
extern int pick_idle_fidget(int *);
extern int try_start_hero_item_idle_animation(void);
extern void update_hero_locomotion_state(void);

#endif /* OVERLAYS_SHARED_HERO_HIGH_JUMP_INPUT_H */
