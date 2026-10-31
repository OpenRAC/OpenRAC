// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Canonical probe fingerprints for source snapshots, exported transport and solver settings.

#include "enhanced/tools/baker/sources/probe_fingerprint.h"

#include <algorithm>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "common/profiler.h"
#include "enhanced/tools/baker/sources/level_scene.h"
#include "scene/level.h"

namespace openrac::lighting {
namespace {

// Stable FNV-1a over canonical scalar bytes; a cache identity, not a security
// digest. Bump the domain when scene or solver interpretation changes.
struct Hash {
    // FNV-1a offset basis for a fresh canonical byte stream.
    std::uint64_t value = 14695981039346656037ull;

    // Adds one byte with the FNV-1a 64-bit prime.
    void byte(std::uint8_t b) { value = (value ^ b) * 1099511628211ull; }

    // Adds a 32-bit word in little-endian order.
    void u32(std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i) {
            byte(static_cast<std::uint8_t>(n >> (8 * i)));
        }
    }

    // Preserves exact IEEE float bits rather than formatting decimal text.
    void f32(float n) { u32(std::bit_cast<std::uint32_t>(n)); }

    // Adds vector components in their stored order.
    template <std::size_t N>
    void floats(const std::array<float, N>& v) {
        for (float n : v) {
            f32(n);
        }
    }
};

// Lists transport inputs in canonical relative-path order.
std::vector<std::filesystem::path> source_paths(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    std::vector<fs::path> files;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        // Only regular source files contribute to transport.
        if (!entry.is_regular_file()) {
            continue;
        }

        // PVS and offline reference selection do not change light transport.
        // Their sidecars must not invalidate otherwise identical probe bakes.
        const auto name = entry.path().lexically_relative(root).generic_string();

        if (name == "pvs.json" || name == "sunlight-calibration.json"
            || name.starts_with(".water-native-")) {
            continue;
        }

        const auto extension = entry.path().extension().string();

        // Raw bake sections and exported meshes, images and sidecars affect transport.
        if (extension == ".glb" || extension == ".png" || extension == ".json"
            || (extension == ".bin"
                && entry.path().lexically_relative(root).generic_string().starts_with(
                    "bake-inputs/"
                ))) {
            files.push_back(entry.path());
        }
    }

    // Canonical path order makes directory iteration order irrelevant.
    std::sort(files.begin(), files.end(), [&](const auto& a, const auto& b) {
        return a.lexically_relative(root).generic_string()
               < b.lexically_relative(root).generic_string();
    });
    return files;
}

// Adds a source file's relative path, size and bytes; throws on incomplete reads.
void source_file(Hash& hash, const std::filesystem::path& root, const std::filesystem::path& file) {
    // A bounded streaming buffer avoids loading large meshes for fingerprinting.
    std::array<char, 65'536> buffer{};
    const auto relative = file.lexically_relative(root).generic_string();

    // Stricter ceiling enclosure changes transport only for levels with generated interior sources.
    if (relative == "probe-interiors.json") {
        hash.u32(0x494E5403);  // Interior targets are reflected radiance, converted to irradiance.
    }

    for (char c : relative) {
        hash.byte(static_cast<std::uint8_t>(c));
    }

    hash.byte(0);
    // Encode byte length as two little-endian words before streaming the payload.
    const auto size = std::filesystem::file_size(file);
    hash.u32(static_cast<std::uint32_t>(size));
    hash.u32(static_cast<std::uint32_t>(size >> 32));
    std::ifstream input(file, std::ios::binary);

    // A missing input cannot share an identity with an empty file.
    if (!input) {
        throw std::runtime_error("cannot fingerprint probe scene input");
    }

    // Read through EOF, including a final short block.
    while (input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()))
           || input.gcount() > 0) {
        for (std::streamsize i = 0; i < input.gcount(); ++i) {
            hash.byte(static_cast<std::uint8_t>(buffer[static_cast<std::size_t>(i)]));
        }
    }

    // An interrupted read must not produce a partial cache identity.
    if (!input.eof()) {
        throw std::runtime_error("cannot read probe scene fingerprint");
    }
}

