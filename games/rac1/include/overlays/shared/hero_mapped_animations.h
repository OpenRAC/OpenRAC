#ifndef OVERLAYS_SHARED_HERO_MAPPED_ANIMATIONS_H
#define OVERLAYS_SHARED_HERO_MAPPED_ANIMATIONS_H

/* Functions defined in src/overlays/shared/hero_mapped_animations.c. */

#include "common.h"

extern void HeroAttachManipulators(void *);
extern void HeroClearObjectFlags(void);
extern void HeroMoveDownAlongNormal(void *, void *, float);
extern void HeroOffsetAlongViewZ(void *, void *, float);
extern void HeroSetObjectFlags(void);
extern void HeroStartSequenceInBank(int, int);
extern void Hero_ConstrainToPath(void);
extern int add_ammo(int, int);
extern void add_hero_forward_offset(void *, void *, float);
extern void advance_hero_animation(void);
extern void animate_hero_slot4_and_slot5_visuals(void);
extern void blend_hero_slot3_animation_pair(void);
extern void clear_hero_moby_flag_bit0(void);
extern void clear_hero_queued_animation(void);
extern f32 compute_gravity_relative_vector_inclination(void *);
extern float compute_hero_ground_aligned_heading(void);
extern void configure_hero_point_effect_emission(float a, float b, short c, unsigned char d, unsigned char e,
                                                 unsigned char f);
extern int get_ammo(int);
extern int get_hero_target_pose_status(void);
extern void gravity_flatten(float *, float *);
extern float gravity_length_xy(float *v);
extern void gravity_set_z(float *dst, float *src, float z);
extern float gravity_z(void *);
extern void hero_queue_sequence(int bank, int seq);
extern void hero_set_attack_ticks(int);
extern void offset_hero_position_against_gravity(float *, float *, float);
extern float predict_hero_trajectory_intercept_time(float a, float b, float c);
extern float predict_hero_trajectory_z(float *pos, float *vel, int *steps, float step);
extern void request_hero_target_pose(void *, int, float);
extern char resolve_hero_gravity_mode(void);
extern void set_gravity_relative_vector_length_xy(float *, float *, float);
extern void set_hero_animation(int, int, float);
extern void set_hero_moby_flag_bit0(void);
extern int show_health_hud(void);
extern void transform_hero_local_offset_to_world(void *, float, float, float);
extern void transform_hero_polar_offset_to_world(void *, float, float, float);
extern void update_hero_attachment(int, void *, void *);
extern int use_ammo(int, int);

#endif /* OVERLAYS_SHARED_HERO_MAPPED_ANIMATIONS_H */
