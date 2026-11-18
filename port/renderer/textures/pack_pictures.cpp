// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/textures/pack_pictures.h"

#include <algorithm>
#include <array>
#include <basisu_transcoder.h>
#include <cmath>
#include <cstring>
#include <format>
#include <fstream>
#include <memory>
#include <mutex>
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <vector>
#include <zstd.h>

#include "common/log.h"

namespace openrac::renderer::pack_pictures {
namespace {

namespace fs = std::filesystem;

std::uint64_t le(std::span<const std::uint8_t> bytes, std::size_t at, int size) {
    std::uint64_t value = 0;
    for (int i = 0; i < size; ++i) {
        value |= std::uint64_t{bytes[at + static_cast<std::size_t>(i)]} << (8 * i);
    }
    return value;
}

// Whether a picture of width x height may be read.
bool size_ok(std::uint64_t width, std::uint64_t height) {
    return width > 0 && height > 0 && width <= kMaxSide && height <= kMaxSide
           && width * height <= kMaxTexels;
}

// A whole file, or nothing when it cannot be read or is larger than `cap`.
std::optional<std::vector<std::uint8_t>> read_file(
    const fs::path& file, std::uintmax_t cap, std::string& why
) {
    std::error_code e;
    const std::uintmax_t size = fs::file_size(file, e);
    if (e) {
        why = e.message();
        return std::nullopt;
    }
    if (size > cap) {
        why = std::format("it is larger than {} MB", cap >> 20);
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    std::ifstream in(file, std::ios::binary);
    if (!in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size))) {
        why = "it cannot be read";
        return std::nullopt;
    }
    return bytes;
}

// KTX2's file identifier and the parts of its header read here (the KTX 2.0 specification).
constexpr std::array<std::uint8_t, 12> kKtx2Identifier{
    0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, '\r', '\n', 0x1A, '\n'
};
constexpr std::size_t kKtx2HeaderBytes = 80;
constexpr std::size_t kKtx2LevelBytes = 24;
constexpr std::uint64_t kVkFormatUndefined = 0;  // Basis Universal: the format is in the data
constexpr std::uint64_t kVkFormatRgba8Unorm = 37;
constexpr std::uint64_t kVkFormatRgba8Srgb = 43;
constexpr std::uint64_t kSupercompressionNone = 0;
constexpr std::uint64_t kSupercompressionZstd = 2;

// The levels of a Basis Universal KTX2 (UASTC or ETC1S) in block format `format`, every level the
// file has; null when any level cannot be transcoded.
std::shared_ptr<const CompressedImage> transcode_blocks(
    basist::ktx2_transcoder& transcoder, BlockFormat format
) {
    const auto target = format == BlockFormat::Bc7
                            ? basist::transcoder_texture_format::cTFBC7_RGBA
                            : basist::transcoder_texture_format::cTFASTC_LDR_4x4_RGBA;
    auto blocks = std::make_shared<CompressedImage>();
    blocks->format = format;
    blocks->width = static_cast<int>(transcoder.get_width());
    blocks->height = static_cast<int>(transcoder.get_height());
    for (std::uint32_t level = 0; level < transcoder.get_levels(); ++level) {
        basist::ktx2_image_level_info info{};
        // Each level is half the one before, as KTX2 lays a chain out; one that is not ends it.
        const int width = std::max(blocks->width >> level, 1);
        const int height = std::max(blocks->height >> level, 1);
        if (!transcoder.get_image_level_info(info, level, 0, 0)
            || info.m_orig_width != static_cast<std::uint32_t>(width)
            || info.m_orig_height != static_cast<std::uint32_t>(height)) {
            return nullptr;
        }
        std::vector<std::uint8_t> bytes(block_bytes(width, height));
        const auto count = static_cast<std::uint32_t>(bytes.size() / 16);
        if (!transcoder.transcode_image_level(level, 0, 0, bytes.data(), count, target)) {
            return nullptr;
        }
        blocks->levels.push_back(std::move(bytes));
    }
    return blocks->levels.empty() ? nullptr : blocks;
}

// A Basis Universal KTX2 (UASTC or ETC1S) turned into RGBA8 by its transcoder, and into `blocks`
// as well when a block format is asked and the picture can be one (UASTC or ETC1S, its sides
// multiples of 4).
std::optional<Picture> transcode_basis(
    std::span<const std::uint8_t> bytes,
    std::uint32_t width,
    std::uint32_t height,
    BlockFormat blocks,
    std::string& why
) {
    static std::once_flag tables;
    std::call_once(tables, [] { basist::basisu_transcoder_init(); });
    basist::ktx2_transcoder transcoder;
    if (!transcoder.init(bytes.data(), static_cast<std::uint32_t>(bytes.size()))) {
        why = "it is not a Basis Universal KTX2 file the port can read";
        return std::nullopt;
    }
    // The game's textures are 8-bit colours: an HDR picture has no place in them.
    if (transcoder.is_hdr()) {
        why = "it holds HDR colours; use UASTC or ETC1S (LDR)";
        return std::nullopt;
    }
    if (!transcoder.start_transcoding()) {
        why = "its Basis Universal data cannot be read";
        return std::nullopt;
    }
    Picture picture;
    picture.image = Rgba8Image(static_cast<int>(width), static_cast<int>(height));
    if (!transcoder.transcode_image_level(
            0,
            0,
            0,
            picture.image.pixels.data(),
            width * height,
            basist::transcoder_texture_format::cTFRGBA32
        )) {
        why = "its Basis Universal data cannot be transcoded";
        return std::nullopt;
    }
    // Whole blocks only: the GPU formats are 4x4, and a game texture is a power of two.
    constexpr std::uint32_t kBlockSide = 4;
    if (blocks != BlockFormat::None && (transcoder.is_uastc() || transcoder.is_etc1s())
        && width % kBlockSide == 0 && height % kBlockSide == 0) {
        picture.blocks = transcode_blocks(transcoder, blocks);
    }
    return picture;
}

}  // namespace

