// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A multiplayer UI description: which screens a lobby shows, their tables' columns, their items,
// the modes, maps, options and text a server offers. A server sends one as JSON (schema 1); the
// client validates it whole and keeps the game's own layout (the vanilla description its binding
// gives) when it is missing or invalid. The schema is in docs/port/MULTIPLAYER_UI.md.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/result.h"

namespace openrac::mpui {

// The schema version this client reads; a description with another one is refused.
constexpr int SCHEMA_VERSION = 1;

// The largest description accepted, in bytes of JSON.
constexpr std::size_t MAX_DESCRIPTION_BYTES = 32'768;

// What a component does when chosen: a closed set, so that a server describes and never programs.
enum class ActionKind : std::uint8_t {
    NONE,    // Nothing; the component is shown only.
    BACK,    // Leaves the screen, as Triangle does.
    OPEN,    // Opens the native screen `target`.
    GAME,    // Runs the game binding's action `target` (join, ready, start, leave, team, create).
    SERVER,  // Sends the server the action `target` with the component's value.
};

// An action and its target, parsed from text such as `open:rules` or `server:vote_map`.
struct Action {
    ActionKind kind = ActionKind::NONE;  // What to do.
    std::string target;                  // The screen, game action or server key; empty for Back.
};

// One column of a table: the row field it shows, its heading and its share of the table's width.
struct Column {
    std::string field;  // The row cell it shows (`name`, `players`, `map`, ...).
    std::string label;  // Its heading; empty for none.
    int width = 0;      // Percent of the table's width; the columns' widths sum to 100 or less.
};

// What one item of a screen is.
enum class ItemKind : std::uint8_t {
    TEXT,    // A line of text the cursor passes over.
    BUTTON,  // Cross runs its action.
    CHOICE,  // Left and Right step its value among its choices; Cross steps on.
    TABLE,   // The screen's table (one a screen), at this place in its column.
};

// One item of a screen, in the order the screen stacks them.
struct Item {
    ItemKind kind = ItemKind::TEXT;    // What it is.
    std::string label;                 // Its text, or the choice's name.
    Action action;                     // Button: its action; Table: what Cross on a row does.
    std::string key;                   // Choice: the option it sets; Table: the model table.
    std::vector<std::string> choices;  // Choice: the values, in order.
    int value = 0;                     // Choice: the value shown first.
};

// One screen: a title, its items and, for a table, its columns and the rows it shows at once.
struct ScreenSpec {
    std::string id;               // `game_list`, `staging`, `scoreboard` (bound) or the server's.
    std::string title;            // Shown in the title line; empty keeps the game's.
    int rows = 0;                 // Table rows shown at once; 0 fits the screen's area.
    std::vector<Column> columns;  // The table's columns.
    std::vector<Item> items;      // The components, top to bottom.
};

// A game mode or map the server offers: the game's own, or a custom one on a stock base.
struct Entry {
    int id = 0;              // The server's number for it (the game's own for a stock one).
    std::string name;        // Its name as the lobby shows it.
    int base = -1;           // A custom mode or map: the stock one it runs on; -1 for a stock one.
    std::vector<int> modes;  // A map's modes; empty for a mode.
};

// Who an option belongs to.
enum class OptionScope : std::uint8_t {
    SERVER,  // The player's own choice, sent to the server (a vote, a preference).
    GAME,    // A rule of one game: its host sets it in the staging room, every member gets it.
};

// An option the server offers beside the game's own (a rule, a vote).
struct Option {
    std::string key;                          // What ACTION names it by.
    std::string label;                        // Its name on the screen.
    std::vector<std::string> choices;         // Its values, in order.
    int value = 0;                            // The value in force.
    OptionScope scope = OptionScope::SERVER;  // Whose choice it is.
};

// A whole description: limits, text, modes, maps, options and screens.
struct Description {
    std::string name;                        // The server's name, for the title line.
    int revision = 0;                        // The server's own number for this description.
    int max_players = 0;                     // Players a room holds; 0 keeps the game's.
    std::string motd;                        // A message shown on the lobby; empty for none.
    std::vector<std::string> announcements;  // Lines shown under the message.
    std::vector<Entry> modes;                // Modes offered; empty keeps the game's.
    std::vector<Entry> maps;                 // Maps offered; empty keeps the game's.
    std::vector<Option> options;             // Server options beside the game's.
    std::vector<ScreenSpec> screens;         // The screens, bound and native.

    // Returns the screen with `id`, or null when the description has none.
    const ScreenSpec* screen(std::string_view id) const;
};

// Parses `text` as an action (`back`, `open:<screen>`, `game:<name>`, `server:<key>`). Empty when it
// is none of those or the target is missing.
std::optional<Action> parse_action(std::string_view text);

// Parses and validates `json` as a description of schema 1. The error names the first key or value
// that breaks the schema; nothing of a refused description is kept.
[[nodiscard]] Result<Description> parse_description(std::string_view json);

}  // namespace openrac::mpui
