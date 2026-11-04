// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Error-bounded probe-density reduction using packed-map precision and playable receiver checks.
#include "enhanced/lighting/probe_density.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/probe_texture_encode.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr std::array<Float3, 6> kNormals{
    Float3{1, 0, 0},
    Float3{-1, 0, 0},
    Float3{0, 1, 0},
    Float3{0, -1, 0},
    Float3{0, 0, 1},
    Float3{0, 0, -1}
};

// A missing required receiver may not silently fall back to a different lighting field.
bool inside(const ProbeGrid& grid, Float3 p) {
    p = grid.frame.to_local(add(p, mul(grid.origin, -1)));

    for (unsigned a = 0; a < 3; ++a) {
        if (p[a] < 0 || p[a] > grid.spacing[a] * float(grid.dimensions[a] - 1)) {
            return false;
        }
    }

    return true;
}

// Copy only lattice members; interpolating distance moments into new probes would hide blockers.
ProbeVolume decimate(const ProbeVolume& reference, std::array<unsigned, 3> stride) {
    const profiling::Zone timing("Decimate probe grid");

    auto grid = reference.grid;

    for (unsigned a = 0; a < 3; ++a) {
        grid.spacing[a] *= float(stride[a]);
        grid.dimensions[a] = (grid.dimensions[a] - 1) / stride[a] + 1;
    }

    auto result = ProbeVolume::allocate(grid);
    const auto texels = grid.texels_per_probe();

    for (unsigned z = 0; z < grid.dimensions[2]; ++z) {
        for (unsigned y = 0; y < grid.dimensions[1]; ++y) {
            for (unsigned x = 0; x < grid.dimensions[0]; ++x) {
                const auto from =
                    x * stride[0]
                    + reference.grid.dimensions[0]
                          * (y * stride[1] + reference.grid.dimensions[1] * z * stride[2]);
                const auto to = x + grid.dimensions[0] * (y + grid.dimensions[1] * z);
                result.probes[to] = reference.probes[from];
                std::copy_n(
                    reference.irradiance.begin() + from * texels,
                    texels,
                    result.irradiance.begin() + to * texels
                );
                std::copy_n(
                    reference.distance_moments.begin() + from * texels,
                    texels,
                    result.distance_moments.begin() + to * texels
                );
            }
        }
    }

    return result;
}

// Keep both local directional variation and the final sparse/local blend within the same bound.
bool irradiance_matches(
    const ProbeSample& reference,
    const ProbeSample& candidate,
    const ProbeDensitySettings& settings,
    float& maximum
) {
    return probe_density_matches(reference.irradiance, candidate.irradiance, settings, maximum);
}

// Borrows the two packed fields and ordered error accumulation for one density trial.
struct DensityComparison {
    const ProbeVolume& reference;          // Full-density codec result.
    const ProbeVolume& candidate;          // Proposed reduced codec result.
    const ProbeVolume& coarse;             // Unchanged coarse fallback.
    const ProbeDensitySettings& settings;  // Receiver error and support policy.
    ProbeDensityReport& report;            // Ordered comparison counts.
    float& maximum;                        // Largest error for the current candidate.
};

// Wrap weights and receiver-plane weights depend on spacing even in empty space. Compare visible
// irradiance and validity coverage rather than treating those geometric weights as occlusion.
bool receiver_matches(
    const DensityComparison& comparison, const ProbeDensityReceiver& receiver, Float3 view
) {
    const auto p = receiver.position, n = receiver.normal;
    const auto a = sample_local_probes(comparison.reference, p, n, .5f, view);
    const auto b = sample_local_probes(comparison.candidate, p, n, .5f, view);
    ++comparison.report.comparisons;

    if (bool(a) != bool(b)) {
        return false;
    }

    if (a && b
        && (std::abs(a->coverage - b->coverage) > .05f
            || std::abs(a->visibility - b->visibility) > .05f
            || !irradiance_matches(*a, *b, comparison.settings, comparison.maximum))) {
        return false;
    }

    const auto full =
        sample_probe_volumes(comparison.coarse, &comparison.reference, p, n, .5f, view);
    const auto reduced =
        sample_probe_volumes(comparison.coarse, &comparison.candidate, p, n, .5f, view);

    if (bool(full) != bool(reduced)) {
        return false;
    }

    if (!full || !reduced) {
        return true;
    }

    return irradiance_matches(*full, *reduced, comparison.settings, comparison.maximum);
}

