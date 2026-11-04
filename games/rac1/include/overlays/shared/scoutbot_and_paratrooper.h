#ifndef OVERLAYS_SHARED_SCOUTBOT_AND_PARATROOPER_H
#define OVERLAYS_SHARED_SCOUTBOT_AND_PARATROOPER_H

/* Functions defined in src/overlays/shared/scoutbot_and_paratrooper.c. */

#include "common.h"

extern void Moby600_DrawQuads11(int);
extern void advance_scroll_entry(int);
extern void advance_scroll_offsets(int);
extern void advance_wrapped_sine_pair(void *, float, float, float, void *);
extern void create_moby_with_particles(void *, void *, void *, int);
extern void draw_scrolled_mesh_batch(int);
extern void emit_pair_offset_render_batch(int);
extern void scale_rgba_tables(void *, int, void *, float, float, float, float, int);
extern void set_level_state_flag_88(void);

#endif /* OVERLAYS_SHARED_SCOUTBOT_AND_PARATROOPER_H */
