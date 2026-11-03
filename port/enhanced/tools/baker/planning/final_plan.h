// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native final plan assembly and registration after complete batch verification.
#pragma once

#include <string>

#include "common/json.h"
#include "common/result.h"

namespace openrac::lighting {
struct ProbeBatchOptions;
// Plans local coverage, records resume identity and returns the complete transport request.
// Only output-owned grid files are removed when an existing seed plan has changed.
[[nodiscard]] Result<std::string> prepare_final_plan(
    const ProbeBatchOptions& options, const json::Value& request
);
// Registers fixed output names only after every grid and source identity has been verified.
[[nodiscard]] Result<void> register_batch_outputs(const ProbeBatchOptions& options);
}  // namespace openrac::lighting
