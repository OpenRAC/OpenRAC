#ifndef CORE_MPEG_CSC_H
#define CORE_MPEG_CSC_H

/* Functions defined in src/core/mpeg/csc.c. */

#include "common.h"

extern void _csc_storeRefImage(void *, void *);
extern int sysbit_get(void *, int);
extern void sysbit_init(void *, void *, void *, int);
extern void sysbit_jump(void *, int);
extern void sysbit_marker(void *);
extern void *sysbit_ptr(void *, int);

#endif /* CORE_MPEG_CSC_H */
