#ifndef OVERLAYS_SHARED_SHOT_TRACKING_H
#define OVERLAYS_SHARED_SHOT_TRACKING_H

/* Functions defined in src/overlays/shared/shot_tracking.c. */

#include "common.h"

extern void collect_mobies_around_tracked_objects(void *, int, int);
extern void effect_slots_fire(void *, void *, void *, float);
extern void initialize_moby_grounding(void *);
extern void reset_shot_tracking(void *);

#endif /* OVERLAYS_SHARED_SHOT_TRACKING_H */
