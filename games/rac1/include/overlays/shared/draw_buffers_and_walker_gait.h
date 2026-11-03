#ifndef OVERLAYS_SHARED_DRAW_BUFFERS_AND_WALKER_GAIT_H
#define OVERLAYS_SHARED_DRAW_BUFFERS_AND_WALKER_GAIT_H

/* Functions defined in src/overlays/shared/draw_buffers_and_walker_gait.c. */

#include "common.h"

extern void WalkCycleStep(void *, void *, float, float, float, int);
extern void append_draw_buffer_upload_packets(int, int, int);
extern void reset_walker_gait_helpers(void *);
extern void transfer_draw_buffer_chunks(int, int, int);
extern int walker_gait_mask(void *, float);

#endif /* OVERLAYS_SHARED_DRAW_BUFFERS_AND_WALKER_GAIT_H */
