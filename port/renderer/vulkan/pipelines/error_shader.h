// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Packaged error graphics, with a matching vertex program when one is available.
#pragma once
#include "renderer/vulkan/pipelines/shader_package.h"

namespace openrac::vkgl {
// Reports whether this executable includes a compiled checkerboard program.
[[nodiscard]] bool error_shader_available();
// Preserves vertex code and uniform offsets; removes bindings used only by the old fragment.
[[nodiscard]] bool load_error_shader(std::string_view vertex, CompiledProgram& out);
}  // namespace openrac::vkgl
