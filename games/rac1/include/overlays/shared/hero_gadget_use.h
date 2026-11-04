#ifndef OVERLAYS_SHARED_HERO_GADGET_USE_H
#define OVERLAYS_SHARED_HERO_GADGET_USE_H

/* Functions defined in src/overlays/shared/hero_gadget_use.c. */

#include "common.h"

extern void apply_hero_item_slot_selection(int);
extern int compute_hero_hoverboard_respawn_transform(void *, void *);
extern int hero_pick_help_mode(void);
extern int hero_select_anim_id_by_flags(void);
extern int process_hero_primary_item_input(void);
extern void process_hero_race_object_interactions(void);
extern int select_hero_hoverboard_animation_variant(void);
extern void smooth_and_clamp_value_pair(float *, float *, float);
extern int try_apply_hero_water_and_ground_transitions(void);
extern void update_hero_hoverboard_race_progress(void);
extern void update_hero_hoverboard_trick_animations(void);
extern void update_hero_hoverboard_trick_score(void);

#endif /* OVERLAYS_SHARED_HERO_GADGET_USE_H */
