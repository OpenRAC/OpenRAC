// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Final adaptive field validation against the runtime three-volume GI selection and blending.
#include "enhanced/tools/baker/output/density_stage.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>

#include "common/profiler.h"
#include "enhanced/lighting/environment_gi.h"
#include "enhanced/lighting/probe_field_sampling.h"
#include "enhanced/tools/baker/output/report.h"

namespace openrac::lighting {
namespace {
namespace fs = std::filesystem;
using namespace bake_detail;

// Read-only evidence used by the deterministic candidate-field reduction.
struct FieldComparison {
    std::span<const ProbeVolume>
        volumes;                        // Decoded candidate field, owned by the verification stage.
    const std::set<unsigned>& changed;  // Stable IDs whose fine references may be restored.
    std::span<const ProbeDensityReceiver> receivers;  // Original receiver order.
    std::span<const ProbeFieldSamples> expected;      // Fine-field evidence in receiver/view order.
    const ProbeDensitySettings& settings;             // Existing RGB error limits.
};

// Controller-owned accumulation; workers never modify rejection counts or error maxima.
struct FieldErrors {
    float maximum = 0;         // Maximum error in this comparison pass.
    std::size_t failures = 0;  // Rejected views across all restoration passes.
};

// Converts sampling errors after all workers have drained their borrowed inputs.
void sample_checked(const ProbeFieldBatch& batch, const BakeExecution& execution) {
    const auto result = sample_probe_field(batch, execution);

    // A failed sampling batch has already drained its borrowed field and output buffers.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Only batch-owned fixed filenames reach this reader.
ProbeVolume read_volume(const fs::path& path) {
    const profiling::Zone timing("Read adaptive GI asset");

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    // Match the runtime loader's maximum file size before allocating readback storage.
    constexpr std::streamoff kMaximumAssetBytes = 512ll * 1024 * 1024;

    if (!file || file.tellg() <= 0 || file.tellg() > kMaximumAssetBytes) {
        throw std::runtime_error("cannot read adaptive probe volume");
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);

    // A single bounded read replaces byte-at-a-time iterator growth.
    if (!file.read(
            reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        )) {
        throw std::runtime_error("incomplete adaptive probe volume");
    }

    return decode_probes(bytes);
}

fs::path reference_path(const fs::path& path) {
    return path.parent_path() / "adaptive-reference" / path.filename();
}

// Samples the fine field by temporarily replacing only reduced grids in the loaded candidate field.
std::vector<ProbeFieldSamples> reference_samples(
    std::vector<ProbeVolume>& volumes,
    std::span<const fs::path> paths,
    std::span<const ProbeDensityReceiver> receivers,
    std::size_t& probes,
    const BakeExecution& execution
) {
    const profiling::Zone timing("Sample reference GI field");
    std::vector<std::pair<std::size_t, ProbeVolume>> candidates;

    // Unchanged volumes are shared by both checks, without a second read, decode or copy.
    for (std::size_t index = 0; index < paths.size(); ++index) {
        const auto reference = reference_path(paths[index]);

        // Fine references exist only for grids whose density was reduced.
        if (fs::exists(reference)) {
            auto fine = read_volume(reference);
            candidates.emplace_back(index, std::move(volumes[index]));
            volumes[index] = std::move(fine);
        }

        probes += volumes[index].probes.size();
    }

    const CaptureGi field(volumes);
    std::vector<ProbeFieldSamples> values(receivers.size());
    sample_checked({field, receivers, values}, execution);

    // Reference workers have drained; return the same candidate objects to their original slots.
    for (auto& [index, candidate] : candidates) {
        volumes[index] = std::move(candidate);
    }

    return values;
}

// Restores every overlapping reduction that could have contributed to a failed receiver.
void reject_changed_regions(
    const FieldComparison& comparison, Float3 position, std::set<unsigned>& rejected
) {
    bool found = false;

    // Stable region IDs keep restoration independent of worker completion order.
    for (const auto id : comparison.changed) {
        // Every changed contributor at this receiver must return to its fine reference.
        if (probe_boundary_weight(comparison.volumes[id].grid, position) > 0) {
            rejected.insert(id);
            found = true;
        }
    }

    // An unrelated mismatch cannot be repaired by restoring a reduced volume.
    if (!found) {
        throw std::runtime_error("adaptive lighting changed outside reduced probe coverage");
    }
}

// Applies support checks, RGB error accumulation and rejection in the original view order.
void compare_receiver(
    const FieldComparison& comparison,
    std::size_t receiver,
    const ProbeFieldSamples& actual,
    FieldErrors& errors,
    std::set<unsigned>& rejected
) {
    const auto& expected = comparison.expected[receiver];

    // The short-circuit order also preserves which errors contribute to the reported maximum.
    for (unsigned view = 0; view < actual.size(); ++view) {
        if (expected[view].supported == actual[view].supported
            && probe_density_matches(
                expected[view].irradiance,
                actual[view].irradiance,
                comparison.settings,
                errors.maximum
            )) {
            continue;
        }

        ++errors.failures;
        reject_changed_regions(comparison, comparison.receivers[receiver].position, rejected);
    }
}

// Samples bounded chunks in parallel, then resolves every decision on the controller.
std::set<unsigned> failed_regions(
    const FieldComparison& comparison, FieldErrors& errors, const BakeExecution& execution
) {
    const profiling::Zone timing("Compare adaptive GI field");
    const CaptureGi field(comparison.volumes);
    // Five views of 512 receivers require about 40 KiB, independent of level size.
    constexpr std::size_t kComparisonReceivers = 512;
    std::vector<ProbeFieldSamples> samples(
        std::min(kComparisonReceivers, comparison.receivers.size())
    );
    std::set<unsigned> rejected;

    // Chunk order preserves original receiver order without retaining the full candidate field samples.
    for (std::size_t begin = 0; begin < comparison.receivers.size();
         begin += kComparisonReceivers) {
        const auto count = std::min(kComparisonReceivers, comparison.receivers.size() - begin);
        const auto output = std::span(samples).first(count);
        sample_checked({field, comparison.receivers.subspan(begin, count), output}, execution);

        // Only this serial reduction updates maxima, failure counts and restoration IDs.
        for (std::size_t index = 0; index < count; ++index) {
            compare_receiver(comparison, begin + index, samples[index], errors, rejected);
        }
    }

    return rejected;
}
}  // namespace

std::vector<ProbeVolume> verify_probe_density_field(
    std::span<const fs::path> paths,
    std::span<const ProbeDensityReceiver> receivers,
    const ProbeDensitySettings& settings,
    std::ostream& output,
    const BakeExecution& execution
) {
    const profiling::Zone timing("Verify adaptive GI field");

    settings.validate();

    if (paths.empty()) {
        throw std::runtime_error("adaptive field verification requires a sparse grid");
    }

    std::set<unsigned> changed;

    for (unsigned i = 1; i < paths.size(); ++i) {
        if (fs::exists(reference_path(paths[i]))) {
            changed.insert(i);
        }
    }

    std::size_t original_probes = 0;
    std::vector<ProbeVolume> volumes;

    for (const auto& path : paths) {
        volumes.push_back(read_volume(path));
    }

    const auto expected = reference_samples(volumes, paths, receivers, original_probes, execution);
    const auto candidates = changed.size();
    std::size_t restored = 0;
    FieldErrors errors;

    for (;;) {
        errors.maximum = 0;
        const auto rejected =
            failed_regions({volumes, changed, receivers, expected, settings}, errors, execution);

        if (rejected.empty()) {
            break;
        }

        for (auto id : rejected) {
            const auto reference = reference_path(paths[id]);
            fs::copy_file(reference, paths[id], fs::copy_options::overwrite_existing);
            fs::copy_file(
                reference.string() + ".txt",
                paths[id].string() + ".txt",
                fs::copy_options::overwrite_existing
            );
            volumes[id] = read_volume(paths[id]);
            fs::remove(reference);
            fs::remove(reference.string() + ".txt");
            changed.erase(id);
            ++restored;
        }
    }

    std::size_t final_probes = 0;

    for (const auto& volume : volumes) {
        final_probes += volume.probes.size();
    }

    // The report describes actual final assets, including reductions rejected by overlap checks.
    const auto text = std::format(
        "{{\n  \"version\": 1,\n  \"original_probes\": {},\n  \"final_probes\": {},\n"
        "  \"candidate_volumes\": {},\n  \"reduced_volumes\": {},\n  \"restored_volumes\": {},\n"
        "  \"receiver_checks\": {},\n  \"rejected_checks\": {},\n  \"maximum_final_error\": {},\n"
        "  \"relative_error_limit\": {},\n  \"absolute_error_limit\": {},\n  \"verified\": "
        "true\n}}\n",
        original_probes,
        final_probes,
        candidates,
        changed.size(),
        restored,
        receivers.size() * 5,
        errors.failures,
        errors.maximum,
        settings.relative_error,
        settings.absolute_error
    );
    fs::remove(paths.front().parent_path() / "probe-density-report.json");
    fs::remove(paths.front().parent_path() / "probe-density-report.json.tmp");
    write_bake_file(
        paths.front().parent_path() / "probe-density-report.json",
        std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size())
    );
    output << "Verified adaptive field " << original_probes << " -> " << final_probes << " probes, "
           << restored << " reductions restored, max RGB error " << errors.maximum << '\n'
           << std::flush;
    return volumes;
}
}  // namespace openrac::lighting
