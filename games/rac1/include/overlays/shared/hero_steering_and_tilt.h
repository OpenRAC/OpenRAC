#ifndef OVERLAYS_SHARED_HERO_STEERING_AND_TILT_H
#define OVERLAYS_SHARED_HERO_STEERING_AND_TILT_H

/* Functions defined in src/overlays/shared/hero_steering_and_tilt.c. */

#include "common.h"

extern void func_L00_00217718(void *, void *, int, int);
extern void adjust_hero_pose_targets_from_motion(void);
extern int allocate_hero_voice_for_entry(int, int);
extern void compute_hero_reference_tilt_rotation(void *, void *, void *, float, float, float);
extern void compute_rotated_vector_angles(void *, void *);
extern void emit_filtered_entry_sparkles(int, int);
extern void help_queue_add(int, int, int);
extern void hero_move_state_velocity(void);
extern void launch_hero_wrench(void);
extern void reduce_and_apply_hero_planar_velocity_addend(float);
extern void release_voice_slot_cf(int, int, int);
extern void spawn_hero_radial_particle_burst(void);
extern void stop_hero_motion_without_forward_support(float, float);
extern void update_hero_delayed_voice_requests(void);
extern void update_hero_lean_and_sway_parameters(void);
extern void update_hero_motion_response_parameters(void);
extern void update_hero_planar_movement_velocity(void);
extern void update_hero_target_heading(float a, float b, float c);
extern void update_hero_tilt(void);
extern void update_hero_transformed_negative_z_angle_parameters(void);

#endif /* OVERLAYS_SHARED_HERO_STEERING_AND_TILT_H */
