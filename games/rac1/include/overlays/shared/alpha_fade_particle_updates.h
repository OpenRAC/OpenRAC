#ifndef OVERLAYS_SHARED_ALPHA_FADE_PARTICLE_UPDATES_H
#define OVERLAYS_SHARED_ALPHA_FADE_PARTICLE_UPDATES_H

/* Functions defined in src/overlays/shared/alpha_fade_particle_updates.c. */

#include "common.h"

extern void *PartType44Spawn(void *, void *, int, int, int, int, float, float, float, float, float);
extern void *PartType53Spawn(void *, int, float, float, int, int, int, void *, float);
extern void *func_L00_00273E08(void *, int, unsigned char, int, int, int, int, float);
extern void load_misc_animations_into_stash(void);
extern void *spawn_color_tween_emitter_particle(void *, unsigned char, unsigned char, unsigned char, unsigned char,
                                                unsigned char, void *, float, float);
extern int spawn_gravity_floor_burst_particle(void *, void *, int, float, float, float, int, int, int);
extern int spawn_moby_point_attached_particle(void *, int, float, float, unsigned int, unsigned int);
extern void *spawn_mode_controlled_alpha_fade_particle(void *, int, int);
extern void *spawn_mode_controlled_orbiting_trail_particle(float, float, int, void *, int, int, int, void *);
extern char *spawn_mode_controlled_rotating_alpha_fade_particle(void *, void *, int, void *);
extern void spawn_moving_hero_relative_alpha_fade_particle(void *, void *, int, int, int, int, float);
extern void *spawn_moving_resizing_height_snapped_alpha_fade_particle(void *, int, int, float, float);
extern void spawn_proximity_corrected_size_alpha_tween_particle(void *, int, int, int, int, void *, void *, float,
                                                                float);
extern unsigned char *spawn_random_alpha_rotated_particle(void *, void *, float);
extern void spawn_random_planar_drift_particle(void *, int, float, float);
extern void *spawn_random_size_rotated_particle(void *, void *, int, float);
extern void *spawn_splash(void *, void *, void *, float, float);
extern void spawn_texture_indexed_rotated_particle(void *, float, int);

#endif /* OVERLAYS_SHARED_ALPHA_FADE_PARTICLE_UPDATES_H */
