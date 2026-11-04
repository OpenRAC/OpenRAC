// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Per-level native interior preparation before the GI scene is fingerprinted or loaded.
#pragma once

#include <filesystem>

#include "common/result.h"

namespace openrac::lighting {

// Prepares interior reports and owned generated sources, preserving explicit overrides and current caches.
// `generate_sources` controls light generation independently of the floor analysis reports.
// The caller supplies exported gameplay bytes at `gameplay`; `output` must already exist.
// Returns whether preparation changed the level-owned source bytes.
[[nodiscard]] Result<bool> prepare_level_interiors(
    const std::filesystem::path& level,
    const std::filesystem::path& gameplay,
    const std::filesystem::path& output,
    bool generate_sources = true
);

}  // namespace openrac::lighting
