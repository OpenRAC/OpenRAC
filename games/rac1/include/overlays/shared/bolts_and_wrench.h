#ifndef OVERLAYS_SHARED_BOLTS_AND_WRENCH_H
#define OVERLAYS_SHARED_BOLTS_AND_WRENCH_H

/* Functions defined in src/overlays/shared/bolts_and_wrench.c. */

#include "common.h"

extern void RippleDisturb(void *, int, int, float, float, float, float);
extern int begin_bolt_attraction(void *, int, float, float);
extern void emit_randomized_sparkle_pairs(void *, int);
extern int fetch_and_add_global_accumulator(int);
extern void initialize_moby_motion_at_position(void *, void *, void *);
extern int predict_mine_impact(void *, void *, int);
extern void remove_tracked_class_5f3_moby(void);
extern void set_tracked_class_5f3_moby_position(int, char *);
extern void spawn_bolt_by_value(void *, void *, void *, int, int, int);
extern void *spawn_class_593_moby(int unused, void *pos, int c, float scale, float a, float b);
extern int spawn_homing_bolt(void *, void *, float, float, float, int, float, float);
extern void *spawn_mine(void *, void *, void *);
extern void update_ambience_target_cycle(void);

#endif /* OVERLAYS_SHARED_BOLTS_AND_WRENCH_H */
