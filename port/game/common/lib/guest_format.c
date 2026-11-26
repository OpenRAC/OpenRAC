/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The games' printf conversions (guest_format.h), with the host's snprintf doing each one. */
#include "game/common/lib/guest_format.h"

#include <stdio.h>
#include <string.h>

#include "openrac/guest.h"

/* True when `s` lies in [base, base + size). */
static int in_region(gaddr s, gaddr base, uint32_t size) {
    return s >= base && s - base < size;
}

/* The text of a %s argument: a game address in RAM (direct or by an uncached alias), in the
 * scratchpad (Up Your Arsenal builds its message boxes' text there: "Progress is saving..."), or in
 * the port's own regions (a translated function's locals, string literals); "(null)" for 0 or for
 * an address that is no memory at all. A format that reads more arguments than its caller handed
 * on takes what the registers or the stack held (a %s over such leftovers read 0xFFFFFFC0 and took
 * the host down), and the console would have printed whatever lay there or faulted. */
static const char* guest_string(gaddr s) {
    const uint32_t ram = openrac_guest_ram_size;
    if (s == 0) {
        return "(null)";
    }
    if (in_region(s, GUEST_SCRATCHPAD, GUEST_SCRATCHPAD_SIZE)
        || in_region(s, GUEST_PORT, GUEST_PORT_SIZE)
        || in_region(s, GUEST_PORT_LOW, GUEST_PORT_LOW_SIZE)) {
        return (const char*)G(s);
    }
    const int in_ram = s < ram || in_region(s, GUEST_RAM_UNCACHED, ram)
                       || in_region(s, GUEST_RAM_UNCACHED_ACCEL, ram);
    return in_ram ? (const char*)G(openrac_guest_ram_address(s)) : "(null)";
}

int openrac_guest_vformat(char* out, size_t size, const char* f, va_list args) {
    size_t n = 0;

    if (size == 0) {
        return 0;
    }
    while (*f != '\0' && n + 1 < size) {
        char spec[32];
        size_t k = 0;
        char piece[512];
        int made = 0;

        if (*f != '%') {
            out[n++] = *f++;
            continue;
        }
        spec[k++] = *f++;
        while (*f != '\0' && strchr("-+ #0123456789.", *f) != NULL && k + 2 < sizeof(spec)) {
            spec[k++] = *f++;
        }
        while (*f == 'l' || *f == 'h') {
            f++; /* the game's int and long are passed as host ints here */
        }
        if (*f == '\0') {
            break;
        }
        spec[k++] = *f;
        spec[k] = '\0';
        switch (*f++) {
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
            case 'c':
                made = snprintf(piece, sizeof(piece), spec, va_arg(args, int));
                break;
            case 's': {
                made = snprintf(piece, sizeof(piece), spec, guest_string((gaddr)va_arg(args, int)));
                break;
            }
            case 'f':
            case 'e':
            case 'g':
                made = snprintf(piece, sizeof(piece), spec, va_arg(args, double));
                break;
            case '%':
                made = snprintf(piece, sizeof(piece), "%%");
                break;
            default:
                made = 0;
                break;
        }
        if (made > 0) {
            size_t take = (size_t)made < sizeof(piece) ? (size_t)made : sizeof(piece) - 1;
            if (n + take >= size) {
                take = size - 1 - n;
            }
            memcpy(out + n, piece, take);
            n += take;
        }
    }
    out[n] = '\0';
    return (int)n;
}
