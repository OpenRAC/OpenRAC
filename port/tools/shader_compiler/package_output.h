// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Serializes compiled variants as the binary package, JSON manifest and embedded C++ source.
#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "renderer/vulkan/pipelines/shader_package.h"

namespace openrac::shader_compiler {
// One compressed result, or an alias/validation record sharing the final archive's blocks.
struct CompiledVariant {
    vkgl::ShaderKey key;      // Fingerprinted identity of the final source.
    std::string name;         // Manifest name, empty for aliases and validation markers.
    std::vector<char> bytes;  // Owned compressed compilation result.
    std::uint64_t raw_size;   // Decoded byte count.
};

// Source-key ordering makes archive publication deterministic across identical invocations.
using CompiledVariants = std::map<std::uint64_t, CompiledVariant>;

// Returns the binary package size; an output error throws to the command-line boundary.
[[nodiscard]] std::size_t write_package(
    const std::filesystem::path& output, const CompiledVariants& variants
);
}  // namespace openrac::shader_compiler
