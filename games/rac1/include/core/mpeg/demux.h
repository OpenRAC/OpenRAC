#ifndef CORE_MPEG_DEMUX_H
#define CORE_MPEG_DEMUX_H

/* Functions defined in src/core/mpeg/demux.c. */

#include "common.h"

extern void _Error(void *, void *);
extern void dispatch_mpeg_cb_nodata(int);
extern void sce_mpeg_reset(void *);

#endif /* CORE_MPEG_DEMUX_H */
