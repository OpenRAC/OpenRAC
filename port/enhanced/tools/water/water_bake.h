// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native numerical water baking from decoded, version-specific surface inputs.
#pragma once
#include <filesystem>

#include "common/result.h"

namespace openrac::lighting::water {
// Writes a new water.bin/water.mesh pair from a bounded version-one request.
// The caller stages both outputs before publication; existing output files are refused.
[[nodiscard]] Result<void> bake(
    const std::filesystem::path& input, const std::filesystem::path& output
);
}  // namespace openrac::lighting::water
