// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/spatial_volume_fit.h"
#include "enhanced/lighting/spatial_watershed.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
void quad(BakeScene& scene, Float3 a, Float3 b, Float3 c, Float3 d) {
    BakeTriangle t;
    t.vertices = {a, b, c};
    scene.triangles.push_back(t);
    t.vertices = {a, c, d};
    scene.triangles.push_back(t);
}

void floor(BakeScene& scene, float z) {
    quad(scene, {-20, -20, z}, {20, -20, z}, {20, 20, z}, {-20, 20, z});
}

EnvironmentFloorSample source(float x, float y, float z) {
    EnvironmentFloorSample s;
    s.position = {x, y, z};
    s.anchor = true;
    return s;
}

void stacked_layers() {
    BakeScene geometry;
    floor(geometry, 0);
    floor(geometry, 6);
    Scene scene(geometry);
    std::vector<EnvironmentFloorSample> floors{
        source(0, 0, 0), source(2, 0, 0), source(0, 0, 6), source(2, 0, 6)
    };
    // Even deliberately misleading navigation/ownership input cannot join slabs.
    floors[0].neighbours = {2};
    const auto field = build_spatial_watershed(scene, floors);
    CHECK(field.spans.size() == 4 && field.components == 2 && field.basins.size() == 2);
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        for (auto j : field.neighbours(i)) {
            CHECK(std::abs(field.cells[i].position[2] - field.cells[j].position[2]) <= 1.01f);
        }
    }
    CHECK(field.spans[0].ceiling && std::abs(*field.spans[0].ceiling - 6) < .01f);
    CHECK(!field.spans[2].ceiling && field.spans[2].capped);
    CHECK(std::abs(field.spans[0].top - 5.95f) < .01f);  // Collision ceiling still wins.
    CHECK(std::abs(field.spans[2].top - 16) < .01f);
    for (const auto& basin : field.basins) {
        const float middle = basin.centre[2] < 6 ? 3.f : 11.025f;
        CHECK(std::abs(basin.centre[2] - middle) <= .51f);
    }
    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].region = i + 10;
        floors[i].island = i + 100;
    }
    const auto relabelled = build_spatial_watershed(scene, floors);
    CHECK(field.cells.size() == relabelled.cells.size());
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        CHECK(field.cells[i].basin == relabelled.cells[i].basin);
    }
}

void blocked_wall() {
    BakeScene geometry;
    floor(geometry, 0);
    quad(geometry, {0, -10, 0}, {0, 10, 0}, {0, 10, 16}, {0, -10, 16});
    Scene scene(geometry);
    const std::vector<EnvironmentFloorSample> floors{source(-1, 0, 0), source(1, 0, 0)};
    const auto field = build_spatial_watershed(scene, floors);
    CHECK(field.components == 2 && field.portals.empty());
}

