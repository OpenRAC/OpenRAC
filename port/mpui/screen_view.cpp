// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A screen's layout, navigation and drawing: screen_view.h.

#include "mpui/screen_view.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace openrac::mpui {
namespace {

// The title line is this much taller than a row, as the games set their titles apart.
constexpr float kTitleLines = 1.5f;

// The space between a table's columns, in the picture's pixels.
constexpr float kColumnGap = 4.0f;

// Where a choice's value starts, as a share of the screen's width.
constexpr float kChoiceValueAt = 0.6f;

// A column's width is a percentage of its table's.
constexpr float kPercent = 100.0f;

// Returns the longest start of `text` that `skin` draws within `width` in `style`.
std::string_view fit(const Skin& skin, std::string_view text, float width, TextStyle style) {
    // Most cells fit whole; only a long name is cut.
    while (!text.empty() && skin.text_width(text, style) > width) {
        text.remove_suffix(1);
    }

    return text;
}

// Returns how many lines `text` takes wrapped at `width`, at least one.
std::size_t wrapped_lines(const Skin& skin, std::string_view text, float width) {
    std::size_t lines = 1;
    std::size_t start = 0;
    std::size_t last_space = std::string_view::npos;

    // Walks the text once, breaking before the word that overflows a line.
    for (std::size_t n = 0; n < text.size(); ++n) {
        // Lines break at spaces, as the games' own wrapped prints do.
        if (text[n] == ' ') {
            last_space = n;
        }

        const std::string_view line = text.substr(start, n + 1 - start);

        // The word that overflows goes to the next line, from the last space.
        if (last_space != std::string_view::npos && last_space > start
            && skin.text_width(line, TextStyle::Normal) > width) {
            ++lines;
            start = last_space + 1;
            last_space = std::string_view::npos;
        }
    }

    return lines;
}

// Returns whether a table's columns have any heading, which takes a line above its rows.
bool has_headings(const ScreenSpec& spec) {
    return std::any_of(spec.columns.begin(), spec.columns.end(), [](const Column& column) {
        return !column.label.empty();
    });
}

// Returns the height an item other than a table takes.
float fixed_height(const Skin& skin, const Item& item, float width) {
    // Texts wrap; the other kinds are one line each.
    if (item.kind == ItemKind::Text) {
        return static_cast<float>(wrapped_lines(skin, item.label, width)) * skin.line_height();
    }

    return item.kind == ItemKind::Table ? 0.0f : skin.line_height();
}

}  // namespace

ScreenView::ScreenView(ScreenSpec spec, Rect area, Mode mode)
    : m_spec(std::move(spec)),
      m_area(area),
      m_mode(mode),
      m_focus(m_spec.items.size()) {
    m_values.resize(m_spec.items.size());

    // Each choice starts at its spec's value.
    for (std::size_t n = 0; n < m_spec.items.size(); ++n) {
        m_values[n] = m_spec.items[n].value;

        // The cursor starts on the first button, choice or table, as the game's screens do.
        if (m_focus == m_spec.items.size() && m_spec.items[n].kind != ItemKind::Text) {
            m_focus = n;
        }
    }
}

int ScreenView::value(std::size_t item) const {
    return item < m_values.size() ? m_values[item] : 0;
}

const Table* ScreenView::rows_of(std::size_t item, const Model& model) const {
    // Only a table item reads rows.
    if (item >= m_spec.items.size() || m_spec.items[item].kind != ItemKind::Table) {
        return nullptr;
    }

    return model.find(m_spec.items[item].key);
}

bool ScreenView::focusable(std::size_t item, const Model& model) const {
    const Item& spec = m_spec.items[item];

    // An empty table has no row to put the cursor on.
    if (spec.kind == ItemKind::Table) {
        const Table* table = rows_of(item, model);
        return table != nullptr && !table->rows().empty();
    }

    return spec.kind != ItemKind::Text;
}

bool ScreenView::move_focus(int step, const Model& model) {
    std::size_t at = m_focus;

    // Walks from the focus towards the end `step` points at; it stops at the first taker or the end.
    for (;;) {
        // No item that way: the focus stays.
        if ((step < 0 && (at == 0 || at > m_spec.items.size()))
            || (step > 0 && at + 1 >= m_spec.items.size())) {
            return false;
        }

        at = step < 0 ? at - 1 : at + 1;

        // The first item that takes the cursor gets it.
        if (focusable(at, model)) {
            m_focus = at;
            return true;
        }
    }
}

