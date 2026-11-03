#ifndef OVERLAYS_L04_EUDORA_WALKER_RIG_GAIT_H
#define OVERLAYS_L04_EUDORA_WALKER_RIG_GAIT_H

/* Functions defined in src/overlays/l04_eudora/walker_rig_gait.c. */

#include "common.h"

extern void initialize_leg_rig(void *, void *);
extern void initialize_leg_rig_angle_defaults(void *, void *);
extern int leg_walker_update(void *, void *, float, float, float);
extern void reset_leg_rig_from_animation(void *, void *);

#endif /* OVERLAYS_L04_EUDORA_WALKER_RIG_GAIT_H */
