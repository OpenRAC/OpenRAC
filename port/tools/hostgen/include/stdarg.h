/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * hostgen reads the decompilation with -nostdinc: the few files that include
 * <stdarg.h> get Clang's own builtins under the standard names. */
#ifndef OPENRAC_HOSTGEN_STDARG_H
#define OPENRAC_HOSTGEN_STDARG_H
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#endif
