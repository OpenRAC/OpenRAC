// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture packs' KTX2 and SVG pictures (renderer/textures/pack_pictures.h). The KTX2 files in
// tests/data/pack_pictures were made for this test from quadrants.png (by make.py there):
// an 8x8 picture, red, green, blue and see-through white quarters.
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/textures/pack_pictures.h"

using namespace openrac;
using namespace openrac::renderer;
namespace fs = std::filesystem;

namespace {

const fs::path kData = PACK_PICTURES_DATA;

std::vector<std::uint8_t> read(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

using Rgba = std::array<int, 4>;

Rgba at(const Rgba8Image& image, int x, int y) {
    const auto i = (static_cast<std::size_t>(y) * image.width + x) * 4;
    return {image.pixels[i], image.pixels[i + 1], image.pixels[i + 2], image.pixels[i + 3]};
}

bool near(const Rgba& a, const Rgba& b, int tolerance) {
    for (int c = 0; c < 4; ++c) {
        if (std::abs(a[c] - b[c]) > tolerance) {
            return false;
        }
    }
    return true;
}

// The four quarters of an 8x8 picture, checked at a texel inside each.
bool quadrants(const Rgba8Image& image, int tolerance) {
    return image.width == 8 && image.height == 8
           && near(at(image, 1, 1), {255, 0, 0, 255}, tolerance)
           && near(at(image, 6, 1), {0, 255, 0, 255}, tolerance)
           && near(at(image, 1, 6), {0, 0, 255, 255}, tolerance) && at(image, 6, 6)[3] <= tolerance;
}

void put32(std::vector<std::uint8_t>& out, std::size_t at, std::uint64_t value, int bytes = 4) {
    for (int i = 0; i < bytes; ++i) {
        out[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(value >> (8 * i));
    }
}

// A KTX2 file of one level: `format` (a Vulkan format), width x height, `data` as its level.
std::vector<std::uint8_t> ktx2(
    std::uint32_t format,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint8_t>& data,
    std::uint32_t faces = 1
) {
    std::vector<std::uint8_t> out(104, 0);
    const std::array<std::uint8_t, 12> id{
        0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, '\r', '\n', 0x1A, '\n'
    };
    std::copy(id.begin(), id.end(), out.begin());
    put32(out, 12, format);
    put32(out, 16, 1);
    put32(out, 20, width);
    put32(out, 24, height);
    put32(out, 36, faces);
    put32(out, 40, 1);
    put32(out, 80, out.size(), 8);
    put32(out, 88, data.size(), 8);
    put32(out, 96, data.size(), 8);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

void check_ktx2() {
    std::string why;
    // Basis Universal, both kinds: UASTC is close to the source, ETC1S rougher.
    const auto uastc =
        pack_pictures::decode_ktx2(read(kData / "quadrants-uastc.ktx2"), BlockFormat::None, why);
    CHECK(uastc && quadrants(uastc->image, 8));
    const auto zstd = pack_pictures::decode_ktx2(
        read(kData / "quadrants-uastc-zstd.ktx2"), BlockFormat::None, why
    );
    CHECK(zstd && quadrants(zstd->image, 8));
    const auto etc1s =
        pack_pictures::decode_ktx2(read(kData / "quadrants-etc1s.ktx2"), BlockFormat::None, why);
    CHECK(etc1s && quadrants(etc1s->image, 40));
    // Plain RGBA8, Zstandard-compressed by a tool and stored as it is.
    const auto packed = pack_pictures::decode_ktx2(
        read(kData / "quadrants-rgba8-zstd.ktx2"), BlockFormat::None, why
    );
    CHECK(packed && quadrants(packed->image, 0));
    const std::vector<std::uint8_t> texels{1, 2, 3, 4, 5, 6, 7, 8};
    const auto plain = pack_pictures::decode_ktx2(ktx2(37, 2, 1, texels), BlockFormat::None, why);
    CHECK(plain && plain->image.width == 2 && plain->image.pixels == texels);
    // No blocks unless asked for.
    CHECK(uastc && !uastc->blocks && etc1s && !etc1s->blocks);

    // Blocks for the GPU: each level of the file, 16 bytes a 4x4 block, beside the RGBA8 picture.
    for (const BlockFormat format : {BlockFormat::Bc7, BlockFormat::Astc4x4}) {
        for (const char* name : {"quadrants-uastc.ktx2", "quadrants-etc1s.ktx2"}) {
            const auto picture = pack_pictures::decode_ktx2(read(kData / name), format, why);
            CHECK(
                picture && quadrants(picture->image, 40) && picture->blocks
                && picture->blocks->format == format && picture->blocks->width == 8
                && picture->blocks->levels.size() == 1 && picture->blocks->bytes() == 64
            );
        }
        // A chain made by basisu -mipmap: 8x8, 4x4, 2x2 and 1x1, a block each below 8x8.
        const auto mips =
            pack_pictures::decode_ktx2(read(kData / "quadrants-uastc-mips.ktx2"), format, why);
        CHECK(
            mips && quadrants(mips->image, 8) && mips->blocks && mips->blocks->levels.size() == 4
            && mips->blocks->bytes() == 64 + 3 * 16
        );
        // Plain texels are never blocks, nor a picture whose sides are not whole blocks.
        const auto texels_only = pack_pictures::decode_ktx2(ktx2(37, 2, 1, texels), format, why);
        CHECK(texels_only && !texels_only->blocks);
    }

    // Refused: formats only some GPUs read, a cube map, a short file, a size past the caps, a file
    // that is not KTX2.
    why.clear();
    CHECK(!pack_pictures::decode_ktx2(
        ktx2(145, 4, 4, std::vector<std::uint8_t>(16)), BlockFormat::None, why
    ));
    CHECK(why.find("some GPUs only") != std::string::npos);
    CHECK(!pack_pictures::decode_ktx2(ktx2(37, 2, 1, texels, 6), BlockFormat::None, why));
    CHECK(!pack_pictures::decode_ktx2(ktx2(37, 4, 4, texels), BlockFormat::None, why));
    CHECK(!pack_pictures::decode_ktx2(ktx2(37, 9000, 1, texels), BlockFormat::None, why));
    CHECK(!pack_pictures::decode_ktx2(ktx2(37, 4096, 4097, texels), BlockFormat::None, why));
    CHECK(!pack_pictures::decode_ktx2(read(kData / "quadrants.png"), BlockFormat::None, why));
    auto damaged = read(kData / "quadrants-uastc-zstd.ktx2");
    damaged.resize(damaged.size() - 8);
    CHECK(!pack_pictures::decode_ktx2(damaged, BlockFormat::None, why));
}

void check_svg() {
    // Four times the texture, the longest side at most 4096, never below the texture.
    CHECK((pack_pictures::svg_size(32, 16) == std::pair{128, 64}));
    CHECK((pack_pictures::svg_size(2048, 512) == std::pair{4096, 1024}));
    CHECK((pack_pictures::svg_size(8192, 8) == std::pair{8192, 8}));

    // Stretched to the size asked: the left half red, the right half see-through.
    const std::string half =
        R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10"><rect width="5" height="10" fill="#ff0000"/></svg>)";
    std::string why;
    const auto image = pack_pictures::rasterize_svg(half, 40, 20, why);
    CHECK(image && image->width == 40 && image->height == 20);
    CHECK(image && near(at(*image, 5, 10), {255, 0, 0, 255}, 0) && at(*image, 30, 10)[3] == 0);

    // Not an SVG, no size, nothing drawn, too large: nothing.
    CHECK(!pack_pictures::rasterize_svg("not an svg", 8, 8, why));
    CHECK(!pack_pictures::rasterize_svg(
        R"(<svg xmlns="http://www.w3.org/2000/svg"></svg>)", 8, 8, why
    ));
    CHECK(!pack_pictures::rasterize_svg(half, 9000, 8, why));
    // A link to another file draws nothing from it: nanosvg reads no images.
    const std::string linked =
        R"(<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="8" height="8"><image xlink:href="/etc/passwd" width="8" height="8"/></svg>)";
    CHECK(!pack_pictures::rasterize_svg(linked, 8, 8, why));

    // From a file, at svg_size() of the texture; a file past the cap is not read.
    const fs::path folder = fs::temp_directory_path() / "openrac_pack_pictures_test";
    fs::create_directories(folder);
    std::ofstream(folder / "half.svg") << half;
    const auto loaded = pack_pictures::load_svg(folder / "half.svg", 8, 4);
    CHECK(loaded && loaded->width == 32 && loaded->height == 16);
    std::ofstream(folder / "big.svg") << half << std::string(pack_pictures::kMaxSvgBytes, ' ');
    CHECK(!pack_pictures::load_svg(folder / "big.svg", 8, 4));
    CHECK(!pack_pictures::load_ktx2(folder / "missing.ktx2", BlockFormat::None));
    fs::remove_all(folder);
}

}  // namespace

int main() {
    check_ktx2();
    check_svg();
    return test::result();
}
