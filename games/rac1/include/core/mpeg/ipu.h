#ifndef CORE_MPEG_IPU_H
#define CORE_MPEG_IPU_H

/* Functions defined in src/core/mpeg/ipu.c. */

#include "common.h"

extern void sce_ipu_init(void);
extern void sce_ipu_restart_dma(void *);
extern void sce_ipu_stop_dma(void *);
extern void sce_ipu_sync(int, int);
extern int sce_scf_get_language(void);
extern void sce_scf_get_local_time_from_rtc(void *);

#endif /* CORE_MPEG_IPU_H */