void doorway() {
    BakeScene geometry;
    floor(geometry, 0);
    quad(geometry, {-6, -6, 6}, {6, -6, 6}, {6, 6, 6}, {-6, 6, 6});
    quad(geometry, {-6, -6, 0}, {-6, 6, 0}, {-6, 6, 6}, {-6, -6, 6});
    quad(geometry, {6, -6, 0}, {6, 6, 0}, {6, 6, 6}, {6, -6, 6});
    quad(geometry, {-6, -6, 0}, {6, -6, 0}, {6, -6, 6}, {-6, -6, 6});
    quad(geometry, {-6, 6, 0}, {6, 6, 0}, {6, 6, 6}, {-6, 6, 6});
    quad(geometry, {0, -6, 0}, {0, -1, 0}, {0, -1, 6}, {0, -6, 6});
    quad(geometry, {0, 1, 0}, {0, 6, 0}, {0, 6, 6}, {0, 1, 6});
    Scene scene(geometry);
    std::vector<EnvironmentFloorSample> floors;
    for (int x = -5; x <= 5; ++x) {
        for (int y = -5; y <= 5; ++y) {
            floors.push_back(source(float(x), float(y), 0));
        }
    }
    SpatialWatershedSettings options;
    options.horizontal_step = 1;
    const auto field = build_spatial_watershed(scene, floors, {}, options);
    CHECK(field.components == 1);
    CHECK(field.basins.size() == 2);
    CHECK(!field.portals.empty());
    for (const auto& p : field.portals) {
        CHECK(p.saddle_clearance < 1.51f);
    }
    auto tiled_options = options;
    tiled_options.graph_memory_budget = 160 * field.cells.size() + 28 * field.link_count();
    tiled_options.graph_memory_budget +=
        TiledSpatialLinks::cache_size(tiled_options.graph_memory_budget);
    const auto tiled = build_spatial_watershed(scene, floors, {}, tiled_options);
    CHECK(tiled.uses_tiled_links());
    CHECK(tiled.components == field.components && tiled.basins.size() == field.basins.size());
    CHECK(tiled.offsets == field.offsets && tiled.incident == field.incident);
    CHECK(tiled.link_count() == field.link_count());
    for (unsigned i = 0; i < field.link_count(); ++i) {
        const auto a = field.link(i), b = tiled.link(i);
        CHECK(a.a == b.a && a.b == b.b && a.aperture == b.aperture);
    }
    const SpatialVolumeFitData fit_data(tiled);
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        CHECK(tiled.cells[i].basin == field.cells[i].basin);
        CHECK(std::ranges::equal(fit_data.neighbours(i), field.neighbours(i)));
    }
    CHECK(tiled.portals.size() == field.portals.size());
    for (unsigned i = 0; i < field.portals.size(); ++i) {
        CHECK(tiled.portals[i].crossings == field.portals[i].crossings);
        CHECK(tiled.portals[i].saddle_clearance == field.portals[i].saddle_clearance);
    }
    // A coarse field must retain a doorway narrower than either endpoint.
    const std::vector<EnvironmentFloorSample> coarse{source(-1, 0, 0), source(1, 0, 0)};
    const auto sparse = build_spatial_watershed(scene, coarse);
    CHECK(sparse.components == 1 && sparse.basins.size() == 2);
    CHECK(sparse.portals.size() == 1 && sparse.portals[0].saddle_clearance < 1.01f);
}

void slope_and_water() {
    BakeScene geometry;
    quad(geometry, {-6, -2, -1.5f}, {6, -2, 1.5f}, {6, 2, 1.5f}, {-6, 2, -1.5f});
    quad(geometry, {-6, -2, -1.5f}, {6, -2, 1.5f}, {6, -2, 5.5f}, {-6, -2, 2.5f});
    quad(geometry, {-6, 2, -1.5f}, {6, 2, 1.5f}, {6, 2, 5.5f}, {-6, 2, 2.5f});
    Scene scene(geometry);
    std::vector<EnvironmentFloorSample> floors;
    for (int x = -4; x <= 4; ++x) {
        floors.push_back(source(float(x), 0, float(x) * .25f));
    }
    const auto slope = build_spatial_watershed(scene, floors);
    CHECK(slope.components == 1 && slope.basins.size() == 1);
    BakeScene pool;
    floor(pool, 0);
    Scene water_scene(pool);
    floors = {source(0, 0, 0), source(0, 0, 3), source(0, 0, 6)};
    for (auto& s : floors) {
        s.water_height = 6;
    }
    floors[0].underwater = floors[1].underwater = true;
    floors[2].swimming_surface = true;
    const auto water = build_spatial_watershed(water_scene, floors);
    CHECK(water.spans.size() == 2 && water.components == 2);
    CHECK(water.spans[0].sources.size() == 2);
    CHECK(water.spans[0].top < 6 && water.spans[1].bottom > 6);
}

void prominence() {
    SpatialWatershed field;
    field.spans.resize(1);
    field.cells.resize(3);
    for (unsigned i = 0; i < 3; ++i) {
        field.cells[i].position = {float(i), 0, 1};
        field.cells[i].lateral_clearance = i == 1 ? 3.8f : 4;
        if (i) {
            field.links.push_back({i - 1, i, INFINITY});
        }
    }
    SpatialWatershedSettings options;
    options.horizontal_step = 1;
    segment_spatial_watershed(field, options);
    CHECK(field.basins.size() == 1);
    field.cells[1].lateral_clearance = 1;
    segment_spatial_watershed(field, options);
    CHECK(field.basins.size() == 2 && field.portals.size() == 1);
}

