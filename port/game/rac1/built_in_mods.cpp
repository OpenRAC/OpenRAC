// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods built into Ratchet & Clank's program (built_in_mods.h): the keyboard and mouse
// (keyboard_mouse/keyboard_mouse.h), with its manifest as cmake/Games.cmake embeds it.

#include "game/common/built_in_mods.h"

#include "game/rac1/keyboard_mouse/keyboard_mouse.h"

// keyboard_mouse/openrac-mod.json's text (generated: openrac_rac1_built_in_manifests.cpp).
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
