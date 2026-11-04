#ifndef OVERLAYS_SHARED_COLLISION_QUERIES_H
#define OVERLAYS_SHARED_COLLISION_QUERIES_H

/* Functions defined in src/overlays/shared/collision_queries.c. */

#include "common.h"

extern void EffectQueue_Add(int, int, float, float, int, int, int, float, int);
extern int find_camera_collision_volume_for_point(void *, float);

#endif /* OVERLAYS_SHARED_COLLISION_QUERIES_H */