void compatible_heights() {
    BakeScene geometry;
    floor(geometry, 0);
    const Scene scene(geometry);
    // Unequal span extents exercise both ends of the window. In open air,
    // every height-compatible pair survives the collision and clearance checks.
    for (const float offset : {0.f, .25f, 4.f}) {
        const std::vector floors{source(0, 0, 0), source(2, 0, offset)};
        SpatialWatershedSettings options;
        const auto field = build_spatial_watershed(scene, floors, {}, options);
        CHECK(field.spans.size() == 2);
        if (field.spans.size() != 2) {
            continue;
        }
        unsigned eligible = 0, skipped = 0;
        for (auto i : field.spans[0].cells) {
            std::vector<unsigned> expected, actual;
            for (auto j : field.spans[1].cells) {
                if (std::abs(field.cells[i].position[2] - field.cells[j].position[2])
                    <= options.vertical_step) {
                    expected.push_back(j);
                } else {
                    ++skipped;
                }
            }
            for (auto j : field.neighbours(i)) {
                if (field.cells[j].span == 1) {
                    actual.push_back(j);
                }
            }
            CHECK(actual == expected);
            eligible += unsigned(expected.size());
        }
        CHECK(eligible > 0 && skipped > 0);
    }
}

void limits_and_roots() {
    BakeScene geometry;
    floor(geometry, 0);
    Scene scene(geometry);
    auto floors = std::vector{source(0, 0, 0), source(2, 0, 0)};
    floors[1].anchor = false;
    const auto rooted = build_spatial_watershed(scene, floors);
    CHECK(rooted.spans.size() == 1 && rooted.rejected_spans == 1);
    SpatialWatershedSettings options;
    options.maximum_cells = 1;
    bool rejected = false;
    try {
        build_spatial_watershed(scene, floors, {}, options);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);

    options.maximum_cells = SpatialWatershedSettings{}.maximum_cells;
    options.graph_memory_budget = 1;
    rejected = false;
    try {
        build_spatial_watershed(scene, floors, {}, options);
    } catch (const std::runtime_error& error) {
        rejected =
            std::string_view(error.what()) == "spatial watershed graph memory budget exceeded";
    }
    CHECK(rejected);
}

void separate_portal_patches() {
    SpatialWatershed field;
    field.spans.resize(1);
    field.cells.resize(6);
    for (unsigned i = 0; i < 6; ++i) {
        field.cells[i].position = {i < 3 ? -1.f : 1.f, float(i % 3), 1};
        field.cells[i].lateral_clearance = 4;
    }
    const auto connect = [&](unsigned a, unsigned b, float aperture) {
        field.links.push_back({std::min(a, b), std::max(a, b), aperture});
    };
    connect(0, 1, 4);
    connect(1, 2, 4);
    connect(3, 4, 4);
    connect(4, 5, 4);
    connect(0, 3, 1);
    connect(2, 5, 1);
    segment_spatial_watershed(field);
    CHECK(field.basins.size() == 2 && field.portals.size() == 2);
    for (const auto& p : field.portals) {
        CHECK(p.connections == 1 && p.crossings.size() == 1);
        CHECK(p.saddle_clearance == 1);
        const auto [a, b] = p.crossings.front();
        CHECK(field.cells[a].basin == p.a && field.cells[b].basin == p.b);
        CHECK(std::abs(p.centre[0]) < .001f);
    }
    // Fill the boundary gap: the three adjoining crossing samples form ONE portal.
    connect(1, 4, 1);
    segment_spatial_watershed(field);
    CHECK(field.basins.size() == 2 && field.portals.size() == 1);
    CHECK(field.portals[0].connections == 3 && field.portals[0].crossings.size() == 3);
    CHECK(std::abs(field.portals[0].centre[1] - 1) < .001f);
}

void whole_peak_plateau() {
    SpatialWatershed field;
    field.spans.resize(1);
    field.cells.resize(8);
    for (unsigned i = 0; i < 8; ++i) {
        field.cells[i].position = {i == 7 ? 9.f : float(i), 0, 1};
        field.cells[i].lateral_clearance = i < 6 ? 4.f : i == 6 ? 1.f : 3.f;
        if (i) {
            field.links.push_back({i - 1, i, INFINITY});
        }
    }
    segment_spatial_watershed(field);
    CHECK(field.basins.size() == 2);
    // The narrow cell is one unit from the broad plateau and three from the
    // other peak. Its distance from the plateau's first cell is irrelevant.
    CHECK(field.cells[6].basin == field.cells[5].basin);
    CHECK(field.cells[6].basin != field.cells[7].basin);
    // Reversing cell order must not move that boundary to the other basin.
    std::reverse(field.cells.begin(), field.cells.end());
    for (auto& link : field.links) {
        const auto a = link.a;
        link.a = 7 - link.b;
        link.b = 7 - a;
    }
    segment_spatial_watershed(field);
    CHECK(field.basins.size() == 2);
    CHECK(field.cells[1].basin == field.cells[2].basin);
    CHECK(field.cells[1].basin != field.cells[0].basin);
}

