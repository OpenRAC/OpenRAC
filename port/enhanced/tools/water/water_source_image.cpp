// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checked section-image loading and profile primitives shared by declarative water table decoders.
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "assets/disc/wad.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/water/water_source_internal.h"

namespace openrac::lighting::water::source {
void keys(const json::Value& row, std::initializer_list<std::string_view> allowed) {
    // Misspelled settings must never silently select another source layout.
    if (!row.is_object()) {
        throw std::runtime_error("water source settings require objects");
    }

    // Each operation has a closed declarative schema rather than arbitrary scripting.
    for (const auto& [key, value] : row.members()) {
        // Unknown fields usually indicate a misspelled or incompatible source profile.
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            throw std::runtime_error("unknown water source setting: " + key);
        }
    }
}

std::size_t integer(const json::Value& value, std::size_t maximum) {
    const double number = value.number(-1);

    // Bounds precede narrowing so malformed offsets cannot wrap into valid addresses.
    if (!value.is_number() || !std::isfinite(number) || number < 0
        || number > static_cast<double>(maximum) || std::floor(number) != number) {
        throw std::runtime_error("invalid water source integer");
    }

    return static_cast<std::size_t>(number);
}

double scalar(const json::Value& value) {
    // World-coordinate limits also bound height adjustments and table transforms.
    if (!value.is_number() || !std::isfinite(value.number())
        || std::abs(value.number()) >= 100'000) {
        throw std::runtime_error("invalid water source scalar");
    }

    return value.number();
}

Point point(const json::Value& value) {
    // Coordinate arrays have fixed arity before component access.
    if (!value.is_array() || value.items().size() != 3) {
        throw std::runtime_error("water source vector requires three components");
    }

    return {scalar(value[0]), scalar(value[1]), scalar(value[2])};
}

void Image::check(std::size_t address, std::size_t size) const {
    // Virtual gaps are not authored zero data and must not satisfy table reads.
    for (const auto& span : sections) {
        // Only completely loaded spans may satisfy a source read.
        if (address >= span[0] && address <= span[1] && size <= span[1] - address) {
            return;
        }
    }

    throw std::runtime_error("water source read outside loaded image sections");
}

std::uint32_t Image::word(std::size_t address) const {
    check(address, sizeof(std::uint32_t));
    return assets::ByteView(bytes).read<std::uint32_t>(address);
}

double Image::scalar(std::size_t address) const {
    check(address, sizeof(float));
    const double value = assets::ByteView(bytes).read<float>(address);

    // Nonfinite table data cannot enter geometry or grid allocation.
    if (!std::isfinite(value) || std::abs(value) >= 100'000) {
        throw std::runtime_error("invalid water source coordinate");
    }

    return value;
}

namespace {
// Copies one validated section, rejecting ambiguous overlapping mappings.
void section(Image& image, assets::ByteView data, std::size_t at, std::uint32_t entry) {
    const auto address = data.read<std::uint32_t>(at);
    const auto size = data.read<std::uint32_t>(at + 4);

    // Section headers are four little-endian words; payloads and addresses remain word-aligned.
    if (data.read<std::uint32_t>(at + 12) != entry || address % 4 || size % 4
        || address > image.bytes.size() || size > image.bytes.size() - address) {
        throw std::runtime_error("invalid water source image section");
    }

    const auto payload = data.sub(at + 16, size);

    // Conflicting source sections must not depend on overwrite order.
    for (const auto& span : image.sections) {
        // Overlapping mappings would make identical source addresses ambiguous.
        if (address < span[1] && address + size > span[0]) {
            throw std::runtime_error("overlapping water source image sections");
        }
    }

    std::copy(payload.span().begin(), payload.span().end(), image.bytes.begin() + address);
    image.sections.push_back({address, address + size});
}
}  // namespace

Image load_image(const std::filesystem::path& path, std::size_t size) {
    auto bytes = bake_json::source_bytes(path);

    // Existing compression belongs to the shared asset decoder, not a second implementation.
    if (assets::disc::is_wad(bytes)) {
        bytes = assets::disc::wad_decompress(bytes);
    }

    assets::ByteView data(bytes);
    const auto entry = data.read<std::uint32_t>(12);

    // An entry point identifies one complete sectioned image, never a collection of overlays.
    if (!entry || entry % 4 || !size || size > 64ull * 1024 * 1024) {
        throw std::runtime_error("invalid water source image size or entry point");
    }

    Image image{std::vector<std::uint8_t>(size), {}};

    // Section count is bounded independently of image size to limit adversarial scan costs.
    for (std::size_t at = 0; at < data.size();) {
        // Many tiny sections can otherwise amplify connectivity and read validation costs.
        if (image.sections.size() >= 4096) {
            throw std::runtime_error("too many water source image sections");
        }

        section(image, data, at, entry);
        at += 16 + data.read<std::uint32_t>(at + 4);
    }

    image.check(entry, sizeof(std::uint32_t));
    return image;
}
}  // namespace openrac::lighting::water::source
