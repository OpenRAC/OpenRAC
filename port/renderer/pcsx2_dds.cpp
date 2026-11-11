// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <array>
#include <fstream>

#include "renderer/pcsx2_pack.h"

namespace openrac::renderer::pcsx2_pack {
namespace {

std::uint32_t le(const std::uint8_t* data, int bytes) {
    std::uint32_t value = 0;
    for (int i = 0; i < bytes; ++i) {
        value |= std::uint32_t{data[i]} << (8 * i);
    }
    return value;
}

using Pixel = std::array<std::uint8_t, 4>;

Pixel rgb565(std::uint32_t word) {
    const auto r = (word >> 11) & 31;
    const auto g = (word >> 5) & 63;
    const auto b = word & 31;
    return {
        static_cast<std::uint8_t>((r << 3) | (r >> 2)),
        static_cast<std::uint8_t>((g << 2) | (g >> 4)),
        static_cast<std::uint8_t>((b << 3) | (b >> 2)),
        255
    };
}

void decode_block(const std::uint8_t* data, int format, Rgba8Image& image, int x, int y) {
    const auto* colour = data + (format == 1 ? 0 : 8);
    const auto first = le(colour, 2);
    const auto second = le(colour + 2, 2);
    const bool transparent = format == 1 && first <= second;
    std::array<Pixel, 4> colours{rgb565(first), rgb565(second), Pixel{}, Pixel{}};
    for (int c = 0; c < 3; ++c) {
        colours[2][c] = static_cast<std::uint8_t>(
            transparent ? (colours[0][c] + colours[1][c]) / 2
                        : (2 * colours[0][c] + colours[1][c]) / 3
        );
        colours[3][c] =
            static_cast<std::uint8_t>(transparent ? 0 : (colours[0][c] + 2 * colours[1][c]) / 3);
    }
    colours[2][3] = 255;
    colours[3][3] = transparent ? 0 : 255;
    std::array<std::uint8_t, 8> alphas{data[0], data[1]};
    if (format == 5) {
        const int count = alphas[0] > alphas[1] ? 7 : 5;
        for (int i = 1; i < count; ++i) {
            alphas[i + 1] =
                static_cast<std::uint8_t>(((count - i) * alphas[0] + i * alphas[1]) / count);
        }
        alphas[7] = count == 5 ? 255 : alphas[7];
    }
    const auto colour_bits = le(colour + 4, 4);
    const auto alpha_bits = std::uint64_t{le(data + 2, 4)} | (std::uint64_t{le(data + 6, 2)} << 32);
    for (int dy = 0; dy < 4 && y + dy < image.height; ++dy) {
        for (int dx = 0; dx < 4 && x + dx < image.width; ++dx) {
            const int i = dy * 4 + dx;
            auto pixel = colours[(colour_bits >> (2 * i)) & 3];
            if (format == 3) {
                pixel[3] = static_cast<std::uint8_t>(((data[i / 2] >> (4 * (i % 2))) & 15) * 17);
            } else if (format == 5) {
                pixel[3] = alphas[(alpha_bits >> (3 * i)) & 7];
            }
            const auto offset = (static_cast<std::size_t>(y + dy) * image.width + x + dx) * 4;
            std::copy(pixel.begin(), pixel.end(), image.pixels.data() + offset);
        }
    }
}

}  // namespace

std::optional<Rgba8Image> load_dds(const std::filesystem::path& file) {
    std::ifstream stream(file, std::ios::binary);
    std::array<std::uint8_t, 128> header{};
    if (!stream.read(reinterpret_cast<char*>(header.data()), header.size())
        || std::string_view(reinterpret_cast<const char*>(header.data()), 4) != "DDS "
        || le(header.data() + 4, 4) != 124 || le(header.data() + 76, 4) != 32) {
        return {};
    }
    const auto w = le(header.data() + 16, 4);
    const auto h = le(header.data() + 12, 4);
    if (!w || !h || w > 8192 || h > 8192 || std::uint64_t{w} * h > 16 * 1024 * 1024) {
        return {};
    }
    const auto flags = le(header.data() + 80, 4);
    const std::string_view fourcc(reinterpret_cast<const char*>(header.data() + 84), 4);
    int format = 0;
    if (flags & 4) {
        if (fourcc == "DXT1") {
            format = 1;
        } else if (fourcc == "DXT3") {
            format = 3;
        } else if (fourcc == "DXT5") {
            format = 5;
        } else {
            return {};
        }
    } else if (!(flags & 0x40) || le(header.data() + 88, 4) != 32) {
        return {};
    }
    const std::size_t block_size = format == 1 ? 8 : 16;
    const std::size_t stride = format ? ((w + 3) / 4) * block_size : w * 4;
    const std::size_t rows = format ? (h + 3) / 4 : h;
    const auto start = stream.tellg();
    stream.seekg(0, std::ios::end);
    if (stream.tellg() - start < static_cast<std::streamoff>(stride * rows)) {
        return {};
    }
    stream.seekg(start);
    Rgba8Image image(static_cast<int>(w), static_cast<int>(h));
    std::vector<std::uint8_t> row(stride);
    std::array<int, 4> channels{};
    if (!format) {
        for (int c = 0; c < 4; ++c) {
            const auto mask = le(header.data() + 92 + c * 4, 4);
            channels[c] = -1;
            for (int b = 0; b < 4; ++b) {
                if (mask == (0xffu << (8 * b))) {
                    channels[c] = b;
                }
            }
            if (channels[c] < 0 && (c < 3 || mask != 0)) {
                return {};
            }
        }
    }
    for (std::size_t y = 0; y < rows; ++y) {
        if (!stream
                 .read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(stride))) {
            return {};
        }
        if (format) {
            for (std::size_t x = 0; x < stride / block_size; ++x) {
                decode_block(
                    row.data() + x * block_size,
                    format,
                    image,
                    static_cast<int>(x * 4),
                    static_cast<int>(y * 4)
                );
            }
        } else {
            auto* output = image.pixels.data() + y * stride;
            for (std::size_t x = 0; x < w; ++x) {
                for (int c = 0; c < 4; ++c) {
                    output[x * 4 + c] = channels[c] < 0 ? 0x80 : row[x * 4 + channels[c]];
                }
            }
        }
    }
    return image;
}

}  // namespace openrac::renderer::pcsx2_pack
