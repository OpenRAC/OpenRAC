// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A shortcut's keys as the controls file writes them (docs/port/CONTROLS.md, shortcut_* keys): up
// to two, comma separated, each a key's name as the SDK names it (sdk/include/openrac/keys.h,
// "F12", "KeyP", "Enter") with "Ctrl+", "Shift+" or "Alt+" before it when that is held too:
// "F11,Alt+Enter". "" binds nothing. A name no table has is read as an SDL scancode name, as the
// settings file's texture_pack_key wrote them ("Return").

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

namespace openrac::platform {

// One key and the modifiers held with it.
struct ShortcutKey {
    SDL_Scancode key = SDL_SCANCODE_UNKNOWN;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    bool operator==(const ShortcutKey&) const = default;
};

// The keys `text` names, those it names well (an unknown name is left out).
std::vector<ShortcutKey> parse_shortcut(std::string_view text);

// `keys` as the controls file writes them.
std::string shortcut_text(const std::vector<ShortcutKey>& keys);

// One key as the controls file writes it ("Ctrl+F3").
std::string shortcut_key_text(const ShortcutKey& key);

// What a player reads: "Ctrl+F3 or F12", with the keys as the keyboard prints them; "None" for
// no key.
std::string shortcut_label(const std::vector<ShortcutKey>& keys);

// The key a key press is, with the modifiers held: what a press during a capture binds.
ShortcutKey shortcut_of(const SDL_KeyboardEvent& key);

// Whether `key` is one of `keys`, its modifiers held exactly (Ctrl+F3 is not F3).
bool shortcut_hit(const std::vector<ShortcutKey>& keys, const SDL_KeyboardEvent& key);

// Whether a key is a modifier on its own: a capture waits for the key it goes with.
bool shortcut_modifier(SDL_Scancode key);

}  // namespace openrac::platform
