/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * What the decompilation's C asks of <stddef.h>; hostgen reads it with -nostdinc.
 */
#ifndef OPENRAC_HOSTGEN_STDDEF_H
#define OPENRAC_HOSTGEN_STDDEF_H

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef NULL
#define NULL ((void*)0)
#endif
#define offsetof(type, member) __builtin_offsetof(type, member)

#endif
