// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Opt-in frame-wide dependency diagnostics, with no change to GPU execution or allocations.
#pragma once

#include <string>

#include "common/result.h"

namespace openrac::vkgl {
// Begins capture of declared work from this point until presentation, on the owning graphics
// thread.
// Call before frame work for a complete frame; rejects an absent context or an active capture.
[[nodiscard]] Result<void> capture_frame_graph();
// Returns the last completed capture or its validation error; no GPU readback or synchronization.
[[nodiscard]] Result<std::string> frame_graph_description();
}  // namespace openrac::vkgl
