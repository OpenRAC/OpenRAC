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
    store.open(dir, "rac1", [&applied](const openrac::platform::Bindings&) { ++applied; });
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
    fs::remove_all(dir, error);
    return openrac::test::result();
}
