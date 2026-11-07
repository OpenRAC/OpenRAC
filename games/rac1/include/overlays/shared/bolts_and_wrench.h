#ifndef OVERLAYS_SHARED_BOLTS_AND_WRENCH_H
#define OVERLAYS_SHARED_BOLTS_AND_WRENCH_H

/* Functions defined in src/overlays/shared/bolts_and_wrench.c. */

#include "common.h"

extern void func_L00_002A5158(void *, int, int, float, float, float, float);
extern int begin_bolt_attraction(void *, int, float, float);
extern void emit_randomized_sparkle_pairs(void *, int);
extern int fetch_and_add_global_accumulator(int);
extern void initialize_moby_motion_at_position(void *, void *, void *);
extern int predict_mine_impact(void *, void *, int);
extern void func_L00_002AAE80(void);
extern void func_L00_002AAE20(int, char *);
extern void spawn_bolt_by_value(void *, void *, void *, int, int, int);
extern void *func_L00_002AAC50(int unused, void *pos, int c, float scale, float a, float b);
extern int func_L00_002AA998(void *, void *, float, float, float, int, float, float);
extern void *spawn_mine(void *, void *, void *);
extern void update_ambience_target_cycle(void);

#endif /* OVERLAYS_SHARED_BOLTS_AND_WRENCH_H */
