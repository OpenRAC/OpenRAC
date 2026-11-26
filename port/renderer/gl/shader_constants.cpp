// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Replaces exact integer uniform declarations with compile-time shader constants.

#include "renderer/gl/shader.h"

namespace openrac::renderer {
std::string shader_constants(std::string source, std::initializer_list<ShaderIntConstant> values) {
    for (const auto& [name, value] : values) {
        const std::string declaration = "uniform int " + std::string(name) + ";";
        const std::string constant =
            "const int " + std::string(name) + " = " + std::to_string(value) + ";";

        for (auto at = source.find(declaration); at != std::string::npos;
             at = source.find(declaration, at + constant.size())) {
            source.replace(at, declaration.size(), constant);
        }
    }

    return source;
}

}  // namespace openrac::renderer