// Several view biases include grazing views; normal-facing cameras alone miss wall/floor leaks.
bool acceptable(
    const DensityComparison& comparison, std::span<const ProbeDensityReceiver> receivers
) {
    const profiling::Zone timing("Check density candidate");

    comparison.maximum = 0;

    for (const auto& receiver : receivers) {
        for (auto view : probe_density_views(receiver.normal)) {
            if (!receiver_matches(comparison, receiver, mul(view, .5f))) {
                return false;
            }
        }
    }

    return true;
}
}  // namespace

std::array<Float3, 5> probe_density_views(Float3 n) {
    const auto tangent = unit(cross(n, std::abs(n[2]) < .9f ? Float3{0, 0, 1} : Float3{0, 1, 0}));
    const auto bitangent = cross(n, tangent);
    return {
        n,
        unit(add(n, tangent)),
        unit(add(n, mul(tangent, -1))),
        unit(add(n, bitangent)),
        unit(add(n, mul(bitangent, -1)))
    };
}

bool probe_density_matches(
    Float3 reference, Float3 candidate, const ProbeDensitySettings& settings, float& maximum
) {
    for (unsigned c = 0; c < 3; ++c) {
        const float error = std::abs(reference[c] - candidate[c]);
        const float absolute = reference[c] < .025f ? std::min(settings.absolute_error, .003f)
                                                    : settings.absolute_error;
        maximum = std::max(maximum, error);

        if (!std::isfinite(error) || error > absolute + settings.relative_error * reference[c]) {
            return false;
        }
    }

    return true;
}

void ProbeDensitySettings::validate() const {
    if (!std::isfinite(relative_error) || relative_error < 0 || relative_error > .25f
        || !std::isfinite(absolute_error) || absolute_error < 0 || absolute_error > .1f) {
        throw std::runtime_error("invalid adaptive probe error limits");
    }
}

std::vector<ProbeDensityReceiver> probe_density_receivers(
    const BakeScene& source,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    const BakeSettings& settings,
    unsigned threads
) {
    const profiling::Zone timing("Build density receivers");

    if (layout.ceilings.size() != floors.size()) {
        throw std::runtime_error("adaptive probe receivers require matching floor/ceiling data");
    }

    Scene scene(source, settings.sunlight_direction, threads);
    std::vector<ProbeDensityReceiver> result;

    for (unsigned id = 0; id < floors.size(); ++id) {
        const auto floor = floors[id].position;

        for (float height : {.15f, 1.25f, 2.5f, 4.f}) {
            auto p = floor;
            p[2] += height;

            // Do not validate fictitious receivers through low ceilings.
            if (std::isfinite(layout.ceilings[id]) && p[2] >= layout.ceilings[id] - .1f) {
                continue;
            }

            for (auto n : kNormals) {
                result.push_back({p, n});
            }
        }

        auto eye = floor;
        eye[2] += 1.25f;

        for (auto direction : kNormals) {
            const auto hit = scene.trace(eye, direction, settings.ray_bias, 32);

            if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
                continue;
            }

            auto n = unit(Float3{hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z});

            if (dot(n, direction) > 0) {
                n = mul(n, -1);
            }

            const auto p = add(add(eye, mul(direction, hit.ray.tfar)), mul(n, .05f));
            result.push_back({p, n});
        }
    }

    scene.check_error();
    return result;
}

