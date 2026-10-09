// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/volumes.rs
// and crates/rc-game/src/moby_update/triggers.rs: ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// The shape and grind path sections, and the point tests over them.

#include "assets/world/volumes.h"

#include <cmath>

namespace openrac::assets {

namespace {

GameplaySection section_of(ShapeKind kind) {
    switch (kind) {
        case ShapeKind::Cuboid:
            return GameplaySection::Cuboids;
        case ShapeKind::Sphere:
            return GameplaySection::Spheres;
        case ShapeKind::Cylinder:
            return GameplaySection::Cylinders;
        case ShapeKind::Pill:
            return GameplaySection::Pills;
    }
    return GameplaySection::Cuboids;
}

// The section's offset and record count, or {0, 0} when absent.
std::pair<std::size_t, std::size_t> counted(const GameplayFile& file, GameplaySection section) {
    const std::size_t s = file.offset(section);
    if (s == 0) {
        return {0, 0};
    }
    const s32 n = file.bytes().s32_at(s);
    if (n < 0 || n > 0x10000) {
        fail("gameplay: implausible {} count {}", gameplay_section_name(section), n);
    }
    return {s, static_cast<std::size_t>(n)};
}

}  // namespace

s32 shape_grid_type(ShapeKind kind) {
    switch (kind) {
        case ShapeKind::Cuboid:
            return 3;
        case ShapeKind::Sphere:
            return 5;
        case ShapeKind::Cylinder:
            return 6;
        case ShapeKind::Pill:
            return 7;
    }
    return 0;
}

std::array<float, 3> GameplayShape::local(const std::array<float, 3>& p) const {
    const auto c = centre();
    const std::array<float, 3> d{p[0] - c[0], p[1] - c[1], p[2] - c[2]};
    std::array<float, 3> l{};
    for (std::size_t k = 0; k < 3; ++k) {
        l[k] = inverse[0][k] * d[0] + inverse[1][k] * d[1] + inverse[2][k] * d[2];
    }
    return l;
}

std::array<float, 3> GameplayShape::world(const std::array<float, 3>& l) const {
    std::array<float, 3> p{};
    for (std::size_t k = 0; k < 3; ++k) {
        p[k] = l[0] * matrix[0][k] + l[1] * matrix[1][k] + l[2] * matrix[2][k] + matrix[3][k];
    }
    return p;
}

std::vector<GameplayShape> read_shapes(const GameplayFile& file, ShapeKind kind) {
    const auto [s, n] = counted(file, section_of(kind));
    const ByteView g = file.bytes();
    std::vector<GameplayShape> out;
    if (s == 0) {
        return out;
    }
    g.check(s + 0x10, n * kShapeRecordSize, "shape records");
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t r = s + 0x10 + i * kShapeRecordSize;
        GameplayShape shape;
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t k = 0; k < 4; ++k) {
                shape.matrix[row][k] = g.f32_at(r + 16 * row + 4 * k);
            }
        }
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t k = 0; k < 4; ++k) {
                shape.inverse[row][k] = g.f32_at(r + 0x40 + 16 * row + 4 * k);
            }
        }
        for (std::size_t k = 0; k < 3; ++k) {
            shape.euler[k] = g.f32_at(r + 0x70 + 4 * k);
        }
        shape.unused_7c = g.f32_at(r + 0x7c);
        out.push_back(shape);
    }
    return out;
}

std::vector<float> read_pill_cap_radii(const GameplayFile& file) {
    const auto [s, n] = counted(file, GameplaySection::Pills);
    std::vector<float> out;
    for (std::size_t i = 0; s != 0 && i < n; ++i) {
        out.push_back(file.bytes().f32_at(s + 0x10 + i * kShapeRecordSize + kShapeRecordSize));
    }
    return out;
}

std::vector<GrindPath> read_grind_paths(const GameplayFile& file) {
    const auto [s, n] = counted(file, GameplaySection::GrindPaths);
    const ByteView g = file.bytes();
    std::vector<GrindPath> out;
    if (s == 0) {
        return out;
    }
    const s32 data_offset = g.s32_at(s + 4);
    const s32 data_size = g.s32_at(s + 8);
    if (data_offset < 0 || data_size < 0) {
        fail("gameplay: negative grind path data field");
    }
    const ByteView data = g.sub(
        s + static_cast<std::size_t>(data_offset),
        static_cast<std::size_t>(data_size),
        "grind path data"
    );
    const std::size_t offsets = s + 0x10 + n * 0x20;
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t r = s + 0x10 + i * 0x20;
        GrindPath path;
        for (std::size_t k = 0; k < 4; ++k) {
            path.bounding_sphere[k] = g.f32_at(r + 4 * k);
        }
        path.flag = g.s32_at(r + 0x10);
        path.wrap = g.s32_at(r + 0x14);
        path.inactive = g.s32_at(r + 0x18);
        const s32 at = g.s32_at(offsets + 4 * i);
        if (at < 0) {
            fail("grind path {}: negative offset", i);
        }
        const s32 count = data.s32_at(static_cast<std::size_t>(at));
        if (count < 0) {
            fail("grind path {}: negative point count", i);
        }
        path.points = data.read_array<std::array<float, 4>>(
            static_cast<std::size_t>(at) + 0x10,
            static_cast<std::size_t>(count),
            "grind path points"
        );
        out.push_back(std::move(path));
    }
    return out;
}

bool in_unit_box(const std::array<float, 3>& l) {
    for (float x : l) {
        if (!(x >= -1.0f && x <= 1.0f)) {
            return false;
        }
    }
    return true;
}

bool point_in_cuboid(const GameplayShape& cuboid, const std::array<float, 3>& p) {
    return in_unit_box(cuboid.local(p));
}

bool point_in_cylinder(const GameplayShape& cylinder, const std::array<float, 3>& p) {
    const auto l = cylinder.local(p);
    return std::sqrt(l[0] * l[0] + l[1] * l[1]) < 1.0f && l[2] >= -1.0f && l[2] <= 1.0f;
}

bool point_in_sphere(const GameplayShape& sphere, const std::array<float, 3>& p) {
    const auto l = sphere.local(p);
    return std::sqrt(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]) < 1.0f;
}

bool point_in_path_polygon(
    const std::array<float, 3>& p, std::span<const std::array<float, 4>> points
) {
    bool inside = false;
    const std::size_t n = points.size();
    for (std::size_t k = 0; k < n; ++k) {
        const auto& a = points[k];
        const auto& b = points[(k + 1) % n];
        const bool spans = (a[1] < p[1] && p[1] <= b[1]) || (b[1] < p[1] && p[1] <= a[1]);
        if (spans && a[0] + ((p[1] - a[1]) / (b[1] - a[1])) * (b[0] - a[0]) < p[0]) {
            inside = !inside;
        }
    }
    return inside;
}

}  // namespace openrac::assets
