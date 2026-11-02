// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Local mean/spread packing with native RG16 precision and legacy depth encoders.
#include "enhanced/lighting/probe_depth_encode.h"

#include <algorithm>
#include <astcenc.h>
#include <atomic>
#include <bit>
#include <cfenv>
#include <cmath>
#include <memory>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/lighting/probe_compression_tile.h"

namespace openrac::lighting {
namespace {
// A worker restores its original rounding even when codec allocation or encoding fails.
class DepthRounding {
public:
    // Arm's codec configuration requires round-to-nearest, independent of game arithmetic.
    DepthRounding() {
        if (std::fegetenv(&m_previous) || std::fesetround(FE_TONEAREST)) {
            throw std::runtime_error("cannot set probe depth encoder rounding");
        }
    }

    // Restores exception flags as well as rounding after the codec has been released.
    ~DepthRounding() { std::fesetenv(&m_previous); }

    DepthRounding(const DepthRounding&) = delete;
    DepthRounding& operator=(const DepthRounding&) = delete;

private:
    std::fenv_t m_previous{};  // Calling worker's floating-point environment.
};

// Keeps a single mutable scalar codec private to the executing worker.
class DepthEncoder {
public:
    // Thorough search matches the quality experiment; replicated RGB uses one scalar weight field.
    DepthEncoder() {
        astcenc_config config{};
        constexpr unsigned kBlockSide = 4;
        if (astcenc_config_init(
                ASTCENC_PRF_LDR, kBlockSide, kBlockSide, 1, ASTCENC_PRE_THOROUGH, 0, &config
            )
            != ASTCENC_SUCCESS) {
            throw std::runtime_error("cannot configure ASTC depth encoder");
        }
        config.cw_a_weight = .001f;  // Opaque alpha carries no depth information.
        config.tune_2plane_early_out_limit_correlation = 1.f;
        astcenc_context* allocated = nullptr;
        const auto status = astcenc_context_alloc(&config, 1, &allocated, nullptr);
        m_context.reset(allocated);
        if (status != ASTCENC_SUCCESS) {
            throw std::runtime_error("cannot allocate ASTC depth encoder");
        }
    }

