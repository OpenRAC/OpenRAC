// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests a multiplayer screen's layout, navigation and drawing (mpui/screen_view.h) with a skin of
// fixed measures: every character 8 wide, every line 16 high. The expected rows and positions are
// worked out from those measures and the areas given.

#include <cstdint>
#include <string>
#include <vector>

#include "mpui/screen_view.h"
#include "tests/check.h"

namespace openrac::mpui {
namespace {

// A character's width and a line's height in the test skin.
constexpr float kCharWidth = 8.0f;
constexpr float kLine = 16.0f;

// A skin that records what it is asked to draw and play.
class RecordingSkin final : public Skin {
public:
    float line_height() const override { return kLine; }

    float text_width(std::string_view text, TextStyle /*style*/) const override {
        return kCharWidth * static_cast<float>(text.size());
    }

    void text(float /*x*/, float y, std::string_view text, TextStyle style) override {
        texts.push_back(std::string(text));
        styles.push_back(style);
        text_y.push_back(y);
    }

    void highlight(const Rect& rect) override { highlights.push_back(rect); }

    void panel(const Rect& /*rect*/) override { ++panels; }

    void scroll_marks(const Rect& /*table*/, bool up, bool down) override {
        marks_up = up;
        marks_down = down;
    }

    void sound(Sound sound) override { sounds.push_back(sound); }

    std::vector<std::string> texts;  // Every text drawn, in order.
    std::vector<TextStyle> styles;   // Each text's style.
    std::vector<float> text_y;       // Each text's top.
    std::vector<Rect> highlights;    // Every highlight bar drawn.
    std::vector<Sound> sounds;       // Every sound played.
    int panels = 0;                  // Panels drawn.
    bool marks_up = false;           // The last scroll marks' up.
    bool marks_down = false;         // The last scroll marks' down.
};

// A screen of one table of `players`, titled, `rows` rows a page (0: what fits).
ScreenSpec player_screen(int rows) {
    ScreenSpec spec;
    spec.id = "staging";
    spec.title = "STAGING";
    spec.rows = rows;
    spec.columns = {Column{"name", "PLAYER", 70}, Column{"ready", "READY", 30}};
    Item table;
    table.kind = ItemKind::Table;
    table.key = "players";
    table.action = Action{ActionKind::Game, "team"};
    spec.items = {table};
    return spec;
}

// Fills the `players` table with `count` players keyed 100, 101, ...
void fill_players(Model& model, int count) {
    std::vector<Row> rows;

    // One row a player, numbered from 0.
    for (int n = 0; n < count; ++n) {
        Row row;
        row.key = static_cast<std::uint32_t>(100 + n);
        row.cells["name"] = "PLAYER" + std::to_string(n);
        rows.push_back(row);
    }

    model.table("players").set_rows(std::move(rows));
}

// A table fills what its area leaves: a 200-high area less a 24-high title and a 16-high heading
// line fits 10 rows of 16.
void test_a_table_shows_the_rows_that_fit() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 24);
    const ScreenView view(player_screen(0), Rect{0.0f, 0.0f, 400.0f, 200.0f}, Mode::Overlay);
    const std::vector<Placed> placed = view.layout(skin, model);
    CHECK(placed.size() == 1);
    CHECK(placed[0].shown_rows == 10);
    CHECK(placed[0].rect.y == kLine * 1.5f);
}

// The spec's rows win when fewer than fit.
void test_the_spec_rows_cap_a_table() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 24);
    const ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    CHECK(view.layout(skin, model)[0].shown_rows == 8);
}

// Twenty-four players scroll through an eight-row page: Down past the page scrolls by one.
void test_down_scrolls_past_the_page() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 24);
    ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);

    // Nine presses: the ninth moves past the eighth row on the page.
    for (int n = 0; n < 9; ++n) {
        CHECK(view.handle(Event::Down, model, skin).consumed);
    }

    CHECK(view.cursor() == 9);
    CHECK(view.first_row() == 9 + 1 - 8);
    CHECK(skin.sounds.size() == 9 && skin.sounds[0] == Sound::Move);
}

// R1 moves a page on and L1 a page back; the last page stops at the last row.
void test_l1_and_r1_page_a_table() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 20);
    ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    view.handle(Event::R1, model, skin);
    CHECK(view.cursor() == 8);
    view.handle(Event::R1, model, skin);
    view.handle(Event::R1, model, skin);
    CHECK(view.cursor() == 19);
    CHECK(view.first_row() == 20 - 8);
    view.handle(Event::L1, model, skin);
    CHECK(view.cursor() == 11);
}

// A table alone on its screen wraps from its first row to its last.
void test_up_from_the_first_row_wraps() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 12);
    ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    view.handle(Event::Up, model, skin);
    CHECK(view.cursor() == 11);
    CHECK(view.first_row() == 4);
}

