// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu's Files tab: a row for each folder OpenRAC keeps the player's files in
// (program/file_places.h), with how many files it holds. A press opens the folder in the system's
// file manager (the Files app on Android); when nothing can open it, its path is copied instead
// and a notification says where it is. This is the only place in the game that opens a folder.

#include <chrono>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <string>
#include <system_error>
#include <vector>

#include <SDL3/SDL.h>

#include "debug/menu_ui.h"
#include "debug/overlay.h"
#include "platform/show_folder.h"
#include "program/file_places.h"

namespace openrac::frontend::debug_overlay {

namespace {

struct FileRow {
    program::FilePlace place;
    std::string path;   // UTF-8, whatever the player's name
    std::string count;  // "12 files", "Empty"
};

std::vector<FileRow> rows;
double read_at = -10.0;  // ImGui's time the rows were last read

std::string utf8(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

std::string count_of(const std::filesystem::path& folder) {
    std::error_code e;
    int files = 0;
    for (std::filesystem::directory_iterator it(folder, e), end; !e && it != end; it.increment(e)) {
        ++files;
        if (files > 999) {
            break;
        }
    }
    if (files == 0) {
        return "Empty";
    }
    return files > 999 ? std::string("1000+ files")
                       : std::format("{} {}", files, files == 1 ? "file" : "files");
}

// The places again, at most once a second: a screenshot taken meanwhile shows in its count.
void refresh() {
    const double now = ImGui::GetTime();
    if (now - read_at < 1.0 && !rows.empty()) {
        return;
    }
    read_at = now;
    rows.clear();
    for (program::FilePlace& place : program::file_places()) {
        FileRow row{.path = utf8(place.folder), .count = count_of(place.folder)};
        row.place = std::move(place);
        rows.push_back(std::move(row));
    }
}

void open(const FileRow& row) {
    std::error_code ignored;
    std::filesystem::create_directories(row.place.folder, ignored);
    if (platform::show_folder(row.place.folder)) {
        return;
    }
    const bool copied = SDL_SetClipboardText(row.path.c_str());
    notify({
        .id = "files.open",
        .title = "This folder cannot be opened here",
        .text = row.path,
        .where = copied ? "Its path is copied" : std::string(),
        .tone = Tone::Warning,
        .seconds = 8.0,
    });
}

}  // namespace

void file_places_panel() {
    refresh();
    const int entry = static_cast<int>(menu::Id::FilePlaces);
    const char* section = menu::entry(menu::Id::FilePlaces).section;
    for (const FileRow& row : rows) {
        const bool pressed = menu_row(
            row.place.title.c_str(),
            ui().text,
            [&](ImDrawList* list, ImVec2 right) {
                const ImVec2 size = ImGui::CalcTextSize(row.count.c_str());
                list->AddText(
                    ImVec2(right.x - size.x, right.y - size.y * 0.5f), ui().dim, row.count.c_str()
                );
            },
            0.0f,
            row.place.id.c_str()
        );
        note_hover_row(entry, section, row.place.title, row.place.help + "\n\n" + row.path);
        if (pressed) {
            open(row);
        }
    }
}

}  // namespace openrac::frontend::debug_overlay
