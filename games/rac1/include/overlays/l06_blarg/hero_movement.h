#ifndef OVERLAYS_L06_BLARG_HERO_MOVEMENT_H
#define OVERLAYS_L06_BLARG_HERO_MOVEMENT_H

/* Functions defined in src/overlays/l06_blarg/hero_movement.c. */

#include "common.h"

extern int HeroL06_TryApplyHitKnockback(int);
extern void apply_hero_ground_material_response(void);
extern void quantize_hero_input_target_speed(void);

#endif /* OVERLAYS_L06_BLARG_HERO_MOVEMENT_H */
