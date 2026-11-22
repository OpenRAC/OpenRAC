// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One multiplayer screen on the picture: a ScreenSpec laid out in an area, its focus, its table's
// cursor and scroll, and its choices' values. It turns the game's menu events into cursor moves,
// sounds and actions, and draws itself through a Skin.
//
// A screen is laid out from the top of its area: the title line, then each item, the table taking
// what the others leave, so a table shows as many rows as fit (or the spec's `rows`) and scrolls
// through any number. Up and Down move the cursor, L1 and R1 a page, Left and Right step a
// choice, Cross chooses. An overlay screen (one drawn over a game screen) lets every event it has
// no use for go on to the game; a modal one keeps them all.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "mpui/description.h"
#include "mpui/model.h"
#include "mpui/skin.h"

namespace openrac::mpui {

// A menu event, as the games' menus read the pad (rac3: 0x00686138's codes, mapped).
enum class Event : std::uint8_t {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    CROSS,
    CIRCLE,
    TRIANGLE,
    SQUARE,
    L1,
    R1,
    START,
    SELECT,
};

// Whether the game's own screen is under this one.
enum class Mode : std::uint8_t {
    OVERLAY,  // Over a game screen: unused events go on to the game.
    MODAL,    // Alone: it keeps every event and draws its own panel.
};

// What one event did on a screen.
struct Outcome {
    bool consumed = false;      // The screen used the event: the game must not see it.
    Action action;              // The action chosen, or ActionKind::NONE.
    std::size_t item = 0;       // The item the action came from.
    std::uint32_t row_key = 0;  // A table's action: the key of the row under the cursor.
    std::size_t row = 0;        // A table's action: that row's index in the table.
    int value = 0;              // A choice's action: its new value.
};

// Where one item of a screen landed, and for a table which rows it shows.
struct Placed {
    std::size_t item = 0;        // The item's index in the spec.
    Rect rect;                   // Its rectangle in the picture.
    std::size_t first_row = 0;   // A table: the first row shown.
    std::size_t shown_rows = 0;  // A table: how many rows fit.
};

// A screen on the picture, with its state.
//
// Copied and moved freely. Only the game's thread uses it.
class ScreenView {
public:
    // Lays `spec` out in `area`, its focus on the first item that takes it.
    ScreenView(ScreenSpec spec, Rect area, Mode mode);

    // Returns the screen's description.
    const ScreenSpec& spec() const { return m_spec; }

    // Handles `event` against the rows of `model`, playing the skin's sounds. A Game action plays
    // no sound: the game plays its own when the binding hands the choice on.
    Outcome handle(Event event, const Model& model, Skin& skin);

    // Draws the screen through `skin`.
    void draw(Skin& skin, const Model& model);

    // Says whether the screen has the cursor; inactive, it draws no highlight bar or focused text.
    void set_active(bool active) { m_active = active; }

    // Returns where every item lands with `skin`'s measures and `model`'s rows.
    std::vector<Placed> layout(const Skin& skin, const Model& model) const;

    // Returns the index of the item under the cursor (the spec's item count when none takes it).
    std::size_t focus() const { return m_focus; }

    // Returns the index of the table row under the cursor.
    std::size_t cursor() const { return m_cursor; }

    // Returns the first table row shown.
    std::size_t first_row() const { return m_first; }

    // Returns the value a choice item shows now (0 for any other item).
    int value(std::size_t item) const;

private:
    // Moves the cursor back onto the row it was on before `model` changed, and into range.
    void follow_rows(const Model& model);

    // Returns the rows of the table item `item`, or null when it has none.
    const Table* rows_of(std::size_t item, const Model& model) const;

    // Returns whether `item` can take the cursor now.
    bool focusable(std::size_t item, const Model& model) const;

    // Moves the focus to the next (`step` 1) or previous (-1) item that takes it. False when there
    // is none that way.
    bool move_focus(int step, const Model& model);

    // Handles Up and Down: the cursor in the table, else the focus.
    Outcome handle_vertical(int step, const Model& model, Skin& skin);

    // Handles L1 and R1: a page of the table under the focus.
    Outcome handle_page(int step, const Model& model, Skin& skin);

    // Handles Left, Right and Cross on a choice: its value stepped by `step`.
    Outcome step_choice(int step, Skin& skin);

    // Handles Cross.
    Outcome handle_cross(const Model& model, Skin& skin);

    // Keeps the cursor's row inside the rows shown, `shown` at a time.
    void scroll_to_cursor(std::size_t shown);

    // Draws one item at its place.
    void draw_item(Skin& skin, const Model& model, const Placed& placed);

    // Draws the table item at its place.
    void draw_table(Skin& skin, const Table* table, const Placed& placed);

    ScreenSpec m_spec;               // What the screen shows.
    Rect m_area;                     // Where it is drawn.
    Mode m_mode = Mode::OVERLAY;     // Whether a game screen is under it.
    std::size_t m_focus = 0;         // The item under the cursor; the item count for none.
    std::size_t m_cursor = 0;        // The table row under the cursor.
    std::size_t m_first = 0;         // The first table row shown.
    std::size_t m_shown = 1;         // Table rows shown at the last layout.
    std::uint32_t m_cursor_key = 0;  // The key of the row under the cursor.
    bool m_has_cursor_key = false;   // False until the cursor has been on a row.
    std::vector<int> m_values;       // Each choice item's value, by item index.
    bool m_active = true;            // False while the game's cursor is elsewhere.
};

}  // namespace openrac::mpui
