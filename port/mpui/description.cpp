// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The description's reader: description.h. Every object is read against the keys its schema
// allows, every string and count against its limit, and the first fault refuses the whole
// description, so a client never shows half of a server's screens.

#include "mpui/description.h"

#include <algorithm>
#include <format>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>

#include "common/json.h"

namespace openrac::mpui {
namespace {

// The longest texts a description may carry, in bytes: a title fits the title line, a label a
// row, a text a short paragraph.
constexpr std::size_t kMaxTitle = 40;
constexpr std::size_t kMaxLabel = 32;
constexpr std::size_t kMaxText = 512;
constexpr std::size_t kMaxName = 24;

// The most of each list a description may hold.
constexpr std::size_t kMaxScreens = 16;
constexpr std::size_t kMaxColumns = 8;
constexpr std::size_t kMaxItems = 24;
constexpr std::size_t kMaxChoices = 16;
constexpr std::size_t kMaxEntries = 32;
constexpr std::size_t kMaxOptions = 16;
constexpr std::size_t kMaxAnnouncements = 8;

// The most players a room may claim, and the most rows a table may show at once.
constexpr int kMaxPlayers = 64;
constexpr int kMaxRows = 32;

// The share of a table's width, in percent, that all its columns may take.
constexpr int kFullWidth = 100;

// Why a description was refused, carried out of the readers below.
class Refused : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Throws Refused with a message in the form of the log's (section 17.3).
[[noreturn]] void refuse(std::string message) {
    throw Refused(message);
}

// Refuses `object` (at `where`) when it is not an object or has a key outside `allowed`.
void check_keys(
    const json::Value& object,
    std::string_view where,
    std::initializer_list<std::string_view> allowed
) {
    // Every part of a description that has keys is an object.
    if (!object.is_object()) {
        refuse(std::format("{} is not an object", where));
    }

    // Every key is checked, so a typo is refused as well as a newer schema's key.
    for (const auto& [key, value] : object.members()) {
        // A key this schema does not know is a newer schema's, which this client cannot show.
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            refuse(std::format("{} has an unknown key {}", where, key));
        }
    }
}

// Returns the string `key` of `object`, refused when it is not a string or longer than `limit`.
// An absent key reads as empty unless it is `required`.
std::string read_text(
    const json::Value& object, std::string_view key, std::size_t limit, bool required = false
) {
    const json::Value& value = object[key];

    // An absent optional text is empty.
    if (value.is_null() && !required) {
        return {};
    }

    // A present text must be a string the screen can hold.
    if (!value.is_string() || value.string().size() > limit) {
        refuse(std::format("{} is not a string of at most {} bytes", key, limit));
    }

    return value.string();
}

// Returns the whole number `key` of `object` in [low, high]; absent reads as `fallback`.
int read_int(const json::Value& object, std::string_view key, int low, int high, int fallback) {
    const json::Value& value = object[key];

    // An absent number takes the documented fallback.
    if (value.is_null()) {
        return fallback;
    }

    const double number = value.number(static_cast<double>(low) - 1.0);

    // Out of range, not a number or not whole: all the same fault.
    if (!value.is_number() || number < low || number > high
        || number != static_cast<double>(static_cast<int>(number))) {
        refuse(std::format("{} is not a whole number from {} to {}", key, low, high));
    }

    return static_cast<int>(number);
}

// Returns whether `name` is an identifier: 1 to kMaxName of a-z, 0-9 and underscores.
bool is_identifier(std::string_view name) {
    const auto allowed = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    };
    return !name.empty() && name.size() <= kMaxName
           && std::all_of(name.begin(), name.end(), allowed);
}

// Returns the identifier `key` of `object`, refused when it is missing or malformed.
std::string read_identifier(const json::Value& object, std::string_view key) {
    std::string name = read_text(object, key, kMaxName, true);

    // Identifiers name screens, fields and options on the wire and in the binding.
    if (!is_identifier(name)) {
        refuse(std::format("{} '{}' is not an identifier of a-z, 0-9 and _", key, name));
    }

    return name;
}

// Returns the array `key` of `object` (empty when absent), refused past `limit` items.
const std::vector<json::Value>& read_list(
    const json::Value& object, std::string_view key, std::size_t limit
) {
    const json::Value& value = object[key];

    // An absent list is empty.
    if (value.is_null()) {
        return value.items();
    }

    // A present list must be an array of bounded length.
    if (!value.is_array() || value.items().size() > limit) {
        refuse(std::format("{} is not a list of at most {} items", key, limit));
    }

    return value.items();
}

