#ifndef OVERLAYS_L01_NOVALIS_SPARK_PARTICLES_H
#define OVERLAYS_L01_NOVALIS_SPARK_PARTICLES_H

/* Functions defined in src/overlays/l01_novalis/spark_particles.c. */

#include "common.h"

extern void create_spark_particle(void *, void *, float, float, float, float, float, float, float, float, float, int,
                                  unsigned char, unsigned char, unsigned char);
extern void pool_bubble(void *, void *, float);
extern void pool_bubbles(void *, void *, float, float);

#endif /* OVERLAYS_L01_NOVALIS_SPARK_PARTICLES_H */
