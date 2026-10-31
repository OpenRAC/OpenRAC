// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Read-only emission inventory over decoded terrain, ties and shrubs; brightness never creates emitters.
#pragma once
#include <cstddef>
#include <filesystem>

#include "common/result.h"

namespace openrac::scene {
struct LevelData;
}

namespace openrac::lighting {
// Writes emission-audit.json from the resident scene without re-reading mesh buffers or image pixels.
// Returns undefined candidate texture count; publication and malformed source errors stop the batch.
[[nodiscard]] Result<std::size_t> audit_level_emission(
    const scene::LevelData& level, const std::filesystem::path& output_directory
);
}  // namespace openrac::lighting