std::optional<Picture> decode_ktx2(
    std::span<const std::uint8_t> bytes, BlockFormat blocks, std::string& why
) {
    if (bytes.size() < kKtx2HeaderBytes + kKtx2LevelBytes
        || !std::equal(kKtx2Identifier.begin(), kKtx2Identifier.end(), bytes.begin())) {
        why = "it is not a KTX2 file";
        return std::nullopt;
    }
    const std::uint64_t format = le(bytes, 12, 4);
    const std::uint64_t width = le(bytes, 20, 4);
    const std::uint64_t height = le(bytes, 24, 4);
    const std::uint64_t depth = le(bytes, 28, 4);
    const std::uint64_t layers = le(bytes, 32, 4);
    const std::uint64_t faces = le(bytes, 36, 4);
    const std::uint64_t scheme = le(bytes, 44, 4);
    // One flat picture: a cube map, an array or a volume replaces no texture of the game.
    if (depth > 1 || layers > 1 || faces != 1) {
        why = "it is not one flat picture (a cube map, an array or a volume)";
        return std::nullopt;
    }
    if (!size_ok(width, height)) {
        why = std::format(
            "it is {}x{}; a picture may be up to {} texels a side and {} in all",
            width,
            height,
            kMaxSide,
            kMaxTexels
        );
        return std::nullopt;
    }
    const auto w = static_cast<std::uint32_t>(width);
    const auto h = static_cast<std::uint32_t>(height);
    if (format == kVkFormatUndefined) {
        return transcode_basis(bytes, w, h, blocks, why);
    }
    // Formats only some GPUs read (BC1-BC7 on desktops, ASTC and ETC2 on phones) would make a
    // pack that works on one device and not another.
    if (format != kVkFormatRgba8Unorm && format != kVkFormatRgba8Srgb) {
        why = std::format(
            "its format (Vulkan format {}) is read by some GPUs only; use Basis Universal (UASTC "
            "or ETC1S) or RGBA8",
            format
        );
        return std::nullopt;
    }
    if (scheme != kSupercompressionNone && scheme != kSupercompressionZstd) {
        why = std::format("its supercompression ({}) is not Zstandard or none", scheme);
        return std::nullopt;
    }
    // The first level index entry is the full-size picture.
    const std::uint64_t offset = le(bytes, kKtx2HeaderBytes, 8);
    const std::uint64_t length = le(bytes, kKtx2HeaderBytes + 8, 8);
    const std::uint64_t texel_bytes = width * height * 4;
    if (offset > bytes.size() || length > bytes.size() - offset) {
        why = "it is cut short";
        return std::nullopt;
    }
    const auto data =
        bytes.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(length));
    Rgba8Image image(static_cast<int>(w), static_cast<int>(h));
    if (scheme == kSupercompressionZstd) {
        const std::size_t made =
            ZSTD_decompress(image.pixels.data(), image.pixels.size(), data.data(), data.size());
        if (ZSTD_isError(made) || made != texel_bytes) {
            why = "its Zstandard data cannot be read";
            return std::nullopt;
        }
    } else {
        if (data.size() < texel_bytes) {
            why = "it is cut short";
            return std::nullopt;
        }
        std::memcpy(image.pixels.data(), data.data(), static_cast<std::size_t>(texel_bytes));
    }
    // Plain texels are uploaded as they are: no blocks.
    return Picture{std::move(image), nullptr};
}

std::pair<int, int> svg_size(int width, int height) {
    const int longest = std::max({width, height, 1});
    const double scale = std::max(
        1.0, std::min(static_cast<double>(kSvgScale), static_cast<double>(kMaxSvgSide) / longest)
    );
    return {
        std::max(1, static_cast<int>(std::lround(width * scale))),
        std::max(1, static_cast<int>(std::lround(height * scale)))
    };
}

