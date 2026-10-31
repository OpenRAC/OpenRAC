// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Portable reflection asset layout and shared cubemap projection arithmetic.
#include "enhanced/lighting/environment.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

#include "common/float_encoding.h"

namespace openrac::lighting {
namespace {
// Read/write little-endian words without native structure padding.
void word(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
    }
}

// Reject truncated input before advancing the cursor.
std::uint32_t word(std::span<const std::uint8_t> bytes, std::size_t& at) {
    if (at > bytes.size() || bytes.size() - at < 4) {
        throw std::runtime_error("truncated environment asset");
    }

    std::uint32_t value = 0;

    for (unsigned i = 0; i < 4; ++i) {
        value |= std::uint32_t(bytes[at++]) << (i * 8);
    }

    return value;
}

// Decode the unsigned floating channels used by R11G11B10F.
float channel(unsigned bits, unsigned mantissa) {
    const unsigned exponent = (bits >> mantissa) & 31;
    return std::ldexp(
        float((bits & ((1u << mantissa) - 1)) + (exponent ? 1u << mantissa : 0)),
        (exponent ? int(exponent) - 15 : -14) - int(mantissa)
    );
}
}  // namespace

std::uint64_t environment_capture_hash(
    std::span<const EnvironmentProbe> probes, std::uint64_t gi_hash
) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned shift = 0; shift < 64; shift += 8) {
        hash = (hash ^ ((gi_hash >> shift) & 255)) * 1099511628211ull;
    }
    for (const auto& probe : probes) {
        for (auto value : probe.frame.axis) {
            const auto bits = std::bit_cast<unsigned>(value);
            for (unsigned shift = 0; shift < 32; shift += 8) {
                hash = (hash ^ ((bits >> shift) & 255)) * 1099511628211ull;
            }
        }
        for (auto p :
             {probe.position,
              probe.minimum,
              probe.maximum,
              probe.projection_minimum,
              probe.projection_maximum}) {
            for (float value : p) {
                const auto bits = std::bit_cast<std::uint32_t>(value);
                for (unsigned shift = 0; shift < 32; shift += 8) {
                    hash = (hash ^ ((bits >> shift) & 255)) * 1099511628211ull;
                }
            }
        }
    }
    return hash;
}

std::vector<std::array<float, 4>> environment_metadata(std::span<const EnvironmentProbe> probes) {
    std::vector<std::array<float, 4>> result;
    for (const auto& p : probes) {
        const auto start = result.size();
        for (auto v :
             {p.position, p.minimum, p.maximum, p.projection_minimum, p.projection_maximum}) {
            result.push_back({v[0], v[1], v[2], 0});
        }
        result[start][3] = p.frame.axis[0];
        result[start + 1][3] = p.frame.axis[1];
    }
    return result;
}

unsigned EnvironmentAsset::levels() const {
    return std::bit_width(resolution >> kEnvironmentFirstMip);
}

unsigned EnvironmentAsset::face_resolution(unsigned mip) const {
    return resolution >> (mip + kEnvironmentFirstMip);
}

unsigned EnvironmentAsset::columns() const {
    unsigned columns = 1;
    while (std::size_t(columns) * columns * 6 * face_pitch() < probes.size() * stack_height()) {
        ++columns;
    }
    return columns;
}

unsigned EnvironmentAsset::probe_x(unsigned probe) const {
    return (probe % columns()) * 6 * face_pitch();
}

unsigned EnvironmentAsset::probe_y(unsigned probe) const {
    return (probe / columns()) * stack_height();
}

unsigned EnvironmentAsset::brdf_row() const {
    return static_cast<unsigned>((probes.size() + columns() - 1) / columns()) * stack_height();
}

unsigned EnvironmentAsset::width() const {
    return std::max(kEnvironmentBrdfSize, columns() * 6 * face_pitch());
}

unsigned EnvironmentAsset::level_row(unsigned mip) const {
    unsigned row = 0;

    for (unsigned i = 0; i < mip; ++i) {
        row += face_pitch(i);
    }

    return row;
}

unsigned EnvironmentAsset::face_pitch(unsigned mip) const {
    const unsigned alignment = probe_texture_block(colour_format);
    return (face_resolution(mip) + 2 + alignment - 1) / alignment * alignment;
}

unsigned EnvironmentAsset::stack_height() const {
    return level_row(levels());
}

unsigned EnvironmentAsset::height() const {
    return brdf_row() + kEnvironmentBrdfSize;
}

