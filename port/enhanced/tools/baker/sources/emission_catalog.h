// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Validated emission catalog matching, keyed by exact exported PNG content rather than brightness.
#pragma once
#include <filesystem>

#include "enhanced/lighting/bake_workers.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"

namespace openrac::lighting {
// Matches a validated optional catalog to texture files using bounded streaming hashes.
// Returns ordinary texture-keyed emission settings; malformed definitions throw inside the prepare boundary.
bake_json::Object resolve_emission_catalog(
    const std::filesystem::path& level,
    const std::filesystem::path& catalog,
    const BakeExecution& execution = {1}
);
}  // namespace openrac::lighting
