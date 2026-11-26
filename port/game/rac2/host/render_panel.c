/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the off-screen panels a pause-menu page's panels and a cutscene's object
 * preview render into, as panels of the frame recorder (docs/port/FRAME_RECORDER.md, section
 * 3.3, docs/port/SCREEN_EFFECTS.md, section 5.1).
 *
 * gfx_build_screen_sprite_packet (func_00285030) is what both gfx_setup_screen_copy_draw_state
 * (func_00280510, the panels) and gfx_setup_offscreen_target (func_0027B9C0, the cutscene object preview's target)
 * run to point the draw environment at an area of the console's frame memory and clear it; what the
 * game draws after goes there until a draw-list header puts the screen back: gfx_run_screen_effect_setup
 * Here
 * the game's own C runs unchanged and the recorder is told where the panel opens and closes, at the
 * list cursor of each: the packets, the 2D quads and the moby markers in between are drawn into a
 * native render target (its id says which block it stood for), and the quad that shows the panel
 * (g_quad_texture, a texture at that block) samples it.
 *
 * Measured in hidden runs on box1 (2026-11-26, NTSC-U): the Ship Shack vendor (level 24), the seven
 * pause pages (level 4) and space flight (level 5) never reach func_00285030, so these wrappers did
 * not run there: the pause pages' panels carry flag bit 1 (drawn directly, not through func_00280510;
 * the Weapons page's panel has flags 9) and the vendor does not use the pause panels at all
 * (menu_draw_pause_panels, func_002CBC58, 0 calls). func_0027B9C0 runs only when a cutscene holds an
 * object to preview (g_scene_playback_state.capture; flight leaves it empty). OPENRAC_UI_LOG logs
 * each panel that opens.
 *
 * No window or OPENRAC_UI_REPLAY: the game's own, nothing recorded.
 */
#include <stdint.h>
#include <stdio.h>

#include "game_protos.h"
#include "openrac/frame.h"
#include "openrac/game_host.h"
#include "openrac/host_env.h"
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
        /* OPENRAC_UI_LOG: each panel that opens (mode 0 gfx_setup_screen_copy_draw_state, 1 the
         * cutscene object's target gfx_setup_offscreen_target), the first few of each mode. */
        if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
            static int logged[2];
            if (mode >= 0 && mode < 2 && logged[mode]++ < 8) {
                fprintf(
                    stderr,
                    "render_panel: open %dx%d tbp %#x mode %d\n",
                    1 << tw,
                    1 << th,
                    tbp,
                    mode
                );
            }
        }
    }
    func_00285030__game(tw, th, tbp, mode);
}

void func_00284EA8__game(void);

/* gfx_append_draw_list_header: the packet that puts the screen's draw environment back (after a
 * panel, gfx_run_screen_effect_setup and gfx_call_sync_helper run it). */
void func_00284EA8(void) {
    if (s_panel_open) {
        s_panel_open = 0;
        if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
            static int logged;
            if (logged++ < 8) {
                fprintf(stderr, "render_panel: closed by the draw-list header\n");
            }
        }
        rac2_anchor();
        openrac_frame_target_end();
    }
    func_00284EA8__game();
}
