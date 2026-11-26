/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal's view in a picture wider than the game's, as Going Commando's
 * (rac2/host/options.c, rac2/host/wide_view.c). The game's projection set-up (func_003877A0,
 * frontbin's func_003873C0; Going Commando's UpdateViewContext, func_0027AB40, paired at 0.79 by
 * opcode sequence) writes the view block's half-angle tangents (0x00225900: +0xB0 across, +0xB4 up,
 * the latter 0.775 of the former, or 0.756 while the options' first word, 0x00143950, is set) and,
 * with the Options page's 16:9 byte set (0x001439FD, +0xAD of those options) or in split screen,
 * divides the vertical one by 4/3 for a television that stretches the picture back.
 *
 * The window's picture has the player's shape (settings `aspect`, the Graphics menu's Picture) and
 * keeps the game's vertical view, wider pictures showing more to the sides (the frontend widens the
 * horizontal tangent by as much: render's projection, picture_scale) and the 2D layer keeping its
 * shape in the middle (renderer::DirectRenderer). With the game's own 16:9 on as well, the world
 * would be squeezed, so the byte is kept off, each frame, as Going Commando's is.
 */
#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

#define WIDESCREEN RAC3_ADDR(0x001439FDu)

/* Each frame's display list as it is sent (host/dma.c): the game's 16:9 off. */
void rac3_view_frame_sent(void) {
    GREF(uint8_t, WIDESCREEN) = 0;
}
