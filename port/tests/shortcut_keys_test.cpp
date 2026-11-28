// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A shortcut's keys as the controls file writes them (platform/shortcut_keys.h): read, written
// back, and matched against key presses with their modifiers.
#include <string>
#include <vector>

#include "platform/shortcut_keys.h"
#include "tests/check.h"

namespace {

using openrac::platform::parse_shortcut;
using openrac::platform::shortcut_hit;
using openrac::platform::shortcut_text;
using openrac::platform::ShortcutKey;

SDL_KeyboardEvent press(SDL_Scancode key, SDL_Keymod mod = SDL_KMOD_NONE) {
    SDL_KeyboardEvent e{};
    e.scancode = key;
    e.mod = mod;
    e.down = true;
    return e;
}

void test_read_and_write() {
    const std::vector<ShortcutKey> keys = parse_shortcut("F11,Alt+Enter");
    CHECK(keys.size() == 2);
    CHECK(keys[0].key == SDL_SCANCODE_F11 && !keys[0].alt);
    CHECK(keys[1].key == SDL_SCANCODE_RETURN && keys[1].alt && !keys[1].ctrl);
    CHECK(shortcut_text(keys) == "F11,Alt+Enter");
    CHECK(shortcut_text(parse_shortcut("Ctrl+Shift+KeyP")) == "Ctrl+Shift+KeyP");
    // Nothing bound, and names no table has.
    CHECK(parse_shortcut("").empty());
    CHECK(parse_shortcut("NotAKey").empty());
    CHECK(parse_shortcut(" F12 , NotAKey ").size() == 1);
    // The settings file's older texture pack key: SDL's names.
    const std::vector<ShortcutKey> old = parse_shortcut("Ctrl+Return");
    CHECK(old.size() == 1 && old[0].key == SDL_SCANCODE_RETURN && old[0].ctrl);
}

void test_hit() {
    const std::vector<ShortcutKey> record = parse_shortcut("Ctrl+F3");
    const std::vector<ShortcutKey> stats = parse_shortcut("F3");
    CHECK(shortcut_hit(record, press(SDL_SCANCODE_F3, SDL_KMOD_LCTRL)));
    CHECK(!shortcut_hit(record, press(SDL_SCANCODE_F3)));
    CHECK(shortcut_hit(stats, press(SDL_SCANCODE_F3)));
    CHECK(!shortcut_hit(stats, press(SDL_SCANCODE_F3, SDL_KMOD_RCTRL)));
    CHECK(!shortcut_hit({}, press(SDL_SCANCODE_F3)));
}

void test_label() {
    CHECK(openrac::platform::shortcut_label({}) == "None");
    CHECK(openrac::platform::shortcut_label(parse_shortcut("Ctrl+F3,F12")) == "Ctrl+F3 or F12");
}

}  // namespace

int main() {
    test_read_and_write();
    test_hit();
    test_label();
    return openrac::test::result();
}
