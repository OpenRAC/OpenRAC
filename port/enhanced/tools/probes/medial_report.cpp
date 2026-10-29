// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Debug paths preserve typed gameplay links without changing the probe-volume floor domain.
#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/room_layout.h"
#include "enhanced/tools/probes/report.h"

namespace openrac::lighting {
namespace {
// Draw above the support surface so the debug path is visible without moving gameplay geometry.
void segment(std::ostream& out, Float3 a, Float3 b, unsigned& count) {
    out << (count++ ? "," : "") << '[' << a[0] << ',' << a[1] << ',' << a[2] + 1.25f << ',' << b[0]
        << ',' << b[1] << ',' << b[2] + 1.25f << ']';
}

// Walking and swimming remain straight links; airborne links use the exact checked trajectory.
void paths(std::ostream& out, const bake_detail::RoomLayout& layout, bool jumps) {
    unsigned count = 0;

    for (auto edge : layout.medial_edges) {
        // Floor edges stay solid; only the checked airborne trajectories use dashes.
        if (!jumps) {
            segment(out, layout.medial_points[edge[0]], layout.medial_points[edge[1]], count);
        }
    }

    for (const auto& link : layout.links) {
        // Keep jump geometry separate so the overlay can distinguish traversal from floor support.
        if ((link.kind == bake_detail::RoomLinkKind::Jump) != jumps) {
            continue;
        }

        const auto a = layout.medial_points[link.from], b = layout.medial_points[link.to];
        const unsigned steps = link.kind == bake_detail::RoomLinkKind::Jump ? 16u : 1u;
        const auto point = [&](float t) {
            if (link.kind == bake_detail::RoomLinkKind::Jump) {
                return bake_detail::room_jump_position(a, b, link.apex, t);
            }

            return bake_detail::add(bake_detail::mul(a, 1 - t), bake_detail::mul(b, t));
        };

        for (unsigned i = 0; i < steps; ++i) {
            segment(out, point(float(i) / float(steps)), point(float(i + 1) / float(steps)), count);
        }
    }
}

// Junction markers reflect the floor skeleton; airborne arcs do not create artificial floor junctions.
void key_points(std::ostream& out, const bake_detail::RoomLayout& layout, bool centres) {
    std::vector<unsigned> degree(layout.medial_points.size());

    for (auto edge : layout.medial_edges) {
        ++degree[edge[0]];
        ++degree[edge[1]];
    }

    unsigned count = 0;

    for (unsigned i = 0; i < layout.medial_points.size(); ++i) {
        const bool selected =
            centres ? std::find(layout.medial_centres.begin(), layout.medial_centres.end(), i)
                          != layout.medial_centres.end()
                    : degree[i] >= 3;

        if (selected) {
            const auto p = layout.medial_points[i];
            out << (count++ ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << p[2] + 1.25f << ']';
        }
    }
}

// Typed links remain available for inspection independently of their rendered line segments.
void link_records(std::ostream& out, const bake_detail::RoomLayout& layout) {
    for (unsigned i = 0; i < layout.links.size(); ++i) {
        const auto& link = layout.links[i];
        const auto a = layout.medial_points[link.from], b = layout.medial_points[link.to];
        out << (i ? "," : "") << '[' << a[0] << ',' << a[1] << ',' << a[2] << ',' << b[0] << ','
            << b[1] << ',' << b[2] << ',' << unsigned(link.kind) << ',' << link.apex << ']';
    }
}
}  // namespace

void write_medial_axis_report(
    const std::filesystem::path& directory,
    const bake_detail::RoomLayout& layout,
    std::span<const EnvironmentFloorSample> floors
) {
    std::ofstream out(directory / "medial-axis.json");
    out << "{\"version\":1,\"method\":\"unsigned-distance-ridges\",\"field_cell_size\":"
        << layout.medial_cell_size << ",\"field_samples\":" << layout.medial_points.size()
        << ",\"ridge_samples\":" << layout.medial_ridge_samples << ",\"paths\":[";
    paths(out, layout, false);
    out << "],\"jump_paths\":[";
    paths(out, layout, true);
    out << "],\"junctions\":[";
    key_points(out, layout, false);
    out << "],\"centres\":[";
    key_points(out, layout, true);
    out << "],\"columns\":[";

    for (unsigned i = 0; i < floors.size(); ++i) {
        const auto p = floors[i].position;
        out << (i ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << layout.height_bounds[i][0]
            << ',' << layout.height_bounds[i][1] << ']';
    }

    out << "],\"traversal_links\":[";
    link_records(out, layout);
    out << "]}\n";

    if (!out) {
        throw std::runtime_error("cannot write medial axis diagnostics");
    }
}
}  // namespace openrac::lighting
