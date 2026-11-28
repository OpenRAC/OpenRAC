// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's keyboard shortcuts (F12 for a screenshot, F11 for full screen...): one list, changed
// in the menu's Controls tab (Keyboard and mouse, Shortcuts) or the launcher's Settings >
// Controls. Each is kept in the global controls file as shortcut_<id> (platform/shortcut_keys.h);
// one the file does not name has its default.

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "platform/shortcut_keys.h"

namespace openrac::frontend::shortcuts {

enum class Id : std::uint8_t {
    Screenshot,
    FullScreen,
    VolumeDown,
    VolumeUp,
    FrameCounter,
    EnhancedGraphics,
    TexturePack,
    StatsPanel,
    RecordDiagnostics,
    Pause,
    StepTick,
    Slower,
    Faster,
    LevelSelect,
    Count
};

struct Shortcut {
    Id id;
    const char* key;       // the controls file's shortcut_<key>, and the launcher's
    const char* name;      // as the player reads it
    const char* section;   // "Game" or "Developer"
    const char* defaults;  // the keys when the file names none
};

std::span<const Shortcut> all();
const Shortcut& of(Id id);

// The keys bound now.
std::vector<platform::ShortcutKey> keys(Id id);

// "Ctrl+F3", "F11 or Alt+Enter", "None": for the menu, the help texts and a notification.
std::string label(Id id);

// Whether the key press is `id`'s.
bool hit(Id id, const SDL_KeyboardEvent& key);

// Makes `key` a press of `id` that no keyboard made (a menu row's, debug/menu_bar.cpp's press):
// hit says it is `id`'s whatever keys are bound.
void as_press(Id id, SDL_KeyboardEvent& key);

// Binds `keys` to `id` (empty: none), written to the global controls file. False when it cannot be
// written.
bool set(Id id, const std::vector<platform::ShortcutKey>& keys);

// Puts `id` back to its default. False when it cannot be written.
bool reset(Id id);

}  // namespace openrac::frontend::shortcuts
