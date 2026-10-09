/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Included first in every unit hostgen reads (clangast.py). An inline
 * assembly statement that Clang cannot read (a register constraint only GCC
 * knows) is replaced by a call to this function in the prepared copy, so the
 * unit still reads; hostgen makes the function that contained it a stub. */
#ifndef OPENRAC_HOSTGEN_H
#define OPENRAC_HOSTGEN_H
void openrac_hostgen_asm(void);
#endif
