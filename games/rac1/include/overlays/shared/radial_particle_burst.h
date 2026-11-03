#ifndef OVERLAYS_SHARED_RADIAL_PARTICLE_BURST_H
#define OVERLAYS_SHARED_RADIAL_PARTICLE_BURST_H

/* Functions defined in src/overlays/shared/radial_particle_burst.c. */

#include "common.h"

extern void emit_radial_particle_burst(void *, void *, int);
extern void rotate_vector_by_scaled_angle_between_vectors(float, void *, void *, void *, int);
extern void trooper_set_knockback(void *, float, float);

#endif /* OVERLAYS_SHARED_RADIAL_PARTICLE_BURST_H */
