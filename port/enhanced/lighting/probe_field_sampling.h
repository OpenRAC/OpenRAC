// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded receiver batches shared by adaptive reference sampling and candidate-field comparison.
#pragma once
#include <array>
#include <span>

#include "common/result.h"
#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/probe_density.h"

namespace openrac::lighting {
namespace bake_detail {
class CaptureGi;
}

// Irradiance and support evidence for one receiver/view pair.
struct ProbeFieldSample {
    Float3 irradiance{};     // Sampled linear irradiance, zero when unsupported.
    bool supported = false;  // Coverage must match independently of RGB error.
};

// The five view biases returned by probe_density_views, in their original order.
using ProbeFieldSamples = std::array<ProbeFieldSample, 5>;

// Borrowed immutable GI/receivers and disjoint writable output, alive until sampling drains.
struct ProbeFieldBatch {
    const bake_detail::CaptureGi& field;  // Shared immutable region index and decoded volumes.
    std::span<const bake_detail::ProbeDensityReceiver> receivers;  // Ordered validation inputs.
    std::span<ProbeFieldSamples> samples;  // Exactly one result per receiver.
};

// Samples independent receivers with shared workers, preserving view order and caller rounding.
// An absent pool evaluates serially; errors drain all submitted work before returning.
[[nodiscard]] Result<void> sample_probe_field(
    const ProbeFieldBatch& batch, const BakeExecution& execution = {}
);
}  // namespace openrac::lighting
