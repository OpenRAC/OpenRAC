#ifndef OVERLAYS_SHARED_HERO_VEHICLE_AND_COLLISION_H
#define OVERLAYS_SHARED_HERO_VEHICLE_AND_COLLISION_H

/* Functions defined in src/overlays/shared/hero_vehicle_and_collision.c. */

#include "common.h"

extern int HeroEnterMoveState(int, int);
extern int get_hero_high_jump_input_window_frames(void);
extern int process_hero_equipped_item_input(void);
extern void remove_hero_cached_moby_and_clear_slot_three_moby_flags(void);
extern void select_hero_grapple_swing_target(void);
extern int select_hero_grapple_target(void);
extern void set_hero_idle_state_for_vehicle_mode(void);
extern int stick_direction_vs_facing(void);
extern int try_enter_hero_water_state(void);
extern int try_start_hero_jump(int, int);
extern int try_start_hero_weapon_fire_state(void);
extern int try_transition_hero_water_state(void);
extern void update_hero_animation_for_movement_speed(void);
extern void update_hero_motion_and_collision_probes(void);
extern void update_hero_planar_distance_spring_motion(void);
extern void update_hero_timers_and_reach_state(void);
extern void update_hero_water_height_and_bobbing(void);

#endif /* OVERLAYS_SHARED_HERO_VEHICLE_AND_COLLISION_H */
