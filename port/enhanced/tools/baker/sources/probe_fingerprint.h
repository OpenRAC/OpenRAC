// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe cache identities shared by bake preparation and runtime activation.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

#include "enhanced/lighting/bake.h"

namespace openrac::scene {
struct LevelData;
}

namespace openrac::lighting {

// Exported transport and source statistics, owned by the caller.
struct BakeSceneExport;

// Per-level content identities shared by the coordinator and its batch; never inferred from mtimes.
struct ProbeSourceHashes {
    std::optional<std::uint64_t> initial;   // Before native interior preparation.
    std::optional<std::uint64_t> verified;  // Snapshot checked before tracing and after output.
};

// Fingerprints canonical inputs; file errors propagate to the tool or activation error boundary.
[[nodiscard]] std::uint64_t probe_input_hash(const std::filesystem::path& directory);

// Fingerprints source bytes for levels, or structural transport for in-memory fixtures.
// `liquid_source` marks automatic liquid interpretation independently of whether it was analysed.
[[nodiscard]] std::uint64_t probe_scene_hash(
    const scene::LevelData& level,
    const BakeSceneExport& out,
    unsigned light_set,
    bool liquid_source
);

// Fingerprints all solver and allocation settings that affect the stored probe data.
[[nodiscard]] std::uint64_t probe_bake_settings_hash(
    const ProbeGrid& grid, const BakeSettings& settings
);

}  // namespace openrac::lighting
