// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Keyboard and mouse for Going Commando (docs/port/KEYBOARD_MOUSE.md), built into the port, as
// Ratchet & Clank's is (game/rac1/keyboard_mouse/keyboard_mouse.h): the two entries a mod's
// library would export.

#pragma once

#include "openrac/openrac_mod.h"

namespace openrac::keyboard_mouse {

// The mod's id and version (openrac_mod_query).
void query(openrac_mod_info* info);

// Loads it (openrac_mod_load): 0 when it loaded.
int load(const openrac_api* api, openrac_mod* mod);

}  // namespace openrac::keyboard_mouse
