// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <algorithm>
#include <ranges>

#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/environment_bake.h"
#include "enhanced/lighting/spatial_link_tiles.h"

namespace openrac::lighting::bake_detail {
struct Scene;

struct SpatialWatershedSettings {
    float horizontal_step = 2, vertical_step = 1;
    float air_height_scale = 4;
    float maximum_height = 16, maximum_clearance = 16;
    // Relative prominence weighted by the candidate's elongated XY support.
    float relative_prominence = .25f;
    bool refine_visibility = true;
    float visibility_radius = 32, visibility_split_gain = .12f;
    unsigned maximum_cells = 1000000;
    std::uint64_t graph_memory_budget = 512ull * 1024 * 1024;
};

struct SpatialSpan {
    Float3 floor{};
    float bottom = 0, top = 0;
    std::optional<float> ceiling;
    bool underwater = false, capped = false;
    std::vector<unsigned> sources, cells;
};

struct SpatialCell {
    Float3 position{};
    float lateral_clearance = 0;
    unsigned span = 0, basin = ~0u, component = ~0u;
};

struct SpatialLink {
    unsigned a, b;
    float aperture;

    unsigned other(unsigned cell) const { return cell == a ? b : a; }
};

struct SpatialBasin {
    Float3 centre{}, minimum{}, maximum{};
    float peak_clearance = 0;
    unsigned seed_cell = 0;
    unsigned watershed_parent = 0;
    float watershed_peak_clearance = 0;
    // Shape support and prominence describe the original watershed candidate.
    float support_length = 0, support_width = 0;
    float relative_prominence = 0, salience = 0;
    unsigned cells = 0, component = 0;
    bool underwater = false;
};

struct SpatialPortal {
    unsigned a = 0, b = 0, connections = 0;
    Float3 centre{};
    float saddle_clearance = 0;
    // Connected patch of collision-checked crossings, ordered from basin a to b.
    std::vector<std::pair<unsigned, unsigned>> crossings;
};

struct SpatialVisibilitySample {
    unsigned cell = 0, basin = 0;
    std::vector<unsigned> visible;
};

struct SpatialVisibilitySplit {
    unsigned parent = 0, a = 0, b = 0;
    float before = 0, after = 0;
    std::vector<Float3> landmarks;
    std::vector<Float3> landmark_normals;
    std::vector<SpatialVisibilitySample> samples;
};

struct SpatialVisibilityAssessment {
    unsigned basin = 0;
    float before = 0, proposed = 0, after = -1;
};

struct SpatialWatershed {
    std::vector<SpatialSpan> spans;
    std::vector<SpatialCell> cells;
    std::vector<SpatialBasin> basins;
    std::vector<SpatialPortal> portals;
    std::vector<SpatialVisibilitySplit> visibility_splits;
    std::vector<SpatialVisibilityAssessment> visibility_assessments;
    unsigned raw_peaks = 0, components = 0, rejected_spans = 0;
    // One record per undirected link. The flat incidence index preserves the
    // order in which links were added at each endpoint.
    std::vector<SpatialLink> links;
    std::vector<unsigned> offsets, incident;

    std::size_t link_count() const { return m_tiled_links ? m_tiled_links->size() : links.size(); }

    bool uses_tiled_links() const { return bool(m_tiled_links); }

    SpatialLink link(unsigned id) const {
        return m_tiled_links ? m_tiled_links->link(id) : links[id];
    }

    void append_link(SpatialLink edge, const SpatialWatershedSettings& options);
    void prepare_link_storage(const SpatialWatershedSettings& options, std::size_t count);
    void index_links();

    std::span<const unsigned> incident_links(unsigned cell) const {
        return std::span(incident).subspan(offsets[cell], offsets[cell + 1] - offsets[cell]);
    }

    auto neighbours(unsigned cell) const {
        return incident_links(cell) | std::views::transform([this, cell](unsigned link) {
                   return this->link(link).other(cell);
               });
    }

    float clearance(SpatialLink e) const {
        return std::min({cells[e.a].lateral_clearance, cells[e.b].lateral_clearance, e.aperture});
    }

    float clearance(unsigned id) const { return clearance(link(id)); }

private:
    std::unique_ptr<TiledSpatialLinks> m_tiled_links;
};

// Shared production spatial graph, independent of saved floor/lighting labels.
// Sparse floor/water evidence bounds the footprint. Air stays in the receiver
// envelope (four times player height); underwater spans extend towards the trusted
// waterline. Ceilings and every graph connection are checked against collision,
// never inferred as solid caps.
SpatialWatershed build_spatial_watershed(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings = {},
    const SpatialWatershedSettings& options = {},
    const BakeExecution& execution = {1}
);
SpatialWatershed build_spatial_watershed(
    const BakeScene& geometry,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings = {},
    const SpatialWatershedSettings& options = {},
    const BakeExecution& execution = {1}
);
// Shape-aware persistence filtering over the measured lateral-clearance graph.
// Headroom lives in the span topology, rather than dominating the scalar field.
void segment_spatial_watershed(
    SpatialWatershed& field, const SpatialWatershedSettings& options = {}
);
// Recompute centres, bounds and connected portal patches after label refinement.
void rebuild_spatial_basins(SpatialWatershed& field);
void refine_spatial_visibility(
    const Scene& scene,
    SpatialWatershed& field,
    const BakeSettings& settings = {},
    const SpatialWatershedSettings& options = {}
);
}  // namespace openrac::lighting::bake_detail
