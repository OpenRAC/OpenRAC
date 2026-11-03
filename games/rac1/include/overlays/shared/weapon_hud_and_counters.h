#ifndef OVERLAYS_SHARED_WEAPON_HUD_AND_COUNTERS_H
#define OVERLAYS_SHARED_WEAPON_HUD_AND_COUNTERS_H

/* Functions defined in src/overlays/shared/weapon_hud_and_counters.c. */

#include "common.h"

extern int DefaultVtbl_DeleteMoby(float, int, int);
extern void HudFrame(void *, int, int, int, int, int);
extern void Hud_UpdateCounter(void);
extern void apply_group_entry_adjustment(int);
extern void decompress_and_relocate_selected_groups(int);
extern void draw_bolt_counter_hud_panel(void);
extern void draw_cropped_hud_sprite(int, int, int, int, int, int);
extern void draw_health_hud_panel(void);
extern void draw_hud_bolt_exclamation_panel(void);
extern void draw_hud_item_selector_panel(void);
extern void draw_hud_radial_selector_panel(void);
extern void draw_hud_stars(void);
extern void draw_hud_text_panel(void);
extern void draw_uniform_size_hud_stars(void);
extern void draw_weapon_ammo_hud_panel(void);
extern void hud_draw_gauge_frame(void);
extern void hud_init_particles(void);
extern int hud_keep_visible(int, int);
extern int hud_screen_position(void *, void *, void *);
extern void hud_update_particles(void);
extern void init_level_runtime_state(void);
extern void initialize_bolt_counter_hud(void);
extern void initialize_eight_slot_hud_selector(void);
extern void initialize_four_slot_hud_selector(void);
extern void initialize_hud_element_with_counters(void);
extern void initialize_numeric_hud_element(void);
extern void initialize_numeric_hud_timer_and_base_dimensions(void);
extern void initialize_numeric_hud_values_and_dimensions(void *);
extern void reset_hud_and_suspend_weapon_updates(void);
extern void reset_numeric_hud_element();
extern void reset_numeric_hud_element_32x32_base(void);
extern void select_moby_context(int);
extern void update_bolt_counter_hud_animation_and_visibility(void);
extern void update_health_hud_animation_and_visibility(void);
extern void update_hud_counter_cap_and_visibility(void);
extern int update_hud_elements(int);
extern void update_hud_item_selector_selection_and_visibility(void);
extern void update_hud_panel_visibility(void);
extern void update_hud_radial_selector_selection_and_visibility(void);

#endif /* OVERLAYS_SHARED_WEAPON_HUD_AND_COUNTERS_H */
