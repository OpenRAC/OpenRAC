// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the C contract of the native multiplayer screens (openrac/mp_ui.h, game/common/lib/mp_ui.cpp)
// as a game's binding uses it: a description, a screen over the game's, a modal screen opened with
// open:, a notice, and which events the game still sees. The skin is a recording one with fixed
// measures; the expected values follow from the descriptions written here.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "openrac/mp_ui.h"
#include "tests/check.h"

namespace {

// A description with a list whose button opens a rules screen, and the rules screen.
constexpr const char* DESCRIPTION = R"({"schema": 1, "screens": [
  {"id": "game_list", "items": [{"kind": "button", "label": "RULES", "action": "open:rules"},
                                {"kind": "table", "key": "games", "action": "game:join"}],
   "columns": [{"field": "name", "width": 100}]},
  {"id": "rules", "title": "RULES", "items": [{"kind": "text", "label": "Play fair"},
                                              {"kind": "button", "label": "OK", "action": "back"}]}]})";

// What the skin was asked to do, for the checks.
struct Record {
    std::vector<std::string> texts;  // Every text drawn.
    std::vector<int> layers;         // Every layer begun, 0 or 1.
    int panels = 0;                  // Panels drawn.
};

// The skin's width: 8 a byte.
float text_width(void* /*user*/, const char* /*text*/, std::size_t length, int /*style*/) {
    return 8.0f * static_cast<float>(length);
}

// The skin's text: recorded.
void text(
    void* user, float /*x*/, float /*y*/, const char* text, std::size_t length, int /*style*/
) {
    static_cast<Record*>(user)->texts.emplace_back(text, length);
}

// The skin's highlight bar: nothing to record.
void highlight(void* /*user*/, float /*x*/, float /*y*/, float /*w*/, float /*h*/) {}

// The panel's field colour, told apart from its edge's.
constexpr std::uint32_t FILL_COLOUR = 0x7C000000u;

// The skin's rectangles: the panel's fields counted.
void rect(void* user, float /*x*/, float /*y*/, float /*w*/, float /*h*/, std::uint32_t rgba) {
    // Only the field is drawn in the fill colour.
    if (rgba == FILL_COLOUR) {
        ++static_cast<Record*>(user)->panels;
    }
}

// The skin's images: none used.
void sprite(void* /*user*/, float /*x*/, float /*y*/, float /*w*/, float /*h*/, int /*sprite*/) {}

// The skin's sounds: nothing to record.
void sound(void* /*user*/, int /*sound*/) {}

// The skin's layers: recorded.
void layer(void* user, int modal) {
    static_cast<Record*>(user)->layers.push_back(modal);
}

// Returns a skin recording into `record`.
openrac_mp_ui_skin skin_of(Record& record) {
    return openrac_mp_ui_skin{
        &record,
        16.0f,
        FILL_COLOUR,
        0x8066CCFFu,
        text_width,
        text,
        rect,
        sprite,
        highlight,
        sound,
        layer
    };
}

// Returns a context with the description above and the game list open over the game's screen.
openrac_mp_ui* open_list() {
    openrac_mp_ui* ui = openrac_mp_ui_create();
    CHECK(openrac_mp_ui_describe(ui, DESCRIPTION, std::string(DESCRIPTION).size()) == 1);
    CHECK(
        openrac_mp_ui_open(ui, "game_list", 0.0f, 0.0f, 400.0f, 300.0f, OPENRAC_MP_UI_OVERLAY) == 1
    );
    openrac_mp_ui_set_modal_area(ui, 50.0f, 50.0f, 300.0f, 200.0f);
    return ui;
}

// An open: action opens its screen modally; the modal one takes every event, and Back closes it.
void test_open_shows_a_modal_screen_until_back() {
    Record record;
    const openrac_mp_ui_skin skin = skin_of(record);
    openrac_mp_ui* ui = open_list();
    openrac_mp_ui_outcome out;
    openrac_mp_ui_event(ui, &skin, OPENRAC_MP_UI_CROSS, &out);
    CHECK(out.consumed == 1 && out.action == OPENRAC_MP_UI_ACTION_NONE);
    CHECK(openrac_mp_ui_modal(ui) == 1);
    openrac_mp_ui_event(ui, &skin, OPENRAC_MP_UI_SQUARE, &out);
    CHECK(out.consumed == 1);
    openrac_mp_ui_event(ui, &skin, OPENRAC_MP_UI_TRIANGLE, &out);
    CHECK(out.consumed == 1 && openrac_mp_ui_modal(ui) == 0);
    openrac_mp_ui_event(ui, &skin, OPENRAC_MP_UI_SQUARE, &out);
    CHECK(out.consumed == 0);
    openrac_mp_ui_destroy(ui);
}

// A notice is drawn as a second layer with its own panel, over the screen under it.
void test_a_notice_is_a_modal_layer() {
    Record record;
    const openrac_mp_ui_skin skin = skin_of(record);
    openrac_mp_ui* ui = open_list();
    openrac_mp_ui_notice(ui, "Vote counted");
    openrac_mp_ui_draw(ui, &skin);
    CHECK(record.layers.size() == 2 && record.layers[0] == 0 && record.layers[1] == 1);
    CHECK(record.panels == 1);
    CHECK(!record.texts.empty() && record.texts.back() == "\x10 OK");
    openrac_mp_ui_outcome out;
    openrac_mp_ui_event(ui, &skin, OPENRAC_MP_UI_CROSS, &out);
    CHECK(openrac_mp_ui_modal(ui) == 0);
    openrac_mp_ui_destroy(ui);
}

// A refused description keeps the one before.
void test_a_refused_description_keeps_the_last() {
    openrac_mp_ui* ui = open_list();
    const std::string bad = R"({"schema": 1, "screens": [{"id": "x", "onload": 1}]})";
    CHECK(openrac_mp_ui_describe(ui, bad.c_str(), bad.size()) == 0);
    CHECK(openrac_mp_ui_open(ui, "rules", 0.0f, 0.0f, 10.0f, 10.0f, OPENRAC_MP_UI_MODAL) == 1);
    openrac_mp_ui_destroy(ui);
}

}  // namespace

int main() {
    test_open_shows_a_modal_screen_until_back();
    test_a_notice_is_a_modal_layer();
    test_a_refused_description_keeps_the_last();
    return openrac::test::result();
}
