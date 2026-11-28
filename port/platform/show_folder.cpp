// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/show_folder.h"

#include <format>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>

namespace openrac::platform {

#if defined(__ANDROID__)
bool android_show_files(const std::string& path);  // android/show_folder.cpp
#endif

std::string folder_target(const std::string& folder, bool windows) {
    if (windows) {
        std::string native = folder;
        for (char& ch : native) {
            ch = ch == '/' ? '\\' : ch;
        }
        return native;
    }
    std::string url = "file://";
    if (!folder.starts_with('/')) {
        url += '/';
    }
    for (const char ch : folder) {
        const auto byte = static_cast<unsigned char>(ch);
        const bool plain = (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z')
                           || (byte >= '0' && byte <= '9') || ch == '/' || ch == '-' || ch == '.'
                           || ch == '_' || ch == '~';
        url += plain ? std::string(1, ch) : std::format("%{:02X}", byte);
    }
    return url;
}

bool show_folder(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::path folder = std::filesystem::absolute(path, ignored);
    if (folder.empty()) {
        folder = path;
    }
    if (!std::filesystem::is_directory(folder, ignored)) {
        folder = folder.parent_path();
    }
    // UTF-8 whatever the user's name: path.string() throws on Windows for a letter outside the
    // system's code page.
    const std::u8string path8 = folder.generic_u8string();
    const std::string where(path8.begin(), path8.end());
#if defined(__ANDROID__)
    return android_show_files(where);
#elif defined(_WIN32)
    return SDL_OpenURL(folder_target(where, true).c_str());
#else
    return SDL_OpenURL(folder_target(where, false).c_str());
#endif
}

}  // namespace openrac::platform
