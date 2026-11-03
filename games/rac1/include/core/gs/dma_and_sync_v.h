#ifndef CORE_GS_DMA_AND_SYNC_V_H
#define CORE_GS_DMA_AND_SYNC_V_H

/* Functions defined in src/core/gs/dma_and_sync_v.c. */

#include "common.h"

extern int sceDmaReset(int);
extern int sceGsSyncVCallback(void *);
extern void *sce_dma_get_chan(int);
extern void sce_dma_send(void *, int);

#endif /* CORE_GS_DMA_AND_SYNC_V_H */
