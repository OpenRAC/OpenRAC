// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Prepares the image assets referenced by one material profile.
#pragma once

#include <filesystem>
#include <string>

namespace openrac::lighting {

// Installs recipe outputs and independently authored PNGs before publishing the catalog.
void prepare_material_textures(
    const std::filesystem::path& level,
    const std::filesystem::path& profile,
    const std::filesystem::path& output
);

// Identifies the recipe inputs and published textures without decoding any images.
std::string emission_texture_fingerprint(
    const std::filesystem::path& level, const std::filesystem::path& profiles
);

}  // namespace openrac::lighting
