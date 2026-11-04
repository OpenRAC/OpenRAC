#ifndef GAME_RENDER_EFFECTS_H
#define GAME_RENDER_EFFECTS_H

/* Functions defined in src/game/render/effects.c. */

#include "common.h"

extern void draw_lens_flare(void);
extern void render_queued_rotated_sprites(void);
extern void update_camera_environment_from_regions(void *);

#endif /* GAME_RENDER_EFFECTS_H */
