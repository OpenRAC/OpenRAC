// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Surface-coverage requests share the batch's existing native process and collision loader.
#pragma once
#include <filesystem>
#include <istream>
#include <ostream>
#include <string>

#include "common/result.h"

namespace openrac::lighting {
struct ProbeBatchOptions;
// Accepts automatic native assembly or reference coverage followed by a client-supplied final plan.
[[nodiscard]] Result<std::string> read_covered_plan(
    ProbeBatchOptions& options, std::istream& input, std::ostream& output
);
}  // namespace openrac::lighting
