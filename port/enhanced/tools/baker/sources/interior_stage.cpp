// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Interior inference stage: reads collision/transition inputs and writes the shared room report.
#include "enhanced/tools/baker/sources/interior_stage.h"

#include <cstddef>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "common/json.h"
#include "common/profiler.h"
#include "enhanced/lighting/interiors.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"

namespace openrac::lighting {
namespace {
namespace in = interiors;

// Rejects source identities unsupported by the exported interior inference schema.
void validate_identity(const json::Value& data) {
    // The planner writes schema version one.
    if (data["format"].number() != 1) {
        throw std::runtime_error("unsupported inference input");
    }

    // Level IDs span 0-18 and the source identity is a 64-character SHA-256 digest.
    if ((data["game"].string() != "rac1-ntsc" && data["game"].string() != "rac1-pal")
        || data["level"].number(-1) < 0 || data["level"].number() > 18
        || data["source_sha256"].string().size() != 64) {
        throw std::runtime_error("invalid source identity");
    }
}

// Reads world-space triangles; the tenth component marks walkable support.
std::vector<in::Triangle> read_triangles(const json::Value& data) {
    std::vector<in::Triangle> triangles;

    // Preserve triangle order so inference ties resolve as before.
    for (const auto& row : data["triangles"].items()) {
        // Each row contains nine XYZ coordinates followed by its support flag.
        if (row.items().size() != 10) {
            throw std::runtime_error("invalid collision triangle");
        }

        in::Triangle triangle{};

        // Three vertices each contribute three coordinates.
        for (unsigned i = 0; i < 9; ++i) {
            triangle.vertices[i / 3][i % 3] = static_cast<float>(row[i].number());
        }

        triangle.support = row[9].number() != 0;
        triangles.push_back(triangle);
    }

    return triangles;
}

// Reads authored world-to-local matrices, light-set pairs and ambient RGB pairs.
std::vector<in::Transition> read_transitions(const json::Value& data) {
    std::vector<in::Transition> transitions;

    // Stable authored order preserves transition label assignment.
    for (const auto& row : data["transitions"].items()) {
        // A transition has a 4x4 inverse matrix and two light/ambient endpoints.
        if (row["inverse"].items().size() != 16 || row["light"].items().size() != 2
            || row["ambient"].items().size() != 2) {
            throw std::runtime_error("invalid transition");
        }

        in::Transition transition{};
        transition.id = static_cast<unsigned>(row["id"].number());

        // Copy the authored row-vector matrix without transposing it.
        for (unsigned i = 0; i < 16; ++i) {
            transition.inverse[i] = static_cast<float>(row["inverse"][i].number());
        }

        // Each endpoint carries a light-set ID and three ambient colour channels.
        for (unsigned i = 0; i < 2; ++i) {
            transition.light[i] = static_cast<int>(row["light"][i].number());

            // RGB channels retain their authored byte values.
            for (unsigned k = 0; k < 3; ++k) {
                transition.ambient[i][k] = static_cast<unsigned>(row["ambient"][i][k].number());
            }
        }

        transitions.push_back(transition);
    }

    return transitions;
}

// Separates ordered JSON records without leaving a trailing comma.
void write_separator(std::ostream& out, std::size_t index) {
    // The first record follows the opening bracket directly.
    if (index != 0) {
        out << ',';
    }
}

// Writes one world-space point in the report's fixed XYZ order.
void write_point(std::ostream& out, in::Point point) {
    out << '[' << point[0] << ',' << point[1] << ',' << point[2] << ']';
}

// Writes the transitions portion without changing report ordering.
void write_transitions(std::ostream& out, const std::vector<in::Transition>& transitions) {
    // Keep the inference result's stable record order.
    for (unsigned i = 0; i < transitions.size(); ++i) {
        write_separator(out, i);

        const auto& transition = transitions[i];
        out << "{\"id\":" << transition.id << ",\"corners\":[";

        // Unit-box corner bits select the three signed coordinates of its eight corners.
        for (unsigned k = 0; k < 8; ++k) {
            write_separator(out, k);

            write_point(
                out,
                in::world(transition, {k & 1 ? 1.f : -1.f, k & 2 ? 1.f : -1.f, k & 4 ? 1.f : -1.f})
            );
        }

        out << "],\"light\":[" << transition.light[0] << ',' << transition.light[1]
            << "],\"ambient\":[";

        // Both transition endpoints carry an RGB ambient colour.
        for (unsigned k = 0; k < 2; ++k) {
            write_separator(out, k);

            out << '[' << transition.ambient[k][0] << ',' << transition.ambient[k][1] << ','
                << transition.ambient[k][2] << ']';
        }

        out << "]}";
    }
}

// Writes the cells portion without changing report ordering.
void write_cells(std::ostream& out, const in::Result& result) {
    // Keep the inference result's stable record order.
    for (unsigned i = 0; i < result.cells.size(); ++i) {
        write_separator(out, i);

        const auto& cell = result.cells[i];
        out << '[' << cell.position[0] << ',' << cell.position[1] << ',' << cell.position[2] << ','
            << cell.component << ',' << cell.sky << ']';
    }
}

// Writes the walkable edges portion without changing report ordering.
void write_walkable_edges(std::ostream& out, const in::Result& result) {
    // Keep the inference result's stable record order.
    for (unsigned i = 0; i < result.walkable_edges.size(); ++i) {
        const auto edge = result.walkable_edges[i];
        write_separator(out, i);
        out << '[' << edge[0] << ',' << edge[1] << ']';
    }
}

// Writes the regions portion without changing report ordering.
void write_regions(std::ostream& out, const in::Result& result) {
    // Keep the inference result's stable record order.
    for (unsigned i = 0; i < result.regions.size(); ++i) {
        write_separator(out, i);

        const auto& region = result.regions[i];
        out << "{\"id\":" << i << ",\"count\":" << region.cells.size() << ",\"sky\":" << region.sky
            << ",\"labels\":[";

        // Label order is part of the stable report identity.
        for (unsigned k = 0; k < region.labels.size(); ++k) {
            write_separator(out, k);

            out << region.labels[k];
        }

        out << "]}";
    }
}

// Writes the seeds portion without changing report ordering.
void write_seeds(std::ostream& out, const in::Result& result) {
    // Keep the inference result's stable record order.
    for (unsigned i = 0; i < result.seeds.size(); ++i) {
        write_separator(out, i);

        const auto& seed = result.seeds[i];
        out << "{\"transition\":" << seed.transition << ",\"side\":" << seed.side
            << ",\"label\":" << seed.label << ",\"cell\":" << seed.cell << ",\"target\":";
        write_point(out, seed.target);
        out << '}';
    }
}

// Saves the inference result in the schema consumed by interior lighting and reflection planning.
std::string describe_report(
    const InteriorInput& input, const in::Settings& settings, const in::Result& result
) {
    std::ostringstream out;
    out << std::setprecision(8);
    out << "{\"format\":1,\"game\":\"" << input.game << "\",\"level\":" << input.level
        << ",\"source_sha256\":\"" << input.source_sha256 << "\",\"spacing\":" << settings.spacing
        << ",\"height\":" << settings.height << ",\"radius\":" << settings.radius
        << ",\"step\":" << settings.step << ",\"sky_distance\":" << settings.sky_distance
        << ",\"blocked_portal_edges\":" << result.blocked_portal_edges;

    // Automatic reports carry stage identity; standalone reports preserve their existing schema.
    if (!input.generation_sha256.empty()) {
        out << ",\"generation_sha256\":\"" << input.generation_sha256 << '"';
    }

    out << ",\"transitions\":[";

    write_transitions(out, input.transitions);
    out << "],\"cells\":[";
    write_cells(out, result);
    out << "],\"walkable_edges\":[";
    write_walkable_edges(out, result);
    out << "],\"regions\":[";
    write_regions(out, result);
    out << "],\"seeds\":[";
    write_seeds(out, result);
    out << "]}\n";
    return out.str();
}

}  // namespace

std::string interior_stage_profile() {
    const in::Settings settings;
    // Bump this revision when inference changes independently of GI transport or binary rebuilds.
    constexpr unsigned kInferenceRevision = 1;
    return std::format(
        "{{\"revision\":{},\"spacing\":{},\"height\":{},\"radius\":{},\"step\":{},"
        "\"slope_cosine\":{},\"sky_distance\":{},\"max_cells\":{}}}",
        kInferenceRevision,
        settings.spacing,
        settings.height,
        settings.radius,
        settings.step,
        settings.slope_cosine,
        settings.sky_distance,
        settings.max_cells
    );
}

Result<void> write_interior_report(
    const std::filesystem::path& output,
    const InteriorInput& input,
    const interiors::Settings& settings,
    const interiors::Result& result
) {
    return write_stage_file(output, describe_report(input, settings, result));
}

Result<void> infer_interior_file(
    const std::filesystem::path& input, const std::filesystem::path& output, float spacing
) {
    try {
        profiling::Zone zone("Bake interior inference");

        // Existing reports remain intact when preparation is retried in another staging folder.
        if (std::filesystem::exists(output)) {
            return unexpected(Error{"refusing to overwrite interior inference output"});
        }

        json::Value data;
        std::string error;

        // The report must describe the same validated source as the planner's inputs.
        if (!json::parse_file(input.string(), data, error)) {
            return unexpected(Error{error});
        }

        validate_identity(data);
        const auto triangles = read_triangles(data);
        const auto transitions = read_transitions(data);
        in::Settings settings;
        settings.spacing = spacing;
        const auto result = in::infer(triangles, transitions, settings);
        InteriorInput input_record;
        input_record.game = data["game"].string();
        input_record.level = static_cast<int>(data["level"].number());
        input_record.source_sha256 = data["source_sha256"].string();
        input_record.transitions = transitions;
        return write_interior_report(output, input_record, settings, result);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