namespace {
// Selects receiver evidence once without changing its original order.
std::vector<ProbeDensityReceiver> supported_receivers(
    const ProbeGrid& grid, std::span<const ProbeDensityReceiver> receivers
) {
    const profiling::Zone timing("Select density receivers");
    std::vector<ProbeDensityReceiver> result;

    for (const auto& receiver : receivers) {
        // Evidence outside this grid cannot constrain its density.
        if (inside(grid, receiver.position)) {
            result.push_back(receiver);
        }
    }

    return result;
}

// Immutable fine reference and evidence, borrowed until every axis trial has been compared.
struct DensityTrials {
    const ProbeVolume& reference;          // Original unquantized lattice.
    const ProbeVolume& packed_reference;   // The same original after codec quantization.
    const ProbeDensityInput& input;        // Shared pool, coarse field and identity callback.
    const ProbeDensitySettings& settings;  // Existing codec and error policy.
    std::span<const ProbeDensityReceiver> receivers;  // Selected evidence in its original order.
};

// Tries axes in the original order, retaining only the last accepted lattice and its bytes.
void choose_density_axes(const DensityTrials& trial, ProbeDensityReduction& result) {
    auto& report = result.report;
    std::array<unsigned, 3> stride{1, 1, 1};

    for (unsigned axis = 0; axis < 3; ++axis) {
        // Decimation must keep both original lattice endpoints.
        if (trial.reference.grid.dimensions[axis] < 5
            || (trial.reference.grid.dimensions[axis] - 1) % 2) {
            continue;
        }

        auto next = stride;
        next[axis] = 2;
        auto candidate = decimate(trial.reference, next);

        // Finalize metadata before compression so accepted bytes can become the output unchanged.
        if (trial.input.settings_hash) {
            candidate.grid.settings_hash = trial.input.settings_hash(candidate.grid);
        }

        auto bytes =
            encode_baked_probes(candidate, trial.settings.texture_format, trial.input.execution);
        const auto packed_candidate = decode_probes(bytes);
        float maximum = 0;

        if (acceptable(
                {trial.packed_reference,
                 packed_candidate,
                 trial.input.coarse,
                 trial.settings,
                 report,
                 maximum},
                trial.receivers
            )) {
            stride = next;
            result.volume = std::move(candidate);
            result.bytes = std::move(bytes);
            report.maximum_error = maximum;
            ++report.accepted_axes;
        } else {
            ++report.rejected_axes;
        }
    }
}
}  // namespace

ProbeDensityReduction simplify_probe_density(
    ProbeVolume reference, const ProbeDensityInput& input, const ProbeDensitySettings& settings
) {
    const profiling::Zone timing("Simplify probe density");
    settings.validate();
    ProbeDensityReduction result;
    auto& report = result.report;
    report.original_probes = report.final_probes = reference.grid.probe_count();
    auto original_bytes = encode_baked_probes(reference, settings.texture_format, input.execution);
    const auto supported = settings.enabled ? supported_receivers(reference.grid, input.receivers)
                                            : std::vector<ProbeDensityReceiver>{};
    report.receivers = settings.enabled ? supported.size() : 0;

    // Disabled reduction or missing evidence retains the already encoded original grid.
    if (!settings.enabled || supported.empty()) {
        result.volume = std::move(reference);
        result.bytes = std::move(original_bytes);
        return result;
    }

    const auto packed_reference = decode_probes(original_bytes);
    choose_density_axes({reference, packed_reference, input, settings, supported}, result);

    // Rejected trials retain the original bytes; accepted trials retain their original fallback once.
    if (!report.accepted_axes) {
        result.volume = std::move(reference);
        result.bytes = std::move(original_bytes);
    } else {
        result.reference_bytes = std::move(original_bytes);
    }

    result.volume.validate();
    report.final_probes = result.volume.grid.probe_count();
    return result;
}
}  // namespace openrac::lighting::bake_detail
