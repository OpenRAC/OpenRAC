// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native collision and gameplay decoding for level interior preparation, without JSON handoffs.
#include "enhanced/tools/baker/sources/interior_inputs.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "assets/world/gameplay.h"
#include "common/json.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "scene/level.h"

namespace openrac::lighting {
namespace {
// Converts the editor's collision-surface colour convention to walkable support.
bool water_vertex(const scene::Vertex& vertex) {
    // The collision exporter encodes water as linear RGB from (0.2, 0.45, 1).
    const std::array<float, 3> water{std::pow(.2f, 2.2f), std::pow(.45f, 2.2f), 1};

    for (std::size_t channel = 0; channel < water.size(); ++channel) {
        // Match the established exported-colour tolerance, independent of authored normals.
        if (std::abs(vertex.colour[channel] - water[channel]) >= 1e-5f) {
            return false;
        }
    }

    return true;
}

// Copies one world-space collision triangle with its existing water/support classification.
interiors::Triangle collision_triangle(const scene::LevelData& mesh, std::size_t first) {
    interiors::Triangle triangle;
    bool water = true;

    // Collision primitives contain three indexed corners per triangle.
    for (std::size_t corner = 0; corner < triangle.vertices.size(); ++corner) {
        const auto& vertex = mesh.vertices.at(mesh.indices.at(first + corner));

        // Nonfinite collision could corrupt Embree bounds before inference can reject it.
        if (!std::all_of(
                std::begin(vertex.position), std::end(vertex.position), [](float component) {
                    return std::isfinite(component);
                }
            )) {
            throw std::runtime_error("nonfinite interior collision coordinate");
        }

        std::copy_n(
            vertex.position, triangle.vertices[corner].size(), triangle.vertices[corner].begin()
        );
        water = water && water_vertex(vertex);
    }

    triangle.support = !water;
    return triangle;
}

// Reuses the scene loader's node transforms and bounded GLB decoding, without graphics resources.
std::vector<interiors::Triangle> read_collision(const std::filesystem::path& path) {
    scene::LevelData mesh;
    std::string error;
    const auto model = scene::load_model(path, mesh, error);

    // Invalid exported geometry must stop preparation before any output is published.
    if (!model) {
        throw std::runtime_error(error);
    }

    std::vector<interiors::Triangle> triangles;

    for (const auto& primitive : mesh.models.at(*model).primitives) {
        // Truncated triangle lists cannot provide trustworthy support or enclosure.
        if (primitive.index_count % 3 != 0) {
            throw std::runtime_error("truncated interior collision triangle");
        }

        // Preserve primitive and triangle order for deterministic floor inference.
        for (std::size_t index = 0; index < primitive.index_count; index += 3) {
            triangles.push_back(collision_triangle(mesh, primitive.first_index + index));
        }
    }

    return triangles;
}

// Converts one validated authored boundary while retaining its source index.
interiors::Transition convert_transition(const assets::EnvTransition& source, unsigned index) {
    interiors::Transition transition;
    transition.id = index;

    // The authored inverse matrix contains four row vectors.
    for (std::size_t row = 0; row < source.inverse.size(); ++row) {
        // JSON preparation rejected nonfinite transforms before native inference too.
        if (!std::all_of(
                source.inverse[row].begin(), source.inverse[row].end(), [](float component) {
                    return std::isfinite(component);
                }
            )) {
            throw std::runtime_error("nonfinite interior transition transform");
        }

        std::copy(
            source.inverse[row].begin(),
            source.inverse[row].end(),
            transition.inverse.begin() + row * source.inverse[row].size()
        );
    }

    transition.light = source.hero_light;

    // Each endpoint's packed ambient colour stores RGB in its low three bytes.
    for (std::size_t side = 0; side < transition.ambient.size(); ++side) {
        const auto colour = source.hero_colour[side];
        transition.ambient[side] = {colour & 255u, (colour >> 8) & 255u, (colour >> 16) & 255u};
    }

    return transition;
}

// Converts the validated asset reader's transition records to the inference interface.
std::vector<interiors::Transition> read_transitions(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);

    // The asset reader borrows this owned gameplay buffer only within this function.
    if (!stream) {
        throw std::runtime_error("missing exported gameplay for interior preparation");
    }

    // Exported gameplay uses the same 64 MiB ceiling as the bounded disc decoder.
    if (std::filesystem::file_size(path) > 64ull * 1024u * 1024u) {
        throw std::runtime_error("interior gameplay exceeds the export limit");
    }

    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)), {});
    const assets::GameplayFile gameplay(assets::Game::Rac1, bytes);
    const auto authored = assets::read_env_transitions(gameplay);
    std::vector<interiors::Transition> transitions;

    for (std::size_t index = 0; index < authored.transitions.size(); ++index) {
        const auto& source = authored.transitions[index];

        // Fog-only transitions do not define a lighting mood.
        if (!source.lerps_hero_light()) {
            continue;
        }

        transitions.push_back(convert_transition(source, static_cast<unsigned>(index)));
    }

    return transitions;
}

