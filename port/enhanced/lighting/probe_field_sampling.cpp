// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Receiver sampling shares the fixed bake worker budget without parallel floating-point reductions.
#include "enhanced/lighting/probe_field_sampling.h"

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/lighting/environment_gi.h"

namespace openrac::lighting {
namespace {
// Thirty-two receivers amortise claims while leaving enough chunks to balance small field batches.
constexpr std::size_t kReceiversPerClaim = 32;

// Writes one receiver's view samples; its index is owned by exactly one worker.
void sample_receiver(const ProbeFieldBatch& batch, std::size_t index) {
    const auto& receiver = batch.receivers[index];
    const auto views = bake_detail::probe_density_views(receiver.normal);
    EnvironmentBakeSettings bias;
    // Retain the existing normal/view offsets used for runtime field validation.
    bias.normal_bias = bias.view_bias = .5f;

    // Views remain in the established bias order within each independently owned receiver.
    for (unsigned view = 0; view < views.size(); ++view) {
        const auto sample =
            batch.field.sample_result(receiver.position, receiver.normal, views[view], bias);
        batch.samples[index][view] = {sample ? sample->irradiance : Float3{}, bool(sample)};
    }
}

// Runs on one pool worker until the shared counter has assigned every receiver in the batch.
void sample_worker(
    const ProbeFieldBatch& batch, const std::fenv_t& environment, std::atomic<std::size_t>& next
) {
    const profiling::Zone timing("Sample GI field receivers");

    // Reused workers may retain a preceding grid's directed rounding environment.
    if (std::fesetenv(&environment) != 0) {
        throw std::runtime_error("cannot restore field-sampling floating-point environment");
    }

    // Relaxed claims allocate disjoint output; completion futures publish the results to the caller.
    for (;;) {
        const auto begin = next.fetch_add(kReceiversPerClaim, std::memory_order_relaxed);

        // A claim beyond the output span ends this worker without publishing another sample.
        if (begin >= batch.receivers.size()) {
            return;
        }

        const auto end = std::min(begin + kReceiversPerClaim, batch.receivers.size());

        // Each claimed receiver writes a disjoint group of five view samples.
        for (std::size_t index = begin; index < end; ++index) {
            sample_receiver(batch, index);
        }
    }
}
}  // namespace

Result<void> sample_probe_field(const ProbeFieldBatch& batch, const BakeExecution& execution) {
    // Sampling failures become Result errors after the shared pool drains borrowed state.
    try {
        const profiling::Zone timing("Sample GI field batch");

        // Mismatched buffers must be rejected before any worker can publish partial evidence.
        if (batch.receivers.size() != batch.samples.size()) {
            return unexpected(Error{"field-sampling output count differs from receiver count"});
        }

        // Empty batches need neither worker submission nor a valid sample address.
        if (batch.receivers.empty()) {
            return {};
        }

        // Standalone serial checks retain the caller's arithmetic environment without a handoff.
        if (!execution.pool) {
            // The serial reference follows the original receiver order.
            for (std::size_t index = 0; index < batch.receivers.size(); ++index) {
                sample_receiver(batch, index);
            }

            return {};
        }

        std::fenv_t environment;

        // Worker reuse must capture the current controller environment before submission.
        if (std::fegetenv(&environment) != 0) {
            return unexpected(Error{"cannot read field-sampling floating-point environment"});
        }

        const unsigned requested = execution.threads ? execution.threads : execution.pool->size();
        const unsigned count = static_cast<unsigned>(std::min<std::size_t>(
            std::min(requested, execution.pool->size()), batch.receivers.size()
        ));
        std::atomic<std::size_t> next{0};
        // Workers borrow this request only until run() has drained every submitted task.
        return execution.pool->run(count, [&batch, &environment, &next] {
            sample_worker(batch, environment, next);
        });
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"unknown field-sampling failure"});
    }
}
}  // namespace openrac::lighting
