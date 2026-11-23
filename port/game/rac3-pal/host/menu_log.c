/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * OPENRAC_RAC3_MENU_LOG=PATH: one line a frame of what the front end showed, for the scripted menu
 * walk (port/game/rac3-pal/menus.py): the window frame, the front end's state words (mode, boot
 * state, boot screen, the pad's held and pressed buttons) and every string the font printed that
 * frame (func_0038EBC8, ui2d.c), with its place. Development only: nothing reads it otherwise. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

void rac3_menu_log_text(float x, float y, gaddr text, int length);
void rac3_menu_log_install(void);

static FILE* g_log;
static int g_checked;
static char g_texts[16384];
static size_t g_used;

static uint32_t word(gaddr at) {
    uint32_t v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static int open_log(void) {
    if (!g_checked) {
        g_checked = 1;
        const char* path = openrac_env_string("OPENRAC_RAC3_MENU_LOG");
        if (path != NULL && *path != '\0') {
            g_log = fopen(path, "w");
        }
    }
    return g_log != NULL;
}

void rac3_menu_log_text(float x, float y, gaddr text, int length) {
    if (!open_log()) {
        return;
    }
    /* A print of no characters (a wrapped notice's blank line) shows nothing. Below 0x40000000 the
     * text is in RAM or an alias of it; above, in the scratchpad or a translated function's frame. */
    if (text == 0 || length == 0) {
        return;
    }
    /* The font's own codes (button glyphs below 0x20, accented letters from 0x80) as \xNN, so the
     * line stays plain text. */
    char line[1024];
    const char* s = (const char*)G(text < 0x40000000u ? text & 0x01FFFFFFu : text);
    size_t out = 0;
    for (size_t n = 0; (length < 0 || n < (size_t)length) && s[n] != '\0' && out + 5 < sizeof line;
         ++n) {
        const unsigned char c = (unsigned char)s[n];
        if (c < 0x20u || c >= 0x7Fu || c == '|' || c == '\\') {
            out += (size_t)snprintf(line + out, sizeof line - out, "\\x%02X", c);
        } else {
            line[out++] = (char)c;
        }
    }
    line[out] = '\0';
    const int w =
        snprintf(g_texts + g_used, sizeof g_texts - g_used, "\t%d,%d|%s", (int)x, (int)y, line);
    if (w > 0 && g_used + (size_t)w < sizeof g_texts) {
        g_used += (size_t)w;
    }
}

static void on_frame(unsigned frame) {
    if (!open_log()) {
        return;
    }
    const gaddr pad = word(RAC3_ADDR(0x001D52FCu));
    fprintf(
        g_log,
        "%u\tmode=%d\tstate=%d\tboot=%d\tscreen=%d\theld=%04x\tpressed=%04x%s\n",
        frame,
        (int)word(RAC3_ADDR(0x001D5B94u)),
        (int)word(RAC3_ADDR(0x001D5458u)),
        (int)word(RAC3_ADDR(0x001D568Cu)),
        (int)word(RAC3_ADDR(0x001D5690u)),
        pad ? word(pad + 0x1A0u) & 0xFFFFu : 0u,
        pad ? word(pad + 0x1A4u) & 0xFFFFu : 0u,
        g_texts
    );
    fflush(g_log);
    g_used = 0;
    g_texts[0] = '\0';
}

void rac3_menu_log_install(void) {
    if (open_log()) {
        openrac_game_on_frame = on_frame;
    }
}
