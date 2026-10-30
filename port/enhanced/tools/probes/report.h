// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Coverage reports and output files for offline probe tools.
#pragma once
#include <filesystem>
#include <ostream>
#include <span>
#include <string>

#include "enhanced/tools/probes/level_scene.h"

namespace openrac::lighting {
struct LevelBakeProfile;
struct EnvironmentFloorSample;

namespace bake_detail {
struct RoomLayout;
}

// Shared bounded analysis columns, centre paths and junctions for the optional runtime overlay.
void write_medial_axis_report(
    const std::filesystem::path& directory,
    const bake_detail::RoomLayout& layout,
    std::span<const EnvironmentFloorSample> floors
);

// Snapshot invariant scene/material text once for all grids in a level batch.
class ProbeBakeReport {
public:
    ProbeBakeReport(
        const scene::LevelData& level,
        const BakeSceneExport& exported,
        const BakeSettings& settings,
        unsigned light_set
    );
    // Keep grid identity and allocation explicit without rescanning the scene.
    std::string describe(const ProbeGrid& grid) const;

private:
    BakeSettings m_settings;
    std::string m_scene, m_materials;
};

// Machine-readable resolved transport and grid settings, including room-local emission gains.
std::string describe_level_bake_profile(const LevelBakeProfile& profile);

// Shared human-readable transport/grid description for both CLI paths.
std::string describe_probe_bake(
    const scene::LevelData& level,
    const BakeSceneExport& exported,
    const ProbeGrid& grid,
    const BakeSettings& settings,
    unsigned light_set
);

// Preserve the original geometric coverage diagnostics under runtime rounding.
void report_terrain_coverage(
    const scene::LevelData& level,
    const ProbeVolume& volume,
    const ProbeVolume* coarse,
    std::ostream& output
);

// Write through a temporary file, refusing to overwrite either file.
void write_bake_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);
}  // namespace openrac::lighting
