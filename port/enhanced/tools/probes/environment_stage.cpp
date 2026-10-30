// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reflection configuration, verified GI inputs and walkable room-centre candidates.
#include "enhanced/tools/probes/environment_stage.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>

#include "common/json.h"
#include "enhanced/tools/probes/activation.h"
#include "enhanced/tools/probes/report.h"

namespace openrac::lighting {
namespace {
// Bounded file reads are shared by the profile and already-verified GI payloads.
std::vector<std::uint8_t> read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file || file.tellg() < 0 || file.tellg() > std::streamoff(512 * 1024 * 1024)) {
        throw std::runtime_error("cannot read environment bake input");
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);

    if (!file.read(
            reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        )) {
        throw std::runtime_error("incomplete environment bake input");
    }

    return bytes;
}

}  // namespace

// Reuse collision walkability and connected-room identities from the interior preparation stage.
std::vector<EnvironmentFloorSample> read_environment_floors(
    const std::filesystem::path& output,
    const std::filesystem::path& level,
    const BakeTraversalSettings& traversal
) {
    const auto path = output / "interior-inference.json";

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("environment placement requires interior-inference.json");
    }

    json::Value document;
    std::string error;

    if (!json::parse_file(path.string(), document, error) || document["format"].number() != 1
        || !document["cells"].is_array() || document["cells"].items().size() > 2'000'000) {
        throw std::runtime_error("invalid environment floor inference: " + error);
    }

    std::vector<EnvironmentFloorSample> result;
    std::map<unsigned, std::vector<unsigned>> labels;

    for (const auto& region : document["regions"].items()) {
        const auto id = region["id"].number(-1);

        if (!region["id"].is_number() || id < 0 || id > 2'000'000 || std::floor(id) != id
            || !region["labels"].is_array() || region["labels"].items().size() > 64) {
            throw std::runtime_error("invalid environment component lighting labels");
        }

        auto& values = labels[unsigned(id)];

        for (const auto& label : region["labels"].items()) {
            const auto value = label.number(-1);

            if (!label.is_number() || value < 0 || value > 2'000'000
                || std::floor(value) != value) {
                throw std::runtime_error("invalid environment lighting label");
            }

            values.push_back(unsigned(value));
        }

        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());
    }

    for (const auto& row : document["cells"].items()) {
        if (!row.is_array() || row.items().size() != 5) {
            throw std::runtime_error("invalid environment floor sample");
        }

        for (unsigned a = 0; a < 5; ++a) {
            if (!row[a].is_number() || !std::isfinite(row[a].number())
                || std::abs(row[a].number()) > 1e7) {
                throw std::runtime_error("non-finite environment floor sample");
            }
        }

        const auto region = row[3].number();

        if (region < 0 || region > 2'000'000 || std::floor(region) != region) {
            throw std::runtime_error("invalid environment floor region");
        }

        result.push_back(
            {{float(row[0].number()), float(row[1].number()), float(row[2].number())},
             static_cast<unsigned>(region),
             float(row[4].number()),
             false,
             {}}
        );

        if (const auto found = labels.find(unsigned(region)); found != labels.end()) {
            result.back().lighting_labels = found->second;
        }
    }

    if (!document["walkable_edges"].is_array()) {
        throw std::runtime_error("environment placement needs a current walkable floor graph");
    }

    for (const auto& edge : document["walkable_edges"].items()) {
        if (!edge.is_array() || edge.items().size() != 2) {
            throw std::runtime_error("invalid environment floor edge");
        }

        for (unsigned a = 0; a < 2; ++a) {
            const auto value = edge[a].number(-1);

            if (!edge[a].is_number() || value < 0 || value >= double(result.size())
                || std::floor(value) != value) {
                throw std::runtime_error("invalid environment floor edge index");
            }
        }

        const auto a = static_cast<unsigned>(edge[0].number());
        const auto b = static_cast<unsigned>(edge[1].number());
        result[a].neighbours.push_back(b);
        result[b].neighbours.push_back(a);
    }

    json::Value seeds;

    if (!json::parse_file((output / "seed-plan.json").string(), seeds, error)
        || !seeds["moby_seeds"].is_array()) {
        throw std::runtime_error("environment placement needs gameplay floor anchors: " + error);
    }

    // Ground-snapped gameplay seeds choose a layer; proximity in XY alone is unsafe in stacked rooms.
    for (const auto& seed : seeds["moby_seeds"].items()) {
        if (!seed["position"].is_array() || seed["position"].items().size() != 3
            || !seed["ground_z"].is_number()) {
            continue;
        }

        float best = 16;
        unsigned selected = ~0u;

        for (unsigned i = 0; i < result.size(); ++i) {
            const auto p = result[i].position;
            const float dx = p[0] - float(seed["position"][0].number());
            const float dy = p[1] - float(seed["position"][1].number());
            const float dz = p[2] - float(seed["ground_z"].number());
            const float score = dx * dx + dy * dy + 4 * dz * dz;

            if (std::abs(dz) <= 1.25f && score < best) {
                best = score;
                selected = i;
            }
        }

        if (selected != ~0u) {
            result[selected].anchor = true;
        }
    }

    // Preserve the old exterior height envelope in the analysis space. Enclosed ceilings
    // may extend these columns later; nearby wall vertices never enter this calculation.
    for (auto& floor : result) {
        for (const auto& region : seeds["regions"].items()) {
            const auto& bounds = region["bounds"];

            if (!bounds.is_array() || bounds.items().size() != 6) {
                continue;
            }

            bool inside = true;

            for (unsigned a = 0; a < 3; ++a) {
                inside = inside && floor.position[a] >= bounds[a].number() - .001
                         && floor.position[a] <= bounds[a + 3].number() + .001;
            }

            if (!inside) {
                continue;
            }

            const Float2 height{float(bounds[2].number()), float(bounds[5].number())};

            if (!floor.height_bounds) {
                floor.height_bounds = height;
            } else {
                (*floor.height_bounds)[0] = std::min((*floor.height_bounds)[0], height[0]);
                (*floor.height_bounds)[1] = std::max((*floor.height_bounds)[1], height[1]);
            }
        }
    }

    // Share only gameplay-reachable space with GI fitting and medial diagnostics. The full
    // inferred collision graph includes disconnected roofs and cavities that are not gameplay.
    std::vector<unsigned> pending, remap(result.size(), ~0u);

    for (unsigned a = 0; a < result.size(); ++a) {
        if (result[a].anchor) {
            remap[a] = 0;
            pending.push_back(a);
        }
    }

    if (pending.empty()) {
        throw std::runtime_error("medial planning requires gameplay floor anchors");
    }

    for (unsigned i = 0; i < pending.size(); ++i) {
        for (auto b : result[pending[i]].neighbours) {
            if (remap[b] == ~0u) {
                remap[b] = 0;
                pending.push_back(b);
            }
        }
    }

    std::sort(pending.begin(), pending.end());

    for (unsigned i = 0; i < pending.size(); ++i) {
        remap[pending[i]] = i;
    }

    std::vector<EnvironmentFloorSample> reachable;
    reachable.reserve(pending.size());

    for (auto a : pending) {
        auto floor = std::move(result[a]);

        for (auto& b : floor.neighbours) {
            b = remap[b];
        }

        reachable.push_back(std::move(floor));
    }

    append_exported_water(output, reachable, level, traversal);
    return reachable;
}

