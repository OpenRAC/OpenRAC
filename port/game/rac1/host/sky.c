/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the sky routine a level runs.
 *
 * The frame draw (func_001F3D78, DrawDebugProfiler) calls Transition_DrawSky (func_001E9E70) for
 * the sky. Each level's program has its own copy of the frame draw, and that copy calls the level's
 * own sky routine instead (Novalis: render_sky_shells, which picks animated shells on Novalis and
 * static ones elsewhere). The executable's Transition_DrawSky runs the title's star field
 * (func_0022C188): on a level it reseeded rand() once, called it about 250 times a frame, and wrote
 * 256 stars to address 0, because the level's sky record has no star array.
 *
 * Here, on a level, the call goes where the level's copy of the frame draw calls; elsewhere, or
 * where that copy calls nothing else, the executable's C runs as before.
 */
#include "rac1.h"

#if !RAC1_OMIT(func_001E9E70)
void func_001E9E70(void) {
    const gaddr exe = RAC1_ADDR(0x001E9E70u);
    const gaddr own = openrac_level_call(RAC1_ADDR(0x001F3D78u), 0x200u, exe);
    openrac_host_fn fn = own != exe ? openrac_guest_function(own) : 0;
    if (fn != 0 && fn != (openrac_host_fn)func_001E9E70) {
        fn();
        return;
    }
    func_001E9E70__game();
}
#endif
