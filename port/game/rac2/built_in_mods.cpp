// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods built into Going Commando's program (built_in_mods.h): the keyboard and mouse
// (keyboard_mouse/keyboard_mouse.h), with the manifest it shares with Ratchet & Clank's.

#include "game/common/built_in_mods.h"

#include "game/rac2/keyboard_mouse/keyboard_mouse.h"

// game/rac1/keyboard_mouse/openrac-mod.json's text (generated: openrac_rac2_built_in_manifests.cpp).
extern const unsigned char openrac_keyboard_mouse_manifest[];

namespace openrac::mods {

std::vector<BuiltIn> built_in_mods() {
    return {
        {reinterpret_cast<const char*>(openrac_keyboard_mouse_manifest),
         keyboard_mouse::query,
         keyboard_mouse::load}
    };
}

}  // namespace openrac::mods
