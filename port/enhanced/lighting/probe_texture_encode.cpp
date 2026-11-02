// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline HDR image encoding and local mean/spread compression for probe depth.
#include "enhanced/lighting/probe_texture_encode.h"

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/lighting/environment.h"
#include "enhanced/lighting/probe_astc.h"
#include "enhanced/lighting/probe_atlas.h"
#include "enhanced/lighting/probe_bc6.h"
#include "enhanced/lighting/probe_depth_encode.h"

namespace openrac::lighting {
namespace {
// Replicates edge pixels when an ordinary reflection image ends inside a compression block.
compression::Tile image_tile(
    unsigned width,
    unsigned height,
    std::span<const Float3> pixels,
    unsigned origin_x,
    unsigned origin_y
) {
    compression::Tile tile{};

    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            tile[y * 8 + x] = pixels
                [std::size_t(std::min(origin_y + y, height - 1)) * width
                 + std::min(origin_x + x, width - 1)];
        }
    }

    return tile;
}

// Reflection blocks are independent; GI shares a palette across each complete bordered tile.
std::array<compression::Block, 4> bc6_tile(const compression::Tile& tile, bool probe_gutters) {
    // Paired GI gutters must not be encoded as four unrelated blocks.
    if (probe_gutters) {
        return compression::encode_bc6_probe(tile);
    }

    std::array<compression::Block, 4> blocks{};

    for (unsigned block = 0; block < blocks.size(); ++block) {
        std::array<Float3, 16> pixels{};
        const unsigned origin = block / 2 * 32 + block % 2 * 4;

        for (unsigned pixel = 0; pixel < pixels.size(); ++pixel) {
            pixels[pixel] = tile[origin + pixel / 4 * 8 + pixel % 4];
        }

        blocks[block] = compression::encode_bc6_block(pixels);
    }

    return blocks;
}

// Block rows retain GPU memory order, including ordinary reflection images with partial edge blocks.
void write_bc6_tile(
    std::span<std::uint8_t> output,
    unsigned width,
    unsigned height,
    std::array<unsigned, 2> origin,
    const std::array<compression::Block, 4>& blocks
) {
    for (unsigned by = 0; by < 2 && origin[1] + by * 4 < height; ++by) {
        for (unsigned bx = 0; bx < 2 && origin[0] + bx * 4 < width; ++bx) {
            const auto offset =
                (std::size_t(origin[1] / 4 + by) * ((width + 3) / 4) + origin[0] / 4 + bx) * 16;
            std::copy(
                blocks[by * 2 + bx].begin(), blocks[by * 2 + bx].end(), output.begin() + offset
            );
        }
    }
}

// The raw fallback uses little-endian GPU channel words without changing its precision.
void encode_raw(std::span<const Float3> pixels, std::span<std::uint8_t> output) {
    for (std::size_t pixel = 0; pixel < pixels.size(); ++pixel) {
        const auto word = pack_environment_colour(pixels[pixel]);

        for (unsigned byte = 0; byte < 4; ++byte) {
            output[pixel * 4 + byte] = static_cast<std::uint8_t>(word >> (byte * 8));
        }
    }
}

// Borrows one immutable image and disjoint output blocks until the worker batch drains.
struct TextureJob {
    ProbeTextureFormat format;       // Selects the codec independently of scheduling.
    unsigned width, height;          // Source image extent in texels.
    std::span<const Float3> pixels;  // Shared read-only source texels.
    std::span<std::uint8_t> output;  // Workers write separate physical blocks.
    bool probe_gutters;              // Keeps paired borders inside each eight-texel tile.
};

// Encodes one complete tile; its output bytes never overlap another tile's blocks.
void encode_tile(const TextureJob& job, unsigned tile_index, compression::AstcEncoder* astc) {
    const unsigned columns = (job.width + 7) / 8;
    const unsigned x = tile_index % columns * 8, y = tile_index / columns * 8;
    const auto tile = image_tile(job.width, job.height, job.pixels, x, y);

    // ASTC consumes one eight-texel block; BC6H consumes four four-texel blocks.
    if (astc) {
        const auto block = astc->encode(tile, job.probe_gutters);
        std::copy(block.begin(), block.end(), job.output.begin() + std::size_t(tile_index) * 16);
    } else {
        write_bc6_tile(
            job.output, job.width, job.height, {x, y}, bc6_tile(tile, job.probe_gutters)
        );
    }
}

