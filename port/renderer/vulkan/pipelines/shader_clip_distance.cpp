// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Replaces clip-distance outputs for Vulkan devices that cannot consume them.

#include <regex>

#include "renderer/vulkan/pipelines/shader_backend.h"

namespace openrac::vkgl::compiler_detail {
std::string vertex_clip_distance(std::string_view source, bool clip_distance) {
    std::string out(source);

    if (out.find("gl_ClipDistance") == std::string::npos || clip_distance) {
        return out;
    }

    static const std::regex clip(R"(\bgl_ClipDistance\b)");
    static const std::regex main_decl(R"(\bvoid\s+main\s*\()");
    out = std::regex_replace(out, clip, "openrac_clip_distance");
    std::smatch at;

    // Declared before main, the one place the port's shaders write it.
    if (std::regex_search(out, at, main_decl)) {
        out.insert(static_cast<std::size_t>(at.position(0)), "float openrac_clip_distance[8];\n");
    }

    return out;
}
}  // namespace openrac::vkgl::compiler_detail
