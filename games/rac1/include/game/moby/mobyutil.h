#ifndef GAME_MOBY_MOBYUTIL_H
#define GAME_MOBY_MOBYUTIL_H

/* Functions defined in src/game/moby/mobyutil.c. */

#include "common.h"

extern float ApproachValue(void *, float, float);
extern float ApproachWithVelocity(void *, void *, float, float, float, float);
extern void BuildSphericalOffset(void *, float, float, float);
extern float func_00214220(float, float, float);
extern int func_00215B18(void *, float);
extern void func_002150B0(void *, void *);
extern void MobyBlendAnimation(void *, int, int, int);
extern void MobySetAnimation(void *, int, int);
extern float func_00214358(void *, int, float);
extern void func_00215CA8(void *, int, void *, void *, int, float);
extern void assign_moby_class_data(int);
extern void blend_moby_animation_ex(void *, int, int, int, int);
extern void build_look_at_matrix(void *, void *, void *, float);
extern void build_quaternion_from_axis_angle(void *, void *, float);
extern s32 func_00216098(void);
extern int func_00216150(void);
extern int func_00216198(void);
extern int func_002160E0(void);
extern void extract_matrix_rotation_quaternion(void *, void *);
extern int force_help_message(int arg0, int arg1);
extern int is_point_inside_clip_volume(void *, int);
extern void load_display_text_resource_entry(void *, void *);
extern void matrix_to_euler(void *, void *);
extern void normalize_vector_triplet(void *);
extern void pack_vector_xyz_with_shared_scale(void *, void *);
extern float random_angle_radians(void);
extern float random_float_between(float, float);
extern s32 random_integer_below(s32);
extern void random_spherical_offset(void *, float, float);
extern int func_00215078(void *);
extern float resolve_camera_surface_height(void *, void *);
extern void rotate_vector_by_quaternion(void *arg0, void *arg1, void *arg2);
extern void set_video_timing(int);
extern int try_set_help_message(int, int);
extern void update_moby_shadow_range(void *);
extern void func_00213C78(void);

#endif /* GAME_MOBY_MOBYUTIL_H */
