#ifndef OVERLAYS_SHARED_MINE_BOMB_AND_DEBRIS_H
#define OVERLAYS_SHARED_MINE_BOMB_AND_DEBRIS_H

/* Functions defined in src/overlays/shared/mine_bomb_and_debris.c. */

#include "common.h"

extern char *SpawnFallingSpark(char *, char *, int, int, int);
extern float calculate_eased_step_sum(int);
extern void draw_bomb_landing_marker(char *);
extern void emit_object_offset_particle_burst(unsigned char *);
extern void emit_object_offset_particles(unsigned char *);
extern void launch_bomb(float *, unsigned char *, float *);
extern char *spawn_bomb(int, void *);
extern unsigned char *spawn_crate_debris(unsigned char *, void *, void *, void *, int, int, float, float);
extern unsigned char *spawn_devastator_missile(unsigned char *, Vec4 *, unsigned char *, float, float, float, float,
                                               float);
extern char *spawn_glow_particle(void *, void *, void *, float, int, unsigned char, unsigned char, unsigned char,
                                 unsigned char);

#endif /* OVERLAYS_SHARED_MINE_BOMB_AND_DEBRIS_H */
