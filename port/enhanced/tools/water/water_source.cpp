// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// water_source.h's profile-driven decoder, producing the same generic request as native numerical inputs.
#include "enhanced/tools/water/water_source.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "common/profiler.h"
#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/water/water_source_internal.h"

namespace openrac::lighting::water {
namespace {
// Identifies current source semantics; old Python fingerprints intentionally require regeneration.
std::string fingerprint(const std::filesystem::path& level, const json::Value& config) {
    assets::disc::Sha256 hash;
    hash.update("native-water-sources-v1");
    hash.update(bake_json::json_text(config));
    bake_json::hash_file(hash, level / "bake-inputs/overlay.bin");
    bake_json::hash_file(hash, level / "collision.glb");

    // Optional authored outlets affect both fields and the matching rest-mesh identity.
    if (std::filesystem::is_regular_file(level / "water-boundaries.json")) {
        bake_json::hash_file(hash, level / "water-boundaries.json");
    }

    return assets::disc::to_hex(hash.finish());
}

// Serialises exact decoded doubles independently of the caller's stream formatting.
void vector(std::ostream& output, Point point) {
    output << '[' << point[0] << ',' << point[1] << ',' << point[2] << ']';
}

// Writes one generic water body, retaining two-sided triangle and art-layer ordering.
void surface(std::ostream& output, const Surface& value) {
    output << "{\"source\":" << value.source << ",\"pond\":" << (value.pond ? "true" : "false")
           << ",\"flowing\":" << (value.flowing ? "true" : "false") << ",\"flow\":";
    vector(output, value.flow);
    output << ",\"triangles\":[";

    // Vertex order must remain unchanged for byte-identical numerical and rest-mesh results.
    for (std::size_t index = 0; index < value.triangles.size(); ++index) {
        if (index) {
            output << ',';
        }

        output << '[';
        vector(output, value.triangles[index][0]);
        output << ',';
        vector(output, value.triangles[index][1]);
        output << ',';
        vector(output, value.triangles[index][2]);
        output << ']';
    }

    output << "]}";
}

// Expands a closed set of table operations without embedding per-game source knowledge.
std::vector<Surface> surfaces(const source::Image& image, const json::Value& tables) {
    // Table and resulting geometry counts are bounded before request serialization.
    if (!tables.is_array() || tables.items().empty() || tables.items().size() > 64) {
        throw std::runtime_error("invalid water source table list");
    }

    std::vector<Surface> result;
    std::size_t triangles = 0;

    // Physical body order follows configured table order, never hash-map iteration order.
    for (const auto& table : tables.items()) {
        // The operation name selects only one of the implemented declarative formats.
        if (table["kind"].string() == "masked_tiles") {
            auto bodies = source::tiles(image, table);
            result.insert(
                result.end(),
                std::make_move_iterator(bodies.begin()),
                std::make_move_iterator(bodies.end())
            );
        } else if (table["kind"].string() == "triangle_strips") {
            auto body = source::strips(image, table);

            // Degenerate-only art layers do not create empty physical bodies.
            if (!body.triangles.empty()) {
                result.push_back(std::move(body));
            }
        } else {
            throw std::runtime_error("unsupported water source table kind");
        }

        triangles = 0;

        // Bound aggregate storage after every operation, before expanding another table.
        for (const auto& body : result) {
            triangles += body.triangles.size();
        }

        // Per-operation limits cannot permit aggregate geometry to exhaust memory.
        if (result.size() > 64 || triangles > 262'144) {
            throw std::runtime_error("water source geometry exceeds its budget");
        }
    }

    // No decoded geometry means there is no physical water body to bake.
    if (result.empty()) {
        throw std::runtime_error("water source tables have no nondegenerate surfaces");
    }

    return result;
}

// Writes a generic request consumed by the existing native numerical and collision validation stages.
json::Value request(
    const std::filesystem::path& level,
    const json::Value& config,
    const std::vector<Surface>& bodies
) {
    const auto& spacing = config["pool_spacing"];

    // Rest-grid dimensions must be explicit and positive before subdivision.
    if (!spacing.is_array() || spacing.items().size() != 2 || source::scalar(spacing[0]) <= 0
        || source::scalar(spacing[1]) <= 0) {
        throw std::runtime_error("invalid water source pool spacing");
    }

    const auto boundaries =
        bake_json::object_value(bake_json::optional_object(level / "water-boundaries.json"));
    std::ostringstream output;
    output << std::setprecision(17) << "{\"version\":1,\"collision\":"
           << coordinator_string((level / "collision.glb").string())
           << ",\"fingerprint\":" << coordinator_string(fingerprint(level, config))
           << ",\"pool_spacing\":" << bake_json::json_text(spacing)
           << ",\"boundaries\":" << bake_json::json_text(boundaries) << ",\"surfaces\":[";

    // Stable body order determines all runtime source and atlas records.
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        if (index) {
            output << ',';
        }

        surface(output, bodies[index]);
    }

    output << "]}";
    json::Value result;
    std::string error;

    // Serialization failures cannot become a valid native request.
    if (!json::parse(output.str(), result, error)) {
        throw std::runtime_error("cannot encode water source request: " + error);
    }

    return result;
}
}  // namespace

Result<json::Value> decode_sources(const std::filesystem::path& level) {
    // Profile and source errors leave the public boundary as an owned Result.
    try {
        const profiling::Zone timing("Decode water source tables");
        const auto profile = bake_json::document(level / "probe-bake.json");
        const auto& config = profile["water_sources"];
        source::keys(config, {"version", "format", "image_bytes", "pool_spacing", "tables"});

        // Only implemented source container semantics may enter the generic decoder.
        if (config["version"].number() != 1 || config["format"].string() != "sectioned_le32") {
            throw std::runtime_error("unsupported water source profile; re-export the level");
        }

        const auto size = source::integer(config["image_bytes"], 64ull * 1024 * 1024);
        const auto image = source::load_image(level / "bake-inputs/overlay.bin", size);
        return request(level, config, surfaces(image, config["tables"]));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting::water
