// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// emission_audit.h's native inventory, preserving the Python report's thresholds and source order.
#include "enhanced/tools/baker/sources/emission_audit.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "scene/level.h"

namespace openrac::lighting {
namespace {
// Static layer identity, authored output clamp and packed light-table length.
struct AuditLayer {
    scene::Layer layer;  // Decoded instance collection.
    const char* name;    // Existing diagnostic layer spelling.
    unsigned slots;      // Valid colours per instance, or zero for terrain.
    unsigned bit;        // One bit per static layer in candidate membership.
    double maximum;      // Authored encoded colour clamp.
};

// Clamp values match the original Python inventory; GS unity is byte 128 (measured: audit parity).
constexpr std::array<AuditLayer, 3> kLayers{
    {{scene::Layer::Terrain, "terrain", 0, 1, 255.0 / 128},
     {scene::Layer::Ties, "tie", 64, 2, 243.0 / 128},
     {scene::Layer::Shrubs, "shrub", 24, 4, 243.0 / 128}}
};

// One reviewed-or-undefined texture inventory; source order determines its initial defined state.
struct Candidate {
    std::uint64_t triangles = 0;  // Full-bright triangles across all static placements.
    unsigned layers = 0;          // Layer membership bits, serialized in alphabetical name order.
    bool defined = false;         // Explicit glTF emission or a texture-keyed profile object.
};

// Borrows decoded geometry and the profile only for this controller-thread audit.
struct Inventory {
    const scene::LevelData& level;   // Owned by the current native level controller.
    const bake_json::Object& edits;  // Owned by audit_level_emission for the full traversal.
    std::filesystem::path root;      // Canonical export root, resolved once per audit.
    std::map<int, std::string> texture_keys;      // Lazy source URI cache, shared by all instances.
    std::map<std::string, Candidate> candidates;  // Sorted texture-keyed diagnostic records.
};

// Tests original prelighting, ignoring instance tint and retaining unsupported-slot mesh colours.
bool bright_vertex(
    const scene::LevelData& level,
    const scene::Instance& instance,
    const scene::Vertex& vertex,
    const AuditLayer& layer
) {
    // Three RGB channels must reach the original static renderer's clamp.
    for (unsigned channel = 0; channel < 3; ++channel) {
        double colour = vertex.colour[channel];

        // Packed light slots replace mesh colours only within this instance's complete authored table.
        if (instance.lights >= 0 && std::isfinite(vertex.light_slot) && vertex.light_slot >= 0
            && vertex.light_slot < static_cast<float>(layer.slots)) {
            const auto at = static_cast<std::size_t>(instance.lights)
                            + static_cast<std::size_t>(vertex.light_slot);
            colour = static_cast<double>((level.light_colours.at(at) >> (8 * channel)) & 255) / 128;
        }

        // The reference tolerance applies in double precision to exact authored float values.
        if (!(colour >= layer.maximum - 1e-5)) {
            return false;
        }
    }

    return true;
}

// Counts only complete bright triangles, retaining shared mesh data rather than copying per instance.
std::uint64_t bright_triangles(
    const scene::LevelData& level,
    const scene::Instance& instance,
    const scene::Primitive& primitive,
    const AuditLayer& layer
) {
    // A partial triangle cannot be classified reliably.
    if (primitive.index_count % 3 != 0) {
        throw std::runtime_error("incomplete triangle in emission audit");
    }

    std::uint64_t count = 0;

    // Each primitive is an indexed triangle list in the decoded scene.
    for (std::uint32_t index = 0; index < primitive.index_count; index += 3) {
        bool bright = true;

        // All three corners must reach the authored full-bright clamp.
        for (unsigned corner = 0; corner < 3; ++corner) {
            const auto vertex =
                level.indices.at(static_cast<std::size_t>(primitive.first_index) + index + corner);
            bright = bright && bright_vertex(level, instance, level.vertices.at(vertex), layer);
        }

        count += bright;
    }

    return count;
}

// Resolves a referenced source URI once per image, retaining canonical containment checks.
const std::string& texture_key(Inventory& inventory, int image) {
    const auto found = inventory.texture_keys.find(image);

    // Repeated static placements share the same source texture identity.
    if (found != inventory.texture_keys.end()) {
        return found->second;
    }

    const auto path = std::filesystem::weakly_canonical(
        inventory.level.image_paths.at(static_cast<std::size_t>(image))
    );
    const auto relative = path.lexically_relative(inventory.root);

    // Referenced images must remain within the level's exported texture namespace.
    if (relative.empty() || relative.has_root_path() || *relative.begin() == "..") {
        throw std::runtime_error("emission audit texture is outside the level export");
    }

    return inventory.texture_keys.emplace(image, relative.generic_string()).first->second;
}

// Adds one primitive's source-texture record without affecting its transport emission settings.
void inspect_primitive(
    Inventory& inventory,
    const scene::Instance& instance,
    const scene::Primitive& primitive,
    const AuditLayer& layer
) {
    const auto& level = inventory.level;
    const auto& material = level.materials.at(static_cast<std::size_t>(primitive.material));

    // Untextured surfaces have no texture identity for explicit emission review.
    if (material.image < 0) {
        return;
    }

    const auto count = bright_triangles(level, instance, primitive, layer);

    // Ordinary shaded surfaces produce no full-bright candidate record.
    if (count == 0) {
        return;
    }

    const auto& key = texture_key(inventory, material.image);
    const auto edit = inventory.edits.find(key);
    const bool defined =
        material.authored_emission || (edit != inventory.edits.end() && edit->second.is_object());
    const auto found = inventory.candidates.try_emplace(key, Candidate{0, 0, defined}).first;
    found->second.triangles += count;
    found->second.layers |= layer.bit;
}

// Visits one static layer without loading another copy of its geometry.
void inspect_layer(Inventory& inventory, const AuditLayer& layer) {
    const auto& level = inventory.level;

    // Original placement order determines each texture's initial definition state.
    for (const auto& instance : level.instances[static_cast<std::size_t>(layer.layer)]) {
        // Stand-in debug boxes are not authored static surfaces.
        if (instance.stand_in) {
            continue;
        }

        // Decoded static primitives retain the source draw order.
        for (const auto& primitive : level.models.at(instance.model).primitives) {
            inspect_primitive(inventory, instance, primitive, layer);
        }
    }
}

// Emits the unchanged diagnostic schema with deterministic texture and layer ordering.
std::string report(const std::map<std::string, Candidate>& candidates) {
    std::ostringstream output;
    output << "{\"version\":1,\"note\":\"Review candidates only; brightness does not prove "
              "emission.\",\"candidates\":[";
    bool comma = false;

    // Sorted keys keep this diagnostic stable across scene preparation and worker counts.
    for (const auto& [texture, candidate] : candidates) {
        // JSON separators belong between candidates only.
        if (comma) {
            output << ',';
        }

        comma = true;
        output << "{\"texture\":" << coordinator_string(texture)
               << ",\"fullbright_triangles\":" << candidate.triangles
               << ",\"defined\":" << (candidate.defined ? "true" : "false") << ",\"layers\":[";
        bool layer_comma = false;

        // A shared texture can occur in several static layer types.
        // The reference sorts layer names alphabetically: shrub, terrain, tie.
        for (const auto index : {2u, 0u, 1u}) {
            const auto& layer = kLayers[index];

            // Each texture record includes only the layers containing its bright triangles.
            if ((candidate.layers & layer.bit) == 0) {
                continue;
            }

            // JSON separators belong between layer names only.
            if (layer_comma) {
                output << ',';
            }

            layer_comma = true;
            output << coordinator_string(layer.name);
        }

        output << "]}";
    }

    return output.str() + "]}\n";
}
}  // namespace

Result<std::size_t> audit_level_emission(
    const scene::LevelData& level, const std::filesystem::path& output_directory
) {
    // Audit failures propagate before the native batch publishes any verified lighting.
    try {
        const profiling::Zone zone("Audit emission surfaces");
        const auto edits = bake_json::optional_object(level.dir / "probe-emission.json");
        Inventory inventory{level, edits, std::filesystem::weakly_canonical(level.dir), {}, {}};

        // Layer order retains the reference's first-definition-state rule.
        for (const auto& layer : kLayers) {
            inspect_layer(inventory, layer);
        }

        const auto written = write_stage_file(
            output_directory / "emission-audit.json", report(inventory.candidates)
        );

        // A failed diagnostic write must not masquerade as a completed audit.
        if (!written) {
            return unexpected(written.error());
        }

        std::size_t undefined = 0;

        // Undefined counts are per texture, not per placement or primitive.
        for (const auto& [texture, candidate] : inventory.candidates) {
            undefined += !candidate.defined;
        }

        return undefined;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
