// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Explicit emission profile preparation shared by automatic bakes and standalone diagnostics.
#pragma once
#include <filesystem>

#include "common/result.h"
#include "enhanced/lighting/bake_workers.h"

namespace openrac::lighting {
// Refreshes content-matched catalog and level defaults while preserving independent local overrides.
// The caller supplies matching tracked profile paths; absent catalog/default files contribute nothing.
// Returns whether probe-emission.json changed; invalid inputs and publication failures preserve its bytes.
[[nodiscard]] Result<bool> prepare_level_emission(
    const std::filesystem::path& level,
    const std::filesystem::path& catalog,
    const std::filesystem::path& defaults,
    const BakeExecution& execution = {1}
);
}  // namespace openrac::lighting
