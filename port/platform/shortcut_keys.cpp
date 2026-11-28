// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/shortcut_keys.h"

#include "openrac/keys.h"

namespace openrac::platform {

namespace {

constexpr std::string_view kCtrl = "Ctrl+";
constexpr std::string_view kShift = "Shift+";
constexpr std::string_view kAlt = "Alt+";

std::string_view trimmed(std::string_view s) {
    while (!s.empty() && s.front() == ' ') {
        s.remove_prefix(1);
    }
    while (!s.empty() && s.back() == ' ') {
        s.remove_suffix(1);
    }
    return s;
}

// One key's text ("Ctrl+F3"), or a key of SDL_SCANCODE_UNKNOWN.
ShortcutKey parse_one(std::string_view text) {
    ShortcutKey k;
    for (bool more = true; more;) {
        more = false;
        if (text.starts_with(kCtrl)) {
            k.ctrl = more = true;
            text.remove_prefix(kCtrl.size());
        } else if (text.starts_with(kShift)) {
            k.shift = more = true;
            text.remove_prefix(kShift.size());
        } else if (text.starts_with(kAlt)) {
            k.alt = more = true;
            text.remove_prefix(kAlt.size());
        }
    }
    const int bind = openrac_bind_from_name_n(text.data(), text.size());
    if (bind > OPENRAC_BIND_NONE && bind < OPENRAC_KEY_COUNT) {
        k.key = static_cast<SDL_Scancode>(bind);
    } else if (!text.empty()) {
        k.key = SDL_GetScancodeFromName(std::string(text).c_str());
    }
    return k;
}

}  // namespace

std::vector<ShortcutKey> parse_shortcut(std::string_view text) {
    std::vector<ShortcutKey> keys;
    while (!text.empty()) {
        const std::size_t comma = text.find(',');
        const ShortcutKey k = parse_one(trimmed(text.substr(0, comma)));
        if (k.key != SDL_SCANCODE_UNKNOWN) {
            keys.push_back(k);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        text.remove_prefix(comma + 1);
    }
    return keys;
}

std::string shortcut_key_text(const ShortcutKey& key) {
    std::string out;
    out += key.ctrl ? kCtrl : "";
    out += key.shift ? kShift : "";
    out += key.alt ? kAlt : "";
    const char* name = openrac_bind_to_name(static_cast<int>(key.key));
    out += name != nullptr ? name : SDL_GetScancodeName(key.key);
    return out;
}

std::string shortcut_text(const std::vector<ShortcutKey>& keys) {
    std::string out;
    for (const ShortcutKey& k : keys) {
        out += (out.empty() ? "" : ",") + shortcut_key_text(k);
    }
    return out;
}

std::string shortcut_label(const std::vector<ShortcutKey>& keys) {
    if (keys.empty()) {
        return "None";
    }
    std::string out;
    for (const ShortcutKey& k : keys) {
        std::string one;
        one += k.ctrl ? kCtrl : "";
        one += k.shift ? kShift : "";
        one += k.alt ? kAlt : "";
        const char* shown = SDL_GetKeyName(SDL_GetKeyFromScancode(k.key, SDL_KMOD_NONE, false));
        one += shown != nullptr && *shown != '\0' ? std::string(shown) : shortcut_key_text(k);
        out += (out.empty() ? "" : " or ") + one;
    }
    return out;
}

ShortcutKey shortcut_of(const SDL_KeyboardEvent& key) {
    return {
        .key = key.scancode,
        .ctrl = (key.mod & SDL_KMOD_CTRL) != 0,
        .shift = (key.mod & SDL_KMOD_SHIFT) != 0,
        .alt = (key.mod & SDL_KMOD_ALT) != 0,
    };
}

bool shortcut_hit(const std::vector<ShortcutKey>& keys, const SDL_KeyboardEvent& key) {
    const ShortcutKey pressed = shortcut_of(key);
    for (const ShortcutKey& k : keys) {
        if (k == pressed) {
            return true;
        }
    }
    return false;
}

bool shortcut_modifier(SDL_Scancode key) {
    return key == SDL_SCANCODE_LCTRL || key == SDL_SCANCODE_RCTRL || key == SDL_SCANCODE_LSHIFT
           || key == SDL_SCANCODE_RSHIFT || key == SDL_SCANCODE_LALT || key == SDL_SCANCODE_RALT
           || key == SDL_SCANCODE_LGUI || key == SDL_SCANCODE_RGUI;
}

}  // namespace openrac::platform