void EnvironmentAsset::validate() const {
    if (resolution < 4 || resolution > 256 || !std::has_single_bit(resolution) || probes.empty()
        || probes.size() > kEnvironmentProbeLimit || width() > 16384 || height() > 16384
        || (gpu_bytes.empty()
                ? pixels.size() != std::size_t(width()) * height()
                : gpu_bytes.size() != probe_texture_bytes(colour_format, width(), brdf_row())
                      || brdf.size() != std::size_t{kEnvironmentBrdfSize} * kEnvironmentBrdfSize)) {
        throw std::runtime_error("invalid environment atlas layout");
    }

    for (const auto& probe : probes) {
        const auto centre = probe.frame.to_local(probe.position);
        if (!probe.frame.valid()) {
            throw std::runtime_error("invalid environment orientation");
        }
        for (unsigned a = 0; a < 3; ++a) {
            if (!std::isfinite(probe.position[a]) || !std::isfinite(probe.minimum[a])
                || !std::isfinite(probe.maximum[a]) || probe.minimum[a] >= centre[a]
                || probe.maximum[a] <= centre[a] || !std::isfinite(probe.projection_minimum[a])
                || !std::isfinite(probe.projection_maximum[a])
                || probe.projection_minimum[a] >= centre[a]
                || probe.projection_maximum[a] <= centre[a]) {
                throw std::runtime_error(
                    "invalid environment influence/projection box at "
                    + std::to_string(probe.position[0]) + "," + std::to_string(probe.position[1])
                    + "," + std::to_string(probe.position[2]) + " axis " + std::to_string(a)
                );
            }
        }
    }

    for (auto pixel : pixels) {
        if (((pixel >> 6) & 31) == 31 || ((pixel >> 17) & 31) == 31 || (pixel >> 27) == 31) {
            throw std::runtime_error("nonfinite environment radiance");
        }
    }
}

Float3 environment_direction(unsigned face, Float2 uv) {
    const float x = uv[0] * 2 - 1, y = uv[1] * 2 - 1;
    const std::array<Float3, 6> directions{
        {{1, -y, -x}, {-1, -y, x}, {x, 1, y}, {x, -1, -y}, {x, -y, 1}, {-x, -y, -1}}
    };
    auto n = directions.at(face);
    const float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);

    for (auto& value : n) {
        value /= length;
    }

    return n;
}

std::pair<unsigned, Float2> environment_face(Float3 d) {
    const Float3 a{std::abs(d[0]), std::abs(d[1]), std::abs(d[2])};
    unsigned face;
    Float2 uv;

    if (a[0] >= a[1] && a[0] >= a[2]) {
        face = d[0] >= 0 ? 0u : 1u;
        uv = {d[0] >= 0 ? -d[2] / a[0] : d[2] / a[0], -d[1] / a[0]};
    } else if (a[1] >= a[2]) {
        face = d[1] >= 0 ? 2u : 3u;
        uv = {d[0] / a[1], d[1] >= 0 ? d[2] / a[1] : -d[2] / a[1]};
    } else {
        face = d[2] >= 0 ? 4u : 5u;
        uv = {d[2] >= 0 ? d[0] / a[2] : -d[0] / a[2], -d[1] / a[2]};
    }

    return {face, {(uv[0] + 1) * .5f, (uv[1] + 1) * .5f}};
}

