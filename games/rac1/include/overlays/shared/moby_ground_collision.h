#ifndef OVERLAYS_SHARED_MOBY_GROUND_COLLISION_H
#define OVERLAYS_SHARED_MOBY_GROUND_COLLISION_H

/* Functions defined in src/overlays/shared/moby_ground_collision.c. */

#include "common.h"

extern void ClampVectorLength(void *, float);
extern int ClassifyTargetInCone(void *, void *, void *, void *);
extern int ComputeHitResponse(void *, void *, void *, int, void *, void *, int, int);
extern float func_L00_00258E58(float, float, float, float, float);
extern int FindNearestEligibleMoby(float, void *, void *, int, int, void *, int);
extern int FindNearestTargetMoby(void *, void *, float);
extern void *GetFlaggedPvarWord(void *);
extern void func_L00_0025BBA0(void *, void *, void *, void *);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern void InitKnockback(void *, void *, float, int, int, int);
extern int func_L00_0025A778(void *, void *, int);
extern void *MobyGetHitMessage(void *, int, int);
extern int MobySphereHitDirectional(int, float, void *, int, float, float, int, int, int);
extern void MobySpringTurn(void *, void *, float, float, float, float);
extern int func_L00_002594C8(void *, void *, void *, int, float, float, float, float);
extern int PackColorRGBA(float, float, float, float);
extern void PostHitMessage(void *, int, int, void *, void *, float);
extern void RerollEffectCounter(void *, void *);
extern int RotateWithMoby(void *, void *, void *, void *, void *, void *);
extern void SendRadialMessageToMobys(void *, void *, void *, int, int, int, int, int, float, float, float);
extern void func_L00_00260108(void *, void *, int, float, float);
extern int func_L00_0025E860(void *, void *, void *, void *, int, float);
extern float Spring(void *, void *, float, float, float, float);
extern float func_L00_0025CCF0(void *, float, void *, int, float, float, float);
extern float SpringTurn(void *, float, float, float, float, float);
extern float SteerAngleWithVelocity(void *, void *, float, float, float, float);
extern void StepColorFade(void *, void *);
extern void add_random_offset_to_vector_xyz(void *, float);
extern void append_unique_moby_index(void *, void *, int);
extern void build_quaternion_from_axis_rotations(void *, void *);
extern float calculate_triangular_corrected_scalar(float *a, float *b, int *out, float f);
extern void clip_negative_segment_endpoint_xyz(void *, void *);
extern void collect_mobies_in_sphere_at_moby_point(void *, float, float, float, int, int, int, int, int);
extern float compute_directed_angle_delta(int, float, float);
extern float compute_transformed_delta_component(void *, void *, void *, int);
extern int convert_pose_to_reference_local(void *, void *, void *, void *, void *, void *);
extern void copy_hero_moby_field_38(void *);
extern void decrement_packed_countdown(void *, void *);
extern int find_listed_moby_by_data_id(void *);
extern int find_nearest_path_point(void *, void *, void *, void *, void *, int, float, float, float);
extern int func_L00_0025F450(void *);
extern int get_first_moby_in_group(void *, int, int, int);
extern void grant_inventory_item(int, int);
extern void initialize_hit_descriptor(float, void *, void *, int);
extern float interpolate_angle(float, float, float);
extern float interpolate_scalar_linear(float, float, float);
extern void interpolate_vector_xyz_cosine_eased(void *, void *, void *, float);
extern float interpolate_wrapped_angle_cosine_eased(float, float, float);
extern void interpolate_wrapped_rotations_cosine_eased(void *, void *, void *, float);
extern int is_moby_in_class_range_500_to_540(int);
extern int is_nonnull_moby_outside_table_or_class_range_500_to_540_or_class_502(void *);
extern void jet_animate(void *, float);
extern float jump_speed_4318(void *, void *, void *, float, float);
extern int func_L00_0025D6F0(void *, void *);
extern void *func_L00_00261448(void *);
extern void moby_index_list_remove(void *, void *);
extern void func_L00_00259868(int, int, float, float, float, int);
extern int move_moby_with_collision_integer_radius(void *, void *, float, int, float, int);
extern int func_L00_00259B88(void *, void *, void *, void *, float);
extern float normalize_angle(float);
extern int offset_path_index(void *, int, int, int);
extern void predict_position_with_vertical_drop(void *, void *, void *, int, float);
extern float probe_ground_height_flag20(void *);
extern int probe_vertical_ground_collision(void *, int, int, float, float);
extern int func_L00_00258BC8(int, int);
extern void func_L00_00258DB0(void *, float, float);
extern float randf_sym(float lo, float hi);
extern void record_moby_hit(void *, void *);
extern float resolve_level_path_segment_point(void *, void *, void *, void *, float);
extern void set_ground_height_band(void *);
extern s32 solve_quadratic_equation(f32 *, f32 *, f32, f32, f32);
extern void spawn_color_swapped_explosion_burst(void *, void *, int, float, float);
extern void spawn_explosion(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float,
                            int, int, int, int);
extern void spawn_nearby_effect_without_direction(float, void *, void *, int, void *);
extern void spawn_particles_at_shape_points(void *, void *, int);
extern float step_angle_toward_target(void *, int, float, float);
extern void subtract_homogeneous_scaled_vector_xyz(void *, void *, void *, float);
extern void swap_color_channel_words(int *, int *, int *, int);
extern unsigned int swap_packed_color_channels(unsigned int, int);
extern int sweep_hit_segment(void *, void *, void *, void *, void *, void *, int);
extern int test_moby_linked_record_word8(void *);
extern int test_point_in_transformed_unit_cylinder(void *, int);
extern int test_point_in_transformed_unit_sphere(void *, int);
extern int transform_attachment_pose(int, void *, void *, void *, void *, void *);
extern void unlock_planet(int);
extern int update_arc_motion(void *, void *);
extern void update_entity_matrix(void *, void *, void *, void *);

#endif /* OVERLAYS_SHARED_MOBY_GROUND_COLLISION_H */
