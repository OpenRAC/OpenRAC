/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The C library's memory and string functions the game calls whose code is
 * still assembly in the decompilation (libraries.tsv): the host's own, on
 * game memory. */
#include "openrac/game_host.h"
#include "openrac/game_lib.h"

/* memcmp */
int openrac_lib_memcmp(gaddr a, gaddr b, unsigned int n) {
    return memcmp(G(a), G(b), n);
}

/* memcpy */
gaddr openrac_lib_memcpy(gaddr dst, gaddr src, unsigned int n) {
    memmove(G(dst), G(src), n);
    return dst;
}

/* memchr */
gaddr openrac_lib_memchr(gaddr s, int c, unsigned int n) {
    const unsigned char* host = (const unsigned char*)G(s);
    const void* at = memchr(host, c & 0xFF, n);
    return at == NULL ? 0 : s + (gaddr)((const unsigned char*)at - host);
}

/* memset */
gaddr openrac_lib_memset(gaddr dst, int c, unsigned int n) {
    memset(G(dst), c, n);
    return dst;
}

/* strchr */
gaddr openrac_lib_strchr(gaddr s, int c) {
    const char* host = (const char*)G(s);
    const char* at = strchr(host, c);
    return at == NULL ? 0 : s + (gaddr)(at - host);
}

/* strcmp */
int openrac_lib_strcmp(gaddr a, gaddr b) {
    return strcmp((const char*)G(a), (const char*)G(b));
}

/* strcpy */
gaddr openrac_lib_strcpy(gaddr dst, gaddr src) {
    memmove(G(dst), G(src), strlen((const char*)G(src)) + 1);
    return dst;
}

/* strlen */
int openrac_lib_strlen(gaddr s) {
    return (int)strlen((const char*)G(s));
}

/* strncmp */
int openrac_lib_strncmp(gaddr a, gaddr b, int n) {
    return strncmp((const char*)G(a), (const char*)G(b), (size_t)n);
}

/* strncpy */
gaddr openrac_lib_strncpy(gaddr dst, gaddr src, int n) {
    strncpy((char*)G(dst), (const char*)G(src), (size_t)n);
    return dst;
}

/* __extendsfdf2: float to double, which the console did in software. */
double openrac_lib___extendsfdf2(float f) {
    return (double)f;
}

/* __muldi3: the low 64 bits of a 64-bit product, which the console's 32-bit
 * multiplier built from halves (libgcc). Unsigned, so it wraps as there. */
long long openrac_lib___muldi3(long long a, long long b) {
    return (long long)((unsigned long long)a * (unsigned long long)b);
}

/* __udivdi3, __umoddi3: 64-bit unsigned division, which the console's 32-bit
 * divider built from halves (libgcc). A zero divisor, which libgcc divides by
 * on purpose to fault where a CPU does, gives what the EE's divu leaves: all
 * ones for the quotient, the dividend for the remainder. */
unsigned long long openrac_lib___udivdi3(unsigned long long a, unsigned long long b) {
    return b ? a / b : ~0ull;
}

unsigned long long openrac_lib___umoddi3(unsigned long long a, unsigned long long b) {
    return b ? a % b : a;
}

/* __divdi3, __moddi3: 64-bit signed division (libgcc's: the magnitudes divided as
 * __udivdi3 does, the quotient negated when the signs differ, the remainder given
 * the dividend's sign; -2^63 / -1 is -2^63). A zero divisor goes as in __udivdi3. */
long long openrac_lib___divdi3(long long a, long long b) {
    unsigned long long u = a < 0 ? 0ull - (unsigned long long)a : (unsigned long long)a;
    unsigned long long v = b < 0 ? 0ull - (unsigned long long)b : (unsigned long long)b;
    unsigned long long w = openrac_lib___udivdi3(u, v);
    return (long long)((a < 0) != (b < 0) ? 0ull - w : w);
}

long long openrac_lib___moddi3(long long a, long long b) {
    unsigned long long u = a < 0 ? 0ull - (unsigned long long)a : (unsigned long long)a;
    unsigned long long v = b < 0 ? 0ull - (unsigned long long)b : (unsigned long long)b;
    unsigned long long w = openrac_lib___umoddi3(u, v);
    return (long long)(a < 0 ? 0ull - w : w);
}

/* libgcc's soft-float double arithmetic (dp-bit.c), which the console did in software on 64-bit
 * integer registers: the host's doubles are the same IEEE format, rounded to nearest as dp-bit's.
 * __cmpdf2 gives -1, 0 or 1, and 1 when either is a NaN, as dp-bit's dpcmp does. */
double openrac_lib___adddf3(double a, double b) {
    return a + b;
}

double openrac_lib___subdf3(double a, double b) {
    return a - b;
}

double openrac_lib___muldf3(double a, double b) {
    return a * b;
}

double openrac_lib___divdf3(double a, double b) {
    return a / b;
}

int openrac_lib___cmpdf2(double a, double b) {
    if (a != a || b != b) {
        return 1;
    }
    return a < b ? -1 : (a > b ? 1 : 0);
}

double openrac_lib___floatsidf(int i) {
    return (double)i;
}

/* __fixdfsi, __fixunsdfsi: towards zero, saturated past the range (dp-bit's dptoli and dptoul
 * give the limits there; a NaN gives 0), where the host's conversion is not defined. */
int openrac_lib___fixdfsi(double d) {
    if (d != d) {
        return 0;
    }
    if (d >= 2147483648.0) {
        return 0x7FFFFFFF;
    }
    if (d <= -2147483649.0) {
        return (int)0x80000000u;
    }
    return (int)d;
}

unsigned int openrac_lib___fixunsdfsi(double d) {
    if (!(d > 0.0)) {
        return 0;
    }
    if (d >= 4294967296.0) {
        return 0xFFFFFFFFu;
    }
    return (unsigned int)d;
}

float openrac_lib___truncdfsf2(double d) {
    return (float)d;
}

/* __fixunsdfdi: a double to an unsigned 64-bit integer (libgcc's soft-float
 * one: negative gives 0). Past 2^64 the console's result is not defined; this
 * saturates rather than leaving it to the host's conversion. */
unsigned long long openrac_lib___fixunsdfdi(double d) {
    if (!(d > 0.0)) {
        return 0;
    }
    if (d >= 18446744073709551616.0) {
        return ~0ull;
    }
    return (unsigned long long)d;
}

/* __main: what the console's start-up ran before main (global constructors;
 * the game has none). */
void openrac_lib___main(void) {}