namespace {
// Written on success and failure so safety limits and rejected floors remain inspectable.
void placement_report(
    const std::filesystem::path& output, const EnvironmentPlacementReport& report
) {
    std::ofstream out(output / "environment-placement-report.json");
    out << "{\n  \"version\": 1,\n  \"input_floors\": " << report.input_floors
        << ",\n  \"playable_floors\": " << report.playable_floors
        << ",\n  \"rejected_floors\": " << report.rejected_floors
        << ",\n  \"unsupported_floors\": " << report.unsupported_floors
        << ",\n  \"receiver_only_floors\": " << report.receiver_only_floors
        << ",\n  \"spaces_before_merge\": " << report.spaces_before_merge
        << ",\n  \"merged_fragments\": " << report.merged_fragments
        << ",\n  \"merged_graph_gaps\": " << report.merged_graph_gaps
        << ",\n  \"merged_floor_components\": " << report.merged_floor_components
        << ",\n  \"bridged_floor_edges\": " << report.bridged_floor_edges
        << ",\n  \"lighting_spaces\": " << report.lighting_spaces
        << ",\n  \"validation_samples\": " << report.validation_samples
        << ",\n  \"unpruned_probes\": " << report.unpruned_probes
        << ",\n  \"pruned_probes\": " << report.pruned_probes
        << ",\n  \"ownership_conflicts\": " << report.ownership_conflicts
        << ",\n  \"covered_fraction\": " << report.covered_fraction
        << ",\n  \"merge_pair_attempts\": " << report.merge_pair_attempts
        << ",\n  \"merge_position_trials\": " << report.merge_position_trials
        << ",\n  \"minimum_coverage\": " << report.minimum_coverage << ",\n  \"owners\": [";

    for (unsigned i = 0; i < report.owners.size(); ++i) {
        out << (i ? "," : "") << report.owners[i];
    }

    out << "],\n  \"occlusion_separated_pairs\": " << report.occlusion_separated_pairs
        << ",\n  \"spacing_conflicts\": [";

    for (unsigned i = 0; i < report.spacing_conflicts.size(); ++i) {
        out << (i ? "," : "") << '[' << report.spacing_conflicts[i][0] << ','
            << report.spacing_conflicts[i][1] << ']';
    }

    out << "],\n  \"capture_positions\": [";

    for (unsigned i = 0; i < report.capture_positions.size(); ++i) {
        const auto p = report.capture_positions[i];
        out << (i ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
    }

    out << "],\n  \"capture_bounds\": [";

    for (unsigned i = 0; i < report.capture_bounds.size(); ++i) {
        const auto& probe = report.capture_bounds[i];
        out << (i ? "," : "") << '[' << probe.minimum[0] << ',' << probe.minimum[1] << ','
            << probe.minimum[2] << ',' << probe.maximum[0] << ',' << probe.maximum[1] << ','
            << probe.maximum[2] << ',' << probe.frame.axis[0] << ',' << probe.frame.axis[1] << ']';
    }

    out << "],\n  \"submerged_captures\": [";

    for (unsigned i = 0; i < report.submerged_captures.size(); ++i) {
        out << (i ? "," : "") << report.submerged_captures[i];
    }

    out << "],\n  \"capture_moves\": [";

    for (unsigned i = 0; i < report.capture_moves.size(); ++i) {
        const auto& move = report.capture_moves[i];
        out << (i ? "," : "") << "{\"from\":[" << move.from[0] << ',' << move.from[1] << ','
            << move.from[2] << "],\"to\":[" << move.to[0] << ',' << move.to[1] << ',' << move.to[2]
            << "]}";
    }

    out << "],\n  \"spaces\": [";

    for (unsigned i = 0; i < report.spaces.size(); ++i) {
        const auto& space = report.spaces[i];
        out << (i ? "," : "") << "{\"cells\":" << space.cells
            << ",\"authored_region\":" << space.authored_region
            << ",\"sky_band\":" << space.sky_band;
        auto point = [&](const char* name, Float3 p) {
            out << ",\"" << name << "\":[" << p[0] << ',' << p[1] << ',' << p[2] << ']';
        };
        point("minimum", space.minimum);
        point("maximum", space.maximum);
        point("light", space.light);
        out << ",\"underwater\":" << (space.underwater ? "true" : "false") << ",\"water_height\":";

        if (space.water_height) {
            out << *space.water_height;
        } else {
            out << "null";
        }

        out << ",\"neighbours\":[";

        for (unsigned n = 0; n < space.neighbours.size(); ++n) {
            out << (n ? "," : "") << space.neighbours[n];
        }

        out << "]}";
    }

    out << "],\n  \"uncovered\": [";

    for (unsigned i = 0; i < report.uncovered.size(); ++i) {
        const auto p = report.uncovered[i];
        out << (i ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
    }

    out << "],\n  \"uncovered_owners\": [";

    for (unsigned i = 0; i < report.uncovered_owners.size(); ++i) {
        out << (i ? "," : "") << report.uncovered_owners[i];
    }

    out << "],\n  \"unsupported\": [";

    for (unsigned i = 0; i < report.unsupported.size(); ++i) {
        const auto p = report.unsupported[i];
        out << (i ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
    }

    out << "],\n  \"probe_merges\": [";

    for (unsigned i = 0; i < report.probe_merges.size(); ++i) {
        const auto& merge = report.probe_merges[i];
        out << (i ? "," : "") << "{\"overlap\":" << merge.overlap
            << ",\"loss_fraction\":" << merge.loss_fraction << ",\"trials\":" << merge.trials;
        auto point = [&](const char* name, Float3 p) {
            out << ",\"" << name << "\":[" << p[0] << ',' << p[1] << ',' << p[2] << ']';
        };
        point("a", merge.a);
        point("b", merge.b);
        point("replacement", merge.replacement);
        out << '}';
    }

    out << "],\n  \"tolerated_misses\": [";

    for (unsigned i = 0; i < report.tolerated_misses.size(); ++i) {
        const auto p = report.tolerated_misses[i];
        out << (i ? "," : "") << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
    }

    out << "],\n  \"coverage_hash\": \"" << report.coverage_hash
        << "\",\n  \"receiver_coverage\": [";

    for (unsigned i = 0; i < report.receivers.size(); ++i) {
        const auto& receiver = report.receivers[i];
        out << (i ? "," : "") << '[' << receiver.position[0] << ',' << receiver.position[1] << ','
            << receiver.position[2] << ',' << receiver.height << ",[";

        for (unsigned p = 0; p < receiver.probes.size(); ++p) {
            out << (p ? "," : "") << receiver.probes[p];
        }

        out << "]]";
    }

    out << "]\n}\n";

    if (!out) {
        throw std::runtime_error("cannot write environment placement diagnostics");
    }
}

}  // namespace

void bake_level_environment(
    const std::filesystem::path& level,
    const std::filesystem::path& output,
    const BakeScene& scene,
    const BakeSettings& settings,
    unsigned threads,
    std::ostream& log
) {
    const auto config = load_level_bake_profile(level).environment;

    if (!config.enabled) {
        return;
    }

    const auto start = std::chrono::steady_clock::now();
    std::vector<std::filesystem::path> paths;

    for (const auto& entry : std::filesystem::directory_iterator(output)) {
        if (entry.path().filename().string().starts_with("region-")
            && entry.path().extension() == ".orpg") {
            paths.push_back(entry.path());
        }
    }

    std::sort(paths.begin(), paths.end());
    paths.insert(paths.begin(), output / "sparse.orpg");
    std::vector<ProbeVolume> volumes;

    for (const auto& path : paths) {
        auto volume = decode_probes(read(path));

        if (!volumes.empty()) {
            validate_probe_pair(volumes[0], volume);
        }

        volumes.push_back(std::move(volume));
    }

    log << "Baking environment probes after GI\n" << std::flush;
    EnvironmentPlacementReport placement;
    EnvironmentAsset asset;

    try {
        asset = bake_environment(
            scene,
            settings,
            volumes,
            read_environment_floors(output, level, settings.traversal),
            config,
            threads,
            &placement
        );
    } catch (...) {
        placement_report(output, placement);
        throw;
    }

    log << "Environment spaces: " << placement.spaces_before_merge << " -> "
        << placement.lighting_spaces << " (" << placement.merged_graph_gaps
        << " short graph gaps merged)\n";
    log << "Environment pruning: " << placement.unpruned_probes << " -> " << asset.probes.size()
        << " captures (" << placement.pruned_probes << " removed)\n";
    log << "Environment capture merges: " << placement.probe_merges.size() << " replacements, "
        << placement.merge_position_trials << " positions tested; coverage "
        << placement.covered_fraction << "\n";
    log << "Environment coverage: " << placement.playable_floors << " playable floors, "
        << placement.lighting_spaces << " lighting spaces, " << placement.validation_samples
        << " receiver samples, " << placement.uncovered.size() << " uncovered, "
        << placement.tolerated_misses.size() << " tolerated floor misses, "
        << placement.receiver_only_floors << " receiver-only floors, "
        << placement.unsupported_floors << " samples without playable render-floor support\n";
    asset.gi_hash = environment_gi_hash(output);
    const auto bytes = encode_environment(asset);
    const auto path = output / "environment.orpe";
    write_bake_file(path, bytes);

    if (read(path) != bytes || encode_environment(decode_environment(bytes)) != bytes) {
        throw std::runtime_error("environment bake read-back mismatch");
    }

    placement.coverage_hash = environment_capture_hash(asset.probes, asset.gi_hash);
    placement_report(output, placement);
    std::ofstream report(output / "environment-report.json");
    report << "{\n  \"version\": 1,\n  \"resolution\": " << asset.resolution
           << ",\n  \"bytes\": " << bytes.size() << ",\n  \"probes\": [\n";

    for (std::size_t i = 0; i < asset.probes.size(); ++i) {
        const auto& p = asset.probes[i];
        report << (i ? ",\n" : "") << "    {\"position\": [" << p.position[0] << ","
               << p.position[1] << "," << p.position[2] << "], \"minimum\": [" << p.minimum[0]
               << "," << p.minimum[1] << "," << p.minimum[2] << "], \"maximum\": [" << p.maximum[0]
               << "," << p.maximum[1] << "," << p.maximum[2] << "], \"projection_minimum\": ["
               << p.projection_minimum[0] << ',' << p.projection_minimum[1] << ','
               << p.projection_minimum[2] << "], \"projection_maximum\": ["
               << p.projection_maximum[0] << ',' << p.projection_maximum[1] << ','
               << p.projection_maximum[2] << "], \"orientation\": [" << p.frame.axis[0] << ","
               << p.frame.axis[1] << "]}";
    }

    report << "\n  ]\n}\n";

    if (!report) {
        throw std::runtime_error("cannot write environment placement report");
    }

    log << "Verified environment probes " << asset.probes.size() << ", " << bytes.size()
        << " bytes, "
        << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
        << " seconds\n"
        << std::flush;
}
}  // namespace openrac::lighting
