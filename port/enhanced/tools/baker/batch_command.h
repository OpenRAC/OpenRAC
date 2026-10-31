// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The shared per-level batch entry point for standalone and coordinated native baking.
#pragma once

#include <istream>
#include <ostream>
#include <span>
#include <string>

namespace openrac::lighting {
class BakeThreadPool;
struct ProbeSourceHashes;

// Prepares one scene, exchanges its inspection/plan, then verifies GI and reflections.
// Arguments exclude the executable name. Failures are reported to output and returned as status.
// The optional pool is borrowed until return; callbacks use only the supplied streams.
int run_batch_command(
    std::span<const std::string> arguments,
    std::istream& input,
    std::ostream& output,
    BakeThreadPool* pool = nullptr,
    ProbeSourceHashes* sources = nullptr
);
}  // namespace openrac::lighting