    // Encodes four blocks in an eight-texel tile; manual sampling never reads its stored gutters.
    std::array<std::uint8_t, 64> encode(const std::array<float, 64>& values) {
        std::array<std::array<float, 4>, 64> pixels{};
        for (unsigned p = 0; p < pixels.size(); ++p) {
            const auto x = std::clamp(p % 8, 1u, 6u);
            const auto y = std::clamp(p / 8, 1u, 6u);
            const float value = values[y * 8 + x];
            pixels[p] = {value, value, value, 1};
        }
        void* slices[]{pixels.data()};
        astcenc_image image{8, 8, 1, ASTCENC_TYPE_F32, slices};
        const astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
        std::array<std::uint8_t, 64> bytes{};
        astcenc_compress_reset(m_context.get());
        if (astcenc_compress_image(m_context.get(), &image, &swizzle, bytes.data(), bytes.size(), 0)
            != ASTCENC_SUCCESS) {
            throw std::runtime_error("cannot encode ASTC probe depth tile");
        }
        return bytes;
    }

private:
    DepthRounding m_rounding;  // Must outlive the codec context's own destruction.
    std::unique_ptr<astcenc_context, decltype(&astcenc_context_free)> m_context{
        nullptr, astcenc_context_free
    };  // Codec scratch is never shared between workers.
};

// Serializes range words explicitly to keep Android and desktop payloads identical.
void write_range(std::span<std::uint8_t> bytes, std::size_t offset, float value) {
    const auto word = std::bit_cast<std::uint32_t>(value);
    for (unsigned byte = 0; byte < sizeof(float); ++byte) {
        bytes[offset + byte] = static_cast<std::uint8_t>(word >> (byte * 8));
    }
}

// Fits ranges to interior texels; native UNORM gutters copy their matching interior codes.
std::array<float, 3> fit_tile(
    const ProbeVolume& volume, std::size_t probe, std::array<std::array<float, 64>, 2>& encoded
) {
    float low = volume.grid.max_distance, high = 0, spread = 0;
    const auto tile = std::span(volume.distance_moments).subspan(probe * 64, 64);
    for (unsigned y = 1; y <= 6; ++y) {
        for (unsigned x = 1; x <= 6; ++x) {
            const auto value = tile[y * 8 + x];
            low = std::min(low, value[0]);
            high = std::max(high, value[0]);
            spread = std::max(spread, std::sqrt(std::max(0.f, value[1] - value[0] * value[0])));
        }
    }
    float range = high - low;
    // Rounding the subtraction and addition separately must not move the endpoint past the ray cap.
    if (low + range > volume.grid.max_distance) {
        range = std::nextafter(range, 0.f);
    }
    for (unsigned p = 0; p < tile.size(); ++p) {
        // Constant fields use zero codes and retain their exact FP32 range record.
        encoded[0][p] = range > 0 ? std::clamp((tile[p][0] - low) / range, 0.f, 1.f) : 0;
        encoded[1][p] =
            spread > 0 ? std::sqrt(std::max(0.f, tile[p][1] - tile[p][0] * tile[p][0])) / spread
                       : 0;
    }
    return {low, range, spread};
}

// Places complete ASTC block rows without touching another worker's probe tile.
void write_tile(
    std::span<std::uint8_t> output, std::size_t probe, const std::array<std::uint8_t, 64>& bytes
) {
    // An eight-texel tile has two rows of two 16-byte blocks.
    constexpr unsigned kBlockRows = 2, kRowBytes = 32;
    for (unsigned row = 0; row < kBlockRows; ++row) {
        const auto at = ((probe / kProbeAtlasTilesX * kBlockRows + row) * kProbeAtlasTilesX
                         + probe % kProbeAtlasTilesX)
                        * kRowBytes;
        std::copy_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(row) * kRowBytes,
            kRowBytes,
            output.begin() + at
        );
    }
}

// Allocates RG16 precision to depth endpoints and small spreads before native UNORM quantization.
float curve_depth(float value, unsigned channel, ProbeDepthFormat format) {
    // Published RG8 assets retain their original linear encoding.
    if (format != ProbeDepthFormat::Rg16) {
        return value;
    }

    // Spread is nonnegative; a square-root curve reserves more codes for nearly certain blockers.
    if (channel == 1) {
        return std::sqrt(value);
    }

    // Symmetric mean curves preserve precision near both the minimum and maximum blocker depth.
    return value <= .5f ? std::sqrt(.5f * value) : 1.f - std::sqrt(.5f * (1.f - value));
}

// Writes native UNORM components with exact gutters and explicit little-endian byte order.
void encode_unorm_tile(
    const std::array<std::array<float, 64>, 2>& values,
    std::size_t probe,
    std::span<std::uint8_t> output,
    ProbeDepthFormat format
) {
    // Both layouts store two channels across the canonical eight-texel tile.
    constexpr unsigned kSide = 8;
    constexpr unsigned kChannels = 2;
    const unsigned component_bytes = format == ProbeDepthFormat::Rg16 ? 2 : 1;
    const unsigned maximum = component_bytes == 2 ? 65'535 : 255;

    // Each worker owns the entire tile, including each seam's identical interior copy.
    for (unsigned local = 0; local < kSide * kSide; ++local) {
        const std::size_t pixel =
            (probe / kProbeAtlasTilesX * kSide + local / kSide) * kProbeAtlasTilesX * kSide
            + probe % kProbeAtlasTilesX * kSide + local % kSide;
        const unsigned interior = compression::gutter_source(local);

        for (unsigned channel = 0; channel < kChannels; ++channel) {
            const float normalized = std::clamp(values[channel][interior], 0.f, 1.f);
            const float curved = curve_depth(normalized, channel, format);
            const std::uint32_t code =
                static_cast<std::uint32_t>(std::lround(curved * static_cast<float>(maximum)));
            const std::size_t target = (pixel * kChannels + channel) * component_bytes;

            for (unsigned byte = 0; byte < component_bytes; ++byte) {
                output[target + byte] = static_cast<std::uint8_t>(code >> (byte * 8));
            }
        }
    }
}

// Writes one independently owned UNORM or legacy block-compressed depth tile.
void encode_tile(
    const ProbeVolume& volume,
    std::size_t probe,
    EncodedProbeDepth& result,
    DepthEncoder* encoder,
    ProbeDepthFormat format
) {
    std::array<std::array<float, 64>, 2> values{};
    const auto ranges = fit_tile(volume, probe, values);
    for (unsigned c = 0; c < ranges.size(); ++c) {
        write_range(result[2], probe * 16 + static_cast<std::size_t>(c) * sizeof(float), ranges[c]);
    }
    write_range(result[2], probe * 16 + 12, volume.grid.max_distance);
    // Native channels avoid block-fitting error and upload without bit unpacking.
    if (!encoder) {
        encode_unorm_tile(values, probe, result[0], format);
        return;
    }
    for (unsigned channel = 0; channel < values.size(); ++channel) {
        const auto bytes = encoder->encode(values[channel]);
        write_tile(result[channel], probe, bytes);
    }
}

// Each worker owns codec scratch and claims disjoint tiles until the batch has drained.
void encode_depth_tiles(
    const ProbeVolume& volume,
    EncodedProbeDepth& result,
    std::atomic<std::size_t>& next,
    ProbeDepthFormat format
) {
    DepthRounding rounding;
    std::unique_ptr<DepthEncoder> encoder;
    std::array<std::uint8_t, 64> zero{};
    // Native UNORM bakes need no codec tables; legacy ASTC remains available for compatibility checks.
    if (format == ProbeDepthFormat::Astc4x4) {
        encoder = std::make_unique<DepthEncoder>();
        zero = encoder->encode({});
    }
    for (auto p = next.fetch_add(1); p < volume.atlas_capacity(); p = next.fetch_add(1)) {
        // Padding still needs valid ASTC blocks, but carries no range or live probe state.
        if (p < volume.probes.size()) {
            encode_tile(volume, p, result, encoder.get(), format);
        } else if (encoder) {
            for (unsigned plane = 0; plane < 2; ++plane) {
                write_tile(result[plane], p, zero);
            }
        }
    }
}
}  // namespace

