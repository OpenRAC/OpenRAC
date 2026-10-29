// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The controls files (platform/controls_file.h) and the store that layers them over the defaults
// (game/common/controls_store.h): lines kept, a newer format left alone, the global file under the
// game's, every reset, the move of a settings file's bindings, and a file changed on disk.

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "controls_store.h"
#include "platform/controls_file.h"
#include "platform/players.h"
#include "tests/check.h"

namespace {

namespace fs = std::filesystem;
using openrac::controls::Scope;
using openrac::controls::Store;
using openrac::platform::Button;
using openrac::platform::ControlsFile;
using openrac::platform::Layer;

std::string read_file(const fs::path& path) {
    std::ifstream in(path);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

// A change rewrites its own line; comments, unknown keys and the order stay.
void test_lines_are_kept() {
    ControlsFile file = ControlsFile::parse(
        "# mine\nformat = 1\nbind_cross = PadEast\nlauncher_note = keep me\n"
        "bind_cross = PadNorth\n\nbind_l1 = PadMisc1\n"
    );
    CHECK(file.get("bind_cross") == "PadNorth");  // the last line wins, as the settings file's
    file.set("bind_cross", "PadWest");
    CHECK(
        file.text()
        == "# mine\nformat = 1\nlauncher_note = keep me\nbind_cross = PadWest\n\nbind_l1 = "
           "PadMisc1\n"
    );
    CHECK(file.erase("bind_l1"));
    CHECK(!file.erase("bind_l1"));
    CHECK(file.values().size() == 3);
    CHECK(file.erase_prefix("bind_") == 1);
    CHECK(file.get("launcher_note") == "keep me");

    // A new file gets its format line first.
    ControlsFile fresh;
    CHECK(fresh.empty());
    fresh.set("bind_cross", "PadEast");
    CHECK(fresh.format() == openrac::platform::kControlsFormat);
    CHECK(!fresh.empty());
    CHECK(ControlsFile::parse(fresh.text()).get("bind_cross") == "PadEast");
    // Windows' line ends read as any other.
    CHECK(ControlsFile::parse("format = 1\r\nbind_l1 = PadEast\r\n").get("bind_l1") == "PadEast");
}

// A newer build's file is read for what this build knows, and never written.
void test_newer_format_is_read_only() {
    ControlsFile file = ControlsFile::parse("format = 9\nbind_cross = PadEast\nfuture = x\n");
    CHECK(file.read_only());
    CHECK(file.get("bind_cross") == "PadEast");
    const std::string before = file.text();
    file.set("bind_cross", "PadWest");
    CHECK(!file.erase("future"));
    CHECK(file.text() == before);
}

// The game's file over the global one over the defaults, each kind of each control on its own,
// with where each came from; resets put back what was inherited and nothing else.
void test_layers_and_resets(const fs::path& dir) {
    fs::create_directories(dir);
    std::ofstream(dir / "global.ini") << "format = 1\nbind_cross = KeyJ,PadEast\n"
                                         "pad_left_deadzone = 10\nplayer2 = auto\n";
    std::ofstream(dir / "rac1.ini") << "format = 1\nbind_cross = PadNorth\nbind_select = none\n";
    Store store;
    int applied = 0;
    store.open(
        dir,
        "rac1",
        [&applied](const openrac::platform::Bindings&, const openrac::platform::Players&) {
            ++applied;
        }
    );
    CHECK(applied == 1);

    const auto& cross = store.bindings().of(Button::Cross);
    CHECK(cross.key_count == 1 && cross.keys[0] == SDL_SCANCODE_J);  // the global's key
    CHECK(cross.pad_count == 1 && cross.pad[0].input.value == SDL_GAMEPAD_BUTTON_NORTH);
    const int c = openrac::platform::target_of(Button::Cross);
    CHECK(store.origins().keys[static_cast<std::size_t>(c)] == Layer::Global);
    CHECK(store.origins().pad[static_cast<std::size_t>(c)] == Layer::Game);
    const int sq = openrac::platform::target_of(Button::Square);
    CHECK(store.origins().pad[static_cast<std::size_t>(sq)] == Layer::Default);
    CHECK(store.bindings().of(Button::Select).pad_count == 0);  // unbound in the game
    CHECK(store.bindings().tuning.sticks[0].deadzone == 10);
    CHECK(store.origins().tuning.at("pad_left_deadzone") == Layer::Global);

    // A change is written and applied at once.
    CHECK(store.set(Scope::Game, "bind_r1", "PadMisc1"));
    CHECK(applied == 2);
    CHECK(read_file(dir / "rac1.ini").find("bind_r1 = PadMisc1\n") != std::string::npos);
    CHECK(store.bindings().of(Button::R1).pad[0].input.value == SDL_GAMEPAD_BUTTON_MISC1);

    // One control back to what it inherits.
    CHECK(store.erase(Scope::Game, "bind_cross"));
    CHECK(store.bindings().of(Button::Cross).pad[0].input.value == SDL_GAMEPAD_BUTTON_EAST);

    // The game back to the global controls: its file goes, the global one stays.
    CHECK(store.reset(Scope::Game));
    CHECK(!fs::exists(dir / "rac1.ini"));
    CHECK(store.bindings().of(Button::Select).pad_count == 1);
    CHECK(store.bindings().of(Button::Cross).keys[0] == SDL_SCANCODE_J);

    // The global back to the defaults: its bindings and settings go, the players stay.
    CHECK(store.reset(Scope::Global));
    const std::string global = read_file(dir / "global.ini");
    CHECK(global.find("bind_cross") == std::string::npos);
    CHECK(global.find("pad_left_deadzone") == std::string::npos);
    CHECK(global.find("player2 = auto") != std::string::npos);
    CHECK(store.bindings().of(Button::Cross).keys[0] == SDL_SCANCODE_SPACE);
    CHECK(store.bindings().tuning.sticks[0].deadzone == 0);
}

// The bindings the version's settings file held move into the game's file, unless it has them.
void test_adopt(const fs::path& dir) {
    fs::create_directories(dir);
    std::ofstream(dir / "rac2.ini") << "format = 1\nbind_cross = PadEast\n";
    Store store;
    store.open(dir, "rac2", {});
    const int moved = store.adopt({
        {"bind_cross", "PadWest"},  // the game's file has it: it stays as the file says
        {"bind_l1", "KeyQ,PadMisc1"},
        {"game_language", "2"},  // not a binding
    });
    CHECK(moved == 1);
    CHECK(store.file(Scope::Game).get("bind_cross") == "PadEast");
    CHECK(store.file(Scope::Game).get("bind_l1") == "KeyQ,PadMisc1");
    CHECK(!store.file(Scope::Game).get("game_language"));
    CHECK(read_file(dir / "rac2.ini").find("bind_l1 = KeyQ,PadMisc1") != std::string::npos);
    CHECK(store.bindings().of(Button::L1).keys[0] == SDL_SCANCODE_Q);
}

// A file changed beside the game (the launcher) is read again, at most once a second.
void test_poll(const fs::path& dir) {
    fs::create_directories(dir);
    Store store;
    store.open(dir, "rac3", {});
    CHECK(!store.poll(10.0));  // nothing changed
    std::ofstream(dir / "rac3.ini") << "format = 1\nbind_cross = PadEast\n";
    CHECK(!store.poll(10.5));  // too soon
    CHECK(store.poll(11.5));
    CHECK(store.bindings().of(Button::Cross).pad[0].input.value == SDL_GAMEPAD_BUTTON_EAST);
    CHECK(!store.poll(13.0));
    // The file removed: back to the defaults.
    fs::remove(dir / "rac3.ini");
    CHECK(store.poll(14.0));
    CHECK(store.bindings().of(Button::Cross).pad[0].input.value == SDL_GAMEPAD_BUTTON_SOUTH);
}

// A newer build's file in the store: read, not written over.
void test_store_keeps_a_newer_file(const fs::path& dir) {
    fs::create_directories(dir);
    std::ofstream(dir / "rac4.ini") << "format = 7\nbind_cross = PadEast\n";
    Store store;
    store.open(dir, "rac4", {});
    CHECK(store.bindings().of(Button::Cross).pad[0].input.value == SDL_GAMEPAD_BUTTON_EAST);
    CHECK(!store.set(Scope::Game, "bind_cross", "PadWest"));
    CHECK(!store.reset(Scope::Game));
    CHECK(fs::exists(dir / "rac4.ini"));
    CHECK(read_file(dir / "rac4.ini") == "format = 7\nbind_cross = PadEast\n");
}

using openrac::platform::Assignment;
using openrac::platform::Players;

// The defaults are the port's old order: the keyboard and the first controller player 1, the
// second player 2; a third is nobody's.
void test_default_players() {
    const Players players = openrac::platform::default_players();
    CHECK(openrac::platform::assign(players, {}) == (Assignment{{-1, -1}, 0}));
    const std::string one[] = {"a"};
    CHECK(openrac::platform::assign(players, one) == (Assignment{{0, -1}, 0}));
    const std::string three[] = {"a", "b", "c"};
    CHECK(openrac::platform::assign(players, three) == (Assignment{{0, 1}, 0}));
}

// A player's own controller is theirs whichever order it connected in, and comes back to them;
// "auto" takes what is left; the keyboard goes where it is named.
void test_named_players() {
    Players players;
    players.wants[0] = openrac::platform::read_player("keyboard, pad-b, auto");
    players.wants[1] = openrac::platform::read_player("pad-a");
    const std::string both[] = {"pad-a", "pad-b"};
    CHECK(openrac::platform::assign(players, both) == (Assignment{{1, 0}, 0}));
    // Player 2's controller gone: player 2 has none (no "auto"), player 1 keeps theirs.
    const std::string b_only[] = {"pad-b"};
    CHECK(openrac::platform::assign(players, b_only) == (Assignment{{0, -1}, 0}));
    // Player 1's gone: their "auto" takes the one left, unless player 2 named it.
    const std::string a_c[] = {"pad-a", "pad-c"};
    CHECK(openrac::platform::assign(players, a_c) == (Assignment{{1, 0}, 0}));

    players.wants[0] = openrac::platform::read_player("none");
    players.wants[1] = openrac::platform::read_player("keyboard,auto");
    CHECK(openrac::platform::assign(players, both) == (Assignment{{-1, 0}, 1}));
    CHECK(openrac::platform::player_value(players.wants[0]) == "none");
    CHECK(openrac::platform::player_value(players.wants[1]) == "keyboard, auto");
}

// The same model twice without serials is told apart by order; a serial is part of the id.
void test_controller_ids() {
    CHECK(openrac::platform::controller_id("0300abcd", "", {}) == "0300abcd");
    const std::string one[] = {"0300abcd"};
    CHECK(openrac::platform::controller_id("0300abcd", "", one) == "0300abcd#2");
    const std::string two[] = {"0300abcd", "0300abcd#2"};
    CHECK(openrac::platform::controller_id("0300abcd", "", two) == "0300abcd#3");
    CHECK(openrac::platform::controller_id("0300abcd", "SN1", one) == "0300abcd-SN1");
    // The first gone, the next of the model takes its id back.
    const std::string second[] = {"0300abcd#2"};
    CHECK(openrac::platform::controller_id("0300abcd", "", second) == "0300abcd");
}

// The global file's players are the store's; a game's file does not choose them.
void test_store_players(const fs::path& dir) {
    fs::create_directories(dir);
    std::ofstream(dir / "global.ini") << "format = 1\nplayer2 = keyboard, pad-x\n";
    std::ofstream(dir / "rac1.ini") << "format = 1\nplayer1 = none\n";
    Store store;
    Players applied;
    store.open(dir, "rac1", [&applied](const openrac::platform::Bindings&, const Players& players) {
        applied = players;
    });
    CHECK(applied.wants[0] == std::vector<std::string>({"keyboard", "auto"}));
    CHECK(applied.wants[1] == std::vector<std::string>({"keyboard", "pad-x"}));
    store.remember({{"pad-x", "Pad X", 1}, {"pad-y", "Pad Y", -1}});
    store.remember({{"pad-y", "Pad Y", 0}});
    const std::string devices = read_file(dir / "devices.ini");
    CHECK(devices.find("pad-x = Pad X") != std::string::npos);
    CHECK(devices.find("pad-y = Pad Y") != std::string::npos);
}

}  // namespace

int main() {
    const fs::path dir = fs::temp_directory_path() / "openrac_controls_test";
    std::error_code error;
    fs::remove_all(dir, error);
    test_lines_are_kept();
    test_newer_format_is_read_only();
    test_layers_and_resets(dir / "layers");
    test_adopt(dir / "adopt");
    test_poll(dir / "poll");
    test_store_keeps_a_newer_file(dir / "newer");
    test_default_players();
    test_named_players();
    test_controller_ids();
    test_store_players(dir / "players");
    fs::remove_all(dir, error);
    return openrac::test::result();
}
