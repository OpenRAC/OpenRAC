// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <bit>
#include <format>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>

#include "enhanced/lighting/spatial_placement.h"
#include "enhanced/lighting/spatial_volume_fit.h"
#include "enhanced/lighting/spatial_watershed.h"
#include "enhanced/tools/baker/output/debug_preview.h"
#include "enhanced/tools/baker/output/environment_stage.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/sources/activation.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"

namespace openrac::lighting {
namespace {
std::string_view spatial_file_kind(std::ifstream& input) {
    std::array<char, 128> prefix;
    input.read(prefix.data(), prefix.size());
    const std::string_view text(prefix.data(), static_cast<std::size_t>(input.gcount()));
    if (input.bad()) {
        throw std::runtime_error("cannot read watershed debug output");
    }
    input.clear();
    input.seekg(0);
    if (!text.starts_with("{\"version\":")) {
        return {};
    }
    if (text.find("\"placement_source\":\"layered_watershed\"") != std::string_view::npos) {
        return "layered_watershed";
    }
    if (text.find("\"placement_source\":\"prototype\"") != std::string_view::npos) {
        return "prototype";
    }
    return {};
}

void write_inline_json(std::ostream& out, std::string_view text) {
    // A JSON string containing HTML cannot end the surrounding data script.
    for (auto less = text.find('<'); less != std::string_view::npos; less = text.find('<')) {
        out << text.substr(0, less) << "\\u003c";
        text.remove_prefix(less + 1);
    }
    out << text;
}
}  // namespace

void embed_spatial_preview(std::ostream& out, const std::filesystem::path& spatial_file) {
    if (!std::filesystem::exists(spatial_file)) {
        out << "null";
        return;
    }
    std::ifstream input(spatial_file, std::ios::binary);
    if (spatial_file_kind(input).empty()) {
        write_inline_json(out, bake_json::json_text(bake_json::document(spatial_file)));
        return;
    }
    // The exporter already writes compact JSON. Copy it without materializing the graph
    // again, including older exports that exceed the general bake-input size limit.
    std::array<char, 64 * 1024> buffer;
    while (input.read(buffer.data(), buffer.size()) || input.gcount()) {
        write_inline_json(out, {buffer.data(), static_cast<std::size_t>(input.gcount())});
    }
    if (!input.eof() || input.bad()) {
        throw std::runtime_error("cannot read watershed debug output: " + spatial_file.string());
    }
}

void write_debug_spatial(const std::filesystem::path& level, const std::filesystem::path& bake) {
    if (!std::filesystem::exists(bake / "placement-islands.json")
        || !std::filesystem::exists(bake / "interior-inference.json")) {
        return;
    }
    const auto spatial_file = bake / "spatial-watershed.json";
    if (std::filesystem::exists(spatial_file)) {
        std::ifstream input(spatial_file, std::ios::binary);
        const auto kind = spatial_file_kind(input);
        if (kind == "layered_watershed"
            || (kind.empty()
                && bake_json::document(spatial_file)["placement_source"].string()
                       == "layered_watershed")) {
            return;
        }
    }
    const auto profile = load_level_bake_profile(level);
    const auto& settings = profile.activation.bake;
    const auto floors = read_environment_floors(bake, level, settings.traversal);
    const auto collision = load_coverage_collision(level);
    if (!collision) {
        throw std::runtime_error(collision.error().message);
    }
    BakeScene geometry;
    for (const auto& triangle : *collision) {
        if (!triangle.support) {
            continue;  // Water interfaces are medium boundaries, not solid roofs.
        }
        BakeTriangle solid;
        solid.vertices = triangle.vertices;
        geometry.triangles.push_back(solid);
    }
    const bake_detail::SpatialWatershedSettings options;
    bake_detail::SpatialPlacement placement;
    placement.field = bake_detail::build_spatial_watershed(geometry, floors, settings, options);
    const auto& field = placement.field;
    const float outdoor_height = settings.traversal.player_height * options.air_height_scale * 3;
    const auto reflection_tops = bake_detail::spatial_reflection_coverage_tops(
        geometry, field, outdoor_height, settings.ray_bias
    );
    placement.air_height = settings.traversal.player_height * options.air_height_scale;
    placement.gi.settings.kind = bake_detail::SpatialVolumeKind::gi;
    {
        const bake_detail::SpatialVolumeFitData fit_data(field);
        placement.gi = bake_detail::fit_spatial_volumes(fit_data, placement.gi.settings);
        placement.reflection = bake_detail::fit_spatial_volumes(fit_data, {});
        bake_detail::extend_spatial_reflection_coverage(
            fit_data, placement.reflection, reflection_tops
        );
    }
    write_spatial_placement(bake, placement, false);
}

namespace {
constexpr unsigned kConnectionChunkCells = 4096;
constexpr std::string_view kBase64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void append_word(std::string& bytes, std::uint32_t word) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<char>((word >> shift) & 255));
    }
}

