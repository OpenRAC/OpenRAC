// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe scene preparation: sky, authored sunlight, static transport, liquids and receiver bounds.

#include "enhanced/tools/probes/level_scene.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numbers>
#include <stdexcept>

#include "assets/disc/wad.h"
#include "common/json.h"
#include "enhanced/tools/probes/liquids.h"
#include "enhanced/tools/probes/probe_fingerprint.h"
#include "enhanced/tools/probes/scene_geometry.h"

namespace openrac::lighting {
namespace {
// The normal extractor retains raw source sections. Keep legacy explicitly
// supplied surfaces authoritative so old exports never emit the lava twice.
bool automatic_liquids(const scene::LevelData& level) {
    if (level.dir.empty() || level.level != 9) {
        return false;
    }
    json::Value manifest;
    std::string error;
    if (!json::parse_file((level.dir / "manifest.json").string(), manifest, error)) {
        throw std::runtime_error(error);
    }
    const auto& source = manifest["bake_inputs"]["liquid"].string();
    if (source.empty()) {
        return false;
    }
    if (source != "rac1-ntsc-gaspar-v1" || manifest["game"].string() != "rac1"
        || manifest["version"].string() != "ntsc") {
        throw std::runtime_error("Unsupported authored liquid bake input");
    }
    for (const auto& instance : level.bake_surfaces) {
        for (const auto& primitive : level.models.at(instance.model).primitives) {
            if (primitive.material < 0) {
                continue;
            }
            const auto image = level.materials.at(primitive.material).image;
            if (image >= 0
                && std::filesystem::path(level.image_paths.at(static_cast<std::size_t>(image)))
                           .filename()
                       == "probe_gaspar_lava.png") {
                return false;
            }
        }
    }
    return true;
}

std::vector<std::uint8_t> liquid_input(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error("Missing authored liquid bake source: " + path.string());
    }
    const auto size = input.tellg();
    if (size <= 0 || size > 256ll * 1024 * 1024) {
        throw std::runtime_error("Unsupported authored liquid source size");
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("Cannot read authored liquid source");
    }
    return assets::disc::is_wad(bytes) ? assets::disc::wad_decompress(bytes) : bytes;
}

// Averages one sky footprint in linear space, retaining source-pixel accumulation order.
Float3 average_sky_texel(
    const renderer::Rgba8Image& sky,
    const std::array<float, 256>& linear,
    const std::array<unsigned, 4>& bounds
) {
    const auto [x0, x1, y0, y1] = bounds;
    Float3 sum{};
    for (unsigned sy = y0; sy < y1; ++sy) {
        for (unsigned sx = x0; sx < x1; ++sx) {
            const auto at = (std::size_t{sy} * static_cast<unsigned>(sky.width) + sx) * 4;
            for (unsigned c = 0; c < 3; ++c) {
                sum[c] += linear[sky.pixels[at + c]];
            }
        }
    }
    for (float& v : sum) {
        v /= static_cast<float>((x1 - x0) * (y1 - y0));
    }
    return sum;
}

// Converts the composited sky to a bounded linear-radiance panorama.
void export_sky(const scene::LevelData& level, BakeScene& target) {
    // Reuse the editor's composited static sky shells. Its documented axes
    // are +Z at the top and +Y at longitude zero, toward +X. Do not use this
    // distant environment as blocking geometry or include runtime sun sprites.
    const auto& sky = level.sky_panorama;
    // An absent panorama keeps the profile's uniform environment fallback.
    if (sky.width <= 0 || sky.height <= 0) {
        return;
    }
    if (sky.pixels.size()
        != std::size_t{static_cast<unsigned>(sky.width)} * static_cast<unsigned>(sky.height) * 4) {
        throw std::runtime_error("invalid level sky panorama");
    }
    target.sky_width = static_cast<unsigned>(std::min(sky.width, 256));
    target.sky_height = static_cast<unsigned>(std::min(sky.height, 128));
    target.sky_radiance.resize(std::size_t{target.sky_width} * target.sky_height);
    std::array<float, 256> linear{};
    for (unsigned i = 0; i < 256; ++i) {
        const float v = static_cast<float>(i) / 255;
        linear[i] = v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
    }
    for (unsigned y = 0; y < target.sky_height; ++y) {
        const unsigned y0 = y * static_cast<unsigned>(sky.height) / target.sky_height;
        const unsigned y1 = (y + 1) * static_cast<unsigned>(sky.height) / target.sky_height;
        for (unsigned x = 0; x < target.sky_width; ++x) {
            const unsigned x0 = x * static_cast<unsigned>(sky.width) / target.sky_width;
            const unsigned x1 = (x + 1) * static_cast<unsigned>(sky.width) / target.sky_width;
            target.sky_radiance[std::size_t{y} * target.sky_width + x] =
                average_sky_texel(sky, linear, {x0, x1, y0, y1});
        }
    }
}

// Converts the selected authored directional lights to transport irradiance.
void export_suns(const scene::LevelData& level, unsigned light_set, BakeScene& target) {
    for (unsigned c = 0; c < 3; ++c) {
        target.primary_directional_colour[c] = level.light_sets[light_set * 16 + c];
    }
    for (unsigned which : {0u, 2u}) {
        const auto at = light_set * 16 + which * 4;
        BakeSun sun;
        sun.primary = which == 0;
        for (unsigned c = 0; c < 3; ++c) {
            sun.direction[c] = level.light_sets[at + 4 + c];
            sun.irradiance[c] = level.light_sets[at + c] * std::numbers::pi_v<float>;
        }
        if (renderer::dot(sun.direction, sun.direction) > 0
            && *std::max_element(sun.irradiance.begin(), sun.irradiance.end()) > 0) {
            target.suns.push_back(sun);
        }
    }
}

// Adds bake-only liquid surfaces after exported meshes, retaining their source order.
void export_liquids(const scene::LevelData& level, BakeSceneExport& out) {
    const auto directory = level.dir / "bake-inputs";
    const auto overlay = liquid_input(directory / "overlay.bin");
    const auto index = liquid_input(directory / "core_index.bin");
    const auto core = liquid_input(directory / "core_data.bin");
    const auto triangles = gaspar_liquid_triangles(overlay, index, core);
    out.automatic_liquid_triangles = triangles.size();
    out.surface_instances += triangles.empty() ? 0 : 1;
    for (const auto& triangle : triangles) {
        for (const auto& point : triangle.vertices) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                out.bounds_min[axis] = std::min(out.bounds_min[axis], point[axis]);
                out.bounds_max[axis] = std::max(out.bounds_max[axis], point[axis]);
            }
        }
        out.scene.triangles.push_back(triangle);
        ++out.emissive_triangles;
    }
}

