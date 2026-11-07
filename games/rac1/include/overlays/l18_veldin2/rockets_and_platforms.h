#ifndef OVERLAYS_L18_VELDIN2_ROCKETS_AND_PLATFORMS_H
#define OVERLAYS_L18_VELDIN2_ROCKETS_AND_PLATFORMS_H

/* Functions defined in src/overlays/l18_veldin2/rockets_and_platforms.c. */

#include "common.h"

extern int activate_drek_fight_missiles_in_group(void *, int, void *);
extern int classify_moby_state_two_or_three(void *);
extern void *create_class_274_ring_effect(void *, void *, float);
extern void *create_class_29_moby(void *, void *, void *, int);
extern void *func_L18_002EB4E0(void *, void *, int);
extern void *create_rocket_moby(void *, void *, void *, float, float, int, int);
extern void end_thruster_pack_lock_red_wait(void *);
extern void *find_nearest_thruster_pack_lock_green_moby_in_list(void *, int);
extern void *find_thruster_pack_lock_green_moby_with_largest_xy_angle_in_list(void *, void *, int);
extern float func_L18_002E0F80(void *);
extern void *launch_mine_568_moby_from_list(int, int, void *, void *, float, int);
extern void moby_init_action_mode(void *);
extern int poll_moby_state_and_rearm(void *);
extern void refresh_moby_position_and_activate(void *, void *, void *, int, float);
extern void refresh_ring_effect(void *, void *, float);
extern int request_floating_platform_sink(void *);
extern void request_floating_platform_tilt(void *, int, int);
extern void request_thruster_pack_lock_red_wait(void *, int);
extern void set_thruster_pack_lock_green_list_data_fields(int);
extern void func_L18_002DCD28(void *, void *, int, float, float);
extern void func_L18_002E0E90(void *, void *, void *, void *, void *, void *, int, int, int);

#endif /* OVERLAYS_L18_VELDIN2_ROCKETS_AND_PLATFORMS_H */
