// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared source transformation for the vertex-only draw integer push constant.
#pragma once
#include <optional>
#include <regex>
#include <string>

namespace openrac::vkgl::shader_transforms {
inline std::string draw_uniform_name(const std::string& vertex) {
    static const std::regex marker(R"(#define\s+OPENRAC_DRAW_INT\s+([A-Za-z_][A-Za-z_0-9]*))");
    std::smatch match;
    return std::regex_search(vertex, match, marker) ? match[1].str() : std::string{};
}

// Cross-stage uniforms must retain their shared UBO; this transformation is vertex-only.
inline std::optional<std::string> draw_uniform_source(
    const std::string& vertex, const std::string& fragment, const std::string& name
) {
    const std::regex declaration("uniform\\s+int\\s+" + name + R"(\s*;)");

    if (name.empty() || std::regex_search(fragment, declaration)
        || !std::regex_search(vertex, declaration)) {
        return std::nullopt;
    }

    return std::regex_replace(
        vertex, declaration, "layout(push_constant) uniform OpenracDrawInt { int " + name + "; };"
    );
}
}  // namespace openrac::vkgl::shader_transforms
