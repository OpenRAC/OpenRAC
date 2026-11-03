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
#else
    std::string url = "file://";
    if (!where.starts_with('/')) {
        url += '/';  // file:///C:/...
    }
    for (const char ch : where) {
        // Spaces and the URL's own marks, as %XX; the rest as it is.
        if (ch == ' ' || ch == '%' || ch == '#' || ch == '?') {
            url += std::format("%{:02X}", static_cast<unsigned char>(ch));
        } else {
            url += ch;
        }
    }
    return SDL_OpenURL(url.c_str());
#endif
}

}  // namespace openrac::platform
