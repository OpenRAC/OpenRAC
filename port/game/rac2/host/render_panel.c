/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the off-screen panels the Ship Shack's and the pause menu's panels and the cockpit's
 * 256 x 256 picture render into, as panels of the frame recorder (docs/port/FRAME_RECORDER.md, section
 * 3.3, docs/port/SCREEN_EFFECTS.md, section 5.1).
 *
 * gfx_build_screen_sprite_packet (func_00285030) is what both gfx_setup_screen_copy_draw_state
 * (func_00280510, the panels) and gfx_setup_offscreen_target (func_0027B9C0, the cockpit's target)
 * run to point the draw environment at an area of the console's frame memory and clear it; what the
 * game draws after goes there until a draw-list header puts the screen back: gfx_run_screen_effect_setup
 * Here
 * the game's own C runs unchanged and the recorder is told where the panel opens and closes, at the
 * list cursor of each: the packets, the 2D quads and the moby markers in between are drawn into a
 * native render target (its id says which block it stood for), and the quad that shows the panel
 * (g_quad_texture, a texture at that block) samples it.
 *
 * No window or OPENRAC_UI_REPLAY: the game's own, nothing recorded.
 */
#include <stdint.h>

#include "game_protos.h"
#include "openrac/frame.h"
#include "openrac/game_host.h"
#include "rac2.h"

/* A panel's port id: this base and the page its memory starts at (tbp >> 13). */
#define PANEL_BASE 0x52330000u

static int s_panel_open;

void func_00285030__game(int tw, int th, int tbp, int mode);

void func_00285030(int tw, int th, int tbp, int mode) {
    if (openrac_frame_native() && tw > 0 && tw <= 10 && th > 0 && th <= 10) {
        rac2_anchor();
        openrac_frame_target_begin(
            PANEL_BASE | (((unsigned)tbp >> 13) & 0xFFFFu), 1 << tw, 1 << th
        );
        s_panel_open = 1;
    }
    func_00285030__game(tw, th, tbp, mode);
}

void func_00284EA8__game(void);

/* gfx_append_draw_list_header: the packet that puts the screen's draw environment back (after a
 * panel, gfx_run_screen_effect_setup and gfx_call_sync_helper run it). */
void func_00284EA8(void) {
    if (s_panel_open) {
        s_panel_open = 0;
        rac2_anchor();
        openrac_frame_target_end();
    }
    func_00284EA8__game();
}