void corridor_shape_support() {
    const auto make = [](bool compact, float angle, unsigned layers) {
        SpatialWatershed field;
        field.spans.resize(1);
        field.cells.resize(18 * layers);
        const auto connect = [&](unsigned a, unsigned b) {
            field.links.push_back({std::min(a, b), std::max(a, b), INFINITY});
        };
        for (unsigned z = 0; z < layers; ++z) {
            for (unsigned i = 0; i < 18; ++i) {
                auto& cell = field.cells[z * 18 + i];
                const float x = compact ? float(i % 4) : float(i);
                const float y = compact ? float(i / 4) : 0.f;
                cell.position =
                    {x * std::cos(angle) - y * std::sin(angle),
                     x * std::sin(angle) + y * std::cos(angle),
                     float(z)};
                cell.lateral_clearance = i < 16 ? 4.f : i == 16 ? 3.4f : 8.f;
                if (i) {
                    connect(z * 18 + i, z * 18 + i - 1);
                }
                if (z) {
                    connect(z * 18 + i, (z - 1) * 18 + i);
                }
            }
        }
        segment_spatial_watershed(field);
        return field;
    };
    const auto corridor = make(false, 0, 1);
    CHECK(corridor.basins.size() == 2);
    CHECK(corridor.cells[0].basin != corridor.cells[17].basin);
    const auto& basin = corridor.basins[corridor.cells[0].basin];
    CHECK(basin.relative_prominence < .25f && basin.salience > .25f);
    CHECK(basin.support_length > 2 * basin.peak_clearance);
    CHECK(corridor.portals.size() == 1);  // Still connected to the room.
    const auto rotated = make(false, .7f, 1);
    CHECK(rotated.basins.size() == 2);
    CHECK(rotated.cells[0].basin != rotated.cells[17].basin);
    const auto compact = make(true, 0, 1);
    CHECK(compact.basins.size() == 1);  // Same relief, compact footprint: suppress it.
    const auto tall = make(true, .7f, 12);
    CHECK(tall.basins.size() == 1);  // Vertical duplication cannot inflate support.
    const auto layered = make(false, .7f, 12);
    CHECK(layered.basins.size() == 2);
    CHECK(std::abs(layered.basins[layered.cells[0].basin].salience - basin.salience) < .001f);
}

