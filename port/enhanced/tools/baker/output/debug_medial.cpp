// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

#include "enhanced/lighting/bake.h"
#include "enhanced/lighting/room_layout.h"
#include "enhanced/tools/baker/output/debug_preview.h"
#include "enhanced/tools/baker/output/environment_stage.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/sources/activation.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"

namespace openrac::lighting {
void write_debug_medial(const std::filesystem::path& level, const std::filesystem::path& bake) {
    if (!std::filesystem::exists(bake / "placement-islands.json")
        || !std::filesystem::exists(bake / "interior-inference.json")) {
        return;
    }
    const auto partition = bake_json::document(bake / "placement-islands.json");
    if (partition["method"].string() == "layered_watershed") {
        return;  // Production routes were written from this same graph.
    }
    const auto columns = std::filesystem::exists(bake / "medial-axis.json")
                             ? bake_json::document(bake / "medial-axis.json")
                             : json::Value{};
    const auto profile = load_level_bake_profile(level);
    const auto floors = read_environment_floors(bake, level, profile.activation.bake.traversal);
    if (floors.size() != partition["samples"].items().size()) {
        throw std::runtime_error(
            "medial debug floor graph differs from the saved bake; rebuild the bake"
        );
    }
    bake_detail::RoomLayout layout;
    layout.medial_water_footprint = true;
    layout.medial_roots.emplace();
    // Only the collision-floor graph was reached from gameplay evidence. Water receivers
    // are all marked as placement anchors, including disconnected parts of an authored pool.
    std::set<Float3> collision_floors;
    const auto inference = bake_json::document(bake / "interior-inference.json");
    for (const auto& cell : inference["cells"].items()) {
        collision_floors.insert(
            {float(cell[0].number()), float(cell[1].number()), float(cell[2].number())}
        );
    }
    std::vector<unsigned> islands;
    // Extract one skeleton over continuous physical space. Watershed ownership must not
    // create a new root and branches at every lighting island along a single tunnel.
    std::map<std::pair<bool, bool>, unsigned> domains;
    for (unsigned i = 0; i < floors.size(); ++i) {
        const auto& saved = partition["samples"][i];
        const auto owner = saved[3].number(-1);
        if (owner < 0 || owner >= double(partition["islands"].items().size())
            || std::floor(owner) != owner) {
            throw std::runtime_error("invalid saved medial debug island");
        }
        for (unsigned a = 0; a < 3; ++a) {
            if (std::abs(saved[a].number() - floors[i].position[a]) > .001) {
                throw std::runtime_error(
                    "medial debug positions differ from the saved bake; rebuild the bake"
                );
            }
        }
        islands.push_back(unsigned(owner));
        if (collision_floors.contains(floors[i].position)) {
            layout.medial_roots->push_back(i);
        }
        // Water heights vary slightly across triangle interpolation and gently sloping pools.
        // They are positions, not domain identities. Geometry and field-layer checks separate
        // disconnected/stacked water while allowing one surface to share a centreline.
        // Generated basin-bed samples and real submerged collision floors describe the
        // same underwater domain. Their sampling provenance must not produce parallel axes.
        const auto key =
            std::pair{floors[i].swimming_surface && !floors[i].underwater, floors[i].underwater};
        const auto [domain, added] = domains.try_emplace(key, unsigned(layout.rooms.size()));
        if (added) {
            layout.rooms.emplace_back();
        }
        layout.owners.push_back(domain->second);
        layout.rooms[domain->second].push_back(i);
        layout.clearance.push_back(float(saved[4].number()));
        const auto& column = columns["columns"][i];
        layout.height_bounds.push_back(
            column.items().size() == 5
                ? Float2{float(column[3].number()), float(column[4].number())}
                : floors[i].height_bounds.value_or(
                      Float2{floors[i].position[2] - .8f, floors[i].position[2] + 3.8f}
                  )
        );
    }
    // The displayed floor axis must use the displayed gameplay collision. Transport meshes
    // can omit playable decks or have decorative gaps that do not exist in collision.
    const auto collision = load_coverage_collision(level);
    if (!collision) {
        throw std::runtime_error(collision.error().message);
    }
    BakeScene geometry;
    for (const auto& triangle : *collision) {
        if (triangle.support) {
            BakeTriangle solid;
            solid.vertices = triangle.vertices;
            geometry.triangles.push_back(solid);
        }
    }
    auto settings = profile.activation.bake;
    // This overlay is the supported centreline, not a graph of possible gameplay shortcuts.
    // Its field already crosses lighting boundaries; pairwise landing links add redundant webs.
    settings.traversal.jump_distance = 0;
    layout.rebuild_medial(geometry, floors, settings);
    // Keep the viewer's island filter without letting that label shape the centreline.
    for (unsigned i = 0; i < layout.medial_points.size(); ++i) {
        float closest = std::numeric_limits<float>::infinity();
        unsigned selected = 0;
        for (auto source_index : layout.rooms[layout.medial_owners[i]]) {
            float squared = 0;
            for (unsigned a = 0; a < 3; ++a) {
                const float d = layout.medial_points[i][a] - floors[source_index].position[a];
                squared += d * d;
            }
            if (squared < closest) {
                closest = squared;
                selected = source_index;
            }
        }
        layout.medial_owners[i] = islands[selected];
        layout.medial_display_offsets.push_back(
            floors[selected].swimming_surface && !floors[selected].underwater ? .15f : 1.25f
        );
    }
    write_medial_axis_report(bake, layout, floors);
}
}  // namespace openrac::lighting
