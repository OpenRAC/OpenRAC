#ifndef OVERLAYS_SHARED_NUMERIC_HUD_SPRITES_H
#define OVERLAYS_SHARED_NUMERIC_HUD_SPRITES_H

/* Functions defined in src/overlays/shared/numeric_hud_sprites.c. */

#include "common.h"

extern void draw_hud_progress_and_rank(void);
extern void draw_hud_timer(void);
extern void draw_trick_combo_hud(void);
extern void hud_release_held_element(void);
extern void initialize_hud_element_defaults(void);
extern void initialize_trick_combo_hud(void);
extern void set_progress_hud_fade_from_hero_mode(void);
extern void update_trick_combo_hud(void);

#endif /* OVERLAYS_SHARED_NUMERIC_HUD_SPRITES_H */
