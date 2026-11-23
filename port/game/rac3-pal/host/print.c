/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's sprintf, answered by the port.
 *
 * newlib's sprintf (0x0011B2E8) is variadic: its assembly saves $a2-$t3 and $f12-$f19 to an
 * argument save area and walks it, which the translated code has no way to fill (the callers'
 * calls carried the buffer and the format alone, and every %s printed "(null)": the Options page's
 * rows). The port formats it (common/lib/guest_format.h), as Going Commando's port does
 * (rac2/host/print.c). Floats travel in $f12-$f19 on the EE, not with the integers, so a %f is
 * not formatted from them; the front end's rows use %s, %c and %d.
 */
#include <stdarg.h>
#include <string.h>

#include "game/common/lib/guest_format.h"
#include "openrac/guest.h"

/* sprintf: the text is written into game memory at BUFFER. */
int func_0011B2E8(gaddr buffer, gaddr format, ...) {
    char out[1024];
    int n;
    va_list args;

    va_start(args, format);
    n = openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    memcpy(G(buffer), out, (size_t)n + 1);
    return n;
}
