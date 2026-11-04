#ifndef OVERLAYS_SHARED_TRESPASSER_AND_THROWING_GLOVES_H
#define OVERLAYS_SHARED_TRESPASSER_AND_THROWING_GLOVES_H

/* Functions defined in src/overlays/shared/trespasser_and_throwing_gloves.c. */

#include "common.h"

extern void aim_throw_arc(void *, void *, void *, float, float, float);
extern void compute_glove_of_doom_launch_velocity(void *, void *, void *, float, float, float);
extern void release_moby_with_collision_check(void *, void *, void *);
extern int spawn_decoy(void *, void *, void *);
extern void spawn_moby_collision_particles(void *);
extern void *spawn_on_path(void *, int, int, int);
extern int try_collect_ammo_pickup(void *);

#endif /* OVERLAYS_SHARED_TRESPASSER_AND_THROWING_GLOVES_H */
