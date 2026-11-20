// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Keyboard and mouse for Ratchet & Clank (docs/port/KEYBOARD_MOUSE.md), built into the port. It
// loads through the mod interface as a mod with no library (game/common/mods.h, BuiltIn): these
// are the two entries a mod's library would export, by names of their own.

#pragma once

#include "openrac/openrac_mod.h"

namespace openrac::keyboard_mouse {

// The mod's id and version (openrac_mod_query).
void query(openrac_mod_info* info);

// Loads it (openrac_mod_load): 0 when it loaded.
int load(const openrac_api* api, openrac_mod* mod);

}  // namespace openrac::keyboard_mouse