// Writes the authored lighting boundaries for standalone inspection and reproducible diagnostics.
void write_transitions(std::ostream& out, const std::vector<interiors::Transition>& transitions) {
    for (std::size_t index = 0; index < transitions.size(); ++index) {
        const auto& transition = transitions[index];

        // Separate records without a trailing comma.
        if (index != 0) {
            out << ',';
        }

        out << "{\"id\":" << transition.id << ",\"inverse\":[";

        for (std::size_t component = 0; component < transition.inverse.size(); ++component) {
            // The first matrix component follows the opening bracket directly.
            if (component != 0) {
                out << ',';
            }

            out << transition.inverse[component];
        }

        out << "],\"light\":[" << transition.light[0] << ',' << transition.light[1]
            << "],\"ambient\":[";

        for (std::size_t side = 0; side < transition.ambient.size(); ++side) {
            // Each boundary has two endpoint colours in source order.
            if (side != 0) {
                out << ',';
            }

            const auto& ambient = transition.ambient[side];
            out << '[' << ambient[0] << ',' << ambient[1] << ',' << ambient[2] << ']';
        }

        out << "]}";
    }
}

// Serializes collision and lighting-boundary diagnostics in the existing inspection schema.
std::string describe_input(const InteriorInput& input) {
    std::ostringstream out;
    // Nine significant digits round-trip every stored float coordinate.
    out.precision(9);
    out << "{\"format\":1,\"game\":\"" << input.game << "\",\"level\":" << input.level
        << ",\"source_sha256\":\"" << input.source_sha256 << "\",\"triangles\":[";

    for (std::size_t index = 0; index < input.triangles.size(); ++index) {
        const auto& triangle = input.triangles[index];

        // Separate records without a trailing comma.
        if (index != 0) {
            out << ',';
        }

        out << '[';

        for (const auto& vertex : triangle.vertices) {
            for (float component : vertex) {
                out << component << ',';
            }
        }

        out << (triangle.support ? 1 : 0) << ']';
    }

    out << "],\"transitions\":[";
    write_transitions(out, input.transitions);
    out << "]}\n";
    return out.str();
}
}  // namespace

Result<std::string> bake_source_identity(const std::filesystem::path& path) {
    // Read failures must not become reusable cache identities.
    try {
        assets::disc::Sha256 hash;
        bake_json::hash_file(hash, path);
        return assets::disc::to_hex(hash.finish());
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<std::string> interior_source_identity(
    const std::filesystem::path& level, const std::filesystem::path& gameplay
) {
    try {
        assets::disc::Sha256 hash;
        bake_json::hash_file(hash, gameplay);
        bake_json::hash_file(hash, level / "collision.glb");
        return assets::disc::to_hex(hash.finish());
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<std::vector<interiors::Triangle>> load_coverage_collision(
    const std::filesystem::path& level
) {
    try {
        return read_collision(level / "collision.glb");
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<InteriorInput> load_interior_input(
    const std::filesystem::path& level, const std::filesystem::path& gameplay
) {
    try {
        json::Value manifest;
        std::string error;

        // Only the same supported export identities accepted by the baker reach inference.
        if (!json::parse_file((level / "manifest.json").string(), manifest, error)) {
            return unexpected(Error{error});
        }

        // This adapter uses the validated RAC1 gameplay schema shared by PAL and NTSC exports.
        if (manifest["game"].string() != "rac1"
            || (manifest["version"].string() != "pal" && manifest["version"].string() != "ntsc")
            || !manifest["level"].is_number() || manifest["level"].number() < 0
            || manifest["level"].number() > 18
            || std::floor(manifest["level"].number()) != manifest["level"].number()) {
            return unexpected(Error{"unsupported interior level identity"});
        }

        auto source = interior_source_identity(level, gameplay);

        if (!source) {
            return unexpected(source.error());
        }

        InteriorInput input;
        input.game = manifest["game"].string() + "-" + manifest["version"].string();
        input.level = static_cast<int>(manifest["level"].number(-1));
        input.source_sha256 = *source;
        input.triangles = read_collision(level / "collision.glb");
        input.transitions = read_transitions(gameplay);
        return input;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> write_interior_input(const std::filesystem::path& output, const InteriorInput& input) {
    return write_stage_file(output, describe_input(input));
}
}  // namespace openrac::lighting
