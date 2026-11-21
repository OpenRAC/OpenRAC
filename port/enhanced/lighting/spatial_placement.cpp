// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/spatial_placement.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>

#include "common/log.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_gi.h"

namespace openrac::lighting::bake_detail {
SpatialPlacement build_spatial_placement(
    const BakeScene& geometry,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    Float3 spacing,
    unsigned maximum_probes,
    unsigned maximum_grids,
    const EnvironmentBakeSettings& environment,
    const BakeExecution& execution,
    const std::function<void(const SpatialPlacement&)>& on_gi_failure
) {
    SpatialPlacement result;
    result.air_height =
        settings.traversal.player_height * SpatialWatershedSettings{}.air_height_scale;
    result.field = build_spatial_watershed(geometry, floors, settings, {}, execution);
    if (result.field.uses_tiled_links()) {
        log::info("layered placement: link records use tiled scratch storage");
    }
    log::info(
        "layered placement: {} cells, {} basins",
        result.field.cells.size(),
        result.field.basins.size()
    );
    const SpatialVolumeFitData fit_data(result.field);
    SpatialVolumeFitSettings gi;
    gi.kind = SpatialVolumeKind::gi;
    gi.spacing = spacing;
    gi.maximum_dimension =
        std::min(16u, unsigned(std::floor(std::cbrt(double(maximum_probes)) + 1e-5)));
    gi.maximum_parts = 0;
    gi.maximum_volumes = maximum_grids;
    result.gi = fit_spatial_volumes(fit_data, gi, execution);
    if (result.gi.uncovered || result.gi.over_budget || result.gi.volumes.size() > maximum_grids) {
        result.gi_rejected = true;
        for (const auto& box : result.gi.volumes) {
            if (!box.feasible) {
                log::info(
                    "layered GI fit: basin {}, {} cells in oversized grid {} x {} x {}",
                    box.basin,
                    box.cells.size(),
                    box.dimensions[0],
                    box.dimensions[1],
                    box.dimensions[2]
                );
            }
        }
        if (on_gi_failure) {
            on_gi_failure(result);
        }
        throw std::runtime_error(
            std::format(
                "layered GI fit exceeds allocation limits: {} of {} cells uncovered, "
                "{} oversized grids (maximum {} probes per axis), {} grids (level limit {}); "
                "no partial plan published",
                result.gi.uncovered,
                result.field.cells.size(),
                result.gi.over_budget,
                gi.maximum_dimension,
                result.gi.volumes.size(),
                maximum_grids
            )
        );
    }
    SpatialVolumeFitSettings reflection;
    reflection.maximum_extent = 2 * environment.box_distance;
    reflection.maximum_parts = 64;
    result.reflection = fit_spatial_volumes(fit_data, reflection, execution);
    const auto tops = spatial_reflection_coverage_tops(
        geometry, result.field, settings.traversal.player_height * 12, settings.ray_bias
    );
    extend_spatial_reflection_coverage(fit_data, result.reflection, tops, execution);
    if (result.reflection.uncovered || result.reflection.over_budget
        || result.reflection.volume_limit_exceeded) {
        throw std::runtime_error(
            std::format(
                "layered reflection fit: {} basins, {} volumes, {} uncovered, {} infeasible boxes, "
                "count limit {}",
                result.field.basins.size(),
                result.reflection.volumes.size(),
                result.reflection.uncovered,
                result.reflection.over_budget,
                result.reflection.volume_limit_exceeded
            )
        );
    }
    return result;
}

RoomLayout spatial_floor_layout(
    const SpatialWatershed& field, std::vector<EnvironmentFloorSample>& floors
) {
    RoomLayout layout;
    layout.owners.assign(floors.size(), ~0u);
    layout.clearance.resize(floors.size());
    layout.height_bounds.resize(floors.size());
    layout.ceilings.assign(floors.size(), INFINITY);
    layout.rooms.resize(field.basins.size());
    for (auto& floor : floors) {
        floor.island.reset();
    }
    for (const auto& span : field.spans) {
        for (auto source : span.sources) {
            unsigned cell = ~0u;
            float closest = INFINITY;
            const float target = std::min(span.top, floors[source].position[2] + 1.25f);
            for (auto i : span.cells) {
                const float d = std::abs(field.cells[i].position[2] - target);
                if (d < closest) {
                    closest = d;
                    cell = i;
                }
            }
            if (cell == ~0u) {
                continue;
            }
            const auto& c = field.cells[cell];
            layout.owners[source] = c.basin;
            layout.clearance[source] = c.lateral_clearance;
            layout.height_bounds[source] = {span.bottom, span.top};
            layout.ceilings[source] = span.ceiling.value_or(INFINITY);
            floors[source].island = c.basin;
            floors[source].island_clearance = c.lateral_clearance;
            layout.rooms[c.basin].push_back(source);
        }
    }
    // Routes follow the same checked 3D graph, including portal crossings.
    std::vector<unsigned> point(field.cells.size(), ~0u);
    const auto vertex = [&](unsigned cell) {
        if (point[cell] == ~0u) {
            point[cell] = unsigned(layout.medial_points.size());
            layout.medial_points.push_back(field.cells[cell].position);
            layout.medial_owners.push_back(field.cells[cell].basin);
            layout.medial_display_offsets.push_back(0);
        }
        return point[cell];
    };
    std::set<std::array<unsigned, 2>> edges;
    std::vector<unsigned> parent(field.cells.size(), ~0u);
    for (unsigned basin = 0; basin < field.basins.size(); ++basin) {
        const auto seed = field.basins[basin].seed_cell;
        layout.medial_centres.push_back(vertex(seed));
        std::queue<unsigned> queue;
        queue.push(seed);
        parent[seed] = seed;
        while (!queue.empty()) {
            const auto i = queue.front();
            queue.pop();
            for (auto j : field.neighbours(i)) {
                if (field.cells[j].basin == basin && parent[j] == ~0u) {
                    parent[j] = i;
                    queue.push(j);
                }
            }
        }
        for (const auto& portal : field.portals) {
            if (portal.a == basin || portal.b == basin) {
                if (portal.crossings.empty()) {
                    continue;
                }
                auto [from, to] = portal.crossings.front();
                unsigned end = field.cells[from].basin == basin ? from : to;
                if (parent[end] == ~0u) {
                    continue;
                }
                while (end != seed) {
                    auto a = vertex(end), b = vertex(parent[end]);
                    if (a > b) {
                        std::swap(a, b);
                    }
                    edges.insert({a, b});
                    end = parent[end];
                }
            }
        }
    }
    for (const auto& portal : field.portals) {
        if (!portal.crossings.empty()) {
            auto [a, b] = portal.crossings.front();
            a = vertex(a);
            b = vertex(b);
            layout.medial_entrances.push_back(a);
            layout.medial_entrances.push_back(b);
            if (a > b) {
                std::swap(a, b);
            }
            edges.insert({a, b});
        }
    }
    layout.medial_edges.assign(edges.begin(), edges.end());
    return layout;
}

std::vector<EnvironmentProbe> place_spatial_environment_probes(
    const BakeScene& render,
    const BakeScene& geometry,
    std::span<const ProbeVolume> gi,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    const SpatialPlacement& spatial,
    EnvironmentPlacementReport& report,
    std::vector<EnvironmentInfluence>& influences
) {
    report = {};
    report.layered_regions = true;
    const auto& field = spatial.field;
    const auto& plan = spatial.reflection;
    if (field.basins.size() > options.maximum_probes) {
        throw std::runtime_error("layered basins exceed reflection photograph budget");
    }
    Scene scene(render, {}, {.prepare_emission_averages = false});
    Scene collision(geometry, {}, {.prepare_emission_averages = false});
    CaptureGi lighting(gi);
    report.input_floors = unsigned(floors.size());
    for (const auto& span : field.spans) {
        report.playable_floors += unsigned(span.sources.size());
    }
    report.rejected_floors = report.input_floors - report.playable_floors;
    report.spaces_before_merge = report.lighting_spaces = unsigned(field.basins.size());
    report.spaces.resize(field.basins.size());
    std::vector<std::vector<unsigned>> cells(field.basins.size());
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        cells[field.cells[i].basin].push_back(i);
    }
    std::vector<EnvironmentProbe> captures;
    std::vector<unsigned> capture(field.basins.size(), ~0u);
    for (unsigned basin = 0; basin < field.basins.size(); ++basin) {
        const auto& domain = field.basins[basin];
        auto ordered = cells[basin];
        std::stable_sort(ordered.begin(), ordered.end(), [&](unsigned a, unsigned b) {
            const auto da = add(field.cells[a].position, mul(domain.centre, -1));
            const auto db = add(field.cells[b].position, mul(domain.centre, -1));
            return dot(da, da) < dot(db, db);
        });
        unsigned selected = ~0u;
        Float3 camera{};
        const auto clear_camera = [&](Float3 p) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                for (float sign : {-1.f, 1.f}) {
                    Float3 direction{};
                    direction[axis] = sign;
                    if (scene.occluded(p, direction, settings.ray_bias, .2f)
                        || collision.occluded(p, direction, settings.ray_bias, .2f)) {
                        return false;
                    }
                }
            }
            return true;
        };
        // Prefer an existing interior sample. Render/collision seams can require
        // subcell relocation, bounded to the cell footprint and checked free span.
        for (unsigned pass = 0; pass < 2 && selected == ~0u; ++pass) {
            for (auto i : ordered) {
                const auto centre = field.cells[i].position;
                const auto& span = field.spans[field.cells[i].span];
                for (int x = -int(pass); x <= int(pass) && selected == ~0u; ++x) {
                    for (int y = -int(pass); y <= int(pass) && selected == ~0u; ++y) {
                        for (int z = -int(pass); z <= int(pass) && selected == ~0u; ++z) {
                            if (pass && !x && !y && !z) {
                                continue;
                            }
                            const auto candidate =
                                add(centre,
                                    Float3{float(x) * .5f, float(y) * .5f, float(z) * .25f});
                            if (candidate[2] <= span.bottom || candidate[2] >= span.top) {
                                continue;
                            }
                            const auto delta = add(candidate, mul(centre, -1));
                            const float distance = std::sqrt(dot(delta, delta));
                            if (distance > settings.ray_bias
                                && collision.occluded(
                                    centre, mul(delta, 1 / distance), settings.ray_bias, distance
                                )) {
                                continue;
                            }
                            if (clear_camera(candidate)) {
                                selected = i;
                                camera = candidate;
                            }
                        }
                    }
                }
                if (selected != ~0u) {
                    break;
                }
            }
        }
        if (selected == ~0u) {
            throw std::runtime_error(
                std::format(
                    "layered basin {} has no render-clear capture position ({} cells)",
                    basin,
                    domain.cells
                )
            );
        }
        EnvironmentProbe probe;
        probe.position = camera;
        const auto first =
            std::find_if(plan.volumes.begin(), plan.volumes.end(), [&](const auto& v) {
                return v.basin == basin;
            });
        if (first == plan.volumes.end()) {
            throw std::runtime_error("layered basin has no influence volume");
        }
        probe.frame = first->frame;
        probe.minimum = probe.maximum = probe.frame.to_local(probe.position);
        for (const auto& v : plan.volumes) {
            if (v.basin == basin) {
                for (unsigned corner = 0; corner < 8; ++corner) {
                    Float3 local{};
                    for (unsigned k = 0; k < 3; ++k) {
                        if (corner & (1 << k)) {
                            local[k] = v.extent[k];
                        }
                    }
                    const auto p = probe.frame.to_local(add(v.origin, v.frame.to_world(local)));
                    for (unsigned k = 0; k < 3; ++k) {
                        probe.minimum[k] = std::min(probe.minimum[k], p[k] - .5f);
                        probe.maximum[k] = std::max(probe.maximum[k], p[k] + .5f);
                    }
                }
            }
        }
        probe.projection_minimum = probe.minimum;
        probe.projection_maximum = probe.maximum;
        capture[basin] = unsigned(captures.size());
        captures.push_back(probe);
        report.capture_positions.push_back(probe.position);
        report.owners.push_back(basin);
        if (domain.underwater) {
            report.submerged_captures.push_back(capture[basin]);
        }
        auto& space = report.spaces[basin];
        space.cells = domain.cells;
        space.minimum = domain.minimum;
        space.maximum = domain.maximum;
        space.underwater = domain.underwater;
        const auto& span = field.spans[field.cells[selected].span];
        space.sky_band = span.ceiling ? 0 : 1;
        space.authored_region = ~0u;  // No authored label defines a layered basin.
        space.light = lighting.sample(probe.position, {0, 0, 1}, {0, 0, 1}, options);
        if (!span.sources.empty()) {
            space.water_height = floors[span.sources.front()].water_height;
        }
    }
    for (const auto& portal : field.portals) {
        report.spaces[portal.a].neighbours.push_back(portal.b);
        report.spaces[portal.b].neighbours.push_back(portal.a);
    }
    influences.clear();
    for (const auto& v : plan.volumes) {
        EnvironmentInfluence region;
        region.capture = capture[v.basin];
        region.frame = v.frame;
        region.minimum = v.frame.to_local(v.origin);
        region.maximum = add(region.minimum, v.extent);
        const auto& photo = captures[region.capture];
        region.projection =
            EnvironmentProjection{photo.frame, photo.projection_minimum, photo.projection_maximum};
        influences.push_back(region);
        report.influence_captures.push_back(region.capture);
        report.influence_owners.push_back(v.basin);
        EnvironmentProbe bounds;
        bounds.frame = region.frame;
        bounds.minimum = region.minimum;
        bounds.maximum = region.maximum;
        report.capture_bounds.push_back(bounds);
        bounds.frame = photo.frame;
        bounds.projection_minimum = photo.projection_minimum;
        bounds.projection_maximum = photo.projection_maximum;
        report.capture_projections.push_back(bounds);
    }
    report.unpruned_probes = unsigned(influences.size());
    for (const auto& cell : field.cells) {
        EnvironmentCoverageReceiver receiver;
        receiver.position = cell.position;
        for (unsigned i = 0; i < influences.size(); ++i) {
            if (plan.volumes[i].basin == cell.basin
                && environment_probe_coverage(report.capture_bounds[i], cell.position) >= .2f) {
                receiver.probes.push_back(influences[i].capture);
            }
        }
        if (receiver.probes.empty()) {
            report.uncovered.push_back(cell.position);
            report.uncovered_owners.push_back(cell.basin);
        }
        report.receivers.push_back(std::move(receiver));
        report.watershed.push_back({cell.position, cell.basin, cell.lateral_clearance, false});
    }
    report.validation_samples = unsigned(field.cells.size());
    if (!report.uncovered.empty()) {
        throw std::runtime_error("layered reflection cells have missing influence coverage");
    }
    report.coverage_hash = environment_capture_hash(captures, 0, influences);
    scene.check_error();
    collision.check_error();
    return captures;
}
}  // namespace openrac::lighting::bake_detail
