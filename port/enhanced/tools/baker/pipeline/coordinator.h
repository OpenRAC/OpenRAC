// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native admission and shared tracing for multiple levels, with Python preparation callbacks.
#pragma once

#include <functional>
#include <istream>
#include <ostream>

#include "common/json.h"
#include "common/result.h"

namespace openrac::lighting {
class BakeThreadPool;
// One admitted native level controller; called outside the shared tracing pool.
using NativeLevelTask = std::function<void(unsigned, BakeThreadPool&)>;
// Runs native controllers under the same largest-first memory and tracing budgets as protocol jobs.
// The borrowed callback and schedule remain alive until every admitted controller has joined.
[[nodiscard]] Result<void> schedule_native_levels(
    const json::Value& schedule, const NativeLevelTask& task
);
// Runs the versioned JSON-lines session on supplied streams; returns nonzero on protocol failure.
// One initial schedule sets the CPU and estimated scene-memory budgets before admitting work.
int coordinate_levels(std::istream& input, std::ostream& output);
}  // namespace openrac::lighting
