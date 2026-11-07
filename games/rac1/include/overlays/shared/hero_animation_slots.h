#ifndef OVERLAYS_SHARED_HERO_ANIMATION_SLOTS_H
#define OVERLAYS_SHARED_HERO_ANIMATION_SLOTS_H

/* Functions defined in src/overlays/shared/hero_animation_slots.c. */

#include "common.h"

extern int Hero_CanUseGadget(void);
extern void func_L00_00211908(void);
extern void apply_hero_motion_and_resolve_collisions(void);
extern void apply_hero_rotation_offset(float, float, float);
extern void build_scaled_hero_move_impulse(float, float);
extern void cancel_hero_item_animation(void);
extern void clear_hero_item_slot(int, int);
extern void clear_hero_target_and_ground_speeds(void);
extern void clip_hero_velocity_against_probed_surfaces(int);
extern void compute_hero_fixed_offset_world_position(float *);
extern void correct_hero_airborne_wall_velocity(void);
extern void correct_hero_ground_penetration(void);
extern void create_missing_hero_item_slot_animations(int a, int b);
extern void enter_hero_vehicle_mode(int, int, void *);
extern void exit_hero_vehicle_mode(void);
extern void *find_best_hero_heading_target(float, float, float, float);
extern void find_grind_rail(void);
extern float forward_component(void *);
extern float get_hero_yaw_history(int back);
extern int hero_check_damage(int);
extern void hero_get_collision_ray_endpoints(void *, void *, int, float, float);
extern void hero_integrate_velocity(void);
extern int hero_is_in_special_state(void);
extern void hero_knockback(void *, int, float, float);
extern int hero_slot_item_id(int);
extern char *hero_slot_moby(int);
extern void hero_switch_item(int);
extern void hero_update(void);
extern int is_wrench_moby(void *);
extern int nearest_rail_point(void *, void *, void *, void *, void *, void *, int, int);
extern int probe_hero_ledge_edge(float *, float *);
extern void resolve_hero_collisions_with_history(void);
extern void scale_hero_motion(float);
extern float score_hero_target_candidate(void *, float, float, float, float, void *);
extern void set_hero_condition_animation_timer_for_item(int);
extern void set_hero_item_switch_timer_for_item(int);
extern void set_planar_vel(float);
extern void speed_step(float, float);
extern void start_equipped_item_animation(void);
extern void steer_hero_planar_velocity(float);
extern void stick_target(float, int);
extern int test_hero_animation_frame_window(float);
extern void turn_hero_toward_move_direction_for_item(void);
extern void turn_hero_toward_move_direction_with_stick_scale(void);
extern void turn_to(int, float, float, float);
extern void update_hero_equipped_items(void);
extern void update_hero_ground_contact_and_surface_slopes(void);
extern void update_hero_item_mobys(void);
extern void update_hero_offset_rail_candidate(void);
extern void update_hero_target_speed_from_stick(void);

#endif /* OVERLAYS_SHARED_HERO_ANIMATION_SLOTS_H */
