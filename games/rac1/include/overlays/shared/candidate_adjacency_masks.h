#ifndef OVERLAYS_SHARED_CANDIDATE_ADJACENCY_MASKS_H
#define OVERLAYS_SHARED_CANDIDATE_ADJACENCY_MASKS_H

/* Functions defined in src/overlays/shared/candidate_adjacency_masks.c. */

#include "common.h"

extern void build_symmetric_adjacency_masks(int *, int, int);
extern void *select_candidate_by_max_float_2c(void *, void *, void *, int);

#endif /* OVERLAYS_SHARED_CANDIDATE_ADJACENCY_MASKS_H */