// A refresh that moves the chosen row keeps the cursor on it, by key.
void test_the_cursor_follows_its_row_by_key() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 6);
    ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    view.handle(Event::Down, model, skin);
    view.handle(Event::Down, model, skin);
    std::vector<Row> rows = model.table("players").rows();
    rows.erase(rows.begin());
    model.table("players").set_rows(rows);
    const Outcome outcome = view.handle(Event::Cross, model, skin);
    CHECK(outcome.row_key == 102);
    CHECK(outcome.row == 1);
    CHECK(outcome.action.kind == ActionKind::Game && outcome.action.target == "team");
}

// A disabled row refuses Cross with the game's refusal and no action.
void test_a_disabled_row_refuses_cross() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 3);
    std::vector<Row> rows = model.table("players").rows();
    rows[0].enabled = false;
    model.table("players").set_rows(rows);
    ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    const Outcome outcome = view.handle(Event::Cross, model, skin);
    CHECK(outcome.consumed);
    CHECK(outcome.action.kind == ActionKind::None);
    CHECK(skin.sounds.back() == Sound::Denied);
}

// An overlay hands Triangle and Square to the game; a modal screen keeps them.
void test_an_overlay_passes_unused_events_on() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 3);
    ScreenView overlay(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    CHECK(!overlay.handle(Event::Triangle, model, skin).consumed);
    CHECK(!overlay.handle(Event::Square, model, skin).consumed);
    ScreenView modal(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Modal);
    const Outcome back = modal.handle(Event::Triangle, model, skin);
    CHECK(back.consumed && back.action.kind == ActionKind::Back);
    CHECK(modal.handle(Event::Square, model, skin).consumed);
}

// A choice steps with Left, Right and Cross, around its ends, and says its new value.
void test_a_choice_steps_around() {
    RecordingSkin skin;
    Model model;
    ScreenSpec spec;
    spec.id = "rules";
    Item choice;
    choice.kind = ItemKind::Choice;
    choice.key = "vote";
    choice.choices = {"A", "B", "C"};
    choice.action = Action{ActionKind::Server, "vote"};
    spec.items = {choice};
    ScreenView view(spec, Rect{0.0f, 0.0f, 400.0f, 100.0f}, Mode::Modal);
    CHECK(view.handle(Event::Left, model, skin).value == 2);
    CHECK(view.handle(Event::Right, model, skin).value == 0);
    const Outcome cross = view.handle(Event::Cross, model, skin);
    CHECK(
        cross.value == 1 && cross.action.kind == ActionKind::Server && cross.action.target == "vote"
    );
}

// Drawing a long list shows its page, the cursor's bar, the page count and the marks below.
void test_drawing_a_long_list() {
    RecordingSkin skin;
    Model model;
    fill_players(model, 24);
    ScreenView view(player_screen(8), Rect{0.0f, 0.0f, 400.0f, 400.0f}, Mode::Overlay);
    view.draw(skin, model);
    CHECK(skin.panels == 0);
    CHECK(skin.texts[0] == "STAGING");
    CHECK(skin.highlights.size() == 1);
    CHECK(skin.marks_down && !skin.marks_up);
    bool counted = false;
    int names = 0;

    // The page count and the names are among the texts drawn.
    for (const std::string& text : skin.texts) {
        counted = counted || text == "1-8 / 24";
        names += text.rfind("PLAYER", 0) == 0 && text != "PLAYER" ? 1 : 0;
    }

    CHECK(counted);
    CHECK(names == 8);
}

// A cell wider than its column is cut to fit: 70% of 100 less the 4 gap holds 8 characters.
void test_long_cells_are_cut_to_their_column() {
    RecordingSkin skin;
    Model model;
    Row row;
    row.key = 1;
    row.cells["name"] = "AVERYLONGPLAYERNAME";
    model.table("players").set_rows({row});
    ScreenSpec spec = player_screen(8);
    spec.title.clear();
    ScreenView view(spec, Rect{0.0f, 0.0f, 100.0f, 400.0f}, Mode::Overlay);
    view.draw(skin, model);
    bool cut = false;

    // The cut name is among the texts drawn.
    for (const std::string& text : skin.texts) {
        cut = cut || text == "AVERYLON";
    }

    CHECK(cut);
}

}  // namespace
}  // namespace openrac::mpui

int main() {
    using namespace openrac::mpui;
    test_a_table_shows_the_rows_that_fit();
    test_the_spec_rows_cap_a_table();
    test_down_scrolls_past_the_page();
    test_l1_and_r1_page_a_table();
    test_up_from_the_first_row_wraps();
    test_the_cursor_follows_its_row_by_key();
    test_a_disabled_row_refuses_cross();
    test_an_overlay_passes_unused_events_on();
    test_a_choice_steps_around();
    test_drawing_a_long_list();
    test_long_cells_are_cut_to_their_column();
    return openrac::test::result();
}
