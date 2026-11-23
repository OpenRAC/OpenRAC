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
constexpr float TITLE_LINES = 1.5f;

// The space between a table's columns, in the picture's pixels.
constexpr float COLUMN_GAP = 4.0f;

// Where a choice's value starts, as a share of the screen's width.
constexpr float CHOICE_VALUE_AT = 0.6f;

// A notice's screen id and its one button (the Cross token, as the games' dialogs show it).
constexpr const char* NOTICE_SCREEN = "notice";
constexpr const char* NOTICE_BUTTON = "\x10 OK";

// The space between a modal screen's panel edge and its items, in the picture's pixels.
constexpr float MODAL_MARGIN = 8.0f;

// A column's width is a percentage of its table's.
constexpr float PERCENT = 100.0f;

// Returns the longest start of `text` that `skin` draws within `width` in `style`.
std::string_view fit(const Skin& skin, std::string_view text, float width, TextStyle style) {
    // Most cells fit whole; only a long name is cut.
    while (!text.empty() && skin.text_width(text, style) > width) {
        text.remove_suffix(1);
    }

    return text;
}

// Returns `text` broken into the lines it takes at `width`, at least one. Lines break at spaces, as
// the games' own wrapped prints do; a word wider than the line stays whole on its own.
std::vector<std::string_view> wrap(const Skin& skin, std::string_view text, float width) {
    std::vector<std::string_view> lines;
    std::size_t start = 0;
    std::size_t last_space = std::string_view::npos;

    // Walks the text once, breaking before the word that overflows a line.
    for (std::size_t n = 0; n < text.size(); ++n) {
        // A space is where the line may break next.
        if (text[n] == ' ') {
            last_space = n;
        }

        const std::string_view line = text.substr(start, n + 1 - start);

        // The word that overflows goes to the next line, from the last space.
        if (last_space != std::string_view::npos && last_space > start
            && skin.text_width(line, TextStyle::NORMAL) > width) {
            lines.push_back(text.substr(start, last_space - start));
            start = last_space + 1;
            last_space = std::string_view::npos;
        }
    }

    lines.push_back(text.substr(start));
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
    if (item.kind == ItemKind::TEXT) {
        return static_cast<float>(wrap(skin, item.label, width).size()) * skin.line_height();
    }

    return item.kind == ItemKind::TABLE ? 0.0f : skin.line_height();
}

}  // namespace

ScreenView::ScreenView(ScreenSpec spec, Rect area, Mode mode)
    : m_spec(std::move(spec)),
      m_area(area),
      m_panel(area),
      m_mode(mode),
      m_focus(m_spec.items.size()) {
    // A modal screen keeps its items off its panel's edges.
    if (m_mode == Mode::MODAL) {
        m_area = Rect{
            area.x + MODAL_MARGIN,
            area.y + MODAL_MARGIN,
            area.w - 2.0f * MODAL_MARGIN,
            area.h - 2.0f * MODAL_MARGIN
        };
    }

    m_values.resize(m_spec.items.size());

    // Each choice starts at its spec's value.
    for (std::size_t n = 0; n < m_spec.items.size(); ++n) {
        m_values[n] = m_spec.items[n].value;

        // The cursor starts on the first button, choice or table, as the game's screens do.
        if (m_focus == m_spec.items.size() && m_spec.items[n].kind != ItemKind::TEXT) {
            m_focus = n;
        }
    }
}

int ScreenView::value(std::size_t item) const {
    return item < m_values.size() ? m_values[item] : 0;
}

const Table* ScreenView::rows_of(std::size_t item, const Model& model) const {
    // Only a table item reads rows.
    if (item >= m_spec.items.size() || m_spec.items[item].kind != ItemKind::TABLE) {
        return nullptr;
    }

    return model.find(m_spec.items[item].key);
}

bool ScreenView::focusable(std::size_t item, const Model& model) const {
    const Item& spec = m_spec.items[item];

    // An empty table has no row to put the cursor on.
    if (spec.kind == ItemKind::TABLE) {
        const Table* table = rows_of(item, model);
        return table != nullptr && !table->rows().empty();
    }

    return spec.kind != ItemKind::TEXT;
}

std::size_t ScreenView::focusable_count(const Model& model) const {
    std::size_t count = 0;

    // Every item that could take the cursor now.
    for (std::size_t n = 0; n < m_spec.items.size(); ++n) {
        count += focusable(n, model) ? 1 : 0;
    }

    return count;
}

