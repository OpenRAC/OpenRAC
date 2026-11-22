// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The pad's buttons as the player's device has them (mod interface 1.31: openrac_api.button_label
// and the button tokens of the overlay's text, OPENRAC_UI_CROSS ... OPENRAC_UI_RIGHT_STICK): the
// game's icon or name on a PlayStation pad (or none connected), the letter on a lettered pad, the
// key the player bound on the keyboard and mouse. The window sets the device (frontend/input.cpp);
// the built-in keyboard and mouse mods set the keys they paint over the game's own icons
// (key_prompts.h), so a label is what the game's prompts show at the same time.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace openrac::button_labels {

// What kind of pad is connected (the first one), for its buttons' names.
enum class Pad {
    PlayStation,
    Xbox,
    Nintendo
};

// The player's device now.
struct Device {
    bool keys = false;  // on the keyboard and mouse
    Pad pad = Pad::PlayStation;
};

// The game's icon cells, in key_prompts.h's Cell order (Cross, Square, Triangle, Circle, L1, L2, R1,
// R2, the left stick, the right stick): what the keyboard and mouse paint over each, a key's name
// ("KeyF", "MouseLeft") or a label ("WASD", "Mouse"), empty for the game's own icon.
inline constexpr std::size_t kCells = 10;
using KeyNames = std::array<std::string, kCells>;

// From the window's thread and the game's; both are read by any thread.
void set_device(const Device& device);
void set_key_names(const KeyNames& names);
Device device();

// The label of `button` (one OPENRAC_PAD_* bit) for `device` and `keys`, and its kind in `kind`
// (OPENRAC_BUTTONS_*); empty with `kind` OPENRAC_HOOK_UNKNOWN for no single button.
std::string label(std::uint32_t button, const Device& device, const KeyNames& keys, int& kind);

// openrac_api.button_label with the device and keys now: the kind, or OPENRAC_HOOK_UNKNOWN.
int label_into(std::uint32_t button, char* out, std::uint32_t size);

// `text` with each button token (0x10 to 0x19) as the overlay's font shows it for `device`: a
// PlayStation pad's face buttons stay tokens (the font has their icons), the rest become their
// label, a key's in brackets ("[F]").
std::string overlay_text(std::string_view text, const Device& device, const KeyNames& keys);

// The same with the device and keys now.
std::string overlay_text(std::string_view text);

// Whether `text` has a button token.
bool has_token(std::string_view text);

}  // namespace openrac::button_labels
