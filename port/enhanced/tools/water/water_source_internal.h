// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared bounded source-image reads and declarative table operations for water_source.h.
#pragma once

#include <initializer_list>
#include <string_view>

#include "assets/bytes.h"
#include "common/json.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water::source {
// Owns a bounded sectioned image and the spans actually provided by its source records.
struct Image {
    std::vector<std::uint8_t> bytes;  // Zero-filled virtual address space, bounded by the profile.
    std::vector<std::array<std::size_t, 2>> sections;  // Loaded half-open address ranges.
    // Rejects reads outside loaded sections, including otherwise zero-filled virtual gaps.
    void check(std::size_t address, std::size_t size) const;
    // Reads one little-endian table word from retained source bytes.
    std::uint32_t word(std::size_t address) const;
    // Reads one finite float32 without relying on source alignment.
    double scalar(std::size_t address) const;
};

// Rejects unknown profile keys and non-object records before operation-specific parsing.
void keys(const json::Value& row, std::initializer_list<std::string_view> allowed);
// Reads a required unsigned integer bounded before indexing, multiplication or allocation.
std::size_t integer(const json::Value& value, std::size_t maximum);
// Reads a required finite scalar bounded to the supported world-coordinate domain.
double scalar(const json::Value& value);
// Reads exactly three finite components.
Point point(const json::Value& value);
// Loads a little-endian section-record image, accepting the existing bounded WAD decoder.
Image load_image(const std::filesystem::path& path, std::size_t size);
// Expands connected masked tiles in stable source order, with no joins across storeys.
std::vector<Surface> tiles(const Image& image, const json::Value& table);
// Expands pointer-based vertex strips, retaining art-layer and triangle order.
Surface strips(const Image& image, const json::Value& table);
}  // namespace openrac::lighting::water::source