// Keeps one fixed-format encoder per executing thread, released when that thread exits.
compression::AstcEncoder& worker_astc_encoder() {
    thread_local std::unique_ptr<compression::AstcEncoder> tls_encoder;

    // Codec configuration is fixed, so each worker builds its tables only once.
    if (!tls_encoder) {
        const profiling::Zone timing("Prepare ASTC encoder");
        tls_encoder = std::make_unique<compression::AstcEncoder>();
    }

    return *tls_encoder;
}

// Reuses one codec context per worker and claims bounded batches to amortize queue synchronization.
void encode_tiles(const TextureJob& job, const BakeExecution& execution) {
    constexpr unsigned kBc6TilesPerBatch = 32;  // Cheap BC6H tiles amortize atomic claims.
    // Expensive ASTC tiles vary in search cost; single-tile claims keep the whole worker pool busy.
    const unsigned tiles_per_batch =
        job.format == ProbeTextureFormat::Astc8x8 ? 1 : kBc6TilesPerBatch;
    const unsigned tiles = ((job.width + 7) / 8) * ((job.height + 7) / 8);
    std::unique_ptr<BakeThreadPool> local;

    // Standalone callers own their budget; controllers lend the existing process-wide worker pool.
    if (!execution.pool) {
        local = std::make_unique<BakeThreadPool>(execution.threads);
    }

    auto& pool = execution.pool ? *execution.pool : *local;
    const unsigned budget =
        execution.threads ? std::min(execution.threads, pool.size()) : pool.size();
    const unsigned tasks = std::min(budget, (tiles + tiles_per_batch - 1) / tiles_per_batch);
    std::atomic<unsigned> next{0};
    std::fenv_t environment;

    // Encoding must retain the controller's rounding mode regardless of preceding worker tasks.
    if (std::fegetenv(&environment) != 0) {
        throw std::runtime_error("cannot read compression floating-point environment");
    }

    // Pool workers borrow the image until run() drains every task, including failed encodes.
    const auto result = pool.run(tasks, [&] {
        const profiling::Zone timing("Compress probe tiles");

        // Reused workers must inherit the controller's mode before codec context creation.
        if (std::fesetenv(&environment) != 0) {
            throw std::runtime_error("cannot set compression floating-point environment");
        }

        compression::AstcEncoder* astc = nullptr;

        // Mutable codec search state stays worker-owned and survives successive images.
        if (job.format == ProbeTextureFormat::Astc8x8) {
            astc = &worker_astc_encoder();
        }

        for (unsigned begin = next.fetch_add(tiles_per_batch); begin < tiles;
             begin = next.fetch_add(tiles_per_batch)) {
            for (unsigned tile = begin; tile < std::min(begin + tiles_per_batch, tiles); ++tile) {
                encode_tile(job, tile, astc);
            }
        }
    });

    // Every submitted task has drained before borrowed input storage can be released.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

// Encodes GPU block rows with worker-owned contexts and deterministic physical block positions.
std::vector<std::uint8_t> encode_probe_texture(
    ProbeTextureFormat format,
    unsigned width,
    unsigned height,
    std::span<const Float3> pixels,
    const ProbeTextureEncoding& options
) {
    const profiling::Zone timing("Compress probe colour");
    // Scalar depth uses a separate encoder and independent block planes.
    if (format == ProbeTextureFormat::Astc4x4) {
        throw std::runtime_error("ASTC 4x4 requires the probe depth encoder");
    }

    // Never read image texels until their extent and requested format have been validated.
    if (pixels.size() != std::size_t(width) * height) {
        throw std::runtime_error("probe encoder image size mismatch");
    }

    std::vector<std::uint8_t> result(probe_texture_bytes(format, width, height));

    // Raw fallback retains the same GPU-ready packed channel layout.
    if (format == ProbeTextureFormat::R11G11B10) {
        encode_raw(pixels, result);
        return result;
    }

    encode_tiles({format, width, height, pixels, result, options.probe_gutters}, options.execution);

    return result;
}

// Compress irradiance and canonical depth maps while preserving exact relocation metadata.
std::vector<std::uint8_t> encode_baked_probes(
    const ProbeVolume& volume,
    ProbeTextureFormat format,
    const BakeExecution& execution,
    ProbeDepthFormat depth_format
) {
    // Packed raw-colour assets still have compressed depth; reject them before touching any plane offsets.
    if (!volume.gpu_bytes.empty() || volume.colour_format != ProbeTextureFormat::R11G11B10) {
        throw std::runtime_error("probe compression requires decoded probe maps");
    }

    // Nonstandard map sizes retain the portable raw layout rather than crossing probe boundaries.
    if (volume.grid.map_resolution != 6) {
        return encode_probes(volume);
    }

    const auto side = volume.grid.map_resolution + 2;
    const unsigned width = kProbeAtlasTilesX * side;
    const unsigned height = static_cast<unsigned>(volume.atlas_rows()) * side;
    const auto raw_size = probe_texture_bytes(ProbeTextureFormat::R11G11B10, width, height);
    const auto planes = pack_probe_colour_metadata(volume);
    std::span<const std::uint8_t> colour = std::span(planes).first(raw_size);
    const auto metadata = std::span(planes).subspan(raw_size);
    std::vector<std::uint8_t> compressed;

    // Raw colour uploads directly; other formats need only a temporary decoded colour image.
    if (format != ProbeTextureFormat::R11G11B10) {
        const auto pixels =
            decode_probe_texture(ProbeTextureFormat::R11G11B10, width, height, colour);
        compressed = encode_probe_texture(format, width, height, pixels, {true, execution});
        colour = compressed;
    }

    const auto depth = encode_probe_depth(volume, execution, depth_format);
    auto encoded = encode_probe_header(volume, format, depth_format);
    encoded.reserve(
        encoded.size() + colour.size() + metadata.size() + depth[0].size() + depth[1].size()
        + depth[2].size()
    );
    encoded.insert(encoded.end(), colour.begin(), colour.end());
    encoded.insert(encoded.end(), depth[0].begin(), depth[0].end());
    encoded.insert(encoded.end(), metadata.begin(), metadata.end());
    encoded.insert(encoded.end(), depth[1].begin(), depth[1].end());
    encoded.insert(encoded.end(), depth[2].begin(), depth[2].end());
    return encoded;
}

// Preserve prefiltered face padding and move the exact BRDF words into their separate plane.
std::vector<std::uint8_t> encode_baked_environment(
    EnvironmentAsset& asset, ProbeTextureFormat format, const BakeExecution& execution
) {
    asset.validate();

    if (asset.colour_format != format) {
        throw std::runtime_error("environment format must be selected before GGX filtering");
    }

    const auto count = std::size_t(asset.width()) * asset.brdf_row();
    std::vector<Float3> pixels(count);

    for (std::size_t i = 0; i < count; ++i) {
        pixels[i] = unpack_environment_colour(asset.pixels[i]);
    }

    asset.colour_format = format;
    asset.gpu_bytes =
        encode_probe_texture(format, asset.width(), asset.brdf_row(), pixels, {false, execution});
    asset.brdf.resize(std::size_t{kEnvironmentBrdfSize} * kEnvironmentBrdfSize);

    for (unsigned y = 0; y < kEnvironmentBrdfSize; ++y) {
        for (unsigned x = 0; x < kEnvironmentBrdfSize; ++x) {
            asset.brdf[y * kEnvironmentBrdfSize + x] =
                asset.pixels[std::size_t(asset.brdf_row() + y) * asset.width() + x];
        }
    }

    return encode_environment(asset);
}

}  // namespace openrac::lighting