// Returns the strings of the list `key`, each at most `limit` bytes.
std::vector<std::string> read_texts(
    const json::Value& object, std::string_view key, std::size_t count, std::size_t limit
) {
    std::vector<std::string> texts;

    // Every text of the list, in order.
    for (const json::Value& item : read_list(object, key, count)) {
        // Every entry of a list of texts is a bounded string.
        if (!item.is_string() || item.string().size() > limit) {
            refuse(
                std::format("{} holds an item that is not a string of at most {} bytes", key, limit)
            );
        }

        texts.push_back(item.string());
    }

    return texts;
}

// Reads `object`'s action text at `key`; absent is no action.
Action read_action(const json::Value& object, std::string_view key) {
    const std::string text = read_text(object, key, kMaxName + kMaxName);

    // No action: the component is shown only.
    if (text.empty()) {
        return {};
    }

    std::optional<Action> action = parse_action(text);

    // A server may only ask for the closed set of actions.
    if (!action) {
        refuse(std::format("action '{}' is not back, open:, game: or server:", text));
    }

    return *action;
}

// Reads one column of a table.
Column read_column(const json::Value& object) {
    check_keys(object, "a column", {"field", "label", "width"});
    return Column{
        read_identifier(object, "field"),
        read_text(object, "label", kMaxLabel),
        read_int(object, "width", 1, kFullWidth, 0)
    };
}

// Reads one item of a screen.
Item read_item(const json::Value& object) {
    check_keys(object, "an item", {"kind", "label", "action", "key", "choices", "value"});
    const std::string kind = read_text(object, "kind", kMaxName, true);
    Item item;
    item.label = read_text(object, "label", kMaxText);
    item.action = read_action(object, "action");
    item.choices = read_texts(object, "choices", kMaxChoices, kMaxLabel);

    // The kind decides which of the other keys mean something.
    if (kind == "text") {
        item.kind = ItemKind::Text;
    } else if (kind == "button") {
        item.kind = ItemKind::Button;
    } else if (kind == "choice") {
        item.kind = ItemKind::Choice;
        item.key = read_identifier(object, "key");

        // A choice with nothing to choose cannot be stepped.
        if (item.choices.empty()) {
            refuse(std::format("choice {} has no choices", item.key));
        }

        item.value = read_int(object, "value", 0, static_cast<int>(item.choices.size()) - 1, 0);
    } else if (kind == "table") {
        item.kind = ItemKind::Table;
        item.key = read_identifier(object, "key");
    } else {
        refuse(std::format("item kind '{}' is not text, button, choice or table", kind));
    }

    return item;
}

// Reads one screen, its columns and items.
ScreenSpec read_screen(const json::Value& object) {
    check_keys(object, "a screen", {"id", "title", "rows", "columns", "items"});
    ScreenSpec screen;
    screen.id = read_identifier(object, "id");
    screen.title = read_text(object, "title", kMaxTitle);
    screen.rows = read_int(object, "rows", 0, kMaxRows, 0);
    int width = 0;

    // The columns' widths add up across the table.
    for (const json::Value& column : read_list(object, "columns", kMaxColumns)) {
        screen.columns.push_back(read_column(column));
        width += screen.columns.back().width;
    }

    // Columns past the table's width would draw over its edge.
    if (width > kFullWidth) {
        refuse(std::format("screen {} has columns {}% wide", screen.id, width));
    }

    // Each item in the order the screen stacks them.
    for (const json::Value& item : read_list(object, "items", kMaxItems)) {
        screen.items.push_back(read_item(item));
    }

    const auto tables =
        std::count_if(screen.items.begin(), screen.items.end(), [](const Item& item) {
            return item.kind == ItemKind::Table;
        });

    // The columns belong to the screen's one table.
    if (tables > 1) {
        refuse(std::format("screen {} has {} tables", screen.id, tables));
    }

    return screen;
}

// Reads a mode or a map.
Entry read_entry(const json::Value& object, bool is_map) {
    check_keys(object, is_map ? "a map" : "a mode", {"id", "name", "base", "modes"});
    Entry entry;
    entry.id = read_int(object, "id", 0, 0xFFFF, -1);
    entry.name = read_text(object, "name", kMaxLabel, true);
    entry.base = read_int(object, "base", 0, 0xFFFF, -1);

    // Every entry is named by the game's own number.
    if (entry.id < 0) {
        refuse(std::format("{} '{}' has no id", is_map ? "map" : "mode", entry.name));
    }

    // A map lists the modes it can be played in.
    for (const json::Value& mode : read_list(object, "modes", kMaxEntries)) {
        // A map's modes are the game's mode numbers.
        if (!is_map || !mode.is_number()) {
            refuse(std::format("entry '{}' has modes that are not numbers of a map", entry.name));
        }

        entry.modes.push_back(static_cast<int>(mode.number()));
    }

    return entry;
}

