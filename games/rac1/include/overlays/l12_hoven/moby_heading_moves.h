#ifndef OVERLAYS_L12_HOVEN_MOBY_HEADING_MOVES_H
#define OVERLAYS_L12_HOVEN_MOBY_HEADING_MOVES_H

/* Functions defined in src/overlays/l12_hoven/moby_heading_moves.c. */

#include "common.h"

extern void cache_consecutive_point_distances(int);
extern void move_moby_along_heading(void *, void *);
extern int test_point_within_moby_limits(void *, void *, float, float, float);
extern void update_moby_bone_attachment(void *, void *, int);

#endif /* OVERLAYS_L12_HOVEN_MOBY_HEADING_MOVES_H */
