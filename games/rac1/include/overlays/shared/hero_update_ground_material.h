#ifndef OVERLAYS_SHARED_HERO_UPDATE_GROUND_MATERIAL_H
#define OVERLAYS_SHARED_HERO_UPDATE_GROUND_MATERIAL_H

/* Functions defined in src/overlays/shared/hero_update_ground_material.c. */

#include "common.h"

extern void HeroGroundProbe(void);
extern int HeroL01_TryTakeHit(int);
extern void air_accel(float);
extern void clip_hero_horizontal_velocity(int);
extern void enter_hero_vehicle_mode_and_set_moby(int, int, int);
extern void exit_hero_vehicle_mode_and_restore_moby(void);
extern int get_hero_idle_animation_index(int);
extern void hero_frame_update(void);
extern void hero_speed_from_table(void);
extern void request_hero_idle_state_for_vehicle_mode(void);
extern void request_hero_voice_if_enabled(int a0, int a1, int a2);
extern void select_hero_locomotion_state(void);
extern int test_hero_ledge_ahead(float *, float *);
extern void update_hero_blink_and_gold_weapon_tint(void);
extern void update_hero_ground_material_effects(void);
extern void update_hero_ledge_probe(void);

#endif /* OVERLAYS_SHARED_HERO_UPDATE_GROUND_MATERIAL_H */
