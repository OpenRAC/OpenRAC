// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR block decoding for asset verification; the runtime keeps GPU-ready payloads intact.
#include "enhanced/lighting/probe_texture.h"

#include <cfenv>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

#include "enhanced/lighting/environment.h"
#if OPENRAC_PROBE_COMPRESSION
#include <astcenc.h>
#include <astcenc_mathlib.h>
#include <cmp_core.h>
#endif

namespace openrac::lighting {
unsigned probe_texture_block(ProbeTextureFormat format) {
    // Only concrete encodings can describe an asset plane.
    switch (format) {
        case ProbeTextureFormat::R11G11B10:
            return 1;
        case ProbeTextureFormat::Bc6h:
            return 4;
        case ProbeTextureFormat::Astc8x8:
            return 8;
        default:
            throw std::runtime_error("invalid probe texture format");
    }
}

std::size_t probe_texture_bytes(ProbeTextureFormat format, unsigned width, unsigned height) {
    const unsigned side = probe_texture_block(format);
    const std::size_t columns = (std::size_t(width) + side - 1) / side;
    const std::size_t rows = (std::size_t(height) + side - 1) / side;
    const std::size_t bytes = side == 1 ? 4 : 16;

    // Asset bounds must remain finite before allocation or upload.
    if (!width || !height || rows > std::numeric_limits<std::size_t>::max() / columns / bytes) {
        throw std::runtime_error("invalid probe texture extent");
    }

    return columns * rows * bytes;
}

const char* probe_texture_name(ProbeTextureFormat format) {
    switch (format) {
        case ProbeTextureFormat::R11G11B10:
            return "R11G11B10F";
        case ProbeTextureFormat::Bc6h:
            return "BC6H unsigned HDR";
        case ProbeTextureFormat::Astc8x8:
            return "ASTC 8x8 HDR";
        case ProbeTextureFormat::Automatic:
            return "automatic";
    }

    return "invalid";
}

ProbeTextureFormat parse_probe_texture_format(std::string_view token) {
    if (token == "auto") {
        return ProbeTextureFormat::Automatic;
    }

    if (token == "none") {
        return ProbeTextureFormat::R11G11B10;
    }

    if (token == "bc6h") {
        return ProbeTextureFormat::Bc6h;
    }

    if (token == "astc") {
        return ProbeTextureFormat::Astc8x8;
    }

    throw std::runtime_error("compression must be auto, none, bc6h or astc");
}

const char* probe_texture_token(ProbeTextureFormat format) {
    switch (format) {
        case ProbeTextureFormat::Automatic:
            return "auto";
        case ProbeTextureFormat::R11G11B10:
            return "none";
        case ProbeTextureFormat::Bc6h:
            return "bc6h";
        case ProbeTextureFormat::Astc8x8:
            return "astc";
    }

    throw std::runtime_error("invalid probe texture format");
}

namespace {
#if OPENRAC_PROBE_COMPRESSION
// Clip partial edge blocks while converting the codec's RGB half words into CPU colour.
void copy_bc6_block(
    std::span<Float3> output,
    unsigned width,
    unsigned height,
    std::array<unsigned, 2> origin,
    const std::array<unsigned short, 48>& block
) {
    for (unsigned row = 0; row < 4 && origin[1] + row < height; ++row) {
        for (unsigned column = 0; column < 4 && origin[0] + column < width; ++column) {
            for (unsigned channel = 0; channel < 3; ++channel) {
                output[std::size_t(origin[1] + row) * width + origin[0] + column][channel] =
                    sf16_to_float(block[(row * 4 + column) * 3 + channel]);
            }
        }
    }
}
#endif

// Decode one image using only the caller's bounded destination scratch.
void decode_bc6(
    unsigned width, unsigned height, std::span<const std::uint8_t> bytes, std::span<Float3> output
) {
#if OPENRAC_PROBE_COMPRESSION
    const unsigned columns = (width + 3) / 4;

    // Decoding is used only for CPU sampling and verification, never for a GPU upload.
    for (unsigned y = 0; y < height; y += 4) {
        for (unsigned x = 0; x < width; x += 4) {
            const auto offset = (std::size_t(y / 4) * columns + x / 4) * 16;
            const auto mode = bytes[offset] & 31;

            // BC6H reserves four five-bit modes whose two-bit prefix is three.
            if ((mode & 3) == 3 && mode > 15) {
                throw std::runtime_error("reserved BC6H probe mode");
            }

            std::array<unsigned short, 48> block{};

            if (DecompressBlockBC6(
                    bytes.data() + (std::size_t(y / 4) * columns + x / 4) * 16,
                    block.data(),
                    nullptr
                )) {
                throw std::runtime_error("invalid BC6H probe block");
            }

            copy_bc6_block(output, width, height, {x, y}, block);
        }
    }
#else
    (void)width;
    (void)height;
    (void)bytes;
    (void)output;
    throw std::runtime_error("probe compression is unavailable in this build");
#endif
}

#if OPENRAC_PROBE_COMPRESSION
// Isolates Arm's round-to-nearest requirement from runtime-style probe verification arithmetic.
class AstcRounding {
public:
    // Saves complete caller state before codec configuration or decompression can inspect it.
    AstcRounding() {
        // Codec checks reject truncation and directed rounding even for decoder-only contexts.
        if (std::fegetenv(&m_previous) != 0 || std::fesetround(FE_TONEAREST) != 0) {
            throw std::runtime_error("cannot set ASTC decoder rounding");
        }
    }

