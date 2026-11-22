// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the multiplayer UI description reader (mpui/description.h): a whole description of schema
// 1 is read as written, and each kind of fault refuses it whole. The expected values are the
// descriptions' own text.

#include <string>

#include "mpui/description.h"
#include "tests/check.h"

namespace {

using openrac::mpui::ActionKind;
using openrac::mpui::ItemKind;
using openrac::mpui::parse_action;
using openrac::mpui::parse_description;

// A description using every block of the schema.
constexpr const char* kFull = R"({
  "schema": 1, "name": "OpenRAC", "revision": 4,
  "limits": { "max_players": 24 },
  "text": { "motd": "Welcome", "announcements": ["Double XP", "New maps"] },
  "modes": [ { "id": 2, "name": "SIEGE" }, { "id": 101, "name": "KING OF THE HILL", "base": 2 } ],
  "maps": [ { "id": 40, "name": "BAKISI ISLES", "modes": [1, 2, 3] } ],
  "options": [ { "key": "friendly_fire", "label": "FRIENDLY FIRE", "choices": ["OFF", "ON"], "value": 1, "scope": "game" },
               { "key": "vote_map", "label": "NEXT MAP", "choices": ["A", "B"] } ],
  "screens": [
    { "id": "game_list", "title": "FIND GAME", "rows": 10,
      "columns": [ { "field": "name", "label": "GAME", "width": 60 },
                   { "field": "players", "label": "PLAYERS", "width": 40 } ],
      "items": [ { "kind": "table", "key": "games", "action": "game:join" } ] },
    { "id": "rules", "title": "SERVER RULES",
      "items": [ { "kind": "text", "label": "No spawn camping" },
                 { "kind": "choice", "key": "vote", "label": "VOTE", "choices": ["A", "B"], "action": "server:vote" },
                 { "kind": "button", "label": "ACCEPT", "action": "back" } ] }
  ]
})";

// Returns whether `json` is refused.
bool refused(const std::string& json) {
    return !parse_description(json).has_value();
}

// Every block of a full description is read as written.
void test_a_full_description_is_read() {
    auto description = parse_description(kFull);
    CHECK(description.has_value());

    // The rest of the checks need the value.
    if (!description) {
        return;
    }

    CHECK(description->name == "OpenRAC");
    CHECK(description->revision == 4);
    CHECK(description->max_players == 24);
    CHECK(description->announcements.size() == 2);
    CHECK(description->maps.size() == 1 && description->maps[0].modes.size() == 3);
    CHECK(description->modes.size() == 2 && description->modes[0].base == -1);
    CHECK(description->modes[1].base == 2);
    CHECK(description->options.size() == 2 && description->options[0].value == 1);
    CHECK(description->options[0].scope == openrac::mpui::OptionScope::Game);
    CHECK(description->options[1].scope == openrac::mpui::OptionScope::Server);
    const auto* list = description->screen("game_list");
    CHECK(list != nullptr && list->rows == 10 && list->columns.size() == 2);
    CHECK(list != nullptr && list->items[0].kind == ItemKind::Table);
    CHECK(list != nullptr && list->items[0].action.kind == ActionKind::Game);
    CHECK(list != nullptr && list->items[0].action.target == "join");
    const auto* rules = description->screen("rules");
    CHECK(rules != nullptr && rules->items.size() == 3);
    CHECK(rules != nullptr && rules->items[2].action.kind == ActionKind::Back);
    CHECK(description->screen("staging") == nullptr);
}

// The actions are the closed set and nothing else.
void test_actions_are_a_closed_set() {
    CHECK(parse_action("back")->kind == ActionKind::Back);
    CHECK(parse_action("open:rules")->kind == ActionKind::Open);
    CHECK(parse_action("server:vote_map")->target == "vote_map");
    CHECK(!parse_action("run:rm"));
    CHECK(!parse_action("open:"));
    CHECK(!parse_action("game:Join Now"));
}

// A description of another schema, or not JSON, is refused.
void test_other_schemas_and_bad_json_are_refused() {
    CHECK(refused(R"({"schema": 2})"));
    CHECK(refused(R"({"name": "no schema"})"));
    CHECK(refused("{"));
    CHECK(refused(R"({"schema": 1, "schema": 1})"));
    CHECK(!refused(R"({"schema": 1})"));
}

// A key the schema does not know is refused at any depth.
void test_unknown_keys_are_refused() {
    CHECK(refused(R"({"schema": 1, "script": "x"})"));
    CHECK(refused(R"({"schema": 1, "screens": [{"id": "a", "onload": 1}]})"));
    CHECK(refused(R"({"schema": 1, "limits": {"max_players": 8, "min": 2}})"));
}

// Limits on counts, lengths and values hold.
void test_limits_are_enforced() {
    CHECK(refused(R"({"schema": 1, "limits": {"max_players": 65}})"));
    CHECK(refused(R"({"schema": 1, "limits": {"max_players": 2.5}})"));
    CHECK(refused(std::string(R"({"schema": 1, "name": ")") + std::string(41, 'x') + "\"}"));
    CHECK(refused(R"({"schema": 1, "screens": [{"id": "Bad Id"}]})"));
    CHECK(refused(
        R"({"schema": 1, "screens": [{"id": "a", "columns": [{"field": "a", "width": 70},
                                                                       {"field": "b", "width": 40}]}]})"
    ));
    CHECK(refused(R"({"schema": 1, "screens": [{"id": "a"}, {"id": "a"}]})"));
    CHECK(refused(R"({"schema": 1, "options": [{"key": "a", "label": "A", "choices": []}]})"));
    CHECK(refused(
        R"({"schema": 1, "options": [{"key": "a", "label": "A", "choices": ["x"], "scope": "room"}]})"
    ));
    CHECK(refused(
        R"({"schema": 1, "screens": [{"id": "a", "items": [{"kind": "table", "key": "t"},
                                                                     {"kind": "table", "key": "u"}]}]})"
    ));
    CHECK(refused(std::string(R"({"schema": 1, "name": ")") + std::string(40'000, 'x') + "\"}"));
}

}  // namespace

int main() {
    test_a_full_description_is_read();
    test_actions_are_a_closed_set();
    test_other_schemas_and_bad_json_are_refused();
    test_unknown_keys_are_refused();
    test_limits_are_enforced();
    return openrac::test::result();
}
