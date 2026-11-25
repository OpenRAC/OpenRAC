// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Atomic generated-file output and the build tool's persistent compilation cache.
#pragma once
#include <filesystem>
#include <span>
#include <vector>

#include "renderer/vulkan/pipelines/shader_package.h"

namespace openrac::shader_compiler {
// Publishes changed bytes atomically; identical files keep their timestamps.
void write_file(const std::filesystem::path& path, std::span<const char> bytes);
// Missing or damaged entries return no bytes and are compiled again.
[[nodiscard]] std::vector<char> read_cached_program(
    const std::filesystem::path& output, vkgl::ShaderKey key
);
// Publishes compressed compilation bytes under their compiler-specific source key.
void cache_program(
    const std::filesystem::path& output, vkgl::ShaderKey key, std::span<const char> bytes
);
}  // namespace openrac::shader_compiler
