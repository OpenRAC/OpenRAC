// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned grid preparation for bounded compression, density validation and verified publication.
#pragma once
#include "enhanced/lighting/probe_density.h"
#include "enhanced/tools/baker/batch.h"
#include "enhanced/tools/baker/output/probe_output.h"
#include "enhanced/tools/baker/output/report.h"

namespace openrac::lighting {
// Moves one traced grid into its output controller without copying its directional maps.
struct ProbeGridOutput {
    std::filesystem::path path;  // Fixed batch-owned asset name.
    ProbeVolume volume;          // Raw traced grid, released after compression and count capture.
    std::chrono::steady_clock::time_point started;  // Original grid start, including handoff waits.
    std::string density_identity;  // Existing resume policy text, independent of scheduling.
};

// Borrows immutable batch state until every pending output controller has joined.
struct ProbeGridOutputContext {
    const ProbeBatchOptions& options;     // Transport settings and local worker budget.
    const ProbeBakeReport& descriptions;  // Shared immutable scene/material report text.
    std::span<const bake_detail::ProbeDensityReceiver> receivers;  // Shared reduction evidence.
    const bake_detail::ProbeDensitySettings& density;  // Quality policy, unchanged by scheduling.
    ProbeOutputContext verification;  // Stable level, settings and optional verified coarse field.
    BakeThreadPool& workers;          // Shared compute pool, never the output controller itself.
};

// Encodes each retained lattice once, then writes and verifies its exact GPU payload.
[[nodiscard]] Result<ProbeOutputCompletion> prepare_probe_grid_output(
    ProbeGridOutput& job, const ProbeGridOutputContext& context
);
}  // namespace openrac::lighting
