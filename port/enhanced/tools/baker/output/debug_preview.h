// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <filesystem>
#include <iosfwd>

namespace openrac::lighting {
namespace bake_detail {
struct SpatialPlacement;
}

void write_spatial_placement(
    const std::filesystem::path& bake,
    const bake_detail::SpatialPlacement& placement,
    bool production = true
);
// Debug-only skeleton generation; never changes placement inputs or baked assets.
void write_debug_medial(const std::filesystem::path& level, const std::filesystem::path& bake);
void write_debug_spatial(const std::filesystem::path& level, const std::filesystem::path& bake);
void embed_spatial_preview(std::ostream& out, const std::filesystem::path& spatial_file);
std::filesystem::path write_debug_preview(
    const std::filesystem::path& level,
    const std::filesystem::path& bake,
    bool generate_prototypes = true
);
}  // namespace openrac::lighting
