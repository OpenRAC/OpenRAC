// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// BC6H encoding uses Compressonator's decoder to select shared endpoints and verify indices.
#include "enhanced/lighting/probe_bc6.h"

#include <algorithm>
#include <astcenc_mathlib.h>
#include <bc6_encode_kernel.h>
#include <cmath>
#include <cmp_core.h>
#include <stdexcept>

namespace openrac::lighting::compression {
namespace {
// Reads a little-endian bit field from one GPU block.
unsigned get(const Block& b, unsigned at, unsigned n) {
    unsigned v = 0;

    for (unsigned i = 0; i < n; i++) {
        v |= ((b[(at + i) / 8] >> ((at + i) % 8)) & 1) << i;
    }

    return v;
}

// Replaces a little-endian bit field without changing its neighbours.
void put(Block& b, unsigned at, unsigned n, unsigned v) {
    for (unsigned i = 0; i < n; i++) {
        unsigned mask = 1 << ((at + i) % 8);
        b[(at + i) / 8] =
            static_cast<std::uint8_t>((b[(at + i) / 8] & ~mask) | (((v >> i) & 1) ? mask : 0));
    }
}

// The upstream decoder defines the exact half-float palette consumed by Vulkan.
std::array<Float3, 16> bc_decode(const Block& block) {
    std::uint16_t out[48];

    if (DecompressBlockBC6(block.data(), out, nullptr)) {
        throw std::runtime_error("BC6 decode");
    }

    std::array<Float3, 16> a;

    for (unsigned p = 0; p < 16; p++) {
        for (unsigned c = 0; c < 3; c++) {
            a[p][c] = sf16_to_float(out[p * 3 + c]);
        }
    }

    return a;
}

// BC6H mode 11 expands ten endpoint bits monotonically into an unsigned half-float value.
unsigned endpoint_half(unsigned quantized) {
    // The endpoint extremes have special values in the BC6H unsigned specification.
    if (quantized == 0) {
        return 0;
    }

    if (quantized == 1023) {
        return (65535u * 31u) >> 6;
    }

    return ((((quantized << 16) + 32768) >> 10) * 31) >> 6;
}

// Binary search gives the same nearest endpoint as an exhaustive ten-bit scan, including ties.
unsigned quant(float value) {
    const auto half = float_to_sf16(std::max(0.f, value));
    unsigned low = 0, high = 1023;

    while (low < high) {
        const auto middle = (low + high) / 2;

        // Keep the first code whose reconstruction reaches the requested half value.
        if (endpoint_half(middle) < half) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }

    // Earlier codes win exact ties, matching the exhaustive encoder's ordering.
    if (low
        && std::abs(static_cast<int>(endpoint_half(low - 1)) - half)
               <= std::abs(static_cast<int>(endpoint_half(low)) - half)) {
        return low - 1;
    }

    return low;
}

// A shared palette makes the same interpolation index decode identically in all four blocks.
Block probe_endpoints(const Tile& input) {
    Float3 lo = input[0], hi = input[0];

    for (auto pixel : input) {
        for (unsigned channel = 0; channel < 3; ++channel) {
            lo[channel] = std::min(lo[channel], pixel[channel]);
            hi[channel] = std::max(hi[channel], pixel[channel]);
        }
    }

    Block block{};
    put(block, 0, 5, 3);  // Single-subset BC6H mode 11, with two explicit ten-bit RGB endpoints.

    for (unsigned channel = 0; channel < 3; ++channel) {
        put(block, 5 + channel * 10, 10, quant(lo[channel]));
        put(block, 35 + channel * 10, 10, quant(hi[channel]));
    }

    return block;
}

// Gutter and interior texels choose one index before the tile is split into BC6H blocks.
std::array<unsigned, 64> probe_indices(const Tile& input, const Block& base) {
    std::array<Float3, 16> palette{};

    for (unsigned i = 0; i < palette.size(); ++i) {
        auto block = base;
        put(block,
            68,
            4,
            i);  // Texel one has a complete four-bit index; the anchor has only three.
        palette[i] = bc_decode(block)[1];
    }

    std::array<unsigned, 64> indices{};

    for (unsigned pixel = 0; pixel < indices.size(); ++pixel) {
        float best = INFINITY;

        for (unsigned i = 0; i < palette.size(); ++i) {
            const float candidate = squared_colour_error(input[gutter_source(pixel)], palette[i]);

            // Ascending palette order makes equal-error choices deterministic.
            if (candidate < best) {
                best = candidate;
                indices[pixel] = i;
            }
        }
    }

    return indices;
}

// Flip endpoints when needed to fit the BC6H anchor's missing high index bit.
Block indexed_block(Block block, const std::array<unsigned, 64>& indices, unsigned tile) {
    const unsigned origin = tile / 2 * 32 + tile % 2 * 4;
    const bool flip = indices[origin] >= 8;

    // Endpoint reversal leaves the palette unchanged when every index is reversed with it.
    if (flip) {
        const auto base = block;

        for (unsigned channel = 0; channel < 3; ++channel) {
            put(block, 5 + channel * 10, 10, get(base, 35 + channel * 10, 10));
            put(block, 35 + channel * 10, 10, get(base, 5 + channel * 10, 10));
        }
    }

    unsigned bit = 65;  // Mode and six ten-bit endpoints occupy the first 65 bits.

    for (unsigned pixel = 0; pixel < 16; ++pixel) {
        const auto index = indices[origin + pixel / 4 * 8 + pixel % 4];
        const auto count = pixel == 0 ? 3u : 4u;  // The subset anchor omits its high bit.
        put(block, bit, count, flip ? 15 - index : index);
        bit += count;
    }

    return block;
}

}  // namespace

std::array<Block, 4> encode_bc6_probe(const Tile& input) {
    const auto endpoints = probe_endpoints(input);
    const auto indices = probe_indices(input, endpoints);
    std::array<Block, 4> result{};

    for (unsigned block = 0; block < result.size(); ++block) {
        result[block] = indexed_block(endpoints, indices, block);
    }

    return result;
}

Block encode_bc6_block(std::span<const Float3, 16> pixels) {
    std::array<std::uint16_t, 48> half{};

    for (unsigned p = 0; p < 16; ++p) {
        for (unsigned c = 0; c < 3; ++c) {
            half[p * 3 + c] = float_to_sf16(pixels[p][c]);
        }
    }

    Block block{};

    BC6H_Encode options{};
    SetDefaultBC6Options(&options);
    // GGX faces are already filtered; bounded endpoint search avoids the codec's maximum-cost default.
    SetQualityBC6(&options, .2f);

    if (CompressBlockBC6(half.data(), 12, block.data(), &options)) {
        throw std::runtime_error("BC6H reflection compression failed");
    }

    return block;
}
}  // namespace openrac::lighting::compression
