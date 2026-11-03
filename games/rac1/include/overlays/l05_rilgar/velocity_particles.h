#ifndef OVERLAYS_L05_RILGAR_VELOCITY_PARTICLES_H
#define OVERLAYS_L05_RILGAR_VELOCITY_PARTICLES_H

/* Functions defined in src/overlays/l05_rilgar/velocity_particles.c. */

#include "common.h"

extern void create_particle_with_velocity_and_lifetime(float, void *, void *, int, int, int);
extern void create_particle_with_velocity_and_randomized_parameters(float, float, float, void *, void *, int, int);

#endif /* OVERLAYS_L05_RILGAR_VELOCITY_PARTICLES_H */
