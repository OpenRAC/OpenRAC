// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Local mean/spread validation and CPU expansion for RG16 and legacy depth assets.
#include "enhanced/lighting/probe_depth.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

#include "enhanced/lighting/probe_compression_tile.h"

namespace openrac::lighting {
namespace {
// Addresses canonical eight-texel tiles in the file's fixed-width atlas.
std::size_t depth_pixel(std::size_t probe, unsigned local) {
    constexpr unsigned kSide = 8;
    return (probe / kProbeAtlasTilesX * kSide + local / kSide) * kProbeAtlasTilesX * kSide
           + probe % kProbeAtlasTilesX * kSide + local % kSide;
}

// Restores linear normalized scalars before visibility interpolation or CPU moment reconstruction.
float uncurve_depth(float value, unsigned channel, ProbeDepthFormat format) {
    // Published RG8 assets retain linear normalized scalars.
    if (format != ProbeDepthFormat::Rg16) {
        return value;
    }

    // Squaring exactly inverts the spread curve apart from native UNORM quantization.
    if (channel == 1) {
        return value * value;
    }

    // Symmetric quadratic segments invert each half of the endpoint-preserving mean curve.
    return value <= .5f ? 2.f * value * value : 1.f - 2.f * (1.f - value) * (1.f - value);
}

// Exact gutter equality lets two hardware gathers preserve the CPU sampler's seam wrapping.
void validate_unorm_gutters(const ProbeVolume& volume) {
    const auto bytes = volume.gpu_plane(1);
    constexpr unsigned kTilePixels = 64;
    const unsigned pixel_bytes = volume.depth_format == ProbeDepthFormat::Rg16 ? 4 : 2;
    for (std::size_t probe = 0; probe < volume.atlas_capacity(); ++probe) {
        for (unsigned local = 0; local < kTilePixels; ++local) {
            const auto interior = compression::gutter_source(local);
            // Interior values already are their own source; every byte pattern is finite.
            if (interior == local) {
                continue;
            }
            const auto target = depth_pixel(probe, local) * pixel_bytes;
            const auto source = depth_pixel(probe, interior) * pixel_bytes;
            if (!std::equal(
                    bytes.begin() + target,
                    bytes.begin() + target + pixel_bytes,
                    bytes.begin() + source
                )) {
                throw std::runtime_error("UNORM probe depth gutter does not match its interior");
            }
        }
    }
}
}  // namespace

std::array<float, 4> probe_depth_parameters(const ProbeVolume& volume, std::size_t probe) {
    const auto bytes = volume.gpu_plane(4);
    // A range record contains four little-endian FP32 values per atlas tile.
    constexpr unsigned kRecordBytes = 16;
    if (probe >= volume.atlas_capacity() || bytes.size() / kRecordBytes <= probe) {
        throw std::runtime_error("missing probe depth ranges");
    }
    std::array<float, 4> result{};
    for (unsigned c = 0; c < result.size(); ++c) {
        std::uint32_t word = 0;
        for (unsigned byte = 0; byte < sizeof(float); ++byte) {
            word |= std::uint32_t(bytes[probe * kRecordBytes + c * sizeof(float) + byte])
                    << (byte * 8);
        }
        result[c] = std::bit_cast<float>(word);
    }
    return result;
}

void validate_probe_depth(const ProbeVolume& volume) {
    const unsigned side = volume.grid.map_resolution + 2;
    const auto height = static_cast<unsigned>(volume.atlas_rows()) * side;
    // Gather filtering requires exact octahedral gutter copies in native UNORM maps.
    if (volume.depth_format == ProbeDepthFormat::Rg8
        || volume.depth_format == ProbeDepthFormat::Rg16) {
        validate_unorm_gutters(volume);
    }
    // Native UNORM codes are finite; legacy block-compressed fields still need a codec scan.
    if (volume.depth_format == ProbeDepthFormat::Astc4x4) {
        for (unsigned plane : {1u, 3u}) {
            validate_probe_texture(
                ProbeTextureFormat::Astc4x4,
                kProbeAtlasTilesX * side,
                height,
                volume.gpu_plane(plane)
            );
        }
    }
    for (std::size_t p = 0; p < volume.atlas_capacity(); ++p) {
        const auto values = probe_depth_parameters(volume, p);
        for (float value : values) {
            // Ranges participate in squares, so nonfinite values cannot be tolerated.
            if (!std::isfinite(value) || value < 0 || value > volume.grid.max_distance) {
                throw std::runtime_error("invalid probe depth range");
            }
        }
        // A normalized mean must remain inside the ray's finite distance cap.
        if (values[0] + values[1] > volume.grid.max_distance * 1.00001f
            || values[3] != (p < volume.grid.probe_count() ? volume.grid.max_distance : 0)) {
            throw std::runtime_error("invalid probe depth parameters");
        }
    }
}

std::vector<Float2> decode_probe_depth(const ProbeVolume& volume) {
    // Native UNORM decodes without a block codec or an expanded RGBA scratch image.
    if (volume.depth_format == ProbeDepthFormat::Rg8
        || volume.depth_format == ProbeDepthFormat::Rg16) {
        const auto bytes = volume.gpu_plane(1);
        // Native UNORM components are little-endian in both desktop and Android bake files.
        const unsigned component_bytes = volume.depth_format == ProbeDepthFormat::Rg16 ? 2 : 1;
        constexpr unsigned kChannels = 2;
        const float maximum = component_bytes == 2 ? 65'535.f : 255.f;
        std::vector<Float2> result(bytes.size() / (kChannels * component_bytes));

        for (std::size_t pixel = 0; pixel < result.size(); ++pixel) {
            for (unsigned channel = 0; channel < kChannels; ++channel) {
                std::uint32_t code = 0;
                const std::size_t at = (pixel * kChannels + channel) * component_bytes;

                for (unsigned byte = 0; byte < component_bytes; ++byte) {
                    code |= std::uint32_t(bytes[at + byte]) << (byte * 8);
                }

                result[pixel][channel] =
                    uncurve_depth(static_cast<float>(code) / maximum, channel, volume.depth_format);
            }
        }
        return result;
    }
    constexpr unsigned kSide = 8;
    const auto height = static_cast<unsigned>(volume.atlas_rows()) * kSide;
    const auto mean = decode_probe_texture(
        ProbeTextureFormat::Astc4x4, kProbeAtlasTilesX * kSide, height, volume.gpu_plane(1)
    );
    const auto spread = decode_probe_texture(
        ProbeTextureFormat::Astc4x4, kProbeAtlasTilesX * kSide, height, volume.gpu_plane(3)
    );
    std::vector<Float2> result(mean.size());
    for (std::size_t p = 0; p < result.size(); ++p) {
        result[p] = {mean[p][0], spread[p][0]};
    }
    return result;
}

std::vector<Float2> decode_probe_depth_moment_atlas(const ProbeVolume& volume) {
    const auto codes = decode_probe_depth(volume);
    std::vector<Float2> result(codes.size());
    constexpr unsigned kTileSide = 8, kTilePixels = kTileSide * kTileSide;
    for (std::size_t probe = 0; probe < volume.atlas_capacity(); ++probe) {
        const auto ranges = probe_depth_parameters(volume, probe);
        for (unsigned p = 0; p < kTilePixels; ++p) {
            const auto interior = compression::gutter_source(p);
            const auto source = depth_pixel(probe, interior);
            const auto target = depth_pixel(probe, p);
            result[target] = probe_depth_moments(codes[source], ranges);
        }
    }
    return result;
}

Float2 probe_depth_moments(Float2 encoded, const std::array<float, 4>& parameters) {
    const float mean = parameters[0] + parameters[1] * encoded[0];
    const float spread = parameters[2] * encoded[1];
    // A bounded ray distance has variance at most mean * (cap - mean).
    const float variance = std::min(spread * spread, std::max(0.f, mean * (parameters[3] - mean)));
    return {mean, mean * mean + variance};
}
}  // namespace openrac::lighting
