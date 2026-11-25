// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/spatial_watershed.h"

namespace openrac::lighting::bake_detail {
enum class SpatialVolumeKind {
    reflection,
    gi
};

struct SpatialVolumeFitSettings {
    SpatialVolumeKind kind = SpatialVolumeKind::reflection;
    Float3 spacing{2.6666667f, 2.6666667f, 2.6666667f};
    float padding = .75f, maximum_extent = 48;
    unsigned maximum_dimension = 16, maximum_parts = 16;
    // Zero uses the format's volume limit. GI can also use zero for maximum_parts
    // to let each basin use the level budget after reserving a grid per other basin.
    unsigned maximum_volumes = 0;
    float volume_cost = 32;
};

struct SpatialFittedVolume {
    unsigned basin = 0;
    ProbeFrame frame;
    Float3 origin{}, extent{}, representative{};
    std::array<unsigned, 3> dimensions{};
    std::vector<unsigned> cells;
    double score = 0, volume = 0, foreign_volume = 0, occupied_volume = 0;
    bool feasible = false;
    float coverage_growth = 1;
};

struct SpatialVolumePlan {
    SpatialVolumeFitSettings settings;
    std::vector<SpatialFittedVolume> volumes;
    unsigned covered = 0, uncovered = 0, over_budget = 0;
    double single_box_volume = 0, fitted_volume = 0;
    std::uint64_t estimated_probes = 0;
    bool volume_limit_exceeded = false;
};

// Geometry shared by the GI and reflection fits. Tiled graphs materialize neighbour
// IDs once so fitting can traverse them without reading the link apertures.
class SpatialVolumeFitData {
public:
    explicit SpatialVolumeFitData(const SpatialWatershed& field);

    const SpatialWatershed& field() const { return m_field; }

    auto neighbours(unsigned cell) const {
        const bool direct = !m_neighbours.empty();
        const auto range = direct ? std::span<const unsigned>(m_neighbours)
                                        .subspan(
                                            m_field.offsets[cell],
                                            m_field.offsets[cell + 1] - m_field.offsets[cell]
                                        )
                                  : m_field.incident_links(cell);
        return range | std::views::transform([this, cell, direct](unsigned index) {
                   return direct ? index : m_field.link(index).other(cell);
               });
    }

    std::span<const unsigned> sorted_cells() const { return m_sorted; }

    std::span<const float> cell_weights() const { return m_weights; }

private:
    const SpatialWatershed& m_field;
    std::vector<unsigned> m_neighbours, m_sorted;
    std::vector<float> m_weights;
};

// Basin-owned allocation. Feasible boxes are never subdivided for a smaller score.
// Every cell is assigned once; feasibility failures
// are reported, never hidden by dropping support or publishing a partial bake.
SpatialVolumePlan fit_spatial_volumes(
    const SpatialWatershed& field,
    const SpatialVolumeFitSettings& settings = {},
    const BakeExecution& execution = {1}
);
SpatialVolumePlan fit_spatial_volumes(
    const SpatialVolumeFitData& data,
    const SpatialVolumeFitSettings& settings,
    const BakeExecution& execution = {1}
);

// Influence-only pass: preserves allocation, frames and representatives. Heights
// are world-space column tops; underwater columns retain their existing support.
void extend_spatial_reflection_coverage(
    const SpatialWatershed& field,
    SpatialVolumePlan& plan,
    std::span<const float> span_tops,
    const BakeExecution& execution = {1}
);
void extend_spatial_reflection_coverage(
    const SpatialVolumeFitData& data,
    SpatialVolumePlan& plan,
    std::span<const float> span_tops,
    const BakeExecution& execution = {1}
);
std::vector<float> spatial_reflection_coverage_tops(
    const BakeScene& geometry, const SpatialWatershed& field, float outdoor_height, float ray_bias
);
}  // namespace openrac::lighting::bake_detail
