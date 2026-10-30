// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned probe-file payloads and unchanged runtime-rounding verification for synchronous or queued output.
#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "common/result.h"
#include "enhanced/lighting/bake.h"
#include "scene/level.h"

namespace openrac::lighting {
// Fine reference retained only when an adaptive local grid is actually reduced.
struct ProbeReferenceOutput {
    std::vector<std::uint8_t> bytes;  // Original GPU-format payload, moved from the controller.
    std::string report;               // Existing density-policy and source metadata.
};

// Counts captured before the controller releases its expanded volume.
struct ProbeOutputCounts {
    std::size_t total = 0;      // Allocated probes, including invalid entries.
    std::size_t valid = 0;      // Probes providing lighting support.
    std::size_t relocated = 0;  // Individually relocated probes.
};

// One owned grid output; queue callbacks share ownership without copying its encoded payloads.
struct ProbeFileOutput {
    std::filesystem::path path;       // Fixed batch-owned output filename.
    ProbeGrid grid;                   // Exact final grid identity, including density changes.
    std::vector<std::uint8_t> bytes;  // GPU-format payload to write and compare on readback.
    std::string report;               // Controller-formatted invariant and density-policy text.
    ProbeOutputCounts counts;         // Original expanded-volume counts.
    std::chrono::steady_clock::time_point started;  // Includes baking, queueing and verification.
    bool reset_reference = false;  // Preserve existing adaptive-reference cleanup policy.
    std::optional<ProbeReferenceOutput> reference;  // Present only for reduced grids.
};

// Immutable context borrowed until the output queue drains.
struct ProbeOutputContext {
    const scene::LevelData& level;  // Original receiver geometry for coverage diagnostics.
    const BakeSettings& settings;   // Shared settings for runtime-rounding identity verification.
    const ProbeVolume* coarse = nullptr;  // Verified coarse field, stable for every local output.
};

// Ordered diagnostics and, only for the first grid, the decoded coarse field.
struct ProbeOutputCompletion {
    std::string diagnostics;  // Published by the controller after successful verification.
    std::optional<ProbeVolume> coarse;  // Moved to the controller before baking dependent grids.
};

// Checks actual file bytes and identity; the caller supplies its existing runtime-rounding guard.
[[nodiscard]] Result<ProbeVolume> verify_probe_file(
    const std::filesystem::path& path, const ProbeGrid& grid, std::span<const std::uint8_t> encoded
);
// Writes, verifies and reports one owned output, preserving every existing check and failure message.
[[nodiscard]] Result<ProbeOutputCompletion> write_probe_output(
    const ProbeFileOutput& file, const ProbeOutputContext& context
);
}  // namespace openrac::lighting
