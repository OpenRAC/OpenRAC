// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// water_internal.h input decoding; source layouts remain in the per-level bake profile.
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/water/water_internal.h"
#include "scene/level.h"

namespace openrac::lighting::water {
namespace {
// Rejects invalid coordinates before bounding-box arithmetic or integer conversion.
Point point(const json::Value& row) {
    // Point arity is checked before reading any coordinate.
    if (!row.is_array() || row.items().size() != 3) {
        throw std::runtime_error("water points require three coordinates");
    }

    Point result{};

    // Runtime water coordinates stay within the reference tool's supported world domain.
    for (unsigned axis = 0; axis < 3; ++axis) {
        // Invalid coordinates cannot enter raster bounds or native integer conversions.
        if (!row[axis].is_number() || !std::isfinite(row[axis].number())
            || std::abs(row[axis].number()) >= 100'000) {
            throw std::runtime_error("invalid water coordinates");
        }

        result[axis] = row[axis].number();
    }

    return result;
}

// Retains original authored triangle order, bounded before rasterisation.
std::vector<Triangle> triangles(const json::Value& rows) {
    // Bound surface storage before accepting arbitrary request arrays.
    if (!rows.is_array() || rows.items().empty() || rows.items().size() > 262'144) {
        throw std::runtime_error("invalid water surface triangle count");
    }

    std::vector<Triangle> result;

    // Each triangle has three independently validated vertices.
    for (const auto& row : rows.items()) {
        // Every authored triangle must contain exactly three vertices.
        if (!row.is_array() || row.items().size() != 3) {
            throw std::runtime_error("incomplete water triangle");
        }

        result.push_back({point(row[0]), point(row[1]), point(row[2])});
    }

    return result;
}

// Copies validated world positions from the shared CPU model decoder.
Triangle collision_triangle(const scene::LevelData& mesh, std::size_t first) {
    Triangle triangle{};

    // Collision is an indexed triangle list with three finite corners.
    for (unsigned corner = 0; corner < 3; ++corner) {
        const auto& vertex = mesh.vertices.at(mesh.indices.at(first + corner));
        std::copy_n(vertex.position, 3, triangle[corner].begin());

        // Reject invalid positions before any projection or integer conversion.
        for (double value : triangle[corner]) {
            // Malformed geometry must not reach raster arithmetic.
            if (!std::isfinite(value) || std::abs(value) >= 100'000) {
                throw std::runtime_error("invalid water collision coordinate");
            }
        }
    }

    return triangle;
}

// Retains the reference first-corner water palette convention from the collision export.
bool water_vertex(const scene::Vertex& vertex) {
    const Point palette{std::pow(.2, 2.2), std::pow(.45, 2.2), 1};

    // All RGB channels must agree within the existing linear-colour tolerance.
    for (unsigned axis = 0; axis < 3; ++axis) {
        // A single differing channel classifies the face as solid.
        if (std::abs(vertex.colour[axis] - palette[axis]) >= 1e-5) {
            return false;
        }
    }

    return true;
}

// Copies the first-vertex water palette classification used by the Python collision reader.
void collision(Input& input, const std::filesystem::path& path) {
    const profiling::Zone timing("Load water collision");
    scene::LevelData mesh;
    std::string error;
    const auto model = scene::load_model(path, mesh, error);

    // A broken collision export cannot produce a valid waterline.
    if (!model) {
        throw std::runtime_error(error);
    }

    // Collision comes from the existing CPU-only model decoder, without graphics resources.
    for (const auto& primitive : mesh.models.at(*model).primitives) {
        // Incomplete primitives cannot supply reliable collision triangles.
        if (primitive.index_count % 3) {
            throw std::runtime_error("incomplete water collision triangle");
        }

        // All source coordinates are already in exported world space.
        for (std::size_t index = 0; index < primitive.index_count; index += 3) {
            const auto at = static_cast<std::size_t>(primitive.first_index) + index;
            const auto& first = mesh.vertices.at(mesh.indices.at(at));
            (water_vertex(first) ? input.water : input.solids)
                .push_back(collision_triangle(mesh, at));
        }
    }
}

// Reads explicit absorbing outlets; unknown settings never silently alter boundary physics.
void outlets(Input& input, const json::Value& config) {
    // Unknown boundary settings must not silently change physics.
    if (!config.is_object()
        || config.members().size() > (config["open_edges"].is_null() ? 0u : 1u)) {
        throw std::runtime_error("water boundaries only support open_edges");
    }

    const auto& rows = config["open_edges"];

    // Bound outlet storage before decoding its segments.
    if (!rows.is_null() && (!rows.is_array() || rows.items().size() > 4096)) {
        throw std::runtime_error("invalid water outlet list");
    }

    // Each outlet controls an independent world-space absorption strip.
    for (const auto& row : rows.items()) {
        // Unknown outlet keys must not be accepted as an implicit default width.
        for (const auto& [key, value] : row.members()) {
            // Reject misspelled boundary properties instead of silently defaulting them.
            if (key != "a" && key != "b" && key != "width") {
                throw std::runtime_error("invalid water outlet setting");
            }
        }

        const double width = row["width"].number(2);

        // Invalid widths or records cannot define absorption.
        if (!row.is_object() || row.members().size() > 3 || !std::isfinite(width) || width <= 0
            || (!row["width"].is_null() && !row["width"].is_number())) {
            throw std::runtime_error("invalid water outlet");
        }

        input.outlets.push_back({point(row["a"]), point(row["b"]), width});
    }
}

// Validates the shared fingerprint and pool template dimensions before field allocation.
void read_metadata(Input& input, const json::Value& request) {
    const auto& digest = request["fingerprint"].string();

    // Both output files require a complete valid shared fingerprint.
    if (digest.size() != 64 || digest.find_first_not_of("0123456789abcdef") != digest.npos) {
        throw std::runtime_error("invalid water source fingerprint");
    }

    // The preparation script supplies the unchanged SHA-256 format identity.
    for (std::size_t index = 0; index < input.fingerprint.size(); ++index) {
        input.fingerprint[index] =
            static_cast<std::uint8_t>(std::stoul(digest.substr(index * 2, 2), nullptr, 16));
    }

    // Pool subdivision requires two independently configured dimensions.
    if (!request["pool_spacing"].is_array() || request["pool_spacing"].items().size() != 2) {
        throw std::runtime_error("invalid water pool spacing");
    }

    // Validate each pool dimension before generating templates.
    for (unsigned axis = 0; axis < 2; ++axis) {
        const auto& value = request["pool_spacing"][axis];

        // Invalid spacing could cause unbounded rest-mesh subdivision.
        if (!value.is_number() || !std::isfinite(value.number()) || value.number() <= 0
            || value.number() > 64) {
            throw std::runtime_error("invalid water pool spacing");
        }

        input.pool_spacing[axis] = value.number();
    }
}
}  // namespace

Input read_input(const std::filesystem::path& path) {
    const auto request = bake_json::document(path);

    // The request schema and runtime body limit are checked before allocation.
    if (request["version"].number() != 1 || !request["surfaces"].is_array()
        || request["surfaces"].items().empty() || request["surfaces"].items().size() > 64) {
        throw std::runtime_error("unsupported water bake request");
    }

    Input input;
    read_metadata(input, request);

    std::size_t triangle_count = 0;

    // Generic flags carry source-specific semantics without embedding game addresses here.
    for (const auto& row : request["surfaces"].items()) {
        const auto source = static_cast<unsigned>(coordinator_integer(row["source"], 65'535));

        // Generic source flags must remain explicit boolean values.
        if (row["pond"].type() != json::Value::Type::Bool
            || row["flowing"].type() != json::Value::Type::Bool) {
            throw std::runtime_error("invalid water surface kind");
        }

        input.surfaces.push_back(
            {source,
             row["pond"].boolean(),
             row["flowing"].boolean(),
             point(row["flow"]),
             triangles(row["triangles"])}
        );
        triangle_count += input.surfaces.back().triangles.size();

        // The input budget applies across bodies, not independently to every array.
        if (triangle_count > 262'144) {
            throw std::runtime_error("water surface triangles exceed the input budget");
        }
    }

    outlets(input, request["boundaries"]);
    collision(input, request["collision"].string());
    return input;
}
}  // namespace openrac::lighting::water