void ScreenView::follow_rows(const Model& model) {
    const Table* table = rows_of(m_focus, model);

    // No table under the cursor: the cursor is the focus alone.
    if (table == nullptr) {
        // An empty table that held the focus gives it to the next item that takes it.
        if (m_focus < m_spec.items.size() && !focusable(m_focus, model)) {
            move_focus(1, model);
        }

        return;
    }

    const std::size_t count = table->rows().size();
    const std::size_t found = m_has_cursor_key ? table->find(m_cursor_key) : count;

    // The row the cursor was on moved: follow it, so a refresh never changes the player's choice.
    if (found < count) {
        m_cursor = found;
    }

    // The list shrank under the cursor: stay on its last row.
    if (count != 0 && m_cursor >= count) {
        m_cursor = count - 1;
    }

    // An empty table gives the focus up.
    if (count == 0) {
        m_cursor = 0;
        m_first = 0;
        move_focus(1, model);
        return;
    }

    m_cursor_key = table->rows()[m_cursor].key;
    m_has_cursor_key = true;
}

void ScreenView::scroll_to_cursor(std::size_t shown) {
    m_shown = std::max<std::size_t>(shown, 1);

    // Scrolled past the cursor: bring its row back to the top.
    if (m_cursor < m_first) {
        m_first = m_cursor;
    }

    // The cursor below the rows shown: bring its row to the bottom.
    if (m_cursor >= m_first + m_shown) {
        m_first = m_cursor + 1 - m_shown;
    }
}

std::vector<Placed> ScreenView::layout(const Skin& skin, const Model& model) const {
    std::vector<Placed> placed;
    const float line = skin.line_height();
    float y = m_area.y + (m_spec.title.empty() ? 0.0f : line * kTitleLines);
    float fixed = 0.0f;

    // The items other than the table take their own height first.
    for (const Item& item : m_spec.items) {
        fixed += fixed_height(skin, item, m_area.w);
    }

    // The table takes the height the other items leave, less its heading line.
    const float headings = has_headings(m_spec) ? line : 0.0f;
    const float table_height = std::max(m_area.y + m_area.h - y - fixed - headings, line);
    const auto fit_rows = static_cast<std::size_t>(table_height / line);

    // Each item goes under the one before it.
    for (std::size_t n = 0; n < m_spec.items.size(); ++n) {
        const Item& item = m_spec.items[n];
        Placed place{n, Rect{m_area.x, y, m_area.w, fixed_height(skin, item, m_area.w)}, 0, 0};

        // A table shows the spec's rows when they fit, else as many as fit.
        if (item.kind == ItemKind::Table) {
            const Table* table = rows_of(n, model);
            const std::size_t wanted =
                m_spec.rows > 0 ? static_cast<std::size_t>(m_spec.rows) : fit_rows;
            place.shown_rows = std::max<std::size_t>(std::min(wanted, fit_rows), 1);
            place.first_row = m_first;

            // A table scrolled past its end after a refresh shows its last page.
            if (table != nullptr && place.first_row + place.shown_rows > table->rows().size()) {
                const std::size_t count = table->rows().size();
                place.first_row = count > place.shown_rows ? count - place.shown_rows : 0;
            }

            place.rect.h = headings + static_cast<float>(place.shown_rows) * line;
        }

        y += place.rect.h;
        placed.push_back(place);
    }

    return placed;
}

Outcome ScreenView::handle(Event event, const Model& model, Skin& skin) {
    follow_rows(model);

    // Lays the screen out to learn how many rows a page holds.
    for (const Placed& placed : layout(skin, model)) {
        // The table's page size is what the last layout shows.
        if (m_spec.items[placed.item].kind == ItemKind::Table) {
            scroll_to_cursor(placed.shown_rows);
        }
    }

    // Each event moves the cursor, steps a choice or chooses.
    switch (event) {
        case Event::Up:
            return handle_vertical(-1, model, skin);
        case Event::Down:
            return handle_vertical(1, model, skin);
        case Event::L1:
            return handle_page(-1, model, skin);
        case Event::R1:
            return handle_page(1, model, skin);
        case Event::Left:
            return step_choice(-1, skin);
        case Event::Right:
            return step_choice(1, skin);
        case Event::Cross:
            return handle_cross(model, skin);
        case Event::Triangle: {
            Outcome outcome;

            // A modal screen goes back itself; an overlay leaves Triangle to the game's screen.
            if (m_mode == Mode::Modal) {
                skin.sound(Sound::Back);
                outcome.consumed = true;
                outcome.action = Action{ActionKind::Back, {}};
            }

            return outcome;
        }
        // The rest are the game's on an overlay, nothing on a modal screen.
        case Event::None:
        case Event::Circle:
        case Event::Square:
        case Event::Start:
        case Event::Select: {
            Outcome outcome;
            outcome.consumed = m_mode == Mode::Modal && event != Event::None;
            return outcome;
        }
    }

    return Outcome{};
}

