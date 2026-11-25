// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The player's keyboard layout, as SDL's keymap gives it: the character each key prints, by the
// key's place (its USB HID usage, which is SDL's scancode). The port names keys by place ("KeyQ" is
// the key right of Tab); on an AZERTY keyboard that key prints A, and the names a player reads
// (game/common/key_labels.h) are the layout's.

#pragma once

namespace openrac::platform {

// Reads the layout again: as the window opens, and when SDL says it changed
// (SDL_EVENT_KEYMAP_CHANGED). On the window's thread, with SDL's video started.
void refresh_key_layout();

// The capital or sign key `usage` prints on the layout, unshifted; 0 for a key that prints none or
// one outside ASCII, an unknown usage, or before refresh_key_layout. Safe from any thread.
char key_layout_char(int usage);

}  // namespace openrac::platform
