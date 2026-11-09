// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <filesystem>
#include <string>

namespace openrac::lighting {
[[nodiscard]] std::string calculate_mesh_emission(const std::filesystem::path& level_dir);
void migrate_mesh_emission(
    const std::filesystem::path& level_dir, const std::filesystem::path& catalog_path
);
}  // namespace openrac::lighting
