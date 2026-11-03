#ifndef OVERLAYS_SHARED_MOBY_PATH_AND_MANIPULATORS_H
#define OVERLAYS_SHARED_MOBY_PATH_AND_MANIPULATORS_H

/* Functions defined in src/overlays/shared/moby_path_and_manipulators.c. */

#include "common.h"

extern void EaseManipulatorOffsets(void *, void *, int, float, float);
extern void Manipulator_ApplyScale(float, int, int, void *);
extern int SegmentCrossPolyline(int, void *, void *, void *);
extern void SetUpdateModeTiming(int);
extern void ShowBanner(int, int);
extern void *SpawnBreakFragment(void *, int, void *, void *, int, int, float, void *, void *, void *);
extern void clear_moby_transform_history(void *);
extern void compose_target_euler_rotation(void *, float, float, float);
extern void create_moby_transform_history_entry(void *, int, int);
extern void cutscene_set_camera_target(int);
extern void draw_offset_textured_effect_quad(void *, int, float, float);
extern void draw_trail(void *, void *, void *, float, float);
extern void fast_add_rotations_cf(void *, void *, float, float, float);
extern float ground_height_at(void *);
extern void initialize_moby_transform_history(int, void *);
extern void manipulator_set_scale(void *, float);
extern void path_sample_at_distance(int, void *, void *, void *, float, float);
extern void queue_formatted_dialogue_message(int, int, int);
extern void queue_planet_unlock_message(int);
extern int resolve_point_path_segment_clearance(int, void *, void *, float);
extern void resolve_point_path_segment_xy_clearance(float, int, void *, void *);
extern void *select_auto_aim_target_moby(void *, void *, void *, void *, void *, void *, float);
extern void spawn_ground_contact_effect_particle(void *);
extern void spawn_moby_break_fragments(void *, int, int, int, int, int, int);
extern void spawn_randomized_bolt_drop(void *, int, int, int, int);
extern int test_path_segment_strict_crossing(int, void *, void *);
extern void update_moby_rotation_oscillation(void *, void *, void *, float, float, float);
extern void update_moby_transform_history(void *, int);
extern void update_moby_vertical_oscillation(float, float, void *, void *, void *);
extern void update_object_wander_movement(char *moby, char *d, float a, float b);
extern void update_timed_hud_visibility(void);

#endif /* OVERLAYS_SHARED_MOBY_PATH_AND_MANIPULATORS_H */
