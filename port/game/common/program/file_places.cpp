// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "program/file_places.h"

#include <optional>
#include <string>
#include <system_error>

#include "common/env.h"
#include "openrac/game_host.h"
#include "program/options.h"
#include "program/program.h"

namespace openrac::program {

namespace {

// `name` in the folder the player's own files go: OpenRAC's data folder, or on Android the app's
// own files (HOME, which the launcher sets; FilesProvider.kt shows its folders by name).
std::filesystem::path own_folder(const char* name) {
#if defined(__ANDROID__)
    std::filesystem::path dir = name;
    if (const std::optional<std::string> home = env::system_variable("HOME");
        home && !home->empty()) {
        dir = std::filesystem::path(*home) / name;
    }
#else
    std::filesystem::path dir = data_home() / name;
#endif
    std::error_code ignored;
    std::filesystem::create_directories(dir, ignored);
    const std::filesystem::path absolute = std::filesystem::absolute(dir, ignored);
    return absolute.empty() ? dir : absolute;
}

}  // namespace

std::filesystem::path screenshots_dir() {
    return own_folder("screenshots");
}

std::filesystem::path recordings_dir() {
    return own_folder("perf");
}

std::vector<FilePlace> file_places() {
    const Options& o = g_options;
    std::vector<FilePlace> places;
    places.push_back({"screenshots", "Screenshots", "Pictures you took.", screenshots_dir()});
    places.push_back({"saves", "Saves", "This game's memory card.", o.cards});
    places.push_back(
        {"settings", "Settings", "Each game's settings file.", o.settings.parent_path()}
    );
    places.push_back({"controls", "Controls", "Your controls and shortcuts.", o.controls});
    places.push_back({"mod-files", "Mod files", "Each mod's own files.", o.mod_data});
    places.push_back(
        {"texture-packs",
         "Texture packs",
         "This game's texture packs.",
         o.data / "texture_packs" / openrac_game.id}
    );
    places.push_back({"mods", "Mods", "This game's mods, a folder each.", o.plugins.parent_path()});
    places.push_back(
        {"exports", "Exports", "Models, textures and sounds you exported.", o.data / "exports"}
    );
    places.push_back(
        {"overrides",
         "Overrides",
         "Replaced files the game reads over the disc's.",
         o.data / "overrides"}
    );
    places.push_back(
        {"logs",
         "Logs and crash reports",
         "The game's folder: a log each time you play, crash reports in crashes.",
         o.data}
    );
    places.push_back(
        {"recordings",
         "Diagnostics recordings",
         "Gameplay recordings and their reports.",
         recordings_dir()}
    );
    return places;
}

}  // namespace openrac::program
