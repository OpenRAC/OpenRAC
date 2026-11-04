#ifndef CORE_LIBC_WRITE_WSETUP_H
#define CORE_LIBC_WRITE_WSETUP_H

/* Functions defined in src/core/libc/write_wsetup.c. */

#include "common.h"

extern long _write_r(void *, void *, void *, void *);
extern int libc_swsetup(void *);

#endif /* CORE_LIBC_WRITE_WSETUP_H */