void ScreenView::enter(bool from_top, const Model& model) {
    m_focus = m_spec.items.size();

    // The first item that takes the cursor from the top, or the last from the bottom.
    for (std::size_t k = 0; k < m_spec.items.size(); ++k) {
        const std::size_t n = from_top ? k : m_spec.items.size() - 1 - k;

        // Text never takes the cursor; an empty table neither.
        if (focusable(n, model)) {
            m_focus = n;
            break;
        }
    }

    const Table* table = rows_of(m_focus, model);
    const std::size_t count = table == nullptr ? 0 : table->rows().size();
    m_cursor = from_top || count == 0 ? 0 : count - 1;

    // The row entered is the one the cursor follows from now on.
    if (count != 0) {
        m_cursor_key = table->rows()[m_cursor].key;
        m_has_cursor_key = true;
    }
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
    float y = m_area.y + (m_spec.title.empty() ? 0.0f : line * TITLE_LINES);
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
        if (item.kind == ItemKind::TABLE) {
            const Table* table = rows_of(n, model);
            const std::size_t wanted =
                m_spec.rows > 0 ? static_cast<std::size_t>(m_spec.rows) : fit_rows;
            place.shown_rows = std::max<std::size_t>(std::min(wanted, fit_rows), 1);
            const std::size_t count = table == nullptr ? 0 : table->rows().size();

            // Without a headings line, a list longer than its page gives a row to its page count.
            if (headings == 0.0f && count > place.shown_rows && place.shown_rows > 1) {
                --place.shown_rows;
                place.rect.h += line;
            }

            place.first_row = m_first;

            // A table scrolled past its end after a refresh shows its last page.
            if (place.first_row + place.shown_rows > count) {
                place.first_row = count > place.shown_rows ? count - place.shown_rows : 0;
            }

            place.rect.h += headings + static_cast<float>(place.shown_rows) * line;
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
        if (m_spec.items[placed.item].kind == ItemKind::TABLE) {
            scroll_to_cursor(placed.shown_rows);
        }
    }

    // Each event moves the cursor, steps a choice or chooses.
    switch (event) {
        case Event::UP:
            return handle_vertical(-1, model, skin);
        case Event::DOWN:
            return handle_vertical(1, model, skin);
        case Event::L1:
            return handle_page(-1, model, skin);
        case Event::R1:
            return handle_page(1, model, skin);
        case Event::LEFT:
            return step_choice(-1, skin);
        case Event::RIGHT:
            return step_choice(1, skin);
        case Event::CROSS:
            return handle_cross(model, skin);
        case Event::TRIANGLE: {
            Outcome outcome;

            // A modal screen goes back itself; an overlay leaves Triangle to the game's screen.
            if (m_mode == Mode::MODAL) {
                skin.sound(Sound::BACK);
                outcome.consumed = true;
                outcome.action = Action{ActionKind::BACK, {}};
            }

            return outcome;
        }
        // The rest are the game's on an overlay, nothing on a modal screen.
        case Event::NONE:
        case Event::CIRCLE:
        case Event::SQUARE:
        case Event::START:
        case Event::SELECT: {
            Outcome outcome;
            outcome.consumed = m_mode == Mode::MODAL && event != Event::NONE;
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
            // A table alone on its screen (or modal) wraps round, as the games' lists do; else the
            // press leaves it for the game's own widgets around it.
            if (m_mode == Mode::OVERLAY && focusable_count(model) > 1) {
                return outcome;
            }

            m_cursor = step < 0 ? count - 1 : 0;
        }

        m_cursor_key = table->rows()[std::min(m_cursor, count - 1)].key;
        m_has_cursor_key = true;
        scroll_to_cursor(m_shown);
        skin.sound(Sound::MOVE);
        outcome.consumed = true;
        return outcome;
    }

    // Between items. An overlay with nowhere to go hands the press to the game.
    if (move_focus(step, model)) {
        skin.sound(Sound::MOVE);
        outcome.consumed = true;
        return outcome;
    }

    outcome.consumed = m_mode == Mode::MODAL;
    return outcome;
}