    // Restores runtime rounding and exception flags on successful decoding and every failure path.
    ~AstcRounding() { std::fesetenv(&m_previous); }

    AstcRounding(const AstcRounding&) = delete;
    AstcRounding& operator=(const AstcRounding&) = delete;

private:
    std::fenv_t
        m_previous{};  // Restored after the decoder context and scratch storage are released.
};

// One decoder owns its codec tables while expanding bounded rows or a complete CPU image.
class AstcDecoder {
public:
    // Decoder-only contexts avoid encoder scratch and use the exact stored HDR profile.
    AstcDecoder() {
        astcenc_config config{};

        // ASTC HDR uses unsigned RGB and an opaque LDR alpha channel.
        if (astcenc_config_init(
                ASTCENC_PRF_HDR_RGB_LDR_A,
                8,
                8,
                1,
                ASTCENC_PRE_FASTEST,
                ASTCENC_FLG_DECOMPRESS_ONLY,
                &config
            )
            != ASTCENC_SUCCESS) {
            throw std::runtime_error("cannot configure ASTC probe decoder");
        }

        astcenc_context* allocated = nullptr;
        const auto status = astcenc_context_alloc(&config, 1, &allocated, nullptr);
        m_context.reset(allocated);

        // A failed allocation must release any partially returned context.
        if (status != ASTCENC_SUCCESS) {
            throw std::runtime_error("cannot allocate ASTC probe decoder");
        }
    }

    // The caller supplies row-aligned complete blocks; output may clip the final image row.
    void decode(
        unsigned width,
        unsigned height,
        std::span<const std::uint8_t> bytes,
        std::span<Float3> output
    ) {
        m_rgba.resize(std::size_t(width) * height);
        void* slices[]{m_rgba.data()};
        astcenc_image image{width, height, 1, ASTCENC_TYPE_F32, slices};
        const astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_1};

        // The upstream decoder also marks malformed physical blocks with nonfinite texels.
        if (astcenc_decompress_image(
                m_context.get(), bytes.data(), bytes.size(), &image, &swizzle, 0
            )
            != ASTCENC_SUCCESS) {
            throw std::runtime_error("invalid ASTC probe blocks");
        }

