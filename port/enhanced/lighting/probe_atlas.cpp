// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe atlas packing and expansion shared by the runtime and offline baker.
#include "enhanced/lighting/probe_atlas.h"

#include <bit>
#include <cmath>
#include <stdexcept>

#include "common/float_encoding.h"
#include "common/profiler.h"
#include "enhanced/lighting/probe_compression_tile.h"
#include "enhanced/lighting/probe_depth.h"

namespace openrac::lighting {
namespace {
// Encodes validity and individual relocation without changing the GPU metadata layout.
float probe_status(const Probe& probe) {
    // Positive values retain the shader's existing valid-probe test.
    if (probe.valid) {
        return 1.f + static_cast<float>(probe.relocated);
    }

    // Invalid probes retain their relocation flag while remaining below the validity threshold.
    return probe.relocated ? -1.f : 0.f;
}

float unpack(unsigned bits, unsigned mantissa_bits, bool signed_value) {
    const auto mantissa = bits & ((1u << mantissa_bits) - 1);
    const auto exponent = (bits >> mantissa_bits) & 31;
    if (exponent == 31) {
        throw std::runtime_error("nonfinite packed probe texel");
    }
    const float value = std::ldexp(
        static_cast<float>(mantissa + (exponent ? 1u << mantissa_bits : 0u)),
        (exponent ? static_cast<int>(exponent) - 15 : -14) - static_cast<int>(mantissa_bits)
    );
    return signed_value && (bits & (1u << (mantissa_bits + 5))) ? -value : value;
}

void put(std::span<std::uint8_t> bytes, std::size_t at, unsigned value, unsigned size) {
    for (unsigned i = 0; i < size; ++i) {
        bytes[at + i] = static_cast<std::uint8_t>(value >> (8 * i));
    }
}

unsigned get(std::span<const std::uint8_t> bytes, std::size_t at, unsigned size) {
    unsigned value = 0;
    for (unsigned i = 0; i < size; ++i) {
        value |= unsigned(bytes[at + i]) << (8 * i);
    }
    return value;
}

// Map from a probe's local octahedral texel to the shared row-major atlas.
std::size_t atlas_texel(std::size_t probe, unsigned x, unsigned y, unsigned side) {
    return (probe / kProbeAtlasTilesX * side + y) * kProbeAtlasTilesX * side
           + probe % kProbeAtlasTilesX * side + x;
}

// Rejects packed inputs and malformed decoded fields before allocating any output plane.
void validate_pack_input(const ProbeVolume& volume) {
    // GPU-ready payloads do not own the decoded arrays read by the offline packers.
    if (!volume.gpu_bytes.empty()) {
        throw std::runtime_error("probe plane packing requires decoded probe maps");
    }

    volume.validate();
}

// Writes raw moments only when requested; the reference avoids loading omitted depth texels.
void pack_depth_texel(std::span<std::uint8_t> bytes, std::size_t pixel, const Float2& moments) {
    // The RG16 baker encodes depth separately and needs no raw moment staging allocation.
    if (bytes.empty()) {
        return;
    }

    // Two FP32 words form the portable eight-byte raw depth texel.
    for (unsigned channel = 0; channel < moments.size(); ++channel) {
        put(bytes,
            pixel * 8 + channel * sizeof(float),
            std::bit_cast<unsigned>(moments[channel]),
            4);
    }
}

// Packs shared colour and relocation bytes, optionally including raw depth, from one validated field.
void pack_planes(
    const ProbeVolume& v,
    std::span<std::uint8_t> colour,
    std::span<std::uint8_t> depth,
    std::span<std::uint8_t> metadata
) {
    const unsigned side = v.grid.map_resolution + 2;
    // Real probes own complete guttered tiles; unused tail tiles retain their zero initialization.
    for (std::size_t p = 0; p < v.probes.size(); ++p) {
        const Probe& probe = v.probes[p];
        // Validity and individual relocation share the existing half-float metadata channel.
        const float status = probe_status(probe);
        // XYZ offsets precede the shared validity/relocation component in RGBA16F metadata.
        for (unsigned c = 0; c < 4; ++c) {
            std::uint32_t value = float_encoding::pack_small_float(
                c < 3 ? v.grid.frame.to_local(probe.offset)[c] : status, 10, true
            );
            // Half rounding must not move a relocated probe past its cell limit.
            if (c < 3 && std::abs(unpack(value, 10, true)) > v.grid.spacing[c] * 0.45f) {
                --value;
            }
            put(metadata, p * 8 + c * 2, value, 2);
        }
        // Logical map rows become the same canonical atlas rows for both output layouts.
        for (unsigned y = 0; y < side; ++y) {
            // Colour uses the existing R11G11B10F encoding regardless of the depth layout.
            for (unsigned x = 0; x < side; ++x) {
                const std::size_t source = p * side * side + y * side + x;
                const std::size_t target = atlas_texel(p, x, y, side);
                const Float3& irradiance = v.irradiance[source];
                put(colour,
                    target * 4,
                    float_encoding::pack_small_float(
                        encode_probe_irradiance(irradiance[0]), 6, false
                    )
                        | (float_encoding::pack_small_float(
                               encode_probe_irradiance(irradiance[1]), 6, false
                           )
                           << 11)
                        | (float_encoding::pack_small_float(
                               encode_probe_irradiance(irradiance[2]), 5, false
                           )
                           << 22),
                    4);
                pack_depth_texel(depth, target, v.distance_moments[source]);
            }
        }
    }
}

}  // namespace

std::size_t ProbeVolume::atlas_rows() const {
    return (grid.probe_count() + kProbeAtlasTilesX - 1) / kProbeAtlasTilesX;
}

std::size_t ProbeVolume::atlas_capacity() const {
    return atlas_rows() * kProbeAtlasTilesX;
}

// Computes every physical plane before trusting payload offsets or allocating an atlas.
std::array<std::size_t, 5> ProbeVolume::plane_sizes() const {
    // Scalar depth formats are not an irradiance storage contract.
    if (colour_format == ProbeTextureFormat::Astc4x4) {
        throw std::runtime_error("ASTC 4x4 is reserved for probe depth planes");
    }
    const auto side = grid.map_resolution + 2;
    const auto texels = atlas_capacity() * grid.texels_per_probe();
    const auto colour = probe_texture_bytes(
        colour_format, kProbeAtlasTilesX * side, static_cast<unsigned>(atlas_rows()) * side
    );
    // Scalar ASTC blocks require the canonical eight-texel probe tile.
    if (depth_format == ProbeDepthFormat::Astc4x4 && side == 8) {
        return {colour, texels, atlas_capacity() * 8, texels, atlas_capacity() * 16};
    }
    // Native UNORM channels retain exact gutters and independent FP32 decoding ranges.
    if ((depth_format == ProbeDepthFormat::Rg8 || depth_format == ProbeDepthFormat::Rg16)
        && side == 8) {
        const std::size_t depth_bytes = depth_format == ProbeDepthFormat::Rg16 ? 4 : 2;
        return {colour, texels * depth_bytes, atlas_capacity() * 8, 0, atlas_capacity() * 16};
    }
    // Unknown depth layouts cannot inherit the raw plane sizes.
    if (depth_format != ProbeDepthFormat::Moments) {
        throw std::runtime_error("unsupported probe depth layout");
    }
    return {colour, texels * 8, atlas_capacity() * 8, 0, 0};
}

std::span<const std::uint8_t> ProbeVolume::gpu_plane(unsigned plane) const {
    const auto sizes = plane_sizes();
    // Reject bad indices before examining any offset.
    if (plane >= sizes.size()) {
        throw std::runtime_error("invalid probe texture plane");
    }
    auto offset = gpu_offset;
    for (unsigned i = 0; i < plane; ++i) {
        offset += sizes[i];
    }
    // A complete plane must fit within its owned file buffer.
    if (offset > gpu_bytes.size() || sizes[plane] > gpu_bytes.size() - offset) {
        throw std::runtime_error("truncated GPU probe planes");
    }
    return std::span(gpu_bytes).subspan(offset, sizes[plane]);
}

std::vector<std::uint8_t> pack_probe_planes(const ProbeVolume& volume) {
    const profiling::Zone timing("Pack probe planes");
    validate_pack_input(volume);
    const std::size_t texels = volume.atlas_capacity() * volume.grid.texels_per_probe();
    // Raw storage contains four-byte colour, eight-byte moments and eight-byte metadata records.
    std::vector<std::uint8_t> bytes(texels * 12 + volume.atlas_capacity() * 8);
    const auto output = std::span(bytes);
    pack_planes(
        volume,
        output.first(texels * 4),
        output.subspan(texels * 4, texels * 8),
        output.subspan(texels * 12)
    );
    return bytes;
}

std::vector<std::uint8_t> pack_probe_colour_metadata(const ProbeVolume& volume) {
    const profiling::Zone timing("Pack probe colour and metadata");
    validate_pack_input(volume);
    const std::size_t colour_bytes = volume.atlas_capacity() * volume.grid.texels_per_probe() * 4;
    // Omit the raw depth plane because the bake output owns its independently encoded depth.
    std::vector<std::uint8_t> bytes(colour_bytes + volume.atlas_capacity() * 8);
    const auto output = std::span(bytes);
    pack_planes(volume, output.first(colour_bytes), {}, output.subspan(colour_bytes));
    return bytes;
}

void read_probe_planes(ProbeVolume& v, bool expand) {
    const profiling::Zone timing("Read probe planes");

    const auto e = v.gpu_plane(0), m = v.gpu_plane(1), metadata = v.gpu_plane(2);
    const unsigned width = kProbeAtlasTilesX * (v.grid.map_resolution + 2);
    const unsigned height = static_cast<unsigned>(v.atlas_rows()) * (v.grid.map_resolution + 2);
    std::vector<Float3> colour;
    // Runtime uploads keep compressed bytes; only offline sampling needs expanded colour.
    if (expand) {
        colour = decode_probe_texture(v.colour_format, width, height, e);
    } else {
        validate_probe_texture(v.colour_format, width, height, e);
    }
    std::vector<Float2> depth;
    if (v.depth_format != ProbeDepthFormat::Moments) {
        validate_probe_depth(v);
        if (expand) {
            depth = decode_probe_depth(v);
        }
    }
    v.probes.resize(v.grid.probe_count());
    if (expand) {
        v.irradiance.resize(v.grid.probe_count() * v.grid.texels_per_probe());
        v.distance_moments.resize(v.irradiance.size());
    }
    const auto side = v.grid.map_resolution + 2;
    for (std::size_t p = 0; p < v.probes.size(); ++p) {
        for (unsigned c = 0; c < 3; ++c) {
            v.probes[p].offset[c] = unpack(get(metadata, p * 8 + c * 2, 2), 10, true);
        }
        v.probes[p].offset = v.grid.frame.to_world(v.probes[p].offset);
        const float status = unpack(get(metadata, p * 8 + 6, 2), 10, true);
        // Only the four defined status combinations are accepted.
        if (status != -1 && status != 0 && status != 1 && status != 2) {
            throw std::runtime_error("invalid packed probe validity");
        }
        v.probes[p].valid = status > 0;
        v.probes[p].relocated = status == -1 || status == 2;
        std::array<float, 4> ranges{};
        if (v.depth_format != ProbeDepthFormat::Moments && expand) {
            ranges = probe_depth_parameters(v, p);
        }
        for (unsigned y = 0; y < side; ++y) {
            for (unsigned x = 0; x < side; ++x) {
                const auto source = atlas_texel(p, x, y, side);
                const auto target = p * side * side + y * side + x;
                Float2 moments{};
                if (v.depth_format == ProbeDepthFormat::Moments) {
                    moments =
                        {std::bit_cast<float>(get(m, source * 8, 4)),
                         std::bit_cast<float>(get(m, source * 8 + 4, 4))};
                } else if (expand) {
                    // Read the interior copy so independently encoded block borders cannot open seams.
                    const auto local = compression::gutter_source(y * side + x);
                    const auto at = atlas_texel(p, local % side, local / side, side);
                    moments = probe_depth_moments(depth[at], ranges);
                }
                if (!std::isfinite(moments[0]) || !std::isfinite(moments[1]) || moments[0] < 0
                    || moments[0]
                           > v.grid.max_distance
                                 * (v.depth_format == ProbeDepthFormat::Moments ? 1.f : 1.00001f)
                    || moments[1] < 0
                    || moments[1] > v.grid.max_distance * v.grid.max_distance * 1.001f) {
                    throw std::runtime_error("invalid packed probe moments");
                }
                if (expand) {
                    for (unsigned c = 0; c < 3; ++c) {
                        v.irradiance[target][c] =
                            std::pow(colour[source][c], kProbeIrradianceGamma);
                    }
                    v.distance_moments[target] = moments;
                }
            }
        }
    }
}
}  // namespace openrac::lighting
