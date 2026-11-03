#ifndef OVERLAYS_SHARED_HERO_UPDATE_LEDGE_GRAB_H
#define OVERLAYS_SHARED_HERO_UPDATE_LEDGE_GRAB_H

/* Functions defined in src/overlays/shared/hero_update_ledge_grab.c. */

#include "common.h"

extern void adjust_hero_velocity_from_collision_probes(int);
extern float calculate_hero_motion_prediction_time(float a, float b, float c);
extern void enter_hero_idle_state_for_current_vehicle_mode(void);
extern int get_hero_help_value(int);
extern void restore_hero_from_vehicle_mode(void);

#endif /* OVERLAYS_SHARED_HERO_UPDATE_LEDGE_GRAB_H */
