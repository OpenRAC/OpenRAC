#ifndef ASM_FUNCTIONS_SHARED_LEVELS_H
#define ASM_FUNCTIONS_SHARED_LEVELS_H

/* Functions with no C definition: shared_levels. */

#include "common.h"

extern int CollType(void);
extern int FastBSphereCheck(void *, float);
extern void KillPart(void *);
extern void func_L00_0025C710(void *, void *, void *, float);
extern void MobyAnimAdvance(void *);
extern void Moby_BuildFramePacket(void *, int, int, int);
extern void Profile_MarkL00_b(void *, int);
extern void Profile_MarkL00_c(void *, int);
extern void UpdatePadL00(void);
extern void add_vectors(void *, void *, void *);
extern int adjust_follow_camera_orbit_probe_for_collisions(void *, void *, int, float, float);
extern void apply_env_zone_lighting(void *);
extern void apply_taunter_to_nearby_mobies(void *, float);
extern void func_L00_001FFED8(void *, int, float);
extern void clamp_vec_length(void *, void *, float);
extern int clamp_vec_length_xy(float, void *, void *);
extern int func_L00_001F1D20(float, float, void *, int, void *);
extern int coll_line(void *, void *, int, int, int);
extern int coll_point_query_stub(void *, int, int, void *, int);
extern int coll_sound_class(void);
extern int coll_sphere(float, void *, int, void *);
extern int coll_sphere_hero_groups(float, void *);
extern int coll_sphere_mobys(float, void *, int, void *, void *);
extern void collect_moby_and_clear_path_keys(void *);
extern void copy_memory_by_alignment(int, void *, unsigned short);
extern void crate_drop_bolts(void *);
extern void *create_part2(int, int);
extern void divide_vector_xyz(void *, void *, float);
extern void draw_env_overlay_mesh(int, int, int, void *, int);
extern void draw_mine_predicted_impact_marker(void);
extern void draw_world_quad(void *, void *, int);
extern float fast_atan2(float, float);
extern void fast_euler_to_matrix(void *, void *);
extern float fast_fmod(float, float);
extern int fast_subtract_rotations_cf(void *);
extern void follow_camera_read_stick(int, void *, void *);
extern void func_L00_00251388(void *, void *);
extern void func_L00_00255B90(int);
extern int generate_random_points_from_shape_entries(void *, int, void *, void *, float);
extern void get_moby_color(void *, void *, void *, void *);
extern void get_moby_light_color(void *, void *);
extern int get_next_moby_in_group(void *, int, int, int);
extern void hud_weapon_update(void);
extern void initialize_suck_cannon_hud_counter(void);
extern float jet_distance(void *, void *);
extern float lensq_cc2e8(void *);
extern void matrix4_add(void *, void *, void *);
extern void matrix4_scale(void *, void *, float);
extern void moby_blend_anim_frames_to_spr(void *);
extern void moby_blend_anim_frames_to_spr_slot(void *, int);
extern void moby_build_matrix(void *);
extern void moby_joints_to_scratchpad(void *, int, int);
extern void moby_set_coll_cell_range(void *, unsigned int);
extern unsigned long pack_vector_s16(int, float);
extern void reset_smoke_fire_spawner_particle(void *, int);
extern void scale_vector_xyz_in_place(void *, void *, float);
extern void set_moby_color(void *, int, int, int);
extern void *spawn_hero_drone(void *, void *);
extern float split_float_fraction(void *, float);
extern void update_vendor_menu(void);
extern void vec3_sub_in_place_second_arg(void *, void *, void *);
extern void vec_mul_components(void *, void *, void *);
extern void func_L00_001FF610(void *, void *, void *);
extern void vec_scale_to_length(void *, void *, float);
extern void vec_set_length_xy(void *, void *, float);

#endif /* ASM_FUNCTIONS_SHARED_LEVELS_H */
