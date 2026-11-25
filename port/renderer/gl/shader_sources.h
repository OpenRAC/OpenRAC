// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Backend-independent source transformations shared by runtime shaders and build recipes.

#pragma once
#include <string>
#include <string_view>

namespace openrac::renderer {
// Shared with the build tool; GL/ES retain their existing translation and precision path.
std::string shader_fragment_source(std::string_view source, bool enhanced);
std::string shader_enhanced_source(std::string_view source, bool enhanced);
}  // namespace openrac::renderer
