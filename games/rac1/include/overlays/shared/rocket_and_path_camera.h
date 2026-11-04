#ifndef OVERLAYS_SHARED_ROCKET_AND_PATH_CAMERA_H
#define OVERLAYS_SHARED_ROCKET_AND_PATH_CAMERA_H

/* Functions defined in src/overlays/shared/rocket_and_path_camera.c. */

#include "common.h"

extern void activate_descriptor(int);
extern int check_descriptor_hero_trigger(void *);
extern void *create_rocket(void *, float, void *, int, int, int);
extern void emit_moby_sprite_particles(void *);
extern void spawn_type_52_particle_near_moby(char *moby);

#endif /* OVERLAYS_SHARED_ROCKET_AND_PATH_CAMERA_H */