// Reads a server option.
Option read_option(const json::Value& object) {
    check_keys(object, "an option", {"key", "label", "choices", "value", "scope"});
    Option option;
    option.key = read_identifier(object, "key");
    option.label = read_text(object, "label", kMaxLabel, true);
    option.choices = read_texts(object, "choices", kMaxChoices, kMaxLabel);

    // An option is a choice; with no values it has nothing to show.
    if (option.choices.empty()) {
        refuse(std::format("option {} has no choices", option.key));
    }

    option.value = read_int(object, "value", 0, static_cast<int>(option.choices.size()) - 1, 0);
    const std::string scope = read_text(object, "scope", kMaxName);

    // A game's rule is the host's to set; anything else is the player's own.
    if (scope == "game") {
        option.scope = OptionScope::Game;
    } else if (!scope.empty() && scope != "server") {
        refuse(std::format("option {} has scope '{}', not server or game", option.key, scope));
    }

    return option;
}

// Reads the whole description from its parsed top value.
Description read_description(const json::Value& top) {
    check_keys(
        top,
        "the description",
        {"schema", "name", "revision", "limits", "text", "modes", "maps", "options", "screens"}
    );

    // A description of another schema means keys this client reads otherwise.
    if (read_int(top, "schema", 0, 0xFFFF, 0) != kSchemaVersion) {
        refuse(std::format("schema is not {}", kSchemaVersion));
    }

    Description description;
    description.name = read_text(top, "name", kMaxTitle);
    description.revision = read_int(top, "revision", 0, 0x7FFFFFFF, 0);

    // Limits and text are optional blocks of their own.
    if (!top["limits"].is_null()) {
        check_keys(top["limits"], "limits", {"max_players"});
        description.max_players = read_int(top["limits"], "max_players", 1, kMaxPlayers, 0);
    }

    // The lobby's text block.
    if (!top["text"].is_null()) {
        check_keys(top["text"], "text", {"motd", "announcements"});
        description.motd = read_text(top["text"], "motd", kMaxText);
        description.announcements =
            read_texts(top["text"], "announcements", kMaxAnnouncements, kMaxText);
    }

    // The modes the server offers, in its order.
    for (const json::Value& mode : read_list(top, "modes", kMaxEntries)) {
        description.modes.push_back(read_entry(mode, false));
    }

    // The maps the server offers, in its order.
    for (const json::Value& map : read_list(top, "maps", kMaxEntries)) {
        description.maps.push_back(read_entry(map, true));
    }

    // The server's own options, beside the game's.
    for (const json::Value& option : read_list(top, "options", kMaxOptions)) {
        description.options.push_back(read_option(option));
    }

    // The screens, each checked whole before it is kept.
    for (const json::Value& screen : read_list(top, "screens", kMaxScreens)) {
        ScreenSpec spec = read_screen(screen);

        // Screens are found by id, so two with one id would hide one.
        if (description.screen(spec.id) != nullptr) {
            refuse(std::format("screen {} is described twice", spec.id));
        }

        description.screens.push_back(std::move(spec));
    }

    return description;
}

}  // namespace

const ScreenSpec* Description::screen(std::string_view id) const {
    auto it = std::find_if(screens.begin(), screens.end(), [id](const ScreenSpec& spec) {
        return spec.id == id;
    });
    return it == screens.end() ? nullptr : &*it;
}

std::optional<Action> parse_action(std::string_view text) {
    // Back takes no target.
    if (text == "back") {
        return Action{ActionKind::Back, {}};
    }

    const std::size_t colon = text.find(':');

    // Every other action is `kind:target`.
    if (colon == std::string_view::npos || !is_identifier(text.substr(colon + 1))) {
        return std::nullopt;
    }

    const std::string_view kind = text.substr(0, colon);
    const std::string target(text.substr(colon + 1));

    // The three kinds that carry a target.
    if (kind == "open") {
        return Action{ActionKind::Open, target};
    }

    // The game binding's own actions.
    if (kind == "game") {
        return Action{ActionKind::Game, target};
    }

    // Requests that go to the server.
    if (kind == "server") {
        return Action{ActionKind::Server, target};
    }

    return std::nullopt;
}

Result<Description> parse_description(std::string_view json) {
    // A description larger than this is refused before it is parsed.
    if (json.size() > kMaxDescriptionBytes) {
        return unexpected(
            Error{std::format(
                "description is {} bytes, at most {}", json.size(), kMaxDescriptionBytes
            )}
        );
    }

    json::Value top;
    std::string error;

    // A server's text that is not JSON at all.
    if (!json::parse(json, top, error, json::DuplicateKeys::Reject)) {
        return unexpected(Error{std::format("description is not JSON: {}", error)});
    }

    // The readers throw Refused at the first fault; the whole description goes with it.
    try {
        return read_description(top);
    } catch (const Refused& refused) {
        return unexpected(Error{std::format("description refused: {}", refused.what())});
    }
}

}  // namespace openrac::mpui
