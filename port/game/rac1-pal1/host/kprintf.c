/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The kernel library's debug prints (libkernl's kprintf, src/core/00119D88.c), answered by the
 * port (hostgen.json, "host_functions"). Both hand their format and a va_list to the print core
 * (func_00119DA0), which walks the console's argument area and writes each character through a
 * hook: the debugger's console, or nothing on a retail console. hostgen
 * cannot pass a host va_list as the console's, so the port formats here (common/lib/guest_format.h)
 * and the text goes to the log. PAL only: the NTSC program's names for these differ.
 */
#include <stdarg.h>
#include <stdio.h>

#include "game/common/lib/guest_format.h"
#include "rac1.h"

/* kprintf: the format and its arguments, through the print hook of the moment. */
void func_0011A390(gaddr format, ...) {
    char out[1024];
    va_list args;

    va_start(args, format);
    openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    fprintf(stderr, "[game] %s", out);
}

/* scePrintf: the same with the debugger's console writer (func_00119AC0) as the hook for the
 * call; the count the print core wrote. */
int func_0011A3C8(gaddr format, ...) {
    char out[1024];
    int n;
    va_list args;

    va_start(args, format);
    n = openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    fprintf(stderr, "[game] %s", out);
    return n;
}

/* The print core itself: matched C in the decompilation, left out here (hostgen.json,
 * "host_functions") because it walks the MIPS argument area with pointer arithmetic on its
 * va_list, which Clang's va_list is not on Linux ARM64. Only the two prints above call it,
 * and the port answers those itself, so nothing reaches this. */
int func_00119DA0(gaddr format, va_list arguments) {
    (void)format;
    (void)arguments;
    return 0;
}
