#ifndef OVERLAYS_SHARED_SCOUTBOT_AND_PARATROOPER_H
#define OVERLAYS_SHARED_SCOUTBOT_AND_PARATROOPER_H

/* Functions defined in src/overlays/shared/scoutbot_and_paratrooper.c. */

#include "common.h"

extern void func_L08_002F20D8(int);
extern void advance_scroll_entry(int);
extern void advance_scroll_offsets(int);
extern void advance_wrapped_sine_pair(void *, float, float, float, void *);
extern void func_L08_002E9288(void *, void *, void *, int);
extern void draw_scrolled_mesh_batch(int);
extern void emit_pair_offset_render_batch(int);
extern void scale_rgba_tables(void *, int, void *, float, float, float, float, int);
extern void func_L08_00315068(void);

#endif /* OVERLAYS_SHARED_SCOUTBOT_AND_PARATROOPER_H */