void write_base64(std::ostream& out, std::string_view bytes) {
    std::string encoded;
    encoded.reserve((bytes.size() + 2) / 3 * 4);
    for (std::size_t i = 0; i < bytes.size(); i += 3) {
        const auto size = bytes.size() - i;
        const auto a = static_cast<unsigned char>(bytes[i]);
        const auto b = size > 1 ? static_cast<unsigned char>(bytes[i + 1]) : 0u;
        const auto c = size > 2 ? static_cast<unsigned char>(bytes[i + 2]) : 0u;
        encoded += kBase64[a >> 2];
        encoded += kBase64[((a & 3) << 4) | (b >> 4)];
        encoded += size > 1 ? kBase64[((b & 15) << 2) | (c >> 6)] : '=';
        encoded += size > 2 ? kBase64[c & 63] : '=';
    }
    out << encoded;
}

std::string write_spatial_connections(
    const std::filesystem::path& bake, const bake_detail::SpatialWatershed& field
) {
    // Content-specific paths let older previews keep their own graph across rebakes. Publish
    // the JSON that references this directory only after all the chunks have been written.
    std::uint64_t key = 0xcbf29ce484222325ull;
    const auto mix = [&](std::uint32_t word) {
        key = (key ^ word) * 0x100000001b3ull;
    };
    mix(unsigned(field.cells.size()));
    const auto link_count = field.link_count();
    for (unsigned id = 0; id < link_count; ++id) {
        const auto link = field.link(id);
        mix(link.a);
        mix(link.b);
        mix(std::bit_cast<std::uint32_t>(field.clearance(link)));
    }
    const auto name = std::format("spatial-connections-{:016x}", key);
    std::filesystem::create_directory(bake / name);
    std::vector<unsigned> earlier;
    for (unsigned first = 0; first < field.cells.size(); first += kConnectionChunkCells) {
        const auto end = std::min(unsigned(field.cells.size()), first + kConnectionChunkCells);
        const auto base = field.offsets[first];
        std::string bytes;
        bytes.reserve(4 * (end - first + 1 + 2 * (field.offsets[end] - base)));
        for (auto i = first; i <= end; ++i) {
            append_word(bytes, field.offsets[i] - base);
        }
        for (auto i = first; i < end; ++i) {
            earlier.clear();
            for (const auto other : field.neighbours(i)) {
                if (other < i) {
                    earlier.push_back(other);
                }
            }
            // Match the legacy JSON edge iteration order for copied neighbour lists.
            std::sort(earlier.begin(), earlier.end());
            for (const auto other : earlier) {
                append_word(bytes, other);
            }
            for (const auto other : field.neighbours(i)) {
                if (other > i) {
                    append_word(bytes, other);
                }
            }
        }
        // Store edge clearance once, in cell order for edges directed to higher cell IDs.
        for (auto i = first; i < end; ++i) {
            for (const auto id : field.incident_links(i)) {
                const auto link = field.link(id);
                if (link.a == i) {
                    append_word(bytes, std::bit_cast<std::uint32_t>(field.clearance(link)));
                }
            }
        }
        const auto chunk = first / kConnectionChunkCells;
        const auto result =
            write_stage_stream(bake / name / std::format("{}.js", chunk), [&](std::ostream& out) {
                out << "globalThis.openracWatershedConnections(\"" << name << "\"," << chunk
                    << ",\"";
                write_base64(out, bytes);
                out << "\");\n";
            });
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
    }
    return name;
}

