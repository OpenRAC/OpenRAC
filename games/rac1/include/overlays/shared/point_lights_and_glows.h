#ifndef OVERLAYS_SHARED_POINT_LIGHTS_AND_GLOWS_H
#define OVERLAYS_SHARED_POINT_LIGHTS_AND_GLOWS_H

/* Functions defined in src/overlays/shared/point_lights_and_glows.c. */

#include "common.h"

extern int allocate_colored_effect_slot(void *, float, float, float, float, float);
extern int allocate_packed_color_point_light_slot(void *, float, float, int);
extern void emit_moby_particle_pairs(void *);
extern void glow_quads_draw(void *);
extern void release_point_light_slot(int);

#endif /* OVERLAYS_SHARED_POINT_LIGHTS_AND_GLOWS_H */
