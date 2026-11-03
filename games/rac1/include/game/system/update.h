#ifndef GAME_SYSTEM_UPDATE_H
#define GAME_SYSTEM_UPDATE_H

/* Functions defined in src/game/system/update.c. */

#include "common.h"

extern void DebugProfiler_Hook(void);
extern void project_graphics_bounds(void *, void *, void *, void *, void *, void *);
extern void project_graphics_bounds_float(void *, void *, float *, float *, float *, float *);
extern void set_scrolling_status_message(void *);

#endif /* GAME_SYSTEM_UPDATE_H */