        for (std::size_t pixel = 0; pixel < output.size(); ++pixel) {
            output[pixel] = {m_rgba[pixel][0], m_rgba[pixel][1], m_rgba[pixel][2]};
        }
    }

private:
    AstcRounding m_rounding;  // Constructed before configuration and restored after all codec work.
    std::unique_ptr<astcenc_context, decltype(&astcenc_context_free)> m_context{
        nullptr, astcenc_context_free
    };
    std::vector<std::array<float, 4>>
        m_rgba;  // Reused row scratch; never retained by runtime assets.
};
#endif

// Shared finite checks keep streamed validation and full CPU expansion equally strict.
void validate_colours(std::span<const Float3> colours) {
    for (const auto& colour : colours) {
        for (float channel : colour) {
            // Nonfinite or negative irradiance cannot participate in filtering.
            if (!std::isfinite(channel) || channel < 0) {
                throw std::runtime_error("nonfinite compressed probe colour");
            }
        }
    }
}

// Raw words need no floating-point image allocation to reject nonfinite unsigned channels.
void validate_raw(std::span<const std::uint8_t> bytes) {
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4) {
        std::uint32_t pixel = 0;
        std::memcpy(&pixel, bytes.data() + offset, 4);

        // Exponent 31 denotes infinity or NaN in each unsigned packed channel.
        if (((pixel >> 6) & 31) == 31 || ((pixel >> 17) & 31) == 31 || (pixel >> 27) == 31) {
            throw std::runtime_error("nonfinite packed probe irradiance");
        }
    }
}
}  // namespace

std::vector<Float3> decode_probe_texture(
    ProbeTextureFormat format, unsigned width, unsigned height, std::span<const std::uint8_t> bytes
) {
    // Trailing or truncated blocks are rejected before calling a codec.
    if (bytes.size() != probe_texture_bytes(format, width, height)) {
        throw std::runtime_error("probe colour payload size mismatch");
    }

    std::vector<Float3> result(std::size_t(width) * height);

    if (format == ProbeTextureFormat::R11G11B10) {
        for (std::size_t i = 0; i < result.size(); ++i) {
            std::uint32_t pixel = 0;
            std::memcpy(&pixel, bytes.data() + i * 4, 4);

            // Exponent 31 denotes a nonfinite unsigned-float channel.
            if (((pixel >> 6) & 31) == 31 || ((pixel >> 17) & 31) == 31 || (pixel >> 27) == 31) {
                throw std::runtime_error("nonfinite packed probe irradiance");
            }

            result[i] = unpack_environment_colour(pixel);
        }
    } else if (format == ProbeTextureFormat::Bc6h) {
        decode_bc6(width, height, bytes, result);
    } else {
#if OPENRAC_PROBE_COMPRESSION
        AstcDecoder decoder;
        decoder.decode(width, height, bytes, result);
#else
        throw std::runtime_error("probe compression is unavailable in this build");
#endif
    }

    validate_colours(result);
    return result;
}

// Validate blocks in bounded row scratch without expanding a runtime texture or changing its bytes.
void validate_probe_texture(
    ProbeTextureFormat format, unsigned width, unsigned height, std::span<const std::uint8_t> bytes
) {
    // Reject payload mismatches before entering either codec.
    if (bytes.size() != probe_texture_bytes(format, width, height)) {
        throw std::runtime_error("probe colour payload size mismatch");
    }

    // Raw colour needs no decoder or scratch allocation.
    if (format == ProbeTextureFormat::R11G11B10) {
        validate_raw(bytes);
        return;
    }
#if OPENRAC_PROBE_COMPRESSION
    const auto block = probe_texture_block(format);
    const auto row_bytes = probe_texture_bytes(format, width, block);
    std::vector<Float3> row(std::size_t(width) * block);
    std::unique_ptr<AstcDecoder> astc;

    // Reuse ASTC tables across every row of this asset.
    if (format == ProbeTextureFormat::Astc8x8) {
        astc = std::make_unique<AstcDecoder>();
    }

    for (unsigned y = 0; y < height; y += block) {
        const auto count = std::min(block, height - y);
        const auto source = bytes.subspan(std::size_t(y / block) * row_bytes, row_bytes);
        const auto output = std::span(row).first(std::size_t(width) * count);

        // Each row starts on a physical block boundary for either codec.
        if (astc) {
            astc->decode(width, count, source, output);
        } else {
            decode_bc6(width, count, source, output);
        }

        validate_colours(output);
    }
#else
    throw std::runtime_error("probe compression is unavailable in this build");
#endif
}
}  // namespace openrac::lighting