EncodedProbeDepth encode_probe_depth(
    const ProbeVolume& volume, const BakeExecution& execution, ProbeDepthFormat format
) {
    const profiling::Zone timing("Compress probe depth");
    volume.validate();
    // Other resolutions cannot maintain the four-block canonical tile layout.
    if (volume.grid.map_resolution != 6 || !volume.gpu_bytes.empty()
        || (format != ProbeDepthFormat::Rg8 && format != ProbeDepthFormat::Rg16
            && format != ProbeDepthFormat::Astc4x4)) {
        throw std::runtime_error(
            "depth packing requires decoded six-texel probe maps and an explicit format"
        );
    }
    EncodedProbeDepth result;
    // Native precision determines the single interleaved depth plane size.
    if (format == ProbeDepthFormat::Rg8 || format == ProbeDepthFormat::Rg16) {
        result[0].resize(volume.atlas_capacity() * (format == ProbeDepthFormat::Rg16 ? 256 : 128));
    } else {
        for (unsigned plane = 0; plane < 2; ++plane) {
            result[plane].resize(volume.atlas_capacity() * 64);
        }
    }
    result[2].resize(volume.atlas_capacity() * 16);
    std::unique_ptr<BakeThreadPool> local;
    if (!execution.pool) {
        local = std::make_unique<BakeThreadPool>(execution.threads);
    }
    auto& pool = execution.pool ? *execution.pool : *local;
    const auto budget = execution.threads ? std::min(execution.threads, pool.size()) : pool.size();
    std::atomic<std::size_t> next{0};
    const auto status = pool.run(budget, [&] { encode_depth_tiles(volume, result, next, format); });
    if (!status) {
        throw std::runtime_error(status.error().message);
    }
    return result;
}
}  // namespace openrac::lighting
