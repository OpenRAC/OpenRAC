// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Dyadic longest-edge rest-mesh templates, preserving art layers and first-use shape order.
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <stdexcept>
#include <utility>

#include "common/profiler.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water {
namespace {
using Shape = std::array<std::int32_t, 4>;
using Uv = std::array<double, 2>;

// One first-use quantised shape and its ordered triangle-list barycentric coordinates.
struct Template {
    Shape shape{};             // XY edge vectors quantised to 1/256 world units.
    std::vector<Uv> vertices;  // Dyadic barycentric coordinates, retaining subdivision order.
};

// Classifies translations of the same authored triangle under the existing quantisation.
Shape shape(const Triangle& triangle) {
    Shape result{};

    // Shape identity stores both source edges relative to the first vertex.
    for (unsigned vertex = 1; vertex < 3; ++vertex) {
        // Only XY geometry determines subdivision density.
        for (unsigned axis = 0; axis < 2; ++axis) {
            result[(vertex - 1) * 2 + axis] = static_cast<std::int32_t>(
                std::floor((triangle[vertex][axis] - triangle[0][axis]) * 256 + .5)
            );
        }
    }

    return result;
}

// Finds the first longest XY edge from barycentrically reconstructed positions.
unsigned longest(const Triangle& triangle, const Triangle& weights) {
    Triangle positions{};

    // Reconstructed points retain the reference summation order.
    for (unsigned vertex = 0; vertex < 3; ++vertex) {
        // Only horizontal coordinates contribute to edge length.
        for (unsigned axis = 0; axis < 2; ++axis) {
            // All barycentric components contribute without rounding to float32.
            for (unsigned component = 0; component < 3; ++component) {
                positions[vertex][axis] += triangle[component][axis] * weights[vertex][component];
            }
        }
    }

    std::array<double, 3> lengths{};

    // Stable edge order resolves equal-length ties.
    for (unsigned edge = 0; edge < 3; ++edge) {
        // Squared XY distance avoids an unnecessary root.
        for (unsigned axis = 0; axis < 2; ++axis) {
            const double delta = positions[edge][axis] - positions[(edge + 1) % 3][axis];
            lengths[edge] += delta * delta;
        }
    }

    return static_cast<unsigned>(
        std::max_element(lengths.begin(), lengths.end()) - lengths.begin()
    );
}

// Recursively splits one triangle until every edge meets the half-unit density limit.
void subdivide(
    const Triangle& triangle, const Triangle& weights, std::vector<Uv>& vertices, unsigned depth
) {
    // Bound recursive depth and template storage before extending output.
    if (depth > 64 || vertices.size() > kAtlasCells) {
        throw std::runtime_error("water mesh subdivision exceeds budget");
    }

    const unsigned edge = longest(triangle, weights);
    Point first{};
    Point second{};

    // Both selected endpoints use the original barycentric reconstruction.
    for (unsigned axis = 0; axis < 2; ++axis) {
        // Component order preserves the subdivision threshold decision.
        for (unsigned component = 0; component < 3; ++component) {
            first[axis] += triangle[component][axis] * weights[edge][component];
            second[axis] += triangle[component][axis] * weights[(edge + 1) % 3][component];
        }
    }

    const double x = first[0] - second[0];
    const double y = first[1] - second[1];

    // The reference half-unit spacing includes a small squared-distance tolerance.
    if (x * x + y * y <= .25 + 1e-6) {
        // Leaf vertices retain their original triangle-list ordering.
        for (const auto& weight : weights) {
            vertices.push_back({weight[1], weight[2]});
        }

        return;
    }

    const auto& a = weights[edge];
    const auto& b = weights[(edge + 1) % 3];
    const auto& c = weights[(edge + 2) % 3];
    Point middle{};

    // Dyadic midpoints preserve exact barycentric fractions.
    for (unsigned axis = 0; axis < 3; ++axis) {
        middle[axis] = (a[axis] + b[axis]) * .5;
    }

    subdivide(triangle, {a, middle, c}, vertices, depth + 1);
    subdivide(triangle, {middle, b, c}, vertices, depth + 1);
}

// Adds a template only at its first encounter, preserving binary template indices.
void add_template(
    const Triangle& triangle,
    std::map<Shape, std::size_t>& indices,
    std::vector<Template>& templates,
    std::size_t& total
) {
    const auto key = shape(triangle);

    // Translated triangles can share an already generated template.
    if (indices.contains(key)) {
        return;
    }

    indices.emplace(key, templates.size());
    Template item;
    item.shape = key;
    subdivide(triangle, Triangle{Point{1, 0, 0}, Point{0, 1, 0}, Point{0, 0, 1}}, item.vertices, 0);
    total += item.vertices.size();

    // Template limits cover the complete mesh, not only one unusually large shape.
    if (total > kAtlasCells) {
        throw std::runtime_error("water mesh templates exceed the vertex budget");
    }

    templates.push_back(std::move(item));
}

// Writes one original face without flattening or duplicating the retained art layers.
void write_triangle(std::ostream& output, unsigned source, const Triangle& triangle) {
    write_word(output, source);

    // Original corner order is part of the rest-mesh format.
    for (const auto& point : triangle) {
        // Coordinates narrow only at the float32 boundary.
        for (double value : point) {
            write_float(output, value);
        }
    }
}

// Emits source triangles separately from their shared subdivision templates.
void write_surfaces(std::ostream& output, const Input& input) {
    // Authored art layers remain separate in the rest mesh.
    for (const auto& surface : input.surfaces) {
        // Every original triangle retains its source identifier.
        for (const auto& triangle : surface.triangles) {
            write_triangle(output, surface.source, triangle);
        }
    }
}
}  // namespace