Outcome ScreenView::handle_vertical(int step, const Model& model, Skin& skin) {
    const Table* table = rows_of(m_focus, model);
    Outcome outcome;

    // In a table the cursor moves first; at its ends the focus goes to the item beyond.
    if (table != nullptr && !table->rows().empty()) {
        const std::size_t count = table->rows().size();
        const bool at_end = step < 0 ? m_cursor == 0 : m_cursor + 1 >= count;

        // Within the table: one row.
        if (!at_end) {
            m_cursor = step < 0 ? m_cursor - 1 : m_cursor + 1;
        } else if (!move_focus(step, model)) {
            // A table alone on its screen wraps round, as the games' lists do.
            m_cursor = step < 0 ? count - 1 : 0;
        }

        m_cursor_key = table->rows()[std::min(m_cursor, count - 1)].key;
        m_has_cursor_key = true;
        scroll_to_cursor(m_shown);
        skin.sound(Sound::Move);
        outcome.consumed = true;
        return outcome;
    }

    // Between items. An overlay with nowhere to go hands the press to the game.
    if (move_focus(step, model)) {
        skin.sound(Sound::Move);
        outcome.consumed = true;
        return outcome;
    }

    outcome.consumed = m_mode == Mode::Modal;
    return outcome;
}

Outcome ScreenView::handle_page(int step, const Model& model, Skin& skin) {
    const Table* table = rows_of(m_focus, model);
    Outcome outcome;

    // L1 and R1 page only a table under the cursor.
    if (table == nullptr || table->rows().empty()) {
        outcome.consumed = m_mode == Mode::Modal;
        return outcome;
    }

    const std::size_t count = table->rows().size();
    const std::size_t old_cursor = m_cursor;

    // A page back stops at the first row, a page on at the last.
    if (step < 0) {
        m_cursor = m_cursor > m_shown ? m_cursor - m_shown : 0;
        m_first = m_first > m_shown ? m_first - m_shown : 0;
    } else {
        m_cursor = std::min(m_cursor + m_shown, count - 1);
        m_first = std::min(m_first + m_shown, count > m_shown ? count - m_shown : 0);
    }

    scroll_to_cursor(m_shown);
    m_cursor_key = table->rows()[m_cursor].key;
    m_has_cursor_key = true;

    // A page that moved nothing says so, as the games refuse at a list's end.
    skin.sound(m_cursor == old_cursor ? Sound::Denied : Sound::Move);
    outcome.consumed = true;
    return outcome;
}

Outcome ScreenView::step_choice(int step, Skin& skin) {
    Outcome outcome;

    // Left and Right only step a choice; on an overlay they are the game's otherwise.
    if (m_focus >= m_spec.items.size() || m_spec.items[m_focus].kind != ItemKind::Choice) {
        outcome.consumed = m_mode == Mode::Modal;
        return outcome;
    }

    const Item& item = m_spec.items[m_focus];
    const int count = static_cast<int>(item.choices.size());
    m_values[m_focus] = (m_values[m_focus] + step + count) % count;
    skin.sound(Sound::Move);
    outcome.consumed = true;
    outcome.item = m_focus;
    outcome.action = item.action;
    outcome.value = m_values[m_focus];
    return outcome;
}

Outcome ScreenView::handle_cross(const Model& model, Skin& skin) {
    Outcome outcome;

    // Nothing under the cursor: the game's screen may still use Cross.
    if (m_focus >= m_spec.items.size()) {
        outcome.consumed = m_mode == Mode::Modal;
        return outcome;
    }

    const Item& item = m_spec.items[m_focus];

    // Cross steps a choice on, around its end.
    if (item.kind == ItemKind::Choice) {
        return step_choice(1, skin);
    }

    outcome.consumed = true;
    outcome.item = m_focus;
    outcome.action = item.action;
    const Table* table = rows_of(m_focus, model);

    // A table's action names the row under the cursor; a disabled row refuses.
    if (table != nullptr && m_cursor < table->rows().size()) {
        const Row& row = table->rows()[m_cursor];
        outcome.row_key = row.key;
        outcome.row = m_cursor;

        // A disabled row, or a game that is full, refuses Cross.
        if (!row.enabled) {
            outcome.action = Action{};
        }
    }

    // A Game action is answered by the game's own sound when the binding hands it on.
    if (outcome.action.kind == ActionKind::None) {
        skin.sound(Sound::Denied);
    } else if (outcome.action.kind != ActionKind::Game) {
        skin.sound(outcome.action.kind == ActionKind::Back ? Sound::Back : Sound::Select);
    }

    return outcome;
}

