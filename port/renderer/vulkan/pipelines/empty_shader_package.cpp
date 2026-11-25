// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Empty built-in package for builds with OPENRAC_SHADER_PACKAGE disabled.
#include "renderer/vulkan/pipelines/shader_package.h"

namespace openrac::vkgl {
std::span<const char> embedded_shader_package() {
    return {};
}
}  // namespace openrac::vkgl
