#ifndef GAME_UI_VENDOR_H
#define GAME_UI_VENDOR_H

/* Functions defined in src/game/ui/vendor.c. */

#include "common.h"

extern void configure_capture_target(int, int, float);
extern void render_vendor_capture_pass_sequence(int);
extern void render_vendor_capture_texture_overlays_pass(int, float, float);
extern int sample_surface_height_map(void *, void *, float, float, float);
extern void update_instance_attachment_points_and_lengths(void *);

#endif /* GAME_UI_VENDOR_H */