float environment_probe_coverage(const EnvironmentProbe& probe, Float3 point) {
    point = probe.frame.to_local(point);
    float distance = .75f;
    for (unsigned a = 0; a < 3; ++a) {
        distance = std::min({distance, point[a] - probe.minimum[a], probe.maximum[a] - point[a]});
    }
    const float t = std::clamp(distance / .75f, 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

Float3 environment_box_direction(const EnvironmentProbe& probe, Float3 p, Float3 d) {
    const auto original = d;
    p = probe.frame.to_local(p);
    d = probe.frame.to_local(d);
    float distance = std::numeric_limits<float>::infinity();

    for (unsigned a = 0; a < 3; ++a) {
        if (p[a] < probe.projection_minimum[a] || p[a] > probe.projection_maximum[a]) {
            return original;
        }

        if (std::abs(d[a]) > 1e-8f) {
            distance = std::min(
                distance,
                ((d[a] > 0 ? probe.projection_maximum[a] : probe.projection_minimum[a]) - p[a])
                    / d[a]
            );
        }
    }

    if (!std::isfinite(distance)) {
        return original;
    }

    p = probe.frame.to_world(p);
    d = probe.frame.to_world(d);
    return {
        p[0] + d[0] * distance - probe.position[0],
        p[1] + d[1] * distance - probe.position[1],
        p[2] + d[2] * distance - probe.position[2]
    };
}

std::uint32_t pack_environment_colour(Float3 c) {
    for (auto& v : c) {
        if (!std::isfinite(v) || v < 0) {
            throw std::runtime_error("invalid reflection radiance");
        }

        v = std::min(v, 64000.f);  // Finite range shared by all three unsigned float channels.
    }

    return float_encoding::pack_small_float(c[0], 6, false)
           | (float_encoding::pack_small_float(c[1], 6, false) << 11)
           | (float_encoding::pack_small_float(c[2], 5, false) << 22);
}

Float3 unpack_environment_colour(std::uint32_t c) {
    return {channel(c, 6), channel(c >> 11, 6), channel(c >> 22, 5)};
}

std::uint64_t environment_gi_hash(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        const auto name = entry.path().filename().string();

        if (name == "sparse.orpg"
            || (name.starts_with("region-") && entry.path().extension() == ".orpg")) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    if (files.empty()) {
        throw std::runtime_error("reflection bake requires GI assets");
    }

    std::uint64_t hash = 14695981039346656037ull;

    for (const auto& path : files) {
        std::ifstream file(path, std::ios::binary);
        std::array<char, 8192> buffer{};

        while (file.read(buffer.data(), buffer.size()) || file.gcount()) {
            for (std::streamsize i = 0; i < file.gcount(); ++i) {
                hash =
                    (hash ^ static_cast<unsigned char>(buffer[std::size_t(i)])) * 1099511628211ull;
            }
        }

        if (!file.eof()) {
            throw std::runtime_error("cannot hash reflection GI source");
        }
    }

    return hash;
}

std::vector<std::uint8_t> encode_environment(const EnvironmentAsset& asset) {
    asset.validate();
    std::vector<std::uint8_t> out{'O', 'R', 'E', 'N', 'V', '0', '0', '6'};
    for (unsigned value :
         {6u, asset.resolution, static_cast<unsigned>(asset.probes.size()), asset.level}) {
        word(out, value);
    }
    for (auto hash : {asset.scene_hash, asset.settings_hash, asset.gi_hash}) {
        word(out, static_cast<unsigned>(hash));
        word(out, static_cast<unsigned>(hash >> 32));
    }
    word(out, static_cast<unsigned>(asset.colour_format));
    for (const auto& probe : asset.probes) {
        for (float value : probe.frame.axis) {
            word(out, std::bit_cast<unsigned>(value));
        }
        for (const auto& vector :
             {probe.position,
              probe.minimum,
              probe.maximum,
              probe.projection_minimum,
              probe.projection_maximum}) {
            for (float value : vector) {
                word(out, std::bit_cast<unsigned>(value));
            }
        }
    }
    if (!asset.gpu_bytes.empty()) {
        out.insert(out.end(), asset.gpu_bytes.begin(), asset.gpu_bytes.end());
        for (auto pixel : asset.brdf) {
            word(out, pixel);
        }
    } else {
        if (asset.colour_format != ProbeTextureFormat::R11G11B10) {
            throw std::runtime_error(
                "compressed environment asset requires an encoded radiance plane"
            );
        }
        // Ordinary CPU fixtures use the same two-plane layout, without requiring an encoder.
        for (std::size_t i = 0; i < std::size_t(asset.width()) * asset.brdf_row(); ++i) {
            word(out, asset.pixels[i]);
        }
        for (unsigned y = 0; y < kEnvironmentBrdfSize; ++y) {
            for (unsigned x = 0; x < kEnvironmentBrdfSize; ++x) {
                word(out, asset.pixels[std::size_t(asset.brdf_row() + y) * asset.width() + x]);
            }
        }
    }
    return out;
}

namespace {
// Read only bounded header fields before computing atlas dimensions or allocating texture data.
EnvironmentAsset read_environment_header(std::span<const std::uint8_t> bytes, std::size_t& at) {
    if (bytes.size() < 52
        || std::string_view(reinterpret_cast<const char*>(bytes.data()), 8) != "ORENV006") {
        throw std::runtime_error("unsupported environment asset; rebake reflections");
    }
    at = 8;
    if (word(bytes, at) != 6) {
        throw std::runtime_error("unsupported environment asset version");
    }
    EnvironmentAsset asset;
    asset.resolution = word(bytes, at);
    const auto count = word(bytes, at);
    asset.level = word(bytes, at);
    for (auto* hash : {&asset.scene_hash, &asset.settings_hash, &asset.gi_hash}) {
        *hash = word(bytes, at);
        *hash |= std::uint64_t(word(bytes, at)) << 32;
    }
    asset.colour_format = static_cast<ProbeTextureFormat>(word(bytes, at));
    if (count == 0 || count > kEnvironmentProbeLimit || asset.resolution < 4
        || asset.resolution > 256 || !std::has_single_bit(asset.resolution)) {
        throw std::runtime_error("invalid environment asset bounds");
    }
    asset.probes.resize(count);
    return asset;
}

// Decode oriented influence and projection boxes independently of the texture encoding.
void read_environment_probes(
    EnvironmentAsset& asset, std::span<const std::uint8_t> bytes, std::size_t& at
) {
    for (auto& probe : asset.probes) {
        for (auto& value : probe.frame.axis) {
            value = std::bit_cast<float>(word(bytes, at));
        }
        for (auto* vector :
             {&probe.position,
              &probe.minimum,
              &probe.maximum,
              &probe.projection_minimum,
              &probe.projection_maximum}) {
            for (auto& value : *vector) {
                value = std::bit_cast<float>(word(bytes, at));
            }
        }
    }
}

// Retain physical colour blocks and the exact uncompressed material-response lookup.
void read_environment_payload(
    EnvironmentAsset& asset,
    std::span<const std::uint8_t> bytes,
    std::size_t& at,
    std::size_t radiance_bytes
) {
    asset.gpu_bytes.assign(bytes.begin() + at, bytes.begin() + at + radiance_bytes);
    at += radiance_bytes;
    asset.brdf.resize(std::size_t{kEnvironmentBrdfSize} * kEnvironmentBrdfSize);
    for (auto& pixel : asset.brdf) {
        pixel = word(bytes, at);
        if (((pixel >> 6) & 31) == 31 || ((pixel >> 17) & 31) == 31 || (pixel >> 27) == 31) {
            throw std::runtime_error("nonfinite environment BRDF lookup");
        }
    }
}

// CPU tools request an expanded image; runtime loaders keep only the encoded planes.
void expand_environment(EnvironmentAsset& asset) {
    const auto decoded =
        decode_probe_texture(asset.colour_format, asset.width(), asset.brdf_row(), asset.gpu_bytes);
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    for (std::size_t i = 0; i < decoded.size(); ++i) {
        asset.pixels[i] = pack_environment_colour(decoded[i]);
    }
    for (unsigned y = 0; y < kEnvironmentBrdfSize; ++y) {
        for (unsigned x = 0; x < kEnvironmentBrdfSize; ++x) {
            asset.pixels[std::size_t(asset.brdf_row() + y) * asset.width() + x] =
                asset.brdf[y * kEnvironmentBrdfSize + x];
        }
    }
}

// Reads the current two-plane layout and rejects old versions or malformed payloads.
EnvironmentAsset read_environment(std::span<const std::uint8_t> bytes, bool expand) {
    std::size_t at = 0;
    auto asset = read_environment_header(bytes, at);
    const auto radiance_bytes =
        probe_texture_bytes(asset.colour_format, asset.width(), asset.brdf_row());
    const auto expected = asset.probes.size() * 68 + radiance_bytes
                          + std::size_t{kEnvironmentBrdfSize} * kEnvironmentBrdfSize * 4;
    // Reject truncation and extra bytes before reading any boxes or GPU payload.
    if (bytes.size() - at != expected) {
        throw std::runtime_error("environment asset size mismatch");
    }
    read_environment_probes(asset, bytes, at);
    read_environment_payload(asset, bytes, at, radiance_bytes);
    // Runtime validation uses bounded row scratch rather than a full expanded HDR image.
    if (expand) {
        expand_environment(asset);
    } else {
        validate_probe_texture(
            asset.colour_format, asset.width(), asset.brdf_row(), asset.gpu_bytes
        );
    }
    asset.validate();
    return asset;
}

}  // namespace

EnvironmentAsset decode_environment(std::span<const std::uint8_t> bytes) {
    return read_environment(bytes, true);
}

EnvironmentAsset decode_gpu_environment(std::span<const std::uint8_t> bytes) {
    return read_environment(bytes, false);
}
}  // namespace openrac::lighting
