#ifndef OVERLAYS_SHARED_HERO_IDLE_AND_ANIMATION_H
#define OVERLAYS_SHARED_HERO_IDLE_AND_ANIMATION_H

/* Functions defined in src/overlays/shared/hero_idle_and_animation.c. */

#include "common.h"

extern int Help_Request(int, int);
extern void Hero_SetTiltSpringParams(float, float);
extern float adjust_hero_animation_speed_for_target_frame(float, float, float, float);
extern void apply_hero_and_item_scale_sequence(void);
extern void check_hero_item_reward_conditions(void);
extern void check_ledge_grab(void);
extern void clear_active_help_record(void);
extern void clear_help_record_high_bits(void);
extern void clear_hero_idle_rotation_targets(void);
extern void clear_jet_camera_mode_latch(void);
extern void coordinate_hero_frame_update(void);
extern void copy_hero_color_to_item_mobys(void);
extern void dispatch_hero_animation_timed_events(void);
extern void emit_hero_animation_voice_and_ground_particle(int, int, int);
extern void emit_hero_periodic_surface_effect(int, int);
extern void emit_hero_periodic_surface_particles(int, int);
extern void emit_hero_randomized_motion_effects(int, int);
extern void emit_hero_surface_effect_burst(int, int, int);
extern void emit_hero_timed_effects(void);
extern void enqueue_hero_glow_draw_callback_once(void);
extern void exit_hero_vehicle_mode_if_mode_three(void);
extern void fast_sin_cf(int, int);
extern void hero_change_health(int n);
extern int hero_idle_sequence(int);
extern void initialize_hero(void);
extern void jet_camera_mode(void);
extern void pack_pose(void *, void *, int, int, int);
extern void probe_hero_airborne_wall_contact(void);
extern int probe_hero_forward_collision(float, float, void *);
extern void release_and_clear_hero_voice_slots(void);
extern void reset_hero_easing_records(void);
extern void reset_hero_to_idle(void);
extern int return_hero_animation_id_unchanged(int);
extern void rotate_hero_about_fixed_local_pivot(float, float, float);
extern void rotate_hero_about_local_pivot(float *, float, float, float);
extern void scan_hero_input_sequences(void);
extern void update_hero_bolt_exclamation_hud(void);
extern void update_hero_bone_attachment(void);
extern void update_hero_collision_ease_target(void);
extern void update_hero_easing_record(void *);
extern void update_hero_easing_records(void);
extern void update_hero_easing_targets_and_point_effects(void);
extern void update_hero_glow_and_extra_manipulators(void);
extern void update_hero_idle_random_rotation_targets(void);
extern void update_hero_moby(void);
extern void update_hero_moby_probed_height_bounds(void);
extern void update_hero_pitch_and_yaw_easing_targets(void);
extern void update_hero_rotation_targets_and_random_manipulators(void);
extern void update_hero_scalar_pulse(void);
extern void update_hero_target_tracking(void);
extern void update_hero_vehicle_moby(void);
extern void update_hero_world_matrices(int);
extern void update_item_tint_from_hero(int, int);
extern void update_moby_pulsing_glow_effects(void *);
extern float vector_length_xyz_cf(float *, int);

#endif /* OVERLAYS_SHARED_HERO_IDLE_AND_ANIMATION_H */
