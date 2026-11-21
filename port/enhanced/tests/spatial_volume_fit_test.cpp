// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>
#include <map>

#include "enhanced/lighting/spatial_volume_fit.h"
#include "tests/check.h"
using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
SpatialWatershed shape(
    std::vector<std::array<int, 3>> coordinates, ProbeFrame frame = {}, Float3 translation = {}
) {
    SpatialWatershed field;
    field.spans.resize(1);
    field.spans[0].bottom = 0;
    field.spans[0].top = 10;
    field.basins.resize(1);
    std::map<std::array<int, 3>, unsigned> ids;
    for (auto p : coordinates) {
        const auto i = unsigned(field.cells.size());
        ids[p] = i;
        SpatialCell c;
        c.basin = 0;
        c.component = 0;
        c.lateral_clearance = 2;
        c.position = frame.to_world({float(p[0]), float(p[1]), float(p[2])});
        for (unsigned k = 0; k < 3; ++k) {
            c.position[k] += translation[k];
        }
        field.cells.push_back(c);
    }
    for (const auto& [p, i] : ids) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            for (int sign : {-1, 1}) {
                auto q = p;
                q[axis] += sign;
                const auto j = ids.find(q);
                if (j != ids.end() && i < j->second) {
                    field.links.push_back({i, j->second, INFINITY});
                }
            }
        }
    }
    field.index_links();
    return field;
}

void audit(const SpatialWatershed& field, const SpatialVolumePlan& plan) {
    CHECK(plan.uncovered == 0 && plan.covered == field.cells.size());
    std::vector<unsigned> owner(field.cells.size(), ~0u);
    const auto inside = [&](const SpatialFittedVolume& box, Float3 p) {
        for (unsigned k = 0; k < 3; ++k) {
            p[k] -= box.origin[k];
        }
        p = box.frame.to_local(p);
        for (unsigned k = 0; k < 3; ++k) {
            if (p[k] < plan.settings.padding - .02f
                || p[k] > box.extent[k] - plan.settings.padding + .02f) {
                return false;
            }
        }
        return true;
    };
    for (unsigned v = 0; v < plan.volumes.size(); ++v) {
        const auto& box = plan.volumes[v];
        CHECK(box.frame.valid());
        for (auto i : box.cells) {
            CHECK(owner[i] == ~0u);
            owner[i] = v;
            CHECK(box.basin == field.cells[i].basin);
            CHECK(inside(box, field.cells[i].position));
        }
    }
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        CHECK(owner[i] != ~0u);
        for (auto j : field.neighbours(i)) {
            if (owner[i] != owner[j]) {
                Float3 mid{};
                for (unsigned k = 0; k < 3; ++k) {
                    mid[k] = (field.cells[i].position[k] + field.cells[j].position[k]) * .5f;
                }
                CHECK(inside(plan.volumes[owner[i]], mid));
                CHECK(inside(plan.volumes[owner[j]], mid));
            }
        }
    }
}

void rolled_box() {
    std::vector<std::array<int, 3>> cells;
    for (int x = -12; x <= 12; ++x) {
        for (int y = -3; y <= 3; ++y) {
            for (int z = -1; z <= 1; ++z) {
                cells.push_back({x, y, z});
            }
        }
    }
    const ProbeFrame frame{
        {std::cos(.37f), std::sin(.37f)},
        {std::cos(.29f), std::sin(.29f)},
        {std::cos(.63f), std::sin(.63f)}
    };
    const auto field = shape(cells, frame);
    SpatialVolumeFitSettings settings;
    settings.padding = .25f;
    settings.maximum_extent = 100;
    settings.volume_cost = 10000;
    const auto fitted = fit_spatial_volumes(field, settings);
    audit(field, fitted);
    CHECK(fitted.volumes.size() == 1 && fitted.over_budget == 0);
    CHECK(fitted.fitted_volume < 24.5 * 6.5 * 2.5 * 1.02);
    const auto translated = shape(cells, frame, {50000, -30000, 10000});
    const auto moved = fit_spatial_volumes(translated, settings);
    audit(translated, moved);
    CHECK(
        moved.volumes.size() == 1 && std::abs(moved.fitted_volume / fitted.fitted_volume - 1) < .02
    );
    ProbeFrame tiny_roll;
    tiny_roll.roll = {std::cos(.0003f), std::sin(.0003f)};
    const auto almost_aligned = fit_spatial_volumes(shape(cells, tiny_roll), settings);
    CHECK(almost_aligned.volumes.front().frame.roll == Float2({1, 0}));
}

void large_grid() {
    std::vector<std::array<int, 3>> cells;
    for (int x = 0; x <= 60; ++x) {
        for (int y = 0; y <= 60; ++y) {
            for (int z = 0; z <= 2; ++z) {
                cells.push_back({x, y, z});
            }
        }
    }
    const auto field = shape(cells);
    SpatialVolumeFitSettings settings;
    settings.kind = SpatialVolumeKind::gi;
    settings.spacing = {2, 2, 2};
    const auto fitted = fit_spatial_volumes(field, settings);
    audit(field, fitted);
    CHECK(fitted.volumes.size() > 1 && fitted.volumes.size() <= settings.maximum_parts);
    CHECK(fitted.over_budget == 0);
    for (const auto& volume : fitted.volumes) {
        for (auto d : volume.dimensions) {
            CHECK(d <= settings.maximum_dimension);
        }
    }
    settings.maximum_parts = 1;
    const auto bounded = fit_spatial_volumes(field, settings);
    audit(field, bounded);
    CHECK(bounded.over_budget == 1);  // No silent support loss when the budget cannot be met.
}