void write_mesh(const std::filesystem::path& path, const Input& input) {
    const profiling::Zone timing("Water rest mesh");
    std::vector<Template> templates;
    std::map<Shape, std::size_t> indices;
    std::size_t count = 0;
    std::size_t total = 0;

    // First-use surface order determines template order.
    for (const auto& surface : input.surfaces) {
        count += surface.triangles.size();

        // Each distinct shape is subdivided only once.
        for (const auto& triangle : surface.triangles) {
            add_template(triangle, indices, templates, total);
        }
    }

    const double dx = input.pool_spacing[0];
    const double dy = input.pool_spacing[1];
    add_template({Point{0, 0, 0}, Point{dx, 0, 0}, Point{0, dy, 0}}, indices, templates, total);
    add_template({Point{dx, 0, 0}, Point{0, dy, 0}, Point{dx, dy, 0}}, indices, templates, total);
    std::ofstream output(path, std::ios::binary);
    output.write("ORWMESH2", 8);
    write_word(output, static_cast<std::uint32_t>(count));
    write_word(output, static_cast<std::uint32_t>(templates.size()));
    // Both files carry the same raw source fingerprint.
    output.write(reinterpret_cast<const char*>(input.fingerprint.data()), input.fingerprint.size());

    write_surfaces(output, input);

    // Templates are written in first-use order rather than sorted shape order.
    for (const auto& item : templates) {
        // Signed quantised edges are written as their little-endian bit patterns.
        for (const auto value : item.shape) {
            write_word(output, static_cast<std::uint32_t>(value));
        }

        write_word(output, static_cast<std::uint32_t>(item.vertices.size()));

        // Dyadic barycentric maps stay associated with their owning shape.
        for (const auto& uv : item.vertices) {
            // Each template vertex stores two float32 barycentric coordinates.
            for (double value : uv) {
                write_float(output, value);
            }
        }
    }

    output.close();

    // Delayed stream errors must stop publication of the pair.
    if (!output) {
        throw std::runtime_error("cannot write water rest mesh");
    }
}
}  // namespace openrac::lighting::water
