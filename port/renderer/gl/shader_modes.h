// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Authored shader feature markers and scalar declarations shared by OpenGL and Vulkan variants.

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openrac::renderer::shader_modes {

// The marker of a shader's mode uniforms (pipeline variants): a comment line
// "// openrac_mode: name name ...". A mode is a scalar `uniform int` that picks the code a draw
// runs and keeps its value over many draws (a pass, a setting, a level).
inline constexpr std::string_view kModeMarker = "// openrac_mode:";

// Returns whether `c` may be part of a GLSL name.
inline bool glsl_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Adds the names `source` marks as modes to `names`, each once, in the order named.
inline void append_names(const std::string& source, std::vector<std::string>& names) {
    for (std::size_t at = source.find(kModeMarker); at != std::string::npos;
         at = source.find(kModeMarker, at + 1)) {
        std::size_t i = at + kModeMarker.size();

        // Each name up to the line's end, between spaces or commas.
        while (i < source.size() && source[i] != '\n') {
            if (!glsl_name_char(source[i])) {
                ++i;
                continue;
            }

            const std::size_t from = i;
            while (i < source.size() && glsl_name_char(source[i])) {
                ++i;
            }

            std::string name = source.substr(from, i - from);
            if (std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(std::move(name));
            }
        }
    }
}

// Finds the declaration `uniform int <name>;` in `source` (any whitespace between); its start
// and the place past its semicolon, or npos twice.
inline std::pair<std::size_t, std::size_t> declaration(
    const std::string& source, std::string_view name
) {
    constexpr std::string_view keyword = "uniform";
    constexpr std::string_view integer = "int";
    const auto space = [&](std::size_t i) {
        return i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0;
    };

    for (std::size_t at = source.find(keyword); at != std::string::npos;
         at = source.find(keyword, at + 1)) {
        std::size_t i = at + keyword.size();
        if ((at > 0 && glsl_name_char(source[at - 1])) || !space(i)) {
            continue;
        }
        while (space(i)) {
            ++i;
        }
        if (source.compare(i, integer.size(), integer) != 0 || !space(i + integer.size())) {
            continue;
        }
        i += integer.size();
        while (space(i)) {
            ++i;
        }
        if (source.compare(i, name.size(), name) != 0) {
            continue;
        }
        i += name.size();
        while (space(i)) {
            ++i;
        }
        if (i < source.size() && source[i] == ';') {
            return {at, i + 1};
        }
    }

    return {std::string::npos, std::string::npos};
}

}  // namespace openrac::renderer::shader_modes