// Adds a named source tree without hashing derived transforms or unrelated sidecars.
void source_files(Hash& hash, const std::filesystem::path& root, const char* label) {
    // Separate base and override roots even when their relative paths coincide.
    for (const char* p = label; *p; ++p) {
        hash.byte(static_cast<std::uint8_t>(*p));
    }

    hash.byte(0);
    const auto files = source_paths(root);
    hash.u32(static_cast<std::uint32_t>(files.size()));

    for (const auto& file : files) {
        source_file(hash, root, file);
    }
}

// Adds one fixture triangle in the stored transport field order.
void triangle_hash(Hash& hash, const BakeTriangle& triangle) {
    for (auto p : triangle.vertices) {
        hash.floats(p);
    }

    for (auto uv : triangle.uv) {
        hash.floats(uv);
    }

    hash.floats(triangle.diffuse);
    hash.f32(triangle.diffuse_scale);

    // Authored albedo changes transport even when the geometric mesh stays identical.
    if (triangle.terrain_prelighting) {
        hash.u32(0x54494e54);

        for (auto tint : triangle.authored_colours) {
            hash.floats(tint);
        }
    }

    hash.u32(static_cast<std::uint32_t>(triangle.texture));
    hash.f32(triangle.alpha_cutoff);
    hash.floats(triangle.emission);

    // Explicit bake radiance disables interior boosts, even when its value matches the default.
    if (triangle.fixed_emission) {
        hash.u32(0x46495845);  // Fixed emission transport.
    }

    hash.u32(static_cast<std::uint32_t>(triangle.emission_texture));
    if (triangle.emission_contrast != 1) {
        hash.u32(0x45434f4e);  // Emission contrast changes transport.
        hash.f32(triangle.emission_contrast);
    }

    // Only enabled vertex emission contributes its colour payload.
    if (triangle.emission_vertex_colour) {
        hash.u32(0x54494e54);

        for (const auto& c : triangle.emission_colours) {
            hash.floats(c);
        }
    }

    for (const auto& multiplier : triangle.multipliers) {
        hash.floats(multiplier);
    }
}

// Adds derived fixture transport only when canonical source files do not exist.
void structural_scene_hash(Hash& hash, const BakeScene& scene) {
    hash.u32(scene.sky_width);
    hash.u32(scene.sky_height);

    for (const auto& pixel : scene.sky_radiance) {
        hash.floats(pixel);
    }

    hash.floats(scene.primary_directional_colour);

    for (const auto& sun : scene.suns) {
        hash.floats(sun.direction);
        hash.floats(sun.irradiance);
    }

    hash.u32(static_cast<std::uint32_t>(scene.textures.size()));

    for (const auto& texture : scene.textures) {
        hash.u32(texture.width);
        hash.u32(texture.height);

        for (auto b : texture.rgba) {
            hash.byte(b);
        }
    }

    for (const auto& triangle : scene.triangles) {
        triangle_hash(hash, triangle);
    }
}

// Adds proxy interpretation and, for fixtures, their derived light values.
void point_emitters_hash(Hash& hash, const scene::LevelData& level, const BakeScene& scene) {
    // Only levels with sphere proxies change identity; unaffected bakes stay valid.
    if (!scene.point_emitters.empty()) {
        hash.u32(0x53504801);
        hash.u32(static_cast<std::uint32_t>(scene.point_emitters.size()));
        hash.f32(0.85f);
        hash.floats(Float3{2.0f, 1.0f, 0.2f});

        // Real levels fingerprint source placements below. Do not hash derived
        // additions here: the game runs FE_TOWARDZERO, the offline bake does not.
        if (level.dir.empty()) {
            for (const auto& light : scene.point_emitters) {
                hash.floats(light.position);
                hash.floats(light.intensity);
            }
        }
    }
}

}  // namespace

std::uint64_t probe_input_hash(const std::filesystem::path& directory) {
    const profiling::Zone timing("Fingerprint bake inputs");

    Hash hash;
    source_files(hash, directory, "base");
    // Coastlines affect placement, while the transport scene hash deliberately retains otherwise valid GI.
    if (std::filesystem::exists(directory / "water.bin")) {
        source_file(hash, directory, directory / "water.bin");
    }
    return hash.value;
}