void write_spatial_json(
    std::ostream& out,
    const bake_detail::SpatialPlacement& placement,
    bool production,
    std::string_view connections
) {
    const auto& field = placement.field;
    const bake_detail::SpatialWatershedSettings options;
    out.imbue(std::locale::classic());
    out << std::setprecision(8);
    const auto point = [&](Float3 p) {
        out << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
    };
    out << "{\"version\":3,\"experimental\":" << (production ? "false" : "true")
        << ",\"placement_source\":\"" << (production ? "layered_watershed" : "prototype")
        << "\",\"representation\":\"sparse_layered_receiver_"
           "spans\","
        << "\"horizontal_step\":" << options.horizontal_step
        << ",\"vertical_step\":" << options.vertical_step
        << ",\"air_height\":" << placement.air_height
        << ",\"maximum_height\":" << options.maximum_height
        << ",\"relative_prominence\":" << options.relative_prominence
        << ",\"visibility_radius\":" << options.visibility_radius
        << ",\"visibility_split_gain\":" << options.visibility_split_gain
        << ",\"components\":" << field.components << ",\"raw_peaks\":" << field.raw_peaks
        << ",\"rejected_spans\":" << field.rejected_spans
        << ",\"edge_count\":" << field.link_count() << ",\"connections\":{\"directory\":\""
        << connections << "\",\"cells_per_chunk\":" << kConnectionChunkCells << "},\"spans\":[";
    for (unsigned i = 0; i < field.spans.size(); ++i) {
        const auto& s = field.spans[i];
        out << (i ? ",{" : "{") << "\"floor\":";
        point(s.floor);
        out << ",\"bottom\":" << s.bottom << ",\"top\":" << s.top << ",\"ceiling\":";
        if (s.ceiling) {
            out << *s.ceiling;
        } else {
            out << "null";
        }
        out << ",\"underwater\":" << (s.underwater ? "true" : "false")
            << ",\"bounded_top\":" << (s.capped ? "true" : "false") << ",\"sources\":[";
        for (unsigned j = 0; j < s.sources.size(); ++j) {
            out << (j ? "," : "") << s.sources[j];
        }
        out << "]}";
    }
    out << "],\"cells\":[";
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        const auto& c = field.cells[i];
        out << (i ? ",[" : "[") << c.position[0] << ',' << c.position[1] << ',' << c.position[2]
            << ',' << c.span << ',' << c.basin << ',' << c.lateral_clearance << ',' << c.component
            << ']';
    }
    out << "],\"basins\":[";
    for (unsigned i = 0; i < field.basins.size(); ++i) {
        const auto& b = field.basins[i];
        out << (i ? ",{" : "{") << "\"centre\":";
        point(b.centre);
        out << ",\"minimum\":";
        point(b.minimum);
        out << ",\"maximum\":";
        point(b.maximum);
        out << ",\"peak_clearance\":" << b.peak_clearance << ",\"seed_cell\":" << b.seed_cell
            << ",\"support_length\":" << b.support_length
            << ",\"support_width\":" << b.support_width
            << ",\"watershed_parent\":" << b.watershed_parent
            << ",\"watershed_peak_clearance\":" << b.watershed_peak_clearance
            << ",\"relative_prominence\":" << b.relative_prominence
            << ",\"salience\":" << b.salience << ",\"cells\":" << b.cells
            << ",\"component\":" << b.component
            << ",\"underwater\":" << (b.underwater ? "true" : "false") << '}';
    }
    out << "],\"portals\":[";
    for (unsigned i = 0; i < field.portals.size(); ++i) {
        const auto& p = field.portals[i];
        out << (i ? ",{" : "{") << "\"a\":" << p.a << ",\"b\":" << p.b << ",\"centre\":";
        point(p.centre);
        out << ",\"saddle_clearance\":" << p.saddle_clearance
            << ",\"connections\":" << p.connections << ",\"crossings\":[";
        for (unsigned j = 0; j < p.crossings.size(); ++j) {
            const auto [a, b] = p.crossings[j];
            out << (j ? ",[" : "[") << a << ',' << b << ']';
        }
        out << "]}";
    }
    out << "],\"visibility_splits\":[";
    for (unsigned i = 0; i < field.visibility_splits.size(); ++i) {
        const auto& split = field.visibility_splits[i];
        out << (i ? ",{" : "{") << "\"watershed_parent\":" << split.parent << ",\"a\":" << split.a
            << ",\"b\":" << split.b << ",\"before\":" << split.before
            << ",\"after\":" << split.after << ",\"gain\":" << split.before - split.after
            << ",\"landmarks\":[";
        for (unsigned j = 0; j < split.landmarks.size(); ++j) {
            if (j) {
                out << ',';
            }
            point(split.landmarks[j]);
        }
        out << "],\"landmark_normals\":[";
        for (unsigned j = 0; j < split.landmark_normals.size(); ++j) {
            if (j) {
                out << ',';
            }
            point(split.landmark_normals[j]);
        }
        out << "],\"samples\":[";
        for (unsigned j = 0; j < split.samples.size(); ++j) {
            const auto& sample = split.samples[j];
            out << (j ? ",{" : "{") << "\"cell\":" << sample.cell << ",\"basin\":" << sample.basin
                << ",\"visible\":[";
            for (unsigned k = 0; k < sample.visible.size(); ++k) {
                out << (k ? "," : "") << sample.visible[k];
            }
            out << "]}";
        }
        out << "]}";
    }
    out << "],\"visibility_assessments\":[";
    for (unsigned i = 0; i < field.visibility_assessments.size(); ++i) {
        const auto& a = field.visibility_assessments[i];
        out << (i ? ",{" : "{") << "\"basin\":" << a.basin << ",\"before\":" << a.before
            << ",\"proposed\":" << a.proposed << ",\"after\":" << a.after << '}';
    }
    out << "],\"gi_rejected\":" << (placement.gi_rejected ? "true" : "false")
        << ",\"volume_fits\":{";
    for (unsigned kind = 0; kind < 2; ++kind) {
        const auto& plan = kind ? placement.gi : placement.reflection;
        const auto& fit_options = plan.settings;
        out << (kind ? ",\"gi\":{" : "\"reflection\":{")
            << "\"experimental\":" << (production ? "false" : "true")
            << ",\"covered\":" << plan.covered << ",\"uncovered\":" << plan.uncovered
            << ",\"over_budget\":" << plan.over_budget
            << ",\"estimated_probes\":" << plan.estimated_probes
            << ",\"volume_limit_exceeded\":" << (plan.volume_limit_exceeded ? "true" : "false")
            << ",\"single_box_volume\":" << plan.single_box_volume
            << ",\"fitted_volume\":" << plan.fitted_volume << ",\"padding\":" << fit_options.padding
            << ",\"spacing\":";
        point(fit_options.spacing);
        out << ",\"maximum_dimension\":" << fit_options.maximum_dimension
            << ",\"maximum_extent\":" << fit_options.maximum_extent << ",\"volumes\":[";
        for (unsigned i = 0; i < plan.volumes.size(); ++i) {
            const auto& v = plan.volumes[i];
            out << (i ? ",{" : "{") << "\"basin\":" << v.basin << ",\"origin\":";
            point(v.origin);
            out << ",\"extent\":";
            point(v.extent);
            out << ",\"representative\":";
            point(v.representative);
            out << ",\"axis\":[" << v.frame.axis[0] << ',' << v.frame.axis[1] << "],\"pitch\":["
                << v.frame.pitch[0] << ',' << v.frame.pitch[1] << "],\"roll\":[" << v.frame.roll[0]
                << ',' << v.frame.roll[1] << "],\"dimensions\":[" << v.dimensions[0] << ','
                << v.dimensions[1] << ',' << v.dimensions[2] << "],\"score\":" << v.score
                << ",\"volume\":" << v.volume << ",\"coverage_growth\":" << v.coverage_growth
                << ",\"estimated_foreign_volume\":" << v.foreign_volume
                << ",\"estimated_occupied_volume\":" << std::min(v.volume, v.occupied_volume)
                << ",\"feasible\":" << (v.feasible ? "true" : "false") << ",\"world_corners\":[";
            for (unsigned c = 0; c < 8; ++c) {
                Float3 p{};
                for (unsigned k = 0; k < 3; ++k) {
                    if (c & (1 << k)) {
                        p[k] = v.extent[k];
                    }
                }
                p = v.frame.to_world(p);
                for (unsigned k = 0; k < 3; ++k) {
                    p[k] += v.origin[k];
                }
                if (c) {
                    out << ',';
                }
                point(p);
            }
            out << "],\"cells\":[";
            for (unsigned j = 0; j < v.cells.size(); ++j) {
                out << (j ? "," : "") << v.cells[j];
            }
            out << "]}";
        }
        out << "]}";
    }
    out << "}}\n";
}
}  // namespace

void write_spatial_placement(
    const std::filesystem::path& bake,
    const bake_detail::SpatialPlacement& placement,
    bool production
) {
    const auto connections = write_spatial_connections(bake, placement.field);
    const auto result = write_stage_stream(bake / "spatial-watershed.json", [&](std::ostream& out) {
        write_spatial_json(out, placement, production, connections);
    });
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace openrac::lighting
