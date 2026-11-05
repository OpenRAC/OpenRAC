// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The on-screen pad's actions (touch_pad.h, the Arc style): what each button does in a situation of
// the game, described as data. A game says which situation it is in by a name ("foot", "clank",
// "ship", "cutscene", "menu"; game_host.h, openrac_game_touch), and the sets read here say which
// buttons that situation shows: each an icon, the pad's buttons it holds down and how large it is.
// The pad only presses buttons, as a DualShock 2 would; what they do is the game's.
//
// The sets are JSON (port/game/common/touch/sets/*.json, a mod's "touch" file, docs/port/MODS.md):
//
//   { "contexts": {
//       "foot": { "extends": "base", "actions": [
//         { "id": "jump", "icon": "jump", "press": ["cross"], "size": "large" },
//         { "id": "dash", "icon": "dash", "press": ["r1"], "then": ["cross"], "size": "small" },
//         { "id": "swing", "icon": "swing", "press": ["circle"], "when": "swing_target" } ] } } }
//
// A context's actions come in the order the player reaches for them most, first the largest; the
// pad places them on its arc in that order. "extends" takes another context's actions first (its own
// replace those of the same id); "side" puts one beside the stick ("left") or along the top
// ("top"); "when" shows one, in the hint places unless its "side" says otherwise, only while the game (or a mod) says the hint is up; "then" holds more
// buttons a moment after "press" (a combination the game reads as two presses, such as crouch then
// jump); "toggle" makes a tap hold it until the next tap; "wide" makes it a pill for a wide icon;
// "stick": true makes a finger held on it push the left stick the way it slides (a quick select
// ring); "instead" names the action a "when" one takes the place of while its hint is up (a "press
// triangle" prompt's button where the quick select was). A context with "dpad": true shows the d-pad in the stick's place (menus). A set read later
// adds to one read before: a context of the same name gets the later actions, replacing those of
// the same id (a mod adds a button to the game's "foot"), unless it says "replace": true.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace openrac::platform {

enum class TouchSize : std::uint8_t {
    Large,
    Medium,
    Small
};

enum class TouchSide : std::uint8_t {
    Arc,   // around the right thumb
    Left,  // above the stick, for the left thumb
    Top,   // along the top edge, beside the options button
    Hint   // where the eye is, for a "when" one (its default)
};

struct TouchAction {
    std::string id;
    std::string icon;
    std::uint16_t press = 0;  // the pad's buttons (pad.h's Button bits) held while it is
    std::uint16_t then = 0;   // added kThenDelayMs after the press
    TouchSize size = TouchSize::Medium;
    TouchSide side = TouchSide::Arc;
    std::string when;     // a hint that must be up for it to show; empty: always
    std::string instead;  // with `when`: the action whose place (and size) it takes meanwhile
    bool toggle = false;
    bool wide = false;   // a pill wider than high, for a wide icon
    bool stick = false;  // while held, the finger's slide pushes the left stick

    bool operator==(const TouchAction&) const = default;
};

class TouchActionSets {
public:
    // Reads a set (the JSON above) over those read before. False with `error` set when the text is
    // not such a set; the sets read before stay as they were.
    bool read(std::string_view json, std::string_view source, std::string& error);

    // The actions of `context`, its "extends" chain first; empty for an unknown name.
    std::vector<TouchAction> actions(std::string_view context) const;

    // Whether `context` (or one it extends) shows the d-pad in the stick's place.
    bool dpad(std::string_view context) const;

    bool has(std::string_view context) const { return m_contexts.contains(context); }

    void clear() { m_contexts.clear(); }

private:
    struct Context {
        std::string extends;
        std::vector<TouchAction> actions;
        bool dpad = false;
    };

    // `context` and the contexts it extends, nearest first.
    std::vector<const Context*> chain(std::string_view context) const;

    std::map<std::string, Context, std::less<>> m_contexts;
};

// The pad's buttons by name ("cross", "r1", "up", "start"...), 0 for an unknown name.
std::uint16_t touch_button_bits(std::string_view name);

}  // namespace openrac::platform
