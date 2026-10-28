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

struct Settings {
    float spacing = 2, height = 2.5f, radius = 0.45f, step = 2;
    float slope_cosine = 0.65f, sky_distance = 128;
    unsigned max_cells = 2000000;
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

struct Result {
    std::vector<Cell> cells;
    std::vector<Seed> seeds;
    std::vector<Region> regions;
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
