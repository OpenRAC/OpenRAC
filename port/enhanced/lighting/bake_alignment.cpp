// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic grid phase search with short clearance rays and winding-independent occupancy.

#include "enhanced/lighting/bake_alignment.h"

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <cmath>
#include <span>
#include <stdexcept>
#include <vector>

#include "common/profiler.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"
#include "enhanced/lighting/bake_workers.h"

namespace openrac::lighting::bake_detail {
namespace {

// Small fixed searches keep this preparation bounded relative to transport and integration.
constexpr std::size_t kPreviewProbes = 64;
constexpr float kPhaseFraction = .2f;      // Per-axis step relative to the smallest cell dimension.
constexpr float kClearanceFraction = .2f;  // Reward no clearance beyond this fraction of cell size.
constexpr double kInsidePenalty = 4;       // One inside probe outweighs four fully cleared probes.
constexpr double kNearPenalty = 2;         // Quadratic penalty for insufficient clearance.
constexpr double kImprovement = 1.e-6;     // Ties retain the original phase.

// Fixed paired directions approximate occupancy independently of triangle winding and shading normals.
std::array<std::array<Float3, 2>, 4> occupancy_directions() {
    constexpr float kDiagonal = .577350269f;  // Unit cube diagonals, 1 / sqrt(3).
    const std::array<Float3, 4> forward{
        Float3{1, 0, 0}, Float3{0, 1, 0}, Float3{0, 0, 1}, Float3{kDiagonal, kDiagonal, kDiagonal}
    };
    std::array<std::array<Float3, 2>, 4> pairs{};

    for (std::size_t n = 0; n < forward.size(); ++n) {
        pairs[n] = {forward[n], mul(forward[n], -1)};
    }

    return pairs;
}

// Retains each outer face coordinate, preserving grid bounds without additional probes or atlas rows.
Float3 anchored_offset(const ProbeGrid& grid, std::size_t index, Float3 offset) {
    for (unsigned axis = 0; axis < offset.size(); ++axis) {
        const auto coordinate = index % grid.dimensions[axis];
        index /= grid.dimensions[axis];

        // A boundary probe stays on its original volume face along this axis.
        if (coordinate == 0 || coordinate + 1 == grid.dimensions[axis]) {
            offset[axis] = 0;
        }
    }

    return grid.frame.to_world(offset);
}

// Estimates capped clearance with short rays, including surfaces coincident with the probe position.
float surface_clearance(const Scene& scene, Float3 position, float bias, float cap) {
    float nearest = cap;
    const float margin = 2 * bias;

    for (const auto& pair : occupancy_directions()) {
        for (const auto& direction : pair) {
            const auto hit =
                scene.trace(add(position, mul(direction, -margin)), direction, bias, cap + margin);

            // Misses retain the exact cap, avoiding rounding-driven near-surface counts.
            if (hit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                nearest = std::min(nearest, std::max(0.f, hit.ray.tfar - margin));
            }
        }
    }

    return nearest;
}

// ---- Parallel scoring ----

// Independent ray-query evidence, reduced in the original sample order on the controller.
struct SampleScore {
    double value = 0;     // Capped clearance and near/inside penalties for one probe.
    bool near = false;    // Probe lies below the desired clearance cap.
    bool inside = false;  // Paired crossing parity classified the probe as inside geometry.
};

// A bounded set of phases sharing the same deterministic sampling count.
struct PhaseSamples {
    std::span<const Float3> offsets;  // Borrowed until all scoring tasks drain.
    std::size_t count = 0;            // Samples per phase, stratified across the complete grid.
};

// Immutable scoring inputs shared by parallel workers, with disjoint sample outputs.
struct PhaseWork {
    const Scene& scene;            // Borrowed immutable Embree scene.
    const BakeSettings& settings;  // Borrowed bias and placement thresholds.
    const ProbeVolume& volume;  // Borrowed volume; offsets are applied only after scoring drains.
    PhaseSamples samples;       // Phase offsets and sample count, owned by the controller's caller.
    std::array<std::array<Float3, 2>, 4> directions;  // Fixed winding-independent occupancy rays.
    float cap = 0;                                    // Desired clearance in world units.
};

// Computes ray evidence only; parallel workers never update a shared floating-point sum.
SampleScore score_sample(const PhaseWork& work, std::size_t sample) {
    const auto phase = sample / work.samples.count;
    const auto n = sample % work.samples.count;
    const auto index = (2 * n + 1) * work.volume.probes.size() / (2 * work.samples.count);
    const auto position =
        add(work.volume.position(index),
            anchored_offset(work.volume.grid, index, work.samples.offsets[phase]));
    const float nearest = surface_clearance(work.scene, position, work.settings.ray_bias, work.cap);
    const double clearance = nearest / work.cap;
    const double deficit = 1 - clearance;
    const auto placement = classify(work.scene, position, work.settings, work.directions);
    return {
        clearance * clearance - kNearPenalty * deficit * deficit
            - kInsidePenalty * static_cast<double>(placement.inside),
        nearest < work.cap,
        placement.inside
    };
}

// Runs bounded sample chunks on shared workers, inheriting the controller's arithmetic environment.
void trace_samples(
    const PhaseWork& work, std::span<SampleScore> scores, const BakeExecution& execution
) {
    // Direct fixtures and standalone serial bakes retain caller-thread evaluation.
    if (!execution.pool) {
        // Serial callers retain the same sample order without scheduling overhead.
        for (std::size_t sample = 0; sample < scores.size(); ++sample) {
            scores[sample] = score_sample(work, sample);
        }

        return;
    }

    std::fenv_t environment;

    // Worker reuse must not inherit a preceding job's arithmetic environment.
    if (std::fegetenv(&environment) != 0) {
        throw std::runtime_error("cannot read alignment floating-point environment");
    }

    std::atomic<std::size_t> next{0};
    // Sixteen probes amortise atomic claims without leaving a long final chunk on one worker.
    constexpr std::size_t kSamplesPerClaim = 16;
    const unsigned requested = execution.threads ? execution.threads : execution.pool->size();
    const unsigned workers = static_cast<unsigned>(
        std::min<std::size_t>(std::min(requested, execution.pool->size()), scores.size())
    );
    // Pool workers borrow the request and write only the sample chunks they claim.
    const auto result = execution.pool->run(workers, [&work, scores, &environment, &next] {
        const profiling::Zone zone("Score alignment samples");

        // Every worker restores the controller environment before evaluating ray evidence.
        if (std::fesetenv(&environment) != 0) {
            throw std::runtime_error("cannot restore alignment floating-point environment");
        }

        // The atomic counter assigns disjoint chunks and terminates after all samples are claimed.
        for (;;) {
            const auto begin = next.fetch_add(kSamplesPerClaim, std::memory_order_relaxed);

            // Claims beyond the output span end this worker without writing another sample.
            if (begin >= scores.size()) {
                break;
            }
            const auto end = std::min(begin + kSamplesPerClaim, scores.size());

            // A claimed chunk writes disjoint evidence rather than updating shared phase sums.
            for (std::size_t sample = begin; sample < end; ++sample) {
                scores[sample] = score_sample(work, sample);
            }
        }
        work.scene.check_error();
    });

    // Borrowed inputs remain alive until all pool tasks have drained, including failures.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Preserves the original summation order and comparisons after parallel collision queries drain.
std::vector<AlignmentScore> score_phases(
    const Scene& scene,
    const BakeSettings& settings,
    const ProbeVolume& volume,
    PhaseSamples samples,
    const BakeExecution& execution
) {
    const float cell = *std::min_element(volume.grid.spacing.begin(), volume.grid.spacing.end());
    const PhaseWork work{
        scene,
        settings,
        volume,
        samples,
        occupancy_directions(),
        std::max(kClearanceFraction * cell, settings.ray_bias * 4)
    };
    std::vector<SampleScore> evidence(samples.offsets.size() * samples.count);
    trace_samples(work, evidence, execution);
    std::vector<AlignmentScore> scores(samples.offsets.size());

    // Each phase's original flattened-grid order determines its exact floating-point sum.
    for (std::size_t phase = 0; phase < scores.size(); ++phase) {
        // Each sample contributes in the same order as the original serial phase loop.
        for (std::size_t n = 0; n < samples.count; ++n) {
            const auto& sample = evidence[phase * samples.count + n];
            scores[phase].near += sample.near;
            scores[phase].inside += sample.inside;
            scores[phase].mean += sample.value;
        }

        scores[phase].mean /= static_cast<double>(samples.count);
    }

    return scores;
}

// ---- Phase selection ----

// Selects the best of 27 phases on a small preview; the original phase wins score ties.
Float3 preview_phase(
    const Scene& scene,
    const BakeSettings& settings,
    const ProbeVolume& volume,
    const BakeExecution& execution
) {
    const profiling::Zone zone("Preview alignment phases");
    const auto count = std::min(kPreviewProbes, volume.probes.size());
    const float cell = *std::min_element(volume.grid.spacing.begin(), volume.grid.spacing.end());
    // Leave displacement budget for subsequent per-probe relocation, including anisotropic grids.
    const float step = std::min(kPhaseFraction, settings.relocation_max_fraction / 2) * cell;
    constexpr unsigned kPhasesPerAxis = 3;  // Negative, zero and positive step.
    constexpr unsigned kPhaseCount = kPhasesPerAxis * kPhasesPerAxis * kPhasesPerAxis;
    std::array<Float3, kPhaseCount + 1> offsets{};

    // Keep the original baseline followed by the complete original phase enumeration.
    for (unsigned phase = 0; phase < kPhaseCount; ++phase) {
        unsigned remaining = phase;

        // Base-three digits retain the established XYZ phase enumeration.
        for (float& component : offsets[phase + 1]) {
            component = (static_cast<float>(remaining % kPhasesPerAxis) - 1) * step;
            remaining /= kPhasesPerAxis;
        }
    }

    const auto scores = score_phases(scene, settings, volume, {offsets, count}, execution);
    Float3 best{};
    double best_score = scores.front().mean;

    // Strict ordered comparisons retain the same winner even when workers finish out of order.
    for (std::size_t phase = 1; phase < offsets.size(); ++phase) {
        // Ordered strict comparisons keep the original candidate when scores tie.
        if (scores[phase].mean > best_score + kImprovement) {
            best_score = scores[phase].mean;
            best = offsets[phase];
        }
    }

    return best;
}

}  // namespace

GridAlignment align_probe_grid(
    const Scene& scene,
    const BakeSettings& settings,
    ProbeVolume& volume,
    const BakeExecution& execution
) {
    const profiling::Zone trace("Align probe grid");

    GridAlignment result;

    // No geometry or displacement budget means alignment cannot improve the original placement.
    if (scene.input.triangles.empty() || settings.relocation_max_fraction == 0) {
        return result;
    }

    // A grid consisting entirely of boundary planes has no movable coordinates.
    if (std::all_of(volume.grid.dimensions.begin(), volume.grid.dimensions.end(), [](auto count) {
            return count <= 2;
        })) {
        return result;
    }

    const auto candidate = preview_phase(scene, settings, volume, execution);

    // Avoid two full-grid passes when the preview already prefers the original phase.
    if (candidate == Float3{}) {
        return result;
    }

    const std::array<Float3, 2> offsets{Float3{}, candidate};
    const auto scores =
        score_phases(scene, settings, volume, {offsets, volume.probes.size()}, execution);
    result.before = scores[0];
    result.after = scores[1];

    // A preview improvement must also improve the complete volume before any position changes.
    if (result.after.mean <= result.before.mean + kImprovement) {
        result.after = result.before;
        return result;
    }

    result.offset = candidate;

    for (std::size_t index = 0; index < volume.probes.size(); ++index) {
        volume.probes[index].offset = anchored_offset(volume.grid, index, candidate);
    }

    scene.check_error();
    return result;
}

}  // namespace openrac::lighting::bake_detail
