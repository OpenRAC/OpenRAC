// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/tools/baker/output/debug_preview.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "debug_preview_html.h"
#include "enhanced/lighting/probes.h"
#include "enhanced/tools/baker/output/environment_stage.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/sources/activation.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"

namespace openrac::lighting {
namespace {
// Read the published grids, including their actual relocation and validity metadata.
void write_gi(std::ostream& out, const std::filesystem::path& bake) {
    const auto island_path = bake / "placement-islands.json";
    const auto islands =
        std::filesystem::exists(island_path) ? bake_json::document(island_path) : json::Value{};
    std::vector<std::filesystem::path> paths;
    const auto registry = bake / "sparse.orpg.regions.json";
    if (std::filesystem::exists(registry)) {
        const auto document = bake_json::document(registry);
        for (const auto& entry : document["regions"].items()) {
            const auto name = entry.string();
            const auto path = std::filesystem::path(name);
            if (name.starts_with("region-") && path.extension() == ".orpg"
                && path == path.filename()) {
                paths.push_back(bake / path);
            } else {
                throw std::runtime_error("invalid GI preview region filename");
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(bake)) {
            if (entry.path().filename().string().starts_with("region-")
                && entry.path().extension() == ".orpg") {
                paths.push_back(entry.path());
            }
        }
    }
    std::sort(paths.begin(), paths.end());
    if (std::filesystem::exists(bake / "sparse.orpg")) {
        paths.insert(paths.begin(), bake / "sparse.orpg");
    }
    out << '[';
    bool comma = false;
    for (const auto& path : paths) {
        const auto volume = decode_gpu_probes(bake_json::source_bytes(path));
        const auto& grid = volume.grid;
        out << (comma ? ",{" : "{") << "\"name\":\"";
        // Keep file names safe inside the inline JSON script.
        for (const auto c : path.filename().string()) {
            if (c == '<' || c == '"' || c == '\\' || static_cast<unsigned char>(c) < 32) {
                throw std::runtime_error("invalid GI preview filename");
            }
            out.put(c);
        }
        out << '"';
        const auto& ownership = islands["volumes"][path.filename().string()];
        out << ",\"islands\":";
        if (ownership.is_array()) {
            out << bake_json::json_text(ownership);
        } else if (ownership.is_number()) {
            out << '[' << ownership.number() << ']';
        } else {
            out << "[]";
        }
        out << ",\"island\":";
        if (ownership.is_array() && ownership.items().size() == 1) {
            out << ownership[0].number();
        } else if (ownership.is_number()) {
            out << ownership.number();
        } else {
            out << "null";
        }
        const auto triple = [&](const char* name, const auto& values) {
            out << ",\"" << name << "\":[" << values[0] << ',' << values[1] << ',' << values[2]
                << ']';
        };
        triple("origin", grid.origin);
        triple("spacing", grid.spacing);
        triple("dimensions", grid.dimensions);
        out << ",\"axis\":[" << grid.frame.axis[0] << ',' << grid.frame.axis[1] << ']';
        out << ",\"pitch\":[" << grid.frame.pitch[0] << ',' << grid.frame.pitch[1] << ']';
        out << ",\"roll\":[" << grid.frame.roll[0] << ',' << grid.frame.roll[1] << ']';
        out << ",\"probes\":[";
        for (std::size_t i = 0; i < volume.probes.size(); ++i) {
            const auto p = volume.position(i);
            const auto& probe = volume.probes[i];
            out << (i ? ",[" : "[") << p[0] << ',' << p[1] << ',' << p[2] << ','
                << (!probe.valid      ? 0
                    : probe.relocated ? 2
                                      : 1)
                << ']';
        }
        out << "]}";
        comma = true;
    }
    out << ']';
}

// The viewer reads packed little-endian float32 XYZ triples.
std::vector<unsigned> write_mesh(std::ostream& out, const std::filesystem::path& level) {
    if (!std::filesystem::exists(level / "collision.glb")) {
        return {};
    }
    const auto mesh = load_coverage_collision(level);
    if (!mesh) {
        throw std::runtime_error(mesh.error().message);
    }
    constexpr std::string_view alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<unsigned> water_triangles;
    for (unsigned index = 0; index < mesh->size(); ++index) {
        const auto& triangle = (*mesh)[index];
        if (!triangle.support) {
            water_triangles.push_back(index);
        }
        for (const auto& vertex : triangle.vertices) {
            std::array<unsigned, 12> bytes{};
            for (unsigned axis = 0; axis < 3; ++axis) {
                const auto bits = std::bit_cast<std::uint32_t>(vertex[axis]);
                for (unsigned byte = 0; byte < 4; ++byte) {
                    bytes[4 * axis + byte] = (bits >> (8 * byte)) & 255;
                }
            }
            for (unsigned i = 0; i < bytes.size(); i += 3) {
                const auto bits = bytes[i] << 16 | bytes[i + 1] << 8 | bytes[i + 2];
                for (unsigned shift : {18u, 12u, 6u, 0u}) {
                    out.put(alphabet[(bits >> shift) & 63]);
                }
            }
        }
    }
    return water_triangles;
}
}  // namespace

std::filesystem::path write_debug_preview(
    const std::filesystem::path& level, const std::filesystem::path& bake, bool generate_prototypes
) {
    const auto report_path = bake / "environment-placement-report.json";
    const auto report =
        std::filesystem::exists(report_path) ? bake_json::document(report_path) : json::Value{};
    const bool failed = !report["uncovered"].items().empty()
                        || report["locality_conflicts"].number() > 0
                        || report["ownership_conflicts"].number() > 0;
    // Failed bakes retain their actual placement state; unrelated prototype generation
    // must not prevent inspecting it or rerun expensive analysis just for this page.
    if (!failed && generate_prototypes) {
        write_debug_medial(level, bake);
        write_debug_spatial(level, bake);
    }
    constexpr std::string_view marker = "__BAKE_DATA__";
    constexpr auto split = kDebugPreviewHtml.find(marker);
    static_assert(split != std::string_view::npos);
    std::ostringstream page;
    page.imbue(std::locale::classic());
    page << std::setprecision(std::numeric_limits<double>::max_digits10);
    page << "<!doctype html>\n<meta charset=\"utf-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
            "<title>OpenRAC bake preview</title>\n"
         << kDebugPreviewHtml.substr(0, split);
    // These report fields contain numeric arrays only.
    page << "{\"points\":[";
    bool comma = false;
    for (const auto& receiver : report["receiver_coverage"].items()) {
        if (receiver.items().size() < 5 || receiver[3].number(-1) != 0) {
            continue;
        }
        page << (comma ? ",[" : "[") << receiver[0].number() << ',' << receiver[1].number() << ','
             << receiver[2].number();
        for (const auto& probe : receiver[4].items()) {
            page << ',' << probe.number();
        }
        page << ']';
        comma = true;
    }
    page << ']';
    page << ",\"water\":[";
    comma = false;
    for (const auto& receiver : report["receiver_coverage"].items()) {
        if (receiver.items().size() < 5 || receiver[3].number(-1) != 0) {
            continue;
        }
        page << (comma ? "," : "") << (receiver[5].boolean() ? "true" : "false");
        comma = true;
    }
    page << ']';
    page << ",\"projection_bounds_are_regions\":"
         << (report["projection_bounds_are_regions"].boolean() ? "true" : "false");
    for (const auto& [key, source] :
         {std::pair{"captures", "capture_positions"},
          std::pair{"owners", "owners"},
          std::pair{"boxes", "capture_bounds"},
          std::pair{"influence_captures", "influence_captures"},
          std::pair{"influence_owners", "influence_owners"},
          std::pair{"projection_bounds", "projection_bounds"},
          std::pair{"spaces", "spaces"},
          std::pair{"watershed", "watershed"}}) {
        page << ",\"" << key << "\":";
        auto encoded = report[source].is_array() ? bake_json::json_text(report[source]) : "[]";
        for (const char c : encoded) {
            if (c == '<') {
                page << "\\u003c";
            } else {
                page.put(c);
            }
        }
    }
    page << ",\"placement_failures\":{\"failed\":" << (failed ? "true" : "false");
    for (const auto* name : {"uncovered", "uncovered_owners", "placement_conflicts"}) {
        page << ",\"" << name
             << "\":" << (report[name].is_array() ? bake_json::json_text(report[name]) : "[]");
    }
    page << ",\"locality_conflicts\":" << report["locality_conflicts"].number()
         << ",\"ownership_conflicts\":" << report["ownership_conflicts"].number() << '}';
    page << ",\"gi\":";
    write_gi(page, bake);
    const auto island_path = bake / "placement-islands.json";
    const auto islands =
        std::filesystem::exists(island_path) ? bake_json::document(island_path) : json::Value{};
    page << ",\"islands\":"
         << (islands["islands"].is_array() ? bake_json::json_text(islands["islands"]) : "[]");
    page << ",\"island_samples\":"
         << (islands["samples"].is_array() ? bake_json::json_text(islands["samples"]) : "[]");
    page << ",\"evidence\":[";
    comma = false;
    if (std::filesystem::exists(bake / "interior-inference.json")
        && std::filesystem::exists(bake / "seed-plan.json")) {
        const auto profile = load_level_bake_profile(level);
        const auto floors = read_environment_floors(bake, level, profile.activation.bake.traversal);
        std::vector<bool> gameplay_reached(floors.size());
        std::vector<unsigned> pending;
        for (unsigned i = 0; i < floors.size(); ++i) {
            if (floors[i].gameplay_anchor) {
                gameplay_reached[i] = true;
                pending.push_back(i);
            }
        }
        for (unsigned i = 0; i < pending.size(); ++i) {
            for (auto next : floors[pending[i]].neighbours) {
                if (!gameplay_reached[next]) {
                    gameplay_reached[next] = true;
                    pending.push_back(next);
                }
            }
        }
        for (unsigned i = 0; i < floors.size(); ++i) {
            const auto& floor = floors[i];
            if (!floor.evidence) {
                continue;
            }
            page << (comma ? ",[" : "[") << floor.position[0] << ',' << floor.position[1] << ','
                 << floor.position[2] << ',' << floor.evidence << ','
                 << islands["samples"][i][3].number(-1) << ','
                 << (gameplay_reached[i] ? "true" : "false") << ']';
            comma = true;
        }
    }
    page << "],\"playable_areas\":[";
    comma = false;
    for (const auto& area : load_level_bake_profile(level).playable_areas) {
        page << (comma ? ",[" : "[") << area.minimum[0] << ',' << area.minimum[1] << ','
             << area.minimum[2] << ',' << area.maximum[0] << ',' << area.maximum[1] << ','
             << area.maximum[2] << ']';
        comma = true;
    }
    page << ']';
    const auto medial_path = bake / "medial-axis.json";
    const auto medial =
        std::filesystem::exists(medial_path) ? bake_json::document(medial_path) : json::Value{};
    page << ",\"medial\":{\"segments\":"
         << (medial["segments"].is_array() ? bake_json::json_text(medial["segments"]) : "[]")
         << ",\"markers\":"
         << (medial["markers"].is_array() ? bake_json::json_text(medial["markers"]) : "[]")
         << ",\"entrances\":"
         << (medial["entrances"].is_array() ? bake_json::json_text(medial["entrances"]) : "[]")
         << '}';
    page << ",\"spatial\":";
    embed_spatial_preview(page, bake / "spatial-watershed.json");
    page << ",\"surface_coverage\":"
         << (report["surface_coverage"].is_object()
                 ? bake_json::json_text(report["surface_coverage"])
                 : "null");
    page << ",\"mesh\":\"";
    const auto water_triangles = write_mesh(page, level);
    page << "\",\"water_triangles\":[";
    for (unsigned i = 0; i < water_triangles.size(); ++i) {
        page << (i ? "," : "") << water_triangles[i];
    }
    page << "]}" << kDebugPreviewHtml.substr(split + marker.size());

    const auto output = bake / "bake-preview.html";
    if (const auto result = write_stage_file(output, page.view()); !result) {
        throw std::runtime_error(result.error().message);
    }
    return output;
}
}  // namespace openrac::lighting
