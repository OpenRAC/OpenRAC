#ifndef ASM_FUNCTIONS_LEVEL_01_H
#define ASM_FUNCTIONS_LEVEL_01_H

/* Functions with no C definition: level_01. */

#include "common.h"

extern void build_overlay_layer_wave_vertices(void *);
extern void emit_textured_strip_packet(int, int, int, int, int, int);
extern void ripple_emit_sub_block(int, int);
extern void ripple_heights_to_scratchpad(void *, void *);
extern void ripple_lerp_buffers(void *, void *, void *, float);
extern void ripple_patch_step(void *, int, int, void *);
extern void ripple_shade_vertices(void *);
extern void spawn_explosion_effects(void *, void *, int, float, float);
extern void update_and_draw_scrolling_overlay_layers(int, void *);

#endif /* ASM_FUNCTIONS_LEVEL_01_H */
