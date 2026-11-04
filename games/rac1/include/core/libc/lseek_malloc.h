#ifndef CORE_LIBC_LSEEK_MALLOC_H
#define CORE_LIBC_LSEEK_MALLOC_H

/* Functions defined in src/core/libc/lseek_malloc.c. */

#include "common.h"

extern void __smakebuf(void *);
extern int _lseek_r(void *, void *, void *, void *);
extern void *_malloc_r(void *, unsigned int);
extern int libc_mbtowc_r(void *, void *, void *, int, void *);

#endif /* CORE_LIBC_LSEEK_MALLOC_H */
