#ifndef CORE_LIBC_STDIO_OPS_H
#define CORE_LIBC_STDIO_OPS_H

/* Functions defined in src/core/libc/stdio_ops.c. */

#include "common.h"

extern int __sclose(void *);
extern int __sread(void *arg0, void *arg1, void *arg2);
extern long __sseek(void *, void *, void *);
extern int __swrite(void *arg0, void *arg1, void *arg2);

#endif /* CORE_LIBC_STDIO_OPS_H */