void corner() {
    std::vector<std::array<int, 3>> cells;
    for (int x = 0; x <= 20; ++x) {
        for (int y = 0; y <= 20; ++y) {
            for (int z = 0; z <= 2; ++z) {
                if (x <= 4 || y <= 4) {
                    cells.push_back({x, y, z});
                }
            }
        }
    }
    const auto field = shape(cells);
    SpatialVolumeFitSettings settings;
    settings.volume_cost = 1;
    settings.maximum_extent = 100;
    const auto fitted = fit_spatial_volumes(field, settings);
    audit(field, fitted);
    // A feasible basin stays one volume even when splitting would shrink its volume.
    CHECK(fitted.volumes.size() == 1 && fitted.over_budget == 0);
}

void shared_gi_budget() {
    std::vector<std::array<int, 3>> cells;
    for (int x = 0; x < 800; ++x) {
        cells.push_back({x, 0, 0});
    }
    cells.push_back({0, 10, 0});
    auto field = shape(cells);
    field.basins.resize(2);
    field.cells.back().basin = 1;

    SpatialVolumeFitSettings settings;
    settings.kind = SpatialVolumeKind::gi;
    settings.spacing = {1, 1, 1};
    settings.maximum_dimension = 8;
    settings.maximum_parts = 0;
    settings.maximum_volumes = 256;
    const auto fitted = fit_spatial_volumes(field, settings);
    audit(field, fitted);
    CHECK(fitted.volumes.size() > 65);
    CHECK(fitted.over_budget == 0 && !fitted.volume_limit_exceeded);

    settings.maximum_volumes = 2;
    const auto bounded = fit_spatial_volumes(field, settings);
    audit(field, bounded);
    CHECK(bounded.volumes.size() == 2 && bounded.over_budget == 1);
    CHECK(bounded.volumes.back().basin == 1 && bounded.volumes.back().feasible);

    settings.maximum_volumes = 1;
    const auto too_many_basins = fit_spatial_volumes(field, settings);
    audit(field, too_many_basins);
    CHECK(too_many_basins.volume_limit_exceeded);
}

void reflection_coverage() {
    BakeScene geometry;
    BakeTriangle roof;
    roof.vertices = {Float3{-10, -10, 40}, Float3{10, -10, 40}, Float3{0, 10, 40}};
    geometry.triangles.push_back(roof);
    SpatialWatershed columns;
    columns.spans.resize(3);
    columns.spans[1].floor = {20, 0, 0};
    columns.spans[2].underwater = true;
    columns.spans[2].top = 5;
    const auto tops = spatial_reflection_coverage_tops(geometry, columns, 30, .01f);
    CHECK(std::abs(tops[0] - 40) < .001f);  // Roof beyond watershed's 16-unit query.
    CHECK(tops[1] == 30 && tops[2] == 5);
    std::vector<std::array<int, 3>> cells;
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 10; ++z) {
                cells.push_back({x, y, z});
            }
        }
    }
    auto field = shape(cells);
    SpatialVolumeFitSettings settings;
    settings.volume_cost = 10000;
    settings.maximum_extent = 100;
    const auto original = fit_spatial_volumes(field, settings);
    CHECK(original.volumes.size() == 1);
    const auto& before = original.volumes.front();
    const auto contains = [](const SpatialFittedVolume& v, Float3 p) {
        for (unsigned k = 0; k < 3; ++k) {
            p[k] -= v.origin[k];
        }
        p = v.frame.to_local(p);
        return p[0] >= -.001f && p[1] >= -.001f && p[2] >= -.001f && p[0] <= v.extent[0] + .001f
               && p[1] <= v.extent[1] + .001f && p[2] <= v.extent[2] + .001f;
    };
    for (float top : {18.f, 30.f}) {
        auto plan = original;
        extend_spatial_reflection_coverage(field, plan, std::array{top});
        audit(field, plan);
        CHECK(plan.volumes.size() == original.volumes.size());
        const auto& after = plan.volumes.front();
        CHECK(after.cells == before.cells && after.representative == before.representative);
        CHECK(
            after.frame.axis == before.frame.axis && after.frame.pitch == before.frame.pitch
            && after.frame.roll == before.frame.roll
        );
        CHECK(contains(after, {1, 1, top}));
        CHECK(after.coverage_growth == 1);
    }
    auto plan = original;
    field.spans[0].underwater = true;
    extend_spatial_reflection_coverage(field, plan, std::array{30.f});
    CHECK(plan.volumes[0].origin == before.origin && plan.volumes[0].extent == before.extent);
    field.spans[0].underwater = false;
    SpatialCell upper;
    upper.basin = 1;
    upper.position = {1, 1, 20};
    for (unsigned i = 0; i < 8; ++i) {
        field.cells.push_back(upper);
    }
    field.index_links();
    plan = original;
    extend_spatial_reflection_coverage(field, plan, std::array{30.f});
    CHECK(plan.volumes[0].coverage_growth < 1);
    CHECK(!contains(plan.volumes[0], upper.position));
    plan = original;
    plan.settings.kind = SpatialVolumeKind::gi;
    extend_spatial_reflection_coverage(field, plan, std::array{30.f});
    CHECK(plan.volumes[0].origin == before.origin && plan.volumes[0].extent == before.extent);
}
}  // namespace

int main() {
    rolled_box();
    large_grid();
    shared_gi_budget();
    corner();
    reflection_coverage();
    return openrac::test::result();
}
