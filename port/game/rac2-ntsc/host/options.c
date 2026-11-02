/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the game's own 16:9 option (the title's Options page, "16:9 (widescreen)",
 * 0x001A7BB1). On, UpdateViewContext narrows the vertical view by a quarter for a television that
 * stretches the picture back. The window's picture has the player's shape (settings `aspect`, the
 * Graphics menu's Picture) and keeps the game's vertical view, wider pictures showing more to the
 * sides; on as well, the world would be squeezed. So the flag is kept off, each frame.
 */
#include <stdint.h>

#include "openrac/guest.h"

#define WIDESCREEN 0x001A7BB1u

/* Each frame (tools.c's on_frame): the game's 16:9 off. */
void rac2_options_frame(void) {
    GREF(uint8_t, WIDESCREEN) = 0;
}
