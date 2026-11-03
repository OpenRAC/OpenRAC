#ifndef OVERLAYS_SHARED_EFFECT_PARTICLE_UPDATES_H
#define OVERLAYS_SHARED_EFFECT_PARTICLE_UPDATES_H

/* Functions defined in src/overlays/shared/effect_particle_updates.c. */

#include "common.h"

extern void PartType04Spawn(void *, void *, int, int, int, int, int, int);
extern void PartType05Spawn(void *, unsigned char, unsigned char, unsigned char, float, float, int);
extern void PartType11Spawn(void *, void *, int, int, float, int, int, int, int, float);
extern void *PartType21Spawn(void *, void *, int, int, int, int, float);
extern void *PartType23Spawn(void *, float, float, float, int, void *, float, int);
extern void *PartType27Spawn(void *, void *, int, int, float);
extern int PartType34Spawn(void *, void *, float, float);
extern void PartType35Spawn(void *, void *, int, int);
extern void *mine_debris(void *, void *, int, int, float, int, int);
extern void spawn_color_decaying_effect_particle(void *, void *, int);
extern void spawn_effect_particle_with_target_scalar(void *, void *, int, int, float, int);
extern void spawn_effect_particle_with_transformed_random_offset(void *, void *);
extern void spawn_jittered_effect_particle_with_optional_basis(float, void *, int);
extern void *spawn_moby_bound_effect_particle(void *);
extern void spawn_particle_alt_9768(void *pos, void *vel, int color, int tag, int life, float size);
extern void *spawn_shrinking_effect_particle_with_gravity_and_secondary_spawns(void *, void *, int, int, float, int,
                                                                               int, int, int);
extern void *spawn_source_attached_fading_effect_particle(void *, int, int, int, float);
extern void *spawn_spinning_and_fading_effect_particle(void *, void *, unsigned long, float);
extern unsigned char *spawn_spinning_fade_in_out_effect_particle(void *pos, int spin, int col, float range, float f1,
                                                                 float f2, float f3, float scale);
extern void *spawn_splitting_effect_particle_with_collision_response_and_fade(void *, void *, unsigned char, int);
extern void spawn_two_point_effect_particle(void *, void *);

#endif /* OVERLAYS_SHARED_EFFECT_PARTICLE_UPDATES_H */
