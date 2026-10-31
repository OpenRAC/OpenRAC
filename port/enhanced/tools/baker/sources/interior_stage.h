// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The enhanced baker's interior inference stage, independent of its command-line dispatch.

#pragma once

#include <filesystem>
#include <string>

#include "common/result.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"

namespace openrac::lighting {

// Returns the interior stage's revision and default settings for cache identity, without loading a scene.
std::string interior_stage_profile();

// Saves a typed inference result for diagnostics and downstream cached room consumers.
[[nodiscard]] Result<void> write_interior_report(
    const std::filesystem::path& output,
    const InteriorInput& input,
    const interiors::Settings& settings,
    const interiors::Result& result
);

// Infers rooms from exported collision/transition JSON at `spacing` game units; refuses to overwrite
// `output`. Returns a diagnostic on invalid inputs, inference failure or an unwritable report.
[[nodiscard]] Result<void> infer_interior_file(
    const std::filesystem::path& input, const std::filesystem::path& output, float spacing
);

}  // namespace openrac::lighting
