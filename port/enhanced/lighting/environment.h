// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GPU-ready reflection faces, GGX levels and their spatial influence boxes.
#pragma once
#include <filesystem>

#include "enhanced/lighting/probes.h"

namespace openrac::lighting {
// Bounded per-level capture budget; metadata is shared through a GPU texture buffer.
inline constexpr unsigned kEnvironmentProbeLimit = 512;
inline constexpr unsigned kEnvironmentBrdfSize = 64;
// Omit the sharpest capture level while retaining its source resolution for GGX filtering.
inline constexpr unsigned kEnvironmentFirstMip = 1;

// Per-level capture safety limit, not a placement target; disabled until a level opts in.
struct EnvironmentBakeSettings {
    bool enabled = false;
    unsigned resolution = 128, maximum_probes = kEnvironmentProbeLimit, filter_samples = 128;
    float spacing = 24, box_distance = 64, normal_bias = .5f, view_bias = .5f;
    float minimum_capture_spacing = 6;
    float minimum_space_area = 4;  // World units squared required for an independent open patch.
    bool merge_enabled = true;
    float merge_overlap = .85f, merge_loss_fraction = .005f;
    unsigned merge_max_misses = 2;
    float merge_brightness_ratio = 2, merge_colour_distance = .06f;
};

// The capture point and the geometry proxy used for box-projected reflections.
struct EnvironmentProbe {
    Float3 position{};
    ProbeFrame frame;
    Float3 minimum{};
    Float3 maximum{};
    Float3 projection_minimum{}, projection_maximum{};

    EnvironmentProbe(Float3 point = {}, Float3 lo = {}, Float3 hi = {})
        : position(point),
          minimum(lo),
          maximum(hi),
          projection_minimum(lo),
          projection_maximum(hi) {}
};

// Optional bake diagnostics: accepted visible contributors at floor, body and head height.
struct EnvironmentCoverageReceiver {
    Float3 position{};
    unsigned height = 0;
    std::vector<unsigned> probes;
};

// Five vec4s per capture; unused W components hold the orientation without growing GPU metadata.
std::vector<std::array<float, 4>> environment_metadata(std::span<const EnvironmentProbe> probes);

// Identifies capture ordering and both kinds of bounds without hashing the large radiance atlas.
std::uint64_t environment_capture_hash(
    std::span<const EnvironmentProbe> probes, std::uint64_t gi_hash = 0
);

// Each HDR face/level has a gutter and compression-block padding; the BRDF lookup stays separate.
struct EnvironmentAsset {
    // Source capture face size; stored faces start one mip below this resolution.
    unsigned resolution = 128;
    unsigned level = 0;
    std::uint64_t scene_hash = 0, settings_hash = 0, gi_hash = 0;
    std::vector<EnvironmentProbe> probes;
    std::vector<std::uint32_t> pixels;
    ProbeTextureFormat colour_format =
        ProbeTextureFormat::R11G11B10;    // Radiance encoding on disk/GPU.
    std::vector<std::uint8_t> gpu_bytes;  // Complete directly uploadable radiance plane.
    std::vector<std::uint32_t>
        brdf;  // Uncompressed material-response lookup, independent of radiance.
    unsigned levels() const;
    // Returns the stored face size at `mip`, starting with the first retained GGX level.
    unsigned face_resolution(unsigned mip = 0) const;
    // Block-aligned face pitch keeps independent faces and GGX levels out of shared compression blocks.
    unsigned face_pitch(unsigned mip = 0) const;
    unsigned stack_height() const;
    unsigned columns() const;
    unsigned probe_x(unsigned probe) const;
    unsigned probe_y(unsigned probe) const;
    unsigned brdf_row() const;
    unsigned width() const;
    unsigned height() const;
    unsigned level_row(unsigned mip) const;
    void validate() const;
};

// Matching CPU/GLSL face conventions; UV can extend into gutters.
Float3 environment_direction(unsigned face, Float2 uv);
std::pair<unsigned, Float2> environment_face(Float3 direction);
Float3 environment_box_direction(const EnvironmentProbe& probe, Float3 point, Float3 direction);
// Smooth world-space influence fade, shared by placement validation and the runtime shader.
float environment_probe_coverage(const EnvironmentProbe& probe, Float3 point);
// Finite positive HDR encoding for direct GPU upload.
std::uint32_t pack_environment_colour(Float3 colour);
Float3 unpack_environment_colour(std::uint32_t colour);
// Includes every GI payload so replacing a bake invalidates its reflection captures.
std::uint64_t environment_gi_hash(const std::filesystem::path& directory);
// Portable versioned files; malformed or stale data is never uploaded.
std::vector<std::uint8_t> encode_environment(const EnvironmentAsset& asset);
EnvironmentAsset decode_environment(std::span<const std::uint8_t> bytes);
// Retains compressed blocks for runtime upload, expanding only the small capture metadata.
EnvironmentAsset decode_gpu_environment(std::span<const std::uint8_t> bytes);
}  // namespace openrac::lighting