void visibility_refinement() {
    const auto wall = [](BakeScene& scene, float x, float y, float X, float Y) {
        quad(scene, {x, y, 0}, {X, Y, 0}, {X, Y, 12}, {x, y, 12});
    };
    const auto single_basin = [](const Scene& scene,
                                 const std::vector<EnvironmentFloorSample>& floors) {
        SpatialWatershedSettings options;
        options.refine_visibility = false;
        auto field = build_spatial_watershed(scene, floors, {}, options);
        CHECK(field.components == 1);
        field.basins.resize(1);
        for (auto& cell : field.cells) {
            cell.basin = 0;
        }
        rebuild_spatial_basins(field);
        return field;
    };
    const auto nearest_basin = [](const SpatialWatershed& field, Float3 p) {
        return std::min_element(
                   field.cells.begin(), field.cells.end(), [&](const auto& a, const auto& b) {
                       const auto squared = [&](Float3 q) {
                           float d = 0;
                           for (unsigned k = 0; k < 3; ++k) {
                               d += (q[k] - p[k]) * (q[k] - p[k]);
                           }
                           return d;
                       };
                       return squared(a.position) < squared(b.position);
                   }
        )->basin;
    };
    BakeScene open;
    floor(open, 0);
    floor(open, 12);
    wall(open, -10, -10, 10, -10);
    wall(open, 10, -10, 10, 10);
    wall(open, 10, 10, -10, 10);
    wall(open, -10, 10, -10, -10);
    // A small post changes a few rays, not the identity of the room's surroundings.
    wall(open, -.2f, -.2f, .2f, -.2f);
    wall(open, .2f, -.2f, .2f, .2f);
    wall(open, .2f, .2f, -.2f, .2f);
    wall(open, -.2f, .2f, -.2f, -.2f);
    Scene open_scene(open);
    std::vector<EnvironmentFloorSample> floors;
    for (int x = -8; x <= 8; x += 2) {
        for (int y = -8; y <= 8; y += 2) {
            if (x || y) {
                floors.push_back(source(float(x), float(y), 0));
            }
        }
    }
    auto room = single_basin(open_scene, floors);
    refine_spatial_visibility(open_scene, room);
    CHECK(room.basins.size() == 1 && room.visibility_splits.empty());

    BakeScene corner;
    floor(corner, 0);
    floor(corner, 12);
    const std::array<Float2, 6> outline{{{-10, -2}, {10, -2}, {10, 10}, {6, 10}, {6, 2}, {-10, 2}}};
    for (unsigned i = 0; i < outline.size(); ++i) {
        const auto a = outline[i], b = outline[(i + 1) % outline.size()];
        wall(corner, a[0], a[1], b[0], b[1]);
    }
    Scene corner_scene(corner);
    floors.clear();
    for (int x = -9; x <= 9; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            floors.push_back(source(float(x), float(y), 0));
        }
    }
    for (int x = 7; x <= 9; x += 2) {
        for (int y = 3; y <= 9; y += 2) {
            floors.push_back(source(float(x), float(y), 0));
        }
    }
    auto bent = single_basin(corner_scene, floors);
    refine_spatial_visibility(corner_scene, bent);
    CHECK(bent.basins.size() > 1 && !bent.visibility_splits.empty());
    CHECK(nearest_basin(bent, {-7, 0, 1.5f}) != nearest_basin(bent, {8, 8, 1.5f}));
    CHECK(!bent.portals.empty());
    for (const auto& split : bent.visibility_splits) {
        CHECK(split.before - split.after >= .12f);
    }

    BakeScene doorway;
    floor(doorway, 0);
    floor(doorway, 12);
    wall(doorway, -10, -6, 10, -6);
    wall(doorway, 10, -6, 10, 6);
    wall(doorway, 10, 6, -10, 6);
    wall(doorway, -10, 6, -10, -6);
    wall(doorway, 0, -6, 0, -1);
    wall(doorway, 0, 1, 0, 6);
    Scene doorway_scene(doorway);
    floors.clear();
    for (int x = -9; x <= 9; x += 2) {
        for (int y = -5; y <= 5; y += 2) {
            floors.push_back(source(float(x), float(y), 0));
        }
    }
    auto shared = single_basin(doorway_scene, floors);
    CHECK(!doorway_scene.occluded({-4, 0, 1.5f}, {1, 0, 0}, .01f, 7.99f));
    refine_spatial_visibility(doorway_scene, shared);
    // Seeing one another through an opening does not imply seeing the same room.
    CHECK(nearest_basin(shared, {-4, 0, 1.5f}) != nearest_basin(shared, {4, 0, 1.5f}));
}
}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--stress-tiled") {
        BakeScene geometry;
        quad(geometry, {-2, -2, 0}, {580, -2, 0}, {580, 580, 0}, {-2, 580, 0});
        std::vector<EnvironmentFloorSample> floors;
        for (unsigned y = 0; y < 290; ++y) {
            for (unsigned x = 0; x < 290; ++x) {
                floors.push_back(source(float(x * 2), float(y * 2), 0));
            }
        }
        SpatialWatershedSettings options;
        options.refine_visibility = false;
        const auto field = build_spatial_watershed(geometry, floors, {}, options, {6});
        const auto previous_estimate = 160ull * field.cells.size() + 48ull * field.link_count();
        CHECK(previous_estimate > options.graph_memory_budget);
        CHECK(field.uses_tiled_links() && field.components == 1 && field.basins.size() == 1);
        std::cout << field.cells.size() << " cells, " << field.link_count()
                  << " links, legacy estimate " << previous_estimate / (1024 * 1024) << " MiB\n";
        return openrac::test::result();
    }
    stacked_layers();
    blocked_wall();
    doorway();
    slope_and_water();
    prominence();
    compatible_heights();
    limits_and_roots();
    separate_portal_patches();
    whole_peak_plateau();
    corridor_shape_support();
    visibility_refinement();
    return openrac::test::result();
}