void ScreenView::draw(Skin& skin, const Model& model) {
    follow_rows(model);

    // A screen with no game screen under it brings its own panel.
    if (m_mode == Mode::Modal) {
        skin.panel(m_area);
    }

    // The title line, when the description gives one.
    if (!m_spec.title.empty()) {
        skin.text(m_area.x, m_area.y, m_spec.title, TextStyle::Title);
    }

    // Each item is drawn at the place the layout gives it.
    for (const Placed& placed : layout(skin, model)) {
        // Scroll before drawing, so the cursor's row is on the picture.
        if (m_spec.items[placed.item].kind == ItemKind::Table) {
            scroll_to_cursor(placed.shown_rows);
        }

        draw_item(skin, model, placed);
    }
}

void ScreenView::draw_item(Skin& skin, const Model& model, const Placed& placed) {
    const Item& item = m_spec.items[placed.item];
    const bool focused = m_active && placed.item == m_focus;
    const Rect& rect = placed.rect;

    // Each kind of item draws its own way.
    switch (item.kind) {
        case ItemKind::Text:
            skin.text(rect.x, rect.y, item.label, TextStyle::Normal);
            break;
        case ItemKind::Button:
            // The bar sits behind the component under the cursor, as in the game's menus.
            if (focused) {
                skin.highlight(rect);
            }

            skin.text(rect.x, rect.y, item.label, focused ? TextStyle::Focused : TextStyle::Normal);
            break;
        case ItemKind::Choice: {
            const TextStyle style = focused ? TextStyle::Focused : TextStyle::Normal;
            const std::size_t choice = static_cast<std::size_t>(m_values[placed.item]);

            // The bar behind the choice under the cursor.
            if (focused) {
                skin.highlight(rect);
            }

            skin.text(rect.x, rect.y, item.label, style);
            skin.text(rect.x + rect.w * kChoiceValueAt, rect.y, item.choices[choice], style);
            break;
        }
        case ItemKind::Table:
            draw_table(skin, rows_of(placed.item, model), placed);
            break;
    }
}

void ScreenView::draw_table(Skin& skin, const Table* table, const Placed& placed) {
    const float line = skin.line_height();
    const Rect& rect = placed.rect;
    const std::size_t count = table == nullptr ? 0 : table->rows().size();
    const bool focused = m_active && placed.item == m_focus;
    float y = rect.y;
    std::vector<float> column_x;
    float x = rect.x;

    // Each column starts where the one before it ends, by its share of the width.
    for (const Column& column : m_spec.columns) {
        column_x.push_back(x);
        x += rect.w * static_cast<float>(column.width) / kPercent;
    }

    column_x.push_back(rect.x + rect.w);

    // The headings line, with the page's place in a list longer than one page on its right.
    if (has_headings(m_spec)) {
        // Each heading is cut to its column.
        for (std::size_t n = 0; n < m_spec.columns.size(); ++n) {
            const float width = column_x[n + 1] - column_x[n] - kColumnGap;
            skin.text(
                column_x[n],
                y,
                fit(skin, m_spec.columns[n].label, width, TextStyle::Heading),
                TextStyle::Heading
            );
        }

        y += line;
    }

    // A list that shrank under a scrolled table shows its last page.
    if (m_first + placed.shown_rows > count) {
        m_first = count > placed.shown_rows ? count - placed.shown_rows : 0;
    }

    // The rows a page above or below the ones shown are marked, with "first-last / count".
    if (count > placed.shown_rows) {
        const std::size_t last = std::min(m_first + placed.shown_rows, count);
        const std::string where = std::format("{}-{} / {}", m_first + 1, last, count);
        const float width = skin.text_width(where, TextStyle::Heading);
        skin.text(rect.x + rect.w - width, rect.y, where, TextStyle::Heading);
        skin.scroll_marks(
            Rect{rect.x, y, rect.w, line * static_cast<float>(placed.shown_rows)},
            m_first > 0,
            last < count
        );
    }

    // Only the page of rows shown is drawn.
    for (std::size_t n = m_first; n < count && n < m_first + placed.shown_rows; ++n) {
        const Row& row = table->rows()[n];
        const bool under_cursor = focused && n == m_cursor;
        TextStyle style = under_cursor ? TextStyle::Focused : TextStyle::Normal;

        // A row that refuses Cross is dim, under the cursor or not.
        if (!row.enabled) {
            style = TextStyle::Disabled;
        }

        // The bar behind the row under the cursor.
        if (under_cursor) {
            skin.highlight(Rect{rect.x, y, rect.w, line});
        }

        // Each cell is cut to its column.
        for (std::size_t c = 0; c < m_spec.columns.size(); ++c) {
            const float width = column_x[c + 1] - column_x[c] - kColumnGap;
            skin.text(
                column_x[c], y, fit(skin, row.cell(m_spec.columns[c].field), width, style), style
            );
        }

        y += line;
    }
}

}  // namespace openrac::mpui
