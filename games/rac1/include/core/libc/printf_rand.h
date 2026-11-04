#ifndef CORE_LIBC_PRINTF_RAND_H
#define CORE_LIBC_PRINTF_RAND_H

/* Functions defined in src/core/libc/printf_rand.c. */

#include "common.h"

extern void *_sbrk_r(void *, int);
extern int double_is_not_nan(double);
extern int func_00116078(void *);
extern int func_001160D8(void);
extern int libc_isnan(double);
extern long reentrant_read(void *, void *, void *, void *);
extern void srand(s32);

#endif /* CORE_LIBC_PRINTF_RAND_H */
