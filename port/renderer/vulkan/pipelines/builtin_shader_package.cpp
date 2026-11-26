// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Validates and exposes the built-in package before concurrent shader lookup begins.

#include <cstddef>
#include <stdexcept>

#include "renderer/vulkan/pipelines/shader_package.h"

// The generated package: libopenrac-shaders beside the programs, or built in (iOS).
extern "C" const char* openrac_shader_package(std::size_t* size);

namespace openrac::vkgl {
std::span<const char> embedded_shader_package() {
    std::size_t size = 0;
    const char* data = openrac_shader_package(&size);
    return {data, size};
}

const ShaderPackage& builtin_shader_package() {
    static const ShaderPackage package = [] {
        ShaderPackage out;
        std::string error;

        if (embedded_shader_package().empty()) {
            return out;
        }

        if (!out.load(embedded_shader_package(), shader_package_fingerprint(), error)) {
            throw std::runtime_error(error);
        }

        return out;
    }();
    return package;
}
}  // namespace openrac::vkgl
