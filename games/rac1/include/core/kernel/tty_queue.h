#ifndef CORE_KERNEL_TTY_QUEUE_H
#define CORE_KERNEL_TTY_QUEUE_H

/* Functions defined in src/core/kernel/tty_queue.c. */

#include "common.h"

extern int sceTtyInit(void);
extern int sceTtyRead(int, int);
extern int sceTtyWrite(int, int);

#endif /* CORE_KERNEL_TTY_QUEUE_H */
