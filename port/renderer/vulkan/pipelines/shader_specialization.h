// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reserved specialization IDs shared by shader assembly and pipeline creation, without Vulkan headers.
#pragma once
#include <cstddef>
#include <cstdint>

namespace openrac::vkgl::detail {
inline constexpr std::uint32_t kClipZeroToOneConstant = 900;
inline constexpr std::uint32_t kMaterialTextureCapacityConstant = 901;
inline constexpr std::uint32_t kModeConstantBase = 1000;
inline constexpr std::size_t kModeConstants = 47;
}  // namespace openrac::vkgl::detail
