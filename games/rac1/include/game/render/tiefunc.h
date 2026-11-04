#ifndef GAME_RENDER_TIEFUNC_H
#define GAME_RENDER_TIEFUNC_H

/* Functions defined in src/game/render/tiefunc.c. */

#include "common.h"

extern void DrawTies_1(void);
extern void DrawTies_2(void);
extern void PatchTieGifs(void);
extern void copy_render_buffer_pair(void);
extern void register_entity_render_resources(void);

#endif /* GAME_RENDER_TIEFUNC_H */
