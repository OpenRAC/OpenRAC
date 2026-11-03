// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Opens the system's file browser at a folder: the desktop's (a file:// URL), or on Android the
// Files app at the app's own folders (GameActivity.showFiles, android/show_folder.cpp).
#pragma once

#include <filesystem>

namespace openrac::platform {

// Opens the folder `path` is, or holds when it is a file. False when nothing could open it.
bool show_folder(const std::filesystem::path& path);

}  // namespace openrac::platform