// Includes every initial moby in allocation bounds, even when excluded from transport.
void include_receivers(const scene::LevelData& level, BakeSceneExport& out) {
    // All mobys remain receivers, including those excluded from transport.
    for (const auto& instance : level.instances[static_cast<std::size_t>(scene::Layer::Mobys)]) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            out.bounds_min[axis] = std::min(out.bounds_min[axis], instance.matrix[12 + axis]);
            out.bounds_max[axis] = std::max(out.bounds_max[axis], instance.matrix[12 + axis]);
        }
    }
}

// Adds initial energy-sphere proxies without introducing rotating occluders.
void export_proxy_emitters(const scene::LevelData& level, BakeSceneExport& out) {
    // The holder's update spawns class 1040 at its world position +0.85 Z.
    // Use a warm emission-only proxy for that initial state; no rotating mesh
    // enters visibility or distance moments. Intensity is an artistic bake setting.
    const auto holder = level.moby_classes.find(1038);
    if (holder != level.moby_classes.end() && !holder->second.second) {
        for (const auto& instance :
             level.instances[static_cast<std::size_t>(scene::Layer::Mobys)]) {
            if (instance.model == holder->second.first && !instance.stand_in) {
                out.scene.point_emitters.push_back(
                    {{instance.matrix[12], instance.matrix[13], instance.matrix[14] + 0.85f},
                     {2.0f, 1.0f, 0.2f}}
                );
            }
        }
    }
}

}  // namespace

BakeSceneExport export_probe_scene(
    const scene::LevelData& level, unsigned light_set, Float3 environment, bool analyze_liquids
) {
    if (light_set >= 16 || level.light_sets.size() != 256) {
        throw std::runtime_error("level has no requested directional light set");
    }
    if (level.missing_images) {
        throw std::runtime_error("bake requires all referenced textures; level has missing images");
    }
    BakeSceneExport out;
    out.bounds_min.fill(std::numeric_limits<float>::max());
    out.bounds_max.fill(std::numeric_limits<float>::lowest());
    out.scene.environment = environment;
    export_sky(level, out.scene);
    export_suns(level, light_set, out.scene);
    append_probe_geometry(level, out);
    const bool liquid_source = automatic_liquids(level);
    // Runtime activation fingerprints raw liquid inputs without repeating bake-only analysis.
    if (liquid_source && analyze_liquids) {
        export_liquids(level, out);
    }
    if (out.scene.triangles.empty()) {
        throw NothingToBake("no static render geometry to bake");
    }
    include_receivers(level, out);
    export_proxy_emitters(level, out);
    out.scene_hash = probe_scene_hash(level, out, light_set, liquid_source);
    return out;
}

}  // namespace openrac::lighting
