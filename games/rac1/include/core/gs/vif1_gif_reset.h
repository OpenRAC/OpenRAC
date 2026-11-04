#ifndef CORE_GS_VIF1_GIF_RESET_H
#define CORE_GS_VIF1_GIF_RESET_H

/* Functions defined in src/core/gs/vif1_gif_reset.c. */

#include "common.h"

extern void reset_vif1_gif(void);
extern int wait_for_graphics_pipeline_idle(int, unsigned short);

#endif /* CORE_GS_VIF1_GIF_RESET_H */