std::optional<Rgba8Image> rasterize_svg(std::string text, int width, int height, std::string& why) {
    if (!size_ok(
            static_cast<std::uint64_t>(std::max(width, 0)),
            static_cast<std::uint64_t>(std::max(height, 0))
        )) {
        why = std::format("{}x{} is too large to draw", width, height);
        return std::nullopt;
    }
    // nanosvg parses in place, up to a terminating zero.
    text.push_back('\0');
    const std::unique_ptr<NSVGimage, decltype(&nsvgDelete)> svg(
        nsvgParse(text.data(), "px", 96.0f), nsvgDelete
    );
    if (svg == nullptr || !(svg->width > 0.0f) || !(svg->height > 0.0f)) {
        why = "it is not an SVG with a size (width and height, or a viewBox)";
        return std::nullopt;
    }
    if (svg->shapes == nullptr) {
        why = "it draws nothing";
        return std::nullopt;
    }
    const std::unique_ptr<NSVGrasterizer, decltype(&nsvgDeleteRasterizer)> raster(
        nsvgCreateRasterizer(), nsvgDeleteRasterizer
    );
    // nanosvg draws at one scale for both axes. The picture is drawn at the scale that covers the
    // texture's size, then stretched to its shape (the game samples the whole picture over its
    // area); a picture of the texture's own shape needs no stretch.
    const double scale = std::max(
        width / static_cast<double>(svg->width), height / static_cast<double>(svg->height)
    );
    const int drawn_w = std::max(1, static_cast<int>(std::lround(svg->width * scale)));
    const int drawn_h = std::max(1, static_cast<int>(std::lround(svg->height * scale)));
    if (!size_ok(static_cast<std::uint64_t>(drawn_w), static_cast<std::uint64_t>(drawn_h))) {
        why = std::format("its shape is too far from the texture's {}x{}", width, height);
        return std::nullopt;
    }
    Rgba8Image drawn(drawn_w, drawn_h);
    nsvgRasterize(
        raster.get(),
        svg.get(),
        0.0f,
        0.0f,
        static_cast<float>(scale),
        drawn.pixels.data(),
        drawn_w,
        drawn_h,
        drawn_w * 4
    );
    if (drawn_w == width && drawn_h == height) {
        return drawn;
    }
    // Bilinear, texel centres to texel centres.
    Rgba8Image image(width, height);
    const double step_x = static_cast<double>(drawn_w) / width;
    const double step_y = static_cast<double>(drawn_h) / height;
    for (int y = 0; y < height; ++y) {
        const double fy = std::clamp((y + 0.5) * step_y - 0.5, 0.0, drawn_h - 1.0);
        const int y0 = static_cast<int>(fy);
        const int y1 = std::min(y0 + 1, drawn_h - 1);
        const double wy = fy - y0;
        for (int x = 0; x < width; ++x) {
            const double fx = std::clamp((x + 0.5) * step_x - 0.5, 0.0, drawn_w - 1.0);
            const int x0 = static_cast<int>(fx);
            const int x1 = std::min(x0 + 1, drawn_w - 1);
            const double wx = fx - x0;
            const auto texel = [&](int tx, int ty, int c) {
                return static_cast<double>(
                    drawn.pixels[(static_cast<std::size_t>(ty) * drawn_w + tx) * 4 + c]
                );
            };
            for (int c = 0; c < 4; ++c) {
                const double top = texel(x0, y0, c) * (1 - wx) + texel(x1, y0, c) * wx;
                const double bottom = texel(x0, y1, c) * (1 - wx) + texel(x1, y1, c) * wx;
                image.pixels[(static_cast<std::size_t>(y) * width + x) * 4 + c] =
                    static_cast<std::uint8_t>(std::lround(top * (1 - wy) + bottom * wy));
            }
        }
    }
    return image;
}

std::optional<Picture> load_ktx2(const fs::path& file, BlockFormat blocks) {
    std::string why;
    std::optional<Picture> picture;
    if (const auto bytes = read_file(file, kMaxKtx2Bytes, why)) {
        picture = decode_ktx2(*bytes, blocks, why);
    }
    if (!picture) {
        log::warn("texture pack: {}: {}", file.string(), why);
    }
    return picture;
}

std::optional<Rgba8Image> load_svg(const fs::path& file, int width, int height) {
    std::string why;
    std::optional<Rgba8Image> image;
    if (const auto bytes = read_file(file, kMaxSvgBytes, why)) {
        const auto [w, h] = svg_size(width, height);
        image = rasterize_svg(std::string(bytes->begin(), bytes->end()), w, h, why);
    }
    if (!image) {
        log::warn("texture pack: {}: {}", file.string(), why);
    }
    return image;
}

}  // namespace openrac::renderer::pack_pictures
