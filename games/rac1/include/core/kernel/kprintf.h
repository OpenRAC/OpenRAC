#ifndef CORE_KERNEL_KPRINTF_H
#define CORE_KERNEL_KPRINTF_H

/* Functions defined in src/core/kernel/kprintf.c. */

#include "common.h"

extern int sceLseek(int, int, int);
extern int sceRead(int fd, void *buf, int len);
extern int sceWrite(int, void *, int);
extern int sce_sif_check_stat_rpc(void *);

#endif /* CORE_KERNEL_KPRINTF_H */
