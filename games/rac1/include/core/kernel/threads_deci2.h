#ifndef CORE_KERNEL_THREADS_DECI2_H
#define CORE_KERNEL_THREADS_DECI2_H

/* Functions defined in src/core/kernel/threads_deci2.c. */

#include "common.h"

extern void InitThread(void);
extern int isceDeci2ExRecv(int, void *, unsigned short);
extern int isceDeci2ExSend(int, void *, unsigned short);
extern void kputs(void *);
extern void sceDeci2Poll(int);
extern int sceDeci2ReqSend(int, int);

#endif /* CORE_KERNEL_THREADS_DECI2_H */
