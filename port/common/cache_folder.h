// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The folder the port keeps its caches in, between runs: what the graphics drivers made of its
// shaders (the Vulkan backend's pipelines and SPIR-V, OpenGL's program binaries). Nothing in it is
// needed: a cache that is gone is made again.

#pragma once

#include <filesystem>

namespace openrac {

// The system's folder for a program's caches, with openrac in it: ~/Library/Caches/OpenRAC on
// macOS, $HOME/cache on Android (the app's cache folder), else $XDG_CACHE_HOME/openrac, or
// .cache/openrac in the user's home (%USERPROFILE% on Windows). Not made here.
std::filesystem::path cache_folder();

}  // namespace openrac
