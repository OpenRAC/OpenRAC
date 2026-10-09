/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The C library's memory and string functions the game calls whose code is
 * still assembly in the decompilation (libraries.tsv): the host's own, on
 * game memory. */
#include "game_protos.h"
#include "rac1_host.h"

/* memcmp */
int func_001151B4(gaddr a, gaddr b, unsigned int n) {
    return memcmp(G(a), G(b), n);
}

/* memcpy */
gaddr func_00115248(gaddr dst, gaddr src, unsigned int n) {
    memmove(G(dst), G(src), n);
    return dst;
}

/* memset */
gaddr func_001153FC(gaddr dst, int c, unsigned int n) {
    memset(G(dst), c, n);
    return dst;
}

/* strchr */
gaddr func_00116428(gaddr s, int c) {
    const char* host = (const char*)G(s);
    const char* at = strchr(host, c);
    return at == NULL ? 0 : s + (gaddr)(at - host);
}

/* strcmp */
int func_001165B8(gaddr a, gaddr b) {
    return strcmp((const char*)G(a), (const char*)G(b));
}

/* strcpy */
gaddr func_001166FC(gaddr dst, gaddr src) {
    memmove(G(dst), G(src), strlen((const char*)G(src)) + 1);
    return dst;
}

/* strlen */
int func_00116810(gaddr s) {
    return (int)strlen((const char*)G(s));
}

/* strncmp */
int func_00116948(gaddr a, gaddr b, int n) {
    return strncmp((const char*)G(a), (const char*)G(b), (size_t)n);
}

/* strncpy */
gaddr func_00116B00(gaddr dst, gaddr src, int n) {
    strncpy((char*)G(dst), (const char*)G(src), (size_t)n);
    return dst;
}

/* __extendsfdf2: float to double, which the console did in software. */
double func_00120778(float f) {
    return (double)f;
}

/* __main: what the console's start-up ran before main (global constructors;
 * the game has none). */
void func_0011DFC8(void) {}
