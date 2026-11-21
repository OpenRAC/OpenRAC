// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Debug paths preserve typed gameplay links without changing the probe-volume floor domain.
#include <algorithm>
#include <fstream>
#include <set>
#include <stdexcept>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/room_layout.h"
#include "enhanced/tools/baker/output/report.h"

namespace openrac::lighting {
namespace {
float display_offset(const bake_detail::RoomLayout& layout, unsigned index) {
    return layout.medial_display_offsets.empty() ? 1.25f : layout.medial_display_offsets[index];
}

Float3 display_point(const bake_detail::RoomLayout& layout, unsigned index) {
    auto point = layout.medial_points[index];
    point[2] += display_offset(layout, index);
    return point;
}

// Draw above the support surface so the debug path is visible without moving gameplay geometry.
void segment(std::ostream& out, Float3 a, Float3 b, unsigned& count, float lift = 1.25f) {
    out << (count++ ? "," : "") << '[' << a[0] << ',' << a[1] << ',' << a[2] + lift << ',' << b[0]
        << ',' << b[1] << ',' << b[2] + lift << ']';
}

// Walking and swimming remain straight links; airborne links use the exact checked trajectory.
void paths(std::ostream& out, const bake_detail::RoomLayout& layout, bool jumps) {
    unsigned count = 0;

    for (auto edge : layout.medial_edges) {
        // Floor edges stay solid; only the checked airborne trajectories use dashes.
        if (!jumps) {
            segment(out, display_point(layout, edge[0]), display_point(layout, edge[1]), count, 0);
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
            const auto p = display_point(layout, i);
            out << (count++ ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
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
    out << "{\"version\":1,\"method\":\"room-and-entrance-backbone\",\"field_cell_size\":"
        << layout.medial_cell_size << ",\"field_samples\":" << layout.medial_points.size()
        << ",\"ridge_samples\":" << layout.medial_ridge_samples << ",\"segments\":[";
    std::vector<unsigned> degree(layout.medial_points.size());
    std::set<std::array<unsigned, 2>> drawn;
    unsigned count = 0;
    const auto draw = [&](unsigned from, unsigned to) {
        const std::array<unsigned, 2> key{std::min(from, to), std::max(from, to)};
        if (!drawn.insert(key).second) {
            return;
        }
        const auto a = display_point(layout, from), b = display_point(layout, to);
        out << (count++ ? ",[" : "[") << a[0] << ',' << a[1] << ',' << a[2] << ',' << b[0] << ','
            << b[1] << ',' << b[2] << ',' << layout.medial_owners[from] << ','
            << layout.medial_owners[to] << ']';
        ++degree[from];
        ++degree[to];
    };
    for (unsigned i = 0; i < layout.medial_edges.size(); ++i) {
        const auto edge = layout.medial_edges[i];
        draw(edge[0], edge[1]);
    }
    // Watershed boundaries split ownership, not physically continuous floor paths.
    // Include the checked supported links; airborne jump arcs remain separate diagnostics.
    for (const auto& link : layout.links) {
        if (link.kind != bake_detail::RoomLinkKind::Jump) {
            draw(link.from, link.to);
        }
    }
    out << "],\"markers\":[";
    bool comma = false;
    std::vector<bool> centre(layout.medial_points.size());
    for (auto a : layout.medial_centres) {
        centre[a] = true;
    }
    for (unsigned i = 0; i < layout.medial_points.size(); ++i) {
        if (!centre[i] && (degree[i] == 0 || degree[i] == 2)) {
            continue;
        }
        const auto p = display_point(layout, i);
        out << (comma ? ",[" : "[") << p[0] << ',' << p[1] << ',' << p[2] << ','
            << layout.medial_owners[i] << ',' << degree[i] << ',' << (centre[i] ? 1 : 0) << ']';
        comma = true;
    }
    out << "],\"entrances\":[";
    for (unsigned i = 0; i + 1 < layout.medial_entrances.size(); i += 2) {
        const auto from = layout.medial_entrances[i], to = layout.medial_entrances[i + 1];
        const auto p = bake_detail::mul(
            bake_detail::add(display_point(layout, from), display_point(layout, to)), .5f
        );
        out << (i ? ",[" : "[") << p[0] << ',' << p[1] << ',' << p[2] << ','
            << layout.medial_owners[from] << ',' << layout.medial_owners[to] << ']';
    }
    out << "],\"paths\":[";
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
        throw std::runtime_error("cannot write area route diagnostics");
    }
}
}  // namespace openrac::lighting
