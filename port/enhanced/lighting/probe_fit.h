// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <functional>

#include "enhanced/lighting/probes.h"

namespace openrac::lighting {
// Deterministic multi-start derivative-free rotation search, with optional roll. PCA, hull-edge and
// diameter guesses supplement a coarse global sweep; the caller scores the final
// padded/quantised box, returning infinity for infeasible coverage or budgets.
// Translation and extents follow analytically from the support points per frame.
ProbeFrame search_probe_frame(
    std::span<const Float3> points,
    const std::function<double(ProbeFrame)>& score,
    std::span<const ProbeFrame> seeds = {},
    bool allow_pitch = true,
    bool allow_roll = false
);
}  // namespace openrac::lighting
