#ifndef CORE_CDVD_CALLBACKS_H
#define CORE_CDVD_CALLBACKS_H

/* Functions defined in src/core/cdvd/callbacks.c. */

#include "common.h"

extern int cd_ncmd_disk_ready(void);
extern int cd_ncmd_prechk(int);
extern void cd_read_end_callback(int);
extern int cd_scmd_prechk(int);
extern void *sce_cd_callback(void *);
extern s32 sce_cd_disk_ready(s32);
extern int sce_cd_init(int);
extern int sce_cd_mmode(int);
extern int sce_cd_sync_s(int);

#endif /* CORE_CDVD_CALLBACKS_H */
