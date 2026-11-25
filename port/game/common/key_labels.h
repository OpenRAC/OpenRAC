// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A key's name (openrac/keys.h: "KeyW", "MouseLeft") as a player reads it: in a menu ("W", "Left
// click") and on a button icon of the game's: one or two lines ("\n" between them) of at most 5
// capitals, digits or signs ("W", "LMB", "NUM\n4"). Names are by place, as on a US keyboard, but
// for the letters and signs, which are what the player's layout prints there once the window says
// so (set_layout): on an AZERTY keyboard "KeyW" reads Z.

#pragma once

#include <array>
#include <atomic>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

#include "openrac/keys.h"

namespace openrac::keys {

namespace detail {

struct Words {
    std::string_view name, menu, icon;
};

inline constexpr std::array<Words, 58> kWords{{
    {"Space", "Space", "SPACE"},
    {"Enter", "Enter", "ENTER"},
    {"Escape", "Esc", "ESC"},
    {"Backspace", "Backspace", "BACK\nSPACE"},
    {"Tab", "Tab", "TAB"},
    {"CapsLock", "Caps Lock", "CAPS\nLOCK"},
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
    {"Home", "Home", "HOME"},
    {"End", "End", "END"},
    {"PageUp", "Page Up", "PG\nUP"},
    {"PageDown", "Page Down", "PG\nDN"},
    {"PrintScreen", "Print Screen", "PRT\nSCR"},
    {"ScrollLock", "Scroll Lock", "SCR\nLOCK"},
    {"Pause", "Pause", "PAUSE"},
    {"NumLock", "Num Lock", "NUM\nLOCK"},
    {"ContextMenu", "Menu", "MENU"},
    {"Minus", "-", "-"},
    {"Equal", "=", "="},
    {"BracketLeft", "[", "["},
    {"BracketRight", "]", "]"},
    {"Backslash", "\\", "\\"},
    {"IntlBackslash", "\\ (ISO)", "\\"},
    {"IntlHash", "#", "#"},
    {"Semicolon", ";", ";"},
    {"Quote", "'", "'"},
    {"Backquote", "`", "`"},
    {"Comma", ",", ","},
    {"Period", ".", "."},
    {"Slash", "/", "/"},
    {"NumpadDivide", "Num /", "NUM\n/"},
    {"NumpadMultiply", "Num *", "NUM\n*"},
    {"NumpadSubtract", "Num -", "NUM\n-"},
    {"NumpadAdd", "Num +", "NUM\n+"},
    {"NumpadEnter", "Num Enter", "NUM\nENTER"},
    {"NumpadDecimal", "Num .", "NUM\n."},
    {"NumpadEqual", "Num =", "NUM\n="},
}};

inline std::string upper(std::string_view s) {
    std::string out(s);
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

// The character a key prints on the player's layout, by HID usage, 0 for none
// (platform/key_layout.h, which the window sets as it opens); null without a window.
using LayoutChar = char (*)(int usage);
inline std::atomic<LayoutChar> g_layout{nullptr};

// The keys named by what the layout prints: the letters and the signs. Not the digits: some layouts
// print a sign there unshifted (AZERTY's &), and the number keys are known by their numbers.
inline bool named_by_layout(std::string_view name) {
    static constexpr std::array<std::string_view, 13> kSigns = {
        "Minus",
        "Equal",
        "BracketLeft",
        "BracketRight",
        "Backslash",
        "IntlBackslash",
        "IntlHash",
        "Semicolon",
        "Quote",
        "Backquote",
        "Comma",
        "Period",
        "Slash",
    };

    if (name.size() == 4 && name.starts_with("Key")) {
        return true;
    }

    for (std::string_view s : kSigns) {
        if (s == name) {
            return true;
        }
    }

    return false;
}

// What the layout prints on the key named `name`, as a one-character string; "" for none.
inline std::string layout_label(std::string_view name) {
    const LayoutChar layout = g_layout.load(std::memory_order_relaxed);

    if (layout == nullptr || !named_by_layout(name)) {
        return "";
    }

    const int usage = openrac_bind_from_name_n(name.data(), name.size());
    const char c = usage > 0 && usage < OPENRAC_KEY_COUNT ? layout(usage) : '\0';
    return c != '\0' ? std::string(1, c) : "";
}

}  // namespace detail

// Names the letters and signs by what the player's layout prints (null: by place, as on a US
// keyboard). The window sets it as it opens (game/common/frontend.cpp).
inline void set_layout(char (*layout)(int usage)) {
    detail::g_layout.store(layout, std::memory_order_relaxed);
}

// "W", "1", "Left click", "Num 4", "F5"; on an AZERTY keyboard "KeyW" is "Z".
inline std::string menu_label(std::string_view name) {
    if (std::string own = detail::layout_label(name); !own.empty()) {
        return own;
    }

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

// One or two lines of at most 5 capitals: "W", "1", "LMB", "SPACE", "NUM\n4", "F5".
inline std::string icon_label(std::string_view name) {
    if (std::string own = detail::layout_label(name); !own.empty()) {
        return own;
    }

    for (const detail::Words& w : detail::kWords) {
        if (w.name == name) {
            return std::string(w.icon);
        }
    }
    if (name.starts_with("Numpad")) {
        return "NUM\n" + detail::upper(name.substr(6)).substr(0, 5);
    }
    std::string out = detail::upper(menu_label(name));
    std::erase(out, ' ');
    return out.substr(0, 5);
}

// The same on a narrow key (a round button's place), whose lines hold at most 4: signs for the keys
// with longer names ("\x01" return, "\x02" shift, "\x03" backspace, "\x04" space, as
// game/common/key_prompts.cpp draws them), else each line cut to 4.
inline std::string narrow_icon_label(std::string_view name) {
    if (name == "Enter") {
        return "\x01";
    }
    if (name == "ShiftLeft" || name == "ShiftRight") {
        return "\x02";
    }
    if (name == "Backspace") {
        return "\x03";
    }
    if (name == "Space") {
        return "\x04";
    }
    if (name == "NumpadEnter") {
        return "NUM\nENT";
    }
    std::string out;
    std::size_t from = 0;
    const std::string full = icon_label(name);
    while (from <= full.size()) {
        const std::size_t end = std::min(full.find('\n', from), full.size());
        out += full.substr(from, std::min<std::size_t>(end - from, 4));
        if (end < full.size()) {
            out += '\n';
        }
        from = end + 1;
    }
    return out;
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
