/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank PAL v2.00: the off-screen panels the vendor's monitors and the pause menu render
 * into, as panels of the frame recorder (docs/port/FRAME_RECORDER.md, section 3.3, and
 * docs/port/SCREEN_EFFECTS.md, section 5.1).
 *
 * setup_render_target (func_001FB608) points the draw environment at a (1 << a) x (1 << b) area of
 * the console's frame memory and clears it (the monitors' background func_001FB908 follows it);
 * everything the game then draws goes there until PutDrawBufferLarge (func_001FB498) puts the draw
 * buffer back. Here the game's own C runs unchanged, and the recorder is told where the panel opens
 * and where it closes, at the list cursor of each: the packets, the 2D quads and the moby markers
 * in between are drawn into a native render target (its id says which block it stood for), and the
 * quad that later shows the panel (a texture at that block) samples it.
 *
 * OPENRAC_UI_SKIP naming either function, OPENRAC_UI_REPLAY and no window keep the game's own.
 */
#include <string.h>

#include "openrac/frame.h"
#include "openrac/host_env.h"
#include "rac1.h"

#if !RAC1_OMIT(func_001FB608) && !RAC1_OMIT(func_001FB498)
/* The list cursor the packets are appended through (g_render_packet_cursor). */
#define LIST_CURSOR RAC1_ADDR(0x00161000u)
/* The game's vertical-sync counter (game_frame_counter): one step a displayed frame. */
#define FRAME_COUNTER RAC1_ADDR(0x0015F538u)
/* A panel's port id: this base and the page its memory starts at (c >> 13). */
#define PANEL_BASE 0x52320000u

static int panel_skipped(void) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL
           && (strstr(list, RAC1_NAME(func_001FB608)) != NULL
               || strstr(list, RAC1_NAME(func_001FB498)) != NULL);
}

/* The frame a panel was opened in, or -1: the close only follows an open of the same frame. */
static int s_open_frame = -1;

void func_001FB608(int a, int b, int c) {
    if (openrac_frame_native() && !panel_skipped() && a > 0 && a <= 10 && b > 0 && b <= 10) {
        openrac_frame_anchor_at(OPENRAC_DATA(LIST_CURSOR));
        openrac_frame_target_begin(PANEL_BASE | (((unsigned)c >> 13) & 0xFFFFu), 1 << a, 1 << b);
        s_open_frame = GREF(int, OPENRAC_DATA(FRAME_COUNTER));
    }
    func_001FB608__game(a, b, c);
}

void func_001FB498(void) {
    if (s_open_frame >= 0) {
        if (s_open_frame == GREF(int, OPENRAC_DATA(FRAME_COUNTER))) {
            openrac_frame_anchor_at(OPENRAC_DATA(LIST_CURSOR));
            openrac_frame_target_end();
        }
        s_open_frame = -1;
    }
    func_001FB498__game();
}
#endif
