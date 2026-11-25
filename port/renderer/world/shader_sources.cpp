// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// World source includes and feature preambles shared by runtime draws and build recipes.

#include "renderer/world/shader_sources.h"

namespace openrac::renderer::world {
std::string compose(std::string_view stage, std::initializer_list<std::string_view> parts) {
    const std::size_t eol = stage.find('\n');
    std::string out;

    if (eol == std::string_view::npos) {
        out.assign(stage);
        out += '\n';
    } else {
        out.assign(stage.substr(0, eol + 1));
    }

    for (const std::string_view part : parts) {
        out += part;
        out += '\n';
    }

    // Keep the stage's own line numbers recognisable in compiler logs.
    out += "#line 2\n";

    if (eol != std::string_view::npos) {
        out += stage.substr(eol + 1);
    }

    return out;
}

}  // namespace openrac::renderer::world
