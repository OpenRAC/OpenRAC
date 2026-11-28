// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods built into Ratchet & Clank's program (built_in_mods.h): the keyboard and mouse
// (keyboard_mouse/keyboard_mouse.h) and the player of what OpenRAC Editor packs with a map
// (editor_runtime/editor_runtime.h), with their manifests as cmake/Games.cmake embeds them.

#include "game/common/built_in_mods.h"

#include "game/rac1/editor_runtime/editor_runtime.h"
#include "game/rac1/keyboard_mouse/keyboard_mouse.h"

// Their openrac-mod.json texts (generated: openrac_rac1_built_in_manifests.cpp).
extern const unsigned char openrac_keyboard_mouse_manifest[];
extern const unsigned char openrac_editor_runtime_manifest[];

namespace openrac::mods {

std::vector<BuiltIn> built_in_mods() {
    return {
        {reinterpret_cast<const char*>(openrac_keyboard_mouse_manifest),
         keyboard_mouse::query,
         keyboard_mouse::load},
        {reinterpret_cast<const char*>(openrac_editor_runtime_manifest),
         editor_runtime::query,
         editor_runtime::load}
    };
}

}  // namespace openrac::mods
