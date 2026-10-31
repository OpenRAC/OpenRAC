// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned collision and authored transition inputs shared by native preparation stages.
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "common/result.h"
#include "enhanced/lighting/interiors.h"

namespace openrac::lighting {

// Hashes one source with bounded streaming memory, reporting read failures without a partial digest.
[[nodiscard]] Result<std::string> bake_source_identity(const std::filesystem::path& path);

// Immutable source identity and geometry retained for one level's interior preparation.
struct InteriorInput {
    std::string game;               // Combined game and version identity from the export.
    int level = -1;                 // Exported level number.
    std::string source_sha256;      // Gameplay bytes followed by the collision export bytes.
    std::string generation_sha256;  // Optional native stage identity for cached reports.
    std::vector<interiors::Triangle>
        triangles;  // World-space collision with support classification.
    std::vector<interiors::Transition>
        transitions;  // Authored lighting boundaries in source order.
};

// Hashes the retained gameplay and collision files without decoding their geometry.
[[nodiscard]] Result<std::string> interior_source_identity(
    const std::filesystem::path& level, const std::filesystem::path& gameplay
);

// Loads transformed collision geometry without gameplay or graphics resources.
[[nodiscard]] Result<std::vector<interiors::Triangle>> load_coverage_collision(
    const std::filesystem::path& level
);

// Loads collision and gameplay directly; reports malformed exports without launching another process.
[[nodiscard]] Result<InteriorInput> load_interior_input(
    const std::filesystem::path& level, const std::filesystem::path& gameplay
);

// Writes the collision diagnostic consumed by swimming-space planning until that consumer uses typed inputs.
[[nodiscard]] Result<void> write_interior_input(
    const std::filesystem::path& output, const InteriorInput& input
);

}  // namespace openrac::lighting
