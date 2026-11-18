// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A key's name (openrac/keys.h: "KeyW", "MouseLeft") as a player reads it: in a menu ("W", "Left
// click") and on a button icon of the game's, at most 5 letters ("W", "LMB").

#pragma once

#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

namespace openrac::keys {

namespace detail {

struct Words {
    std::string_view name, menu, icon;
};

inline constexpr std::array<Words, 30> kWords{{
    {"Space", "Space", "SPACE"},
    {"Enter", "Enter", "ENTER"},
    {"Escape", "Esc", "ESC"},
    {"Backspace", "Backspace", "BKSP"},
    {"Tab", "Tab", "TAB"},
    {"CapsLock", "Caps Lock", "CAPS"},
    {"ShiftLeft", "Left Shift", "SHIFT"},
    {"ShiftRight", "Right Shift", "SHIFT"},
    {"ControlLeft", "Left Ctrl", "CTRL"},
    {"ControlRight", "Right Ctrl", "CTRL"},
    {"AltLeft", "Left Alt", "ALT"},
    {"AltRight", "Right Alt", "ALT"},
    {"MetaLeft", "Left Win", "WIN"},
    {"MetaRight", "Right Win", "WIN"},
    {"ArrowUp", "Up", "UP"},
    {"ArrowDown", "Down", "DOWN"},
    {"ArrowLeft", "Left", "LEFT"},
    {"ArrowRight", "Right", "RIGHT"},
    {"MouseLeft", "Left click", "LMB"},
    {"MouseRight", "Right click", "RMB"},
    {"MouseMiddle", "Middle click", "MMB"},
    {"MouseBack", "Mouse back", "M4"},
    {"MouseForward", "Mouse forward", "M5"},
    {"WheelUp", "Wheel up", "WHUP"},
    {"WheelDown", "Wheel down", "WHDN"},
    {"WheelLeft", "Wheel left", "WHLT"},
    {"WheelRight", "Wheel right", "WHRT"},
    {"Delete", "Delete", "DEL"},
    {"Insert", "Insert", "INS"},
    {"PageUp", "Page Up", "PGUP"},
}};

inline std::string upper(std::string_view s) {
    std::string out(s);
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

}  // namespace detail

// "W", "1", "Left click", "Num 4", "F5".
inline std::string menu_label(std::string_view name) {
    for (const detail::Words& w : detail::kWords) {
        if (w.name == name) {
            return std::string(w.menu);
        }
    }
    if (name.size() == 4 && name.starts_with("Key")) {
        return std::string(name.substr(3));
    }
    if (name.size() == 6 && name.starts_with("Digit")) {
        return std::string(name.substr(5));
    }
    if (name.starts_with("Numpad")) {
        return "Num " + std::string(name.substr(6));
    }
    return std::string(name);
}

// At most 5 capitals: "W", "1", "LMB", "SPACE", "NUM4", "F5".
inline std::string icon_label(std::string_view name) {
    for (const detail::Words& w : detail::kWords) {
        if (w.name == name) {
            return std::string(w.icon);
        }
    }
    std::string out = menu_label(name);
    if (name.starts_with("Numpad")) {
        out = "NUM" + std::string(name.substr(6));
    }
    out = detail::upper(out);
    std::erase(out, ' ');
    return out.substr(0, 5);
}

// The movement keys (forward, left, back, right) on one key icon: their four letters ("WASD") when
// each is one letter, else "MOVE".
inline std::string move_label(const std::array<std::string_view, 4>& names) {
    std::string out;
    for (std::string_view n : names) {
        const std::string one = icon_label(n);
        if (one.size() != 1) {
            return "MOVE";
        }
        out += one;
    }
    return out;
}

}  // namespace openrac::keys
