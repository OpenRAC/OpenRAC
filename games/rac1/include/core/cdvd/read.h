#ifndef CORE_CDVD_READ_H
#define CORE_CDVD_READ_H

/* Functions defined in src/core/cdvd/read.c. */

#include "common.h"

extern void *sceGsGetGParam(void);
extern void sceGsResetGraph(int, int, int, int);
extern int sce_cd_break(void);
extern int sce_cd_get_error(void);
extern void sce_cd_read(int, int, int, void *);
extern void sce_cd_read_clock(void *);

#endif /* CORE_CDVD_READ_H */
