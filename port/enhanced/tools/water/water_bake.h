// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native numerical water baking from decoded, version-specific surface inputs.
#pragma once
#include <array>
#include <filesystem>
#include <string>

#include "common/result.h"

namespace openrac::lighting::water {
// Bakes the Pokitaru spectrum and its two dominant coastal responses.
[[nodiscard]] Result<std::string> ocean_spectrum(
    const std::filesystem::path& patch, double offshore_depth
);
// Bakes one unit-amplitude harmonic wave and a flat-water reference from a coastal patch.
[[nodiscard]] Result<std::string> coastal_wave(
    const std::filesystem::path& patch, double period, double heading, double offshore_depth
);
// Extracts a fixed coastal patch as JSON for the offline wave-response prototype.
[[nodiscard]] Result<std::string> pokitaru_coast(
    const std::filesystem::path& level, std::array<double, 2> origin, unsigned size, double cell
);
// Writes a new water.bin/water.mesh pair from a bounded version-one request.
// The caller stages both outputs before publication; existing output files are refused.
[[nodiscard]] Result<void> bake(
    const std::filesystem::path& input, const std::filesystem::path& output
);
}  // namespace openrac::lighting::water
