/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's formatted prints, answered by the port.
 *
 * newlib's printf (0x00115D48) and sprintf (0x00115DA8) and the kernel
 * library's kprintf (0x0011C7E8) and scePrintf (0x0011C820) are variadic:
 * their C (or their assembly) walks the console's argument save area, which a
 * host va_list is not. The port formats them (common/lib/guest_format.h), as
 * rac1's port does, and the prints go to the log.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "rac2.h"
#include "game/common/lib/guest_format.h"
#include "openrac/guest.h"

/* printf: to the log. */
void func_00115D48(gaddr format, ...) {
    char out[1024];
    va_list args;

    va_start(args, format);
    openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    fprintf(stderr, "[game] %s", out);
}

/* sprintf: the text is written into game memory at BUFFER. */
int func_00115DA8(gaddr buffer, gaddr format, ...) {
    char out[1024];
    int n;
    va_list args;

    va_start(args, format);
    n = openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    memcpy(G(buffer), out, (size_t)n + 1);
    return n;
}

/* kprintf: the kernel library's debug print. */
void func_0011C7E8(gaddr format, ...) {
    char out[1024];
    va_list args;

    va_start(args, format);
    openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    fprintf(stderr, "[game] %s", out);
}

/* scePrintf: the same through the debugger's console; the count written. */
int func_0011C820(gaddr format, ...) {
    char out[1024];
    int n;
    va_list args;

    va_start(args, format);
    n = openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    fprintf(stderr, "[game] %s", out);
    return n;
}

/* The kernel's print core: its C does pointer arithmetic on a va_list, which does not compile on
 * ARM64. Only the two prints above call it, so it is never reached (as rac1-pal/host/kprintf.c). */
int func_0011C1F8(gaddr format, va_list arguments) {
    (void)format;
    (void)arguments;
    return 0;
}
