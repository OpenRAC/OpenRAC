#ifndef CORE_KERNEL_LIBKERNL_GLUE_H
#define CORE_KERNEL_LIBKERNL_GLUE_H

/* Functions defined in src/core/kernel/libkernl_glue.c. */

#include "common.h"

extern int libc_glue_isatty(void);
extern int libc_glue_read();
extern int libc_glue_write();
extern void tty_reset_init_flag(void);
extern long wait_vblank_csr(void);
extern void wait_vblank_start(void);

#endif /* CORE_KERNEL_LIBKERNL_GLUE_H */
