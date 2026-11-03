#ifndef OVERLAYS_SHARED_WATER_POOL_AND_SAND_MOUSE_H
#define OVERLAYS_SHARED_WATER_POOL_AND_SAND_MOUSE_H

/* Functions defined in src/overlays/shared/water_pool_and_sand_mouse.c. */

#include "common.h"

extern void RescaleFloatTable(float);
extern void RippleSimClock(void *, int);
extern void compute_sand_mouse_approach_target(void *, void *);
extern void gunship_follow_path(void *);
extern void pool_ripple(float, float, float, float, void *, int);
extern void render_water_grids(char *, int);
extern int reserve_expired_frame_slot(int *, int, int);

#endif /* OVERLAYS_SHARED_WATER_POOL_AND_SAND_MOUSE_H */
