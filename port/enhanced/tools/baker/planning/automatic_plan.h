// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Resolved resident scene inspection to the automatic native plan, without a client exchange.
#pragma once
#include <filesystem>
#include <string>

#include "common/result.h"

namespace openrac::lighting {
struct ProbeBatchOptions;
struct LevelBakeProfile;
// Resolves the same inspection precision, source paths and level settings used by standalone requests.
[[nodiscard]] Result<std::string> automatic_plan(
    const ProbeBatchOptions& options,
    const LevelBakeProfile& profile,
    const std::string& inspection,
    const std::filesystem::path& profiles
);
}  // namespace openrac::lighting