Outcome ScreenView::handle_page(int step, const Model& model, Skin& skin) {
    const Table* table = rows_of(m_focus, model);
    Outcome outcome;

    // L1 and R1 page only a table under the cursor.
    if (table == nullptr || table->rows().empty()) {
        outcome.consumed = m_mode == Mode::MODAL;
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
    skin.sound(m_cursor == old_cursor ? Sound::DENIED : Sound::MOVE);
    outcome.consumed = true;
    return outcome;
}

Outcome ScreenView::step_choice(int step, Skin& skin) {
    Outcome outcome;

    // Left and Right only step a choice; on an overlay they are the game's otherwise.
    if (m_focus >= m_spec.items.size() || m_spec.items[m_focus].kind != ItemKind::CHOICE) {
        outcome.consumed = m_mode == Mode::MODAL;
        return outcome;
    }

    const Item& item = m_spec.items[m_focus];
    const int count = static_cast<int>(item.choices.size());
    m_values[m_focus] = (m_values[m_focus] + step + count) % count;
    skin.sound(Sound::MOVE);
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
        outcome.consumed = m_mode == Mode::MODAL;
        return outcome;
    }

    const Item& item = m_spec.items[m_focus];

    // Cross steps a choice on, around its end.
    if (item.kind == ItemKind::CHOICE) {
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
    if (outcome.action.kind == ActionKind::NONE) {
        skin.sound(Sound::DENIED);
    } else if (outcome.action.kind != ActionKind::GAME) {
        skin.sound(outcome.action.kind == ActionKind::BACK ? Sound::BACK : Sound::SELECT);
    }

    return outcome;
}

void ScreenView::draw(Skin& skin, const Model& model) {
    follow_rows(model);

    // A screen with no game screen under it brings its own panel.
    if (m_mode == Mode::MODAL) {
        skin.panel(m_panel);
    }

    // The title line, when the description gives one.
    if (!m_spec.title.empty()) {
        skin.text(m_area.x, m_area.y, m_spec.title, TextStyle::TITLE);
    }

    // Each item is drawn at the place the layout gives it.
    for (const Placed& placed : layout(skin, model)) {
        // Scroll before drawing, so the cursor's row is on the picture.
        if (m_spec.items[placed.item].kind == ItemKind::TABLE) {
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
        case ItemKind::TEXT: {
            float y = rect.y;

            // A text wraps at the screen's width, a line a row.
            for (const std::string_view line : wrap(skin, item.label, rect.w)) {
                skin.text(rect.x, y, line, TextStyle::NORMAL);
                y += skin.line_height();
            }

            break;
        }
        case ItemKind::BUTTON:
            // The bar sits behind the component under the cursor, as in the game's menus.
            if (focused) {
                skin.highlight(rect);
            }

            skin.text(rect.x, rect.y, item.label, focused ? TextStyle::FOCUSED : TextStyle::NORMAL);
            break;
        case ItemKind::CHOICE: {
            const TextStyle style = focused ? TextStyle::FOCUSED : TextStyle::NORMAL;
            const std::size_t choice = static_cast<std::size_t>(m_values[placed.item]);

            // The bar behind the choice under the cursor.
            if (focused) {
                skin.highlight(rect);
            }

            skin.text(rect.x, rect.y, item.label, style);
            skin.text(rect.x + rect.w * CHOICE_VALUE_AT, rect.y, item.choices[choice], style);
            break;
        }
        case ItemKind::TABLE:
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
        x += rect.w * static_cast<float>(column.width) / PERCENT;
    }

    column_x.push_back(rect.x + rect.w);

    // The headings line, with the page's place in a list longer than one page on its right.
    if (has_headings(m_spec)) {
        // Each heading is cut to its column.
        for (std::size_t n = 0; n < m_spec.columns.size(); ++n) {
            const float width = column_x[n + 1] - column_x[n] - COLUMN_GAP;
            skin.text(
                column_x[n],
                y,
                fit(skin, m_spec.columns[n].label, width, TextStyle::HEADING),
                TextStyle::HEADING
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
        const float width = skin.text_width(where, TextStyle::HEADING);
        const float rows_end = y + line * static_cast<float>(placed.shown_rows);
        const float count_y = has_headings(m_spec) ? rect.y : rows_end;

        // On the headings line when there is one, else on the line the layout gave under the rows.
        skin.text(rect.x + rect.w - width, count_y, where, TextStyle::HEADING);
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
        TextStyle style = under_cursor ? TextStyle::FOCUSED : TextStyle::NORMAL;

        // A row that refuses Cross is dim, under the cursor or not.
        if (!row.enabled) {
            style = TextStyle::DISABLED;
        }

        // The bar behind the row under the cursor.
        if (under_cursor) {
            skin.highlight(Rect{rect.x, y, rect.w, line});
        }

        // Each cell is cut to its column.
        for (std::size_t c = 0; c < m_spec.columns.size(); ++c) {
            const float width = column_x[c + 1] - column_x[c] - COLUMN_GAP;
            skin.text(
                column_x[c], y, fit(skin, row.cell(m_spec.columns[c].field), width, style), style
            );
        }

        y += line;
    }
}

ScreenSpec notice_screen(std::string text) {
    ScreenSpec spec;
    spec.id = NOTICE_SCREEN;
    Item message;
    message.kind = ItemKind::TEXT;
    message.label = std::move(text);
    Item ok;
    ok.kind = ItemKind::BUTTON;
    ok.label = NOTICE_BUTTON;
    ok.action = Action{ActionKind::BACK, {}};
    spec.items = {message, ok};
    return spec;
}

}  // namespace openrac::mpui
