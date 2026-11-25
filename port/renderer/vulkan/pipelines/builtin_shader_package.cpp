// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Validates and exposes the executable-owned package before concurrent shader lookup begins.

#include <stdexcept>

#include "renderer/vulkan/pipelines/shader_package.h"

namespace openrac::vkgl {
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
