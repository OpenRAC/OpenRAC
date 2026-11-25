// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adds the fragment clamp used by normalized colour targets without changing shader interfaces.

#pragma once
#include <optional>
#include <regex>
#include <string>

namespace openrac::vkgl::shader_transforms {
inline std::optional<std::string> clamped_fragment_source(const std::string& source) {
    static const std::regex output(
        R"(layout\s*\(\s*location\s*=\s*0\s*\)\s*out\s+vec4\s+([A-Za-z_][A-Za-z_0-9]*)\s*;)"
    );
    static const std::regex entry(R"(\bvoid\s+main\s*\(\s*(void)?\s*\))");
    // Or its one output without a location (location 0 then).
    static const std::regex lone(R"((^|\n)\s*out\s+vec4\s+([A-Za-z_][A-Za-z_0-9]*)\s*;)");
    std::smatch match;
    std::string colour;

    if (std::regex_search(source, match, output)) {
        colour = match[1];
    } else {
        // Only when there is exactly one.
        const std::sregex_iterator first(source.begin(), source.end(), lone), end;

        if (first != end && std::next(first) == end) {
            colour = (*first)[2];
        }
    }

    if (colour.empty() || !std::regex_search(source, entry)) {
        return std::nullopt;
    }

    // The program's own main runs first, then its colour is clamped.
    std::string fs = std::regex_replace(source, entry, "void openrac_unclamped_main()");
    fs += "\nvoid main() {\n    openrac_unclamped_main();\n    " + colour + " = clamp(" + colour
          + ", 0.0, 1.0);\n}\n";
    return fs;
}
}  // namespace openrac::vkgl::shader_transforms
