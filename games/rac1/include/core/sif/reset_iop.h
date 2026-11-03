#ifndef CORE_SIF_RESET_IOP_H
#define CORE_SIF_RESET_IOP_H

/* Functions defined in src/core/sif/reset_iop.c. */

#include "common.h"

extern void InitTLB(void);
extern void InitTLBFunctions(void);
extern int sce_sif_reboot_iop(void *);
extern int sce_sif_sync_iop(void);

#endif /* CORE_SIF_RESET_IOP_H */
