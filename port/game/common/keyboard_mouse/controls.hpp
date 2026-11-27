// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The keyboard and mouse's Controls page in the game's own pause menu (game/<game>/keyboard_mouse):
// its parts with no game in them, for the tests. The presets as the manifest has them (a test checks
// they agree), the actions on each page of keys with the names the page shows, a bind's text, a key
// given to an action (taken from any other action of its group, so that none is bound twice), the
// first input pressed in a tick, and the sensitivity's steps.

#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "game/common/key_labels.h"
#include "game/common/keyboard_mouse/kbm.hpp"
#include "openrac/keys.h"

namespace kbm::controls {

// ---- The presets ----

// One setting a preset sets: its key and its value's text.
struct PresetValue {
    const char* key = nullptr;
    const char* value = nullptr;
};

// The most settings one preset names.
inline constexpr std::size_t kPresetValuesMax = 16;

// A preset of the manifest ("presets"): its id, its name, and the keys it sets (the others go back
// to their defaults, as the launcher applies it). "default" is the defaults themselves.
struct Preset {
    const char* id;
    const char* name;
    std::array<PresetValue, kPresetValuesMax> values;
};

// In the manifest's order, after Default.
inline const std::array<Preset, 4> kPresets = {{
    {"default", "Default", {}},
    {"left_handed",
     "Left-handed",
     {{{"move_forward", "KeyI"},
       {"move_back", "KeyK"},
       {"move_left", "KeyJ"},
       {"move_right", "KeyL"},
       {"walk", "AltRight"},
       {"wrench", "KeyH,MouseMiddle"},
       {"crouch", "ControlRight,KeyN"},
       {"strafe", "ShiftRight"},
       {"first_person", "KeyU,MouseBack"},
       {"r2", "KeyY,MouseForward"},
       {"map", "KeyM,Backslash"},
       {"camera_reset", "KeyO"},
       {"pause", "Escape,KeyP"}}}},
    {"arrows",
     "Arrow keys",
     {{{"move_forward", "ArrowUp"},
       {"move_back", "ArrowDown"},
       {"move_left", "ArrowLeft"},
       {"move_right", "ArrowRight"},
       {"walk", "AltRight"},
       {"crouch", "ControlRight,KeyC"},
       {"strafe", "ShiftRight"}}}},
    {"modern",
     "Modern",
     {{{"aim", "MouseRight"},
       {"quick_select", "MouseMiddle"},
       {"wrench", "KeyF"},
       {"strafe_fire", "firing"},
       {"dodge", "ShiftLeft"},
       {"strafe", ""},
       {"weapon_last", "KeyX"}}}},
}};

// The settings besides the keys a preset names, with their defaults: a preset that does not name
// one puts it back to its default, as the launcher applies presets.
inline constexpr std::array<PresetValue, 1> kPresetSettings = {{{"strafe_fire", "off"}}};

// The text preset `p` gives the setting `key` of kPresetSettings: its own value, else the default.
inline std::string_view preset_setting(const Preset& p, std::string_view key) {
    for (const PresetValue& v : p.values) {
        if (v.key != nullptr && v.key == key) {
            return v.value;
        }
    }

    for (const PresetValue& d : kPresetSettings) {
        if (d.key == key) {
            return d.value;
        }
    }

    return "";
}

// The text preset `p` gives action `a`: its own value, else the action's default.
inline std::string_view preset_text(const Preset& p, Action a) {
    const ActionInfo& info = kActions[static_cast<std::size_t>(a)];

    for (const PresetValue& v : p.values) {
        if (v.key != nullptr && std::string_view(v.key) == info.key) {
            return v.value;
        }
    }

    return info.fallback;
}

// The preset every action's binds are as, and each setting of kPresetSettings (its text from
// `setting(key)`), or -1 for none (the player's own).
template <class Setting>
int preset_matching(const Bindings& b, Setting&& setting) {
    for (std::size_t p = 0; p < kPresets.size(); ++p) {
        bool all = true;

        for (int a = 0; a < kActionCount && all; ++a) {
            Binds want;
            all = parse_binds(preset_text(kPresets[p], static_cast<Action>(a)), want)
                  && want == b.of(static_cast<Action>(a));
        }

        for (const PresetValue& s : kPresetSettings) {
            all = all && setting(std::string_view(s.key)) == preset_setting(kPresets[p], s.key);
        }

        if (all) {
            return static_cast<int>(p);
        }
    }

    return -1;
}

// The settings the page shows besides the keys, with their defaults as the manifest has them (a test
// checks they agree), for the page's reset.
inline constexpr std::array<PresetValue, 11> kPageDefaults = {{
    {"prompt_device", "auto"},
    {"mouse_with_gamepad", "true"},
    {"mouse_sensitivity", "1.0"},
    {"mouse_sensitivity_y", "1.0"},
    {"invert_y", "false"},
    {"camera_tilt", "hold"},
    {"key_prompts", "true"},
    {"aim_toggle", "false"},
    {"aim_sensitivity", "1.0"},
    {"aim_vignette", "true"},
    {"strafe_fire", "off"},
}};

// ---- The pages of keys ----

// The most actions one page of keys lists (the page's list holds twelve).
inline constexpr std::size_t kPageActionsMax = 12;

// A page of keys: its title and its actions, in order.
struct KeyPage {
    const char* title;
    std::array<Action, kPageActionsMax> actions;
    std::size_t count;
};

// Every action on one page, each once (a test checks it).
inline constexpr std::array<KeyPage, 4> kKeyPages = {{
    {"Moving",
     {Action::MoveForward,
      Action::MoveBack,
      Action::MoveLeft,
      Action::MoveRight,
      Action::Walk,
      Action::Jump,
      Action::Crouch,
      Action::Strafe,
      Action::Dodge,
      Action::R2},
     10},
    {"Actions",
     {Action::Fire,
      Action::Aim,
      Action::Wrench,
      Action::QuickSelect,
      Action::FirstPerson,
      Action::CameraReset,
      Action::Map,
      Action::Pause},
     8},
    {"Weapons",
     {Action::Weapon1,
      Action::Weapon2,
      Action::Weapon3,
      Action::Weapon4,
      Action::Weapon5,
      Action::Weapon6,
      Action::Weapon7,
      Action::Weapon8,
      Action::WeaponNext,
      Action::WeaponPrev,
      Action::WeaponLast},
     11},
    {"Menus",
     {Action::MenuConfirm,
      Action::MenuBack,
      Action::MenuUp,
      Action::MenuDown,
      Action::MenuLeft,
      Action::MenuRight,
      Action::MenuCircle},
     7},
}};

// An action's name on a page of keys, short enough to leave room for its keys.
inline const char* action_name(Action a) {
    static constexpr std::array<const char*, kActionCount> kNames = {{
        "Move forward", "Move back",    "Move left",    "Move right",  "Walk",
        "Jump",         "Fire",         "Quick Select", "Wrench",      "Crouch",
        "Strafe",       "First person", "Aim",          "Dodge",       "R2",
        "Map",          "Camera reset", "Pause",        "Slot 1",      "Slot 2",
        "Slot 3",       "Slot 4",       "Slot 5",       "Slot 6",      "Slot 7",
        "Slot 8",       "Next weapon",  "Prev. weapon", "Last weapon", "Confirm",
        "Back",         "Up",           "Down",         "Left",        "Right",
        "Circle",
    }};
    return kNames[static_cast<std::size_t>(a)];
}

// ---- Binds ----

// The binds as a setting holds them: "KeyF,MouseMiddle", "" for none.
inline std::string binds_text(const Binds& b) {
    std::string out;

    for (int bind : b) {
        if (bind != OPENRAC_BIND_NONE) {
            out += out.empty() ? "" : ",";
            out += openrac_bind_to_name(bind);
        }
    }

    return out;
}

// The binds as the page shows them: "F, Middle click", "none".
inline std::string binds_label(const Binds& b) {
    std::string out;

    for (int bind : b) {
        if (bind != OPENRAC_BIND_NONE) {
            out += out.empty() ? "" : ", ";
            out += openrac::keys::menu_label(openrac_bind_to_name(bind));
        }
    }

    return out.empty() ? "none" : out;
}

// Gives action `a` the one bind `bind`, and takes it from every other action of its group (two of a
// group may not share one; of different groups they may). Returns the actions whose binds changed,
// `a` first.
inline std::vector<Action> give(Bindings& b, Action a, int bind) {
    std::vector<Action> changed;
    const Group group = kActions[static_cast<std::size_t>(a)].group;
    Binds& own = b.binds[static_cast<std::size_t>(a)];
    const Binds wanted = {bind, OPENRAC_BIND_NONE};

    if (own != wanted) {
        own = wanted;
        changed.push_back(a);
    }

    for (int i = 0; i < kActionCount; ++i) {
        if (i == static_cast<int>(a) || kActions[static_cast<std::size_t>(i)].group != group) {
            continue;
        }

        Binds& other = b.binds[static_cast<std::size_t>(i)];

        if (other[0] != bind && other[1] != bind) {
            continue;
        }

        // The other bind, if any, moves to the first place.
        other = {other[0] == bind ? other[1] : other[0], OPENRAC_BIND_NONE};
        changed.push_back(static_cast<Action>(i));
    }

    return changed;
}

// ---- The input a key's capture takes ----

// The Escape key's HID usage: it ends a capture rather than being taken.
inline constexpr int kEscape = 0x29;

// Whether a key or a mouse button is held: a capture waits for none before it takes one.
inline bool any_held(const openrac_input& in) {
    for (int k = 1; k < OPENRAC_KEY_COUNT; ++k) {
        if ((in.keys[k] & OPENRAC_KEY_HELD) != 0) {
            return true;
        }
    }

    return in.mouse_held != 0;
}

// The first input that went down in the tick: a key (Escape left out), a mouse button, a notch of
// the wheel; OPENRAC_BIND_NONE for none.
inline int pressed_bind(const openrac_input& in) {
    for (int k = 1; k < OPENRAC_KEY_COUNT; ++k) {
        if (k != kEscape && (in.keys[k] & OPENRAC_KEY_PRESSED) != 0) {
            return k;
        }
    }

    static constexpr std::array<std::pair<std::uint32_t, int>, 5> kButtons = {{
        {OPENRAC_MOUSE_LEFT, OPENRAC_BIND_MOUSE_LEFT},
        {OPENRAC_MOUSE_RIGHT, OPENRAC_BIND_MOUSE_RIGHT},
        {OPENRAC_MOUSE_MIDDLE, OPENRAC_BIND_MOUSE_MIDDLE},
        {OPENRAC_MOUSE_BACK, OPENRAC_BIND_MOUSE_BACK},
        {OPENRAC_MOUSE_FORWARD, OPENRAC_BIND_MOUSE_FORWARD},
    }};

    for (const auto& [bit, bind] : kButtons) {
        if ((in.mouse_pressed & bit) != 0) {
            return bind;
        }
    }

    if (in.wheel_y >= 0.5f) {
        return OPENRAC_BIND_WHEEL_UP;
    }

    if (in.wheel_y <= -0.5f) {
        return OPENRAC_BIND_WHEEL_DOWN;
    }

    return OPENRAC_BIND_NONE;
}

// Whether Escape went down in the tick.
inline bool escape_pressed(const openrac_input& in) {
    return (in.keys[kEscape] & OPENRAC_KEY_PRESSED) != 0;
}

// ---- The sensitivity's steps ----

// The page's steps of a mouse sensitivity: 0.1 to 3 by tenths (the launcher takes more).
inline constexpr int kSensitivitySteps = 30;

inline std::vector<std::string> sensitivity_choices() {
    std::vector<std::string> out;

    for (int i = 1; i <= kSensitivitySteps; ++i) {
        out.push_back(std::to_string(i / 10) + "." + std::to_string(i % 10));
    }

    return out;
}

// The step nearest to `value` (one past the last shows as the last).
inline int sensitivity_step(float value) {
    const int i = static_cast<int>(std::lround(value * 10.0f)) - 1;
    return i < 0 ? 0 : i >= kSensitivitySteps ? kSensitivitySteps - 1 : i;
}

inline float sensitivity_of(int step) {
    return static_cast<float>(step + 1) / 10.0f;
}

// A setting's steps on the page, in hundredths: from `lo` to `hi` by `by`, shown with two places.
struct Steps {
    int lo;
    int hi;
    int by;
};

inline constexpr Steps kAimSensitivitySteps{10, 200, 5};

inline std::vector<std::string> step_choices(const Steps& s) {
    std::vector<std::string> out;

    for (int v = s.lo; v <= s.hi; v += s.by) {
        const int fraction = v % 100;
        out.push_back(
            std::to_string(v / 100) + "." + (fraction < 10 ? "0" : "") + std::to_string(fraction)
        );
    }

    return out;
}

// The step nearest to `value`, kept within the steps.
inline int step_of(const Steps& s, float value) {
    const int count = (s.hi - s.lo) / s.by + 1;
    const float steps = (value * 100.0f - static_cast<float>(s.lo)) / static_cast<float>(s.by);
    const int i = static_cast<int>(std::lround(steps));
    return i < 0 ? 0 : i >= count ? count - 1 : i;
}

inline float value_of(const Steps& s, int step) {
    return static_cast<float>(s.lo + step * s.by) / 100.0f;
}

}  // namespace kbm::controls
