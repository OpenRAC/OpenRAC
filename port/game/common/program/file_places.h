// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The folders OpenRAC writes the player's files to, in one list: the menu's Files tab shows them,
// and nothing else in the game opens a folder. The launcher's Settings > Files lists the same
// places (launcher/core/src/files.rs).

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace openrac::program {

struct FilePlace {
    std::string id;     // "screenshots"
    std::string title;  // "Screenshots"
    std::string help;   // one line on what is in it
    std::filesystem::path folder;
};

// The places of the running game, in the order the menu shows them.
std::vector<FilePlace> file_places();

// Where screenshots go: screenshots/ in OpenRAC's data folder (on Android the app's own files,
// which the Files app shows as "Screenshots"). Made if missing.
std::filesystem::path screenshots_dir();

// Where diagnostics recordings go: perf/ beside the screenshots. Made if missing.
std::filesystem::path recordings_dir();

// The line a notification adds under a saved file: where the player finds it.
inline constexpr const char* kFilesHint = "Find it in the menu, under Files";

}  // namespace openrac::program
