#ifndef OVERLAYS_SHARED_MORPHORAY_CHICKEN_AND_DRONES_H
#define OVERLAYS_SHARED_MORPHORAY_CHICKEN_AND_DRONES_H

/* Functions defined in src/overlays/shared/morphoray_chicken_and_drones.c. */

#include "common.h"

extern void release_glove_of_doom_held_moby(void *, void *, void *);
extern void replenish_hero_drones(void);
extern void *spawn_feather_moby(void *, void *, void *);
extern void spawn_homing_shot(void *, float, float, float, float, void *, void *);
extern void spawn_morphoray_chicken(void *);

#endif /* OVERLAYS_SHARED_MORPHORAY_CHICKEN_AND_DRONES_H */
