// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Stable material texture identities for runtime matching, export and legacy emission migration.
#include <fstream>

#include "assets/disc/checksum.h"
#include "materials/catalog.h"

namespace openrac::materials {
std::string texture_identity(int width, int height, std::span<const std::uint8_t> rgba) {
    // Four RGBA bytes per positive pixel; host byte order and PNG encoding cannot affect identity.
    if (width <= 0 || height <= 0
        || rgba.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4) {
        return {};
    }
    std::array<std::uint8_t, 8> dimensions{};
    // Width and height are two little-endian 32-bit words.
    for (std::size_t axis = 0; axis < 2; ++axis) {
        const std::uint32_t value = static_cast<std::uint32_t>(axis == 0 ? width : height);
        for (unsigned byte = 0; byte < sizeof(value); ++byte) {
            dimensions[axis * sizeof(value) + byte] =
                static_cast<std::uint8_t>(value >> (byte * 8));
        }
    }
    assets::disc::Sha256 hash;
    hash.update(dimensions);
    hash.update(rgba);
    return assets::disc::to_hex(hash.finish());
}

Result<std::string> file_identity(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return unexpected(Error{"cannot read material texture identity"});
    }
    // Streaming avoids a second full PNG copy when importing older catalogs.
    std::array<std::uint8_t, 65'536> buffer{};
    assets::disc::Sha256 hash;
    while (input.read(reinterpret_cast<char*>(buffer.data()), buffer.size())
           || input.gcount() > 0) {
        hash.update(std::span(buffer.data(), static_cast<std::size_t>(input.gcount())));
    }
    if (!input.eof()) {
        return unexpected(Error{"cannot finish reading material texture identity"});
    }
    return assets::disc::to_hex(hash.finish());
}
}  // namespace openrac::materials
