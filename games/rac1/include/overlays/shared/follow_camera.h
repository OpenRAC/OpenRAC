#ifndef OVERLAYS_SHARED_FOLLOW_CAMERA_H
#define OVERLAYS_SHARED_FOLLOW_CAMERA_H

/* Functions defined in src/overlays/shared/follow_camera.c. */

#include "common.h"

extern void func_L00_002E9900(float, float, int);
extern void func_L00_002E99A0(int, float, float);
extern void clear_follow_camera_volume_if_current(void *);
extern void draw_active_beam_slots(void);
extern void emit_hydrodisplacer_effect(void *);
extern void end_hydrodisplacer_interaction(void *);
extern void follow_camera_apply_overrides(void *);
extern void follow_camera_set_height(float x, float y);
extern void follow_camera_set_lateral_spring(float, float);
extern void follow_camera_springs(void *);
extern void follow_camera_start_slide(void);
extern int register_beam_slot(void *, int, int);
extern void release_beam_slot(int);
extern int release_beam_slot_by_id(void *);
extern void reset_follow_camera_state_from_position(void *);
extern int set_beam_origin_transform_and_endpoint(void *, void *, void *);
extern void set_follow_camera_angle_1cc(float);
extern void set_follow_camera_angle_blend_from_direction(void *, float, float);
extern void set_follow_camera_angle_blend_from_position(int, float, float);
extern void set_follow_camera_blend_output_flags(void);
extern void set_follow_camera_flag_226(void);
extern void set_follow_camera_float_22c(float);
extern void set_follow_camera_orbit_clamp_mode(int);
extern int test_hydrodisplacer_activation_flag(void *);
extern int try_select_follow_camera_volume(void *);
extern float update_follow_camera_angle_blend_from_direction(void *, void *, float);
extern void update_follow_camera_pose(void *);
extern void update_slot_panes_and_effects(int, int);

#endif /* OVERLAYS_SHARED_FOLLOW_CAMERA_H */
