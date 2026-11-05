// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace openrac::lighting::interiors {
using Point = std::array<float, 3>;

struct Triangle {
    std::array<Point, 3> vertices;
    bool support = true;  // water may block rays but is not walkable ground
};

struct Transition {
    unsigned id = 0;
    // World to local, row-vector layout, as stored by RAC1.
    std::array<float, 16> inverse{};
    std::array<int, 2> light{};
    std::array<std::array<unsigned, 3>, 2> ambient{};
};

// A world-space seed for an existing light set and ambient colour.
struct LightingSource {
    Point position;                     // Enclosed-air position.
    int light = 0;                      // Level light-set index.
    std::array<unsigned, 3> ambient{};  // Ambient RGB bytes.
    Point up{0, 0, 1};                  // Local deck normal for ceiling lighting.
    unsigned frame = 0;                 // Shared lighting frame ID. Zero is world up.
};

struct Settings {
    float spacing = 2, height = 2.5f, radius = 0.45f, step = 2;
    float slope_cosine = 0.65f, sky_distance = 128;
    unsigned max_cells = 2000000;
    // Triangulated two-sided polygons used only for lighting connectivity.
    std::vector<Triangle> lighting_barriers;
    std::vector<LightingSource> lighting_sources;  // Authored indoor seeds, in input order.
};

struct Cell {
    Point position;
    unsigned component = 0;
    float sky = 0;
};

struct Seed {
    unsigned transition = 0, side = 0;
    int cell = -1;
    unsigned label = 0;
    Point target{};
};

struct Region {
    std::vector<unsigned> cells, labels;
    float sky = 0;
};

// Authored coverage leaves unreachable cells on the existing transition path.
inline constexpr unsigned kUnreachedSourceCell = ~0u;
inline constexpr unsigned kConflictingSourceCell = ~1u;

struct Result {
    std::vector<Cell> cells;
    std::vector<Seed> seeds;
    std::vector<unsigned> source_cell_labels;  // Optional per-cell overrides in floor sample order.
    std::vector<unsigned> source_labels;       // Label per authored source, or ~0u if rejected.
    std::vector<Region> regions;
    // Physical walkability includes edges across authored lighting transitions.
    std::vector<std::array<unsigned, 2>> walkable_edges;
    unsigned blocked_portal_edges = 0;
};

Point local(const Transition& transition, Point world);
Point world(const Transition& transition, Point local);
bool crosses_transition(const Transition& transition, Point a, Point b);
// Collision-only, winding-independent sampled walkability. This is not a
// gameplay navigation mesh, and creates no lighting/bake output.
Result infer(
    const std::vector<Triangle>& triangles,
    const std::vector<Transition>& transitions,
    const Settings& settings = {}
);
}  // namespace openrac::lighting::interiors
