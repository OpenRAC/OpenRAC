#ifndef OVERLAYS_SHARED_WATER_POOL_AND_SAND_MOUSE_H
#define OVERLAYS_SHARED_WATER_POOL_AND_SAND_MOUSE_H

/* Functions defined in src/overlays/shared/water_pool_and_sand_mouse.c. */

#include "common.h"

extern void func_L01_002B90A8(float);
extern void func_L01_002B9198(void *, int);
extern void compute_sand_mouse_approach_target(void *, void *);
extern void func_L01_002F6540(void *);
extern void pool_ripple(float, float, float, float, void *, int);
extern void render_water_grids(char *, int);
extern int reserve_expired_frame_slot(int *, int, int);

#endif /* OVERLAYS_SHARED_WATER_POOL_AND_SAND_MOUSE_H */