std::uint64_t probe_scene_hash(
    const scene::LevelData& level,
    const BakeSceneExport& out,
    unsigned light_set,
    bool liquid_source
) {
    const Float3 environment = out.scene.environment;
    Hash hash;
    hash.u32(0x53434e07);

    // Liquid interpretation participates even when runtime activation skips its analysis.
    if (liquid_source) {
        hash.u32(0x4c415601);  // authored liquid interpretation revision
    }

    hash.u32(light_set);
    hash.floats(environment);

    // Whole-texture emission changes transport; older emissive bakes must not remain active.
    if (std::any_of(level.materials.begin(), level.materials.end(), [](const auto& material) {
            return material.bake_intensity.value_or(
                       *std::max_element(material.emission.begin(), material.emission.end())
                   )
                   > 0;
        })) {
        hash.u32(
            0x454D4903
        );  // Authored static emission includes the instance's light-slot colours.
    }

    point_emitters_hash(hash, level, out.scene);

    // The running game uses FE_TOWARDZERO, including while loading assets.
    // Fingerprint canonical source bytes rather than rounded transform
    // products. File identity is stable across baker and game FP modes.
    if (!level.dir.empty()) {
        source_files(hash, level.dir, "base");

        if (!level.overrides.empty() && level.overrides != level.dir) {
            source_files(hash, level.overrides, "override");
        }

        return hash.value;
    }

    // In-memory fixtures have no source directory. Keep a structural hash
    // for those providers, which must share their arithmetic convention.
    structural_scene_hash(hash, out.scene);
    return hash.value;
}

std::uint64_t probe_bake_settings_hash(const ProbeGrid& grid, const BakeSettings& settings) {
    Hash hash;
    hash.u32(
        0x42414B0C
    );  // Clearance-scored alignment changes initial probe positions before relocation.
    hash.u32(kProbeTexelSamplesPerAxis);
    hash.u32(settings.use_level_sky ? 1u : 0u);
    hash.f32(settings.sky_intensity);
    hash.f32(settings.sky_weight);

    // Unit gain retains physical fixture identities; other gains invalidate existing bakes.
    if (settings.surface_emission_scale != 1) {
        hash.u32(0x454D5343u);  // EMSC identifies the bake-only surface emission gain.
        hash.f32(settings.surface_emission_scale);
    }

    // Local source gains are transport inputs even when the global gain remains one.
    if (!settings.surface_emission_regions.empty()) {
        hash.u32(0x454D5247u);  // EMRG identifies ordered room-local surface emission overrides.
        hash.u32(static_cast<std::uint32_t>(settings.surface_emission_regions.size()));
        for (const auto& region : settings.surface_emission_regions) {
            hash.floats(region.minimum);
            hash.floats(region.maximum);
            hash.f32(region.scale);
        }
    }
    hash.floats(settings.sunlight_scale);

    // Preserve existing bake identities when no direction override is requested.
    if (settings.sunlight_direction) {
        hash.u32(0x53554E44u);  // SUND marks the optional light travel direction.
        hash.floats(*settings.sunlight_direction);
    }

    hash.u32(settings.terrain_vertex_albedo ? 1u : 0u);
    hash.floats(grid.origin);
    hash.floats(grid.frame.axis);
    hash.floats(grid.spacing);

    for (auto n : grid.dimensions) {
        hash.u32(n);
    }

    hash.u32(grid.map_resolution);
    hash.f32(grid.max_distance);
    hash.u32(settings.rays_per_probe);
    hash.u32(settings.bounces);
    hash.u32(settings.seed);
    hash.f32(settings.ray_bias);
    hash.f32(settings.distance_exponent);
    hash.f32(settings.min_inside_fraction);
    hash.u32(settings.relocation_iterations);
    hash.f32(settings.relocation_max_fraction);
    return hash.value;
}
}  // namespace openrac::lighting
