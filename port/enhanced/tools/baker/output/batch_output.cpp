// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// batch_output.h's density decisions and payload reuse; no callback writes controller diagnostics directly.
#include "enhanced/tools/baker/output/batch_output.h"

#include <algorithm>
#include <format>

#include "common/profiler.h"
#include "enhanced/lighting/probe_texture_encode.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"

namespace openrac::lighting {
namespace {
// Keeps the original fine-grid resume report only when reduction actually needs its fallback.
ProbeReferenceOutput reference_output(
    std::vector<std::uint8_t> bytes,
    const ProbeGrid& original,
    const ProbeGridOutput& job,
    const ProbeGridOutputContext& context
) {
    auto report =
        context.descriptions.describe(original) + job.density_identity
        + std::format(
            "adaptive_original_probes {}\nadaptive_final_probes {}\nadaptive_overlap_restored 1\n",
            original.probe_count(),
            original.probe_count()
        );
    return {std::move(bytes), std::move(report)};
}

// Owns only encoded payloads and small diagnostics after the chosen raw grid has been returned.
struct EncodedOutput {
    std::vector<std::uint8_t> bytes;  // Final asset bytes, including its finalized settings hash.
    std::optional<ProbeReferenceOutput> reference;  // Original bytes only for a changed lattice.
    bake_detail::ProbeDensityReport density;  // Existing comparison counters and error bounds.
    std::string diagnostics;  // Queued adaptive diagnostics, emitted in submission order.
};

// Reuses the original or accepted candidate bytes instead of running the codec again for output.
EncodedOutput encode_output(ProbeGridOutput& job, const ProbeGridOutputContext& context) {
    EncodedOutput result;
    const auto execution = BakeExecution{context.options.threads, &context.workers};

    // The coarse grid and density-disabled requests need only their final encoding.
    if (!context.verification.coarse || !context.density.enabled) {
        result.bytes =
            encode_baked_probes(job.volume, context.options.settings.texture_format, execution);
        return result;
    }

    const auto original = job.volume.grid;
    // Density trials finalize candidate identities before encoding, making accepted bytes publishable.
    const auto identity = [&settings = context.options.settings](const ProbeGrid& grid) {
        return probe_bake_settings_hash(grid, settings);
    };
    auto reduced = bake_detail::simplify_probe_density(
        std::move(job.volume),
        {*context.verification.coarse, context.receivers, execution, identity},
        context.density
    );
    job.volume = std::move(reduced.volume);
    result.bytes = std::move(reduced.bytes);
    result.density = reduced.report;

    // Reduced grids alone need a retained original for whole-field overlap validation.
    if (!reduced.reference_bytes.empty()) {
        result.reference =
            reference_output(std::move(reduced.reference_bytes), original, job, context);
    }

    result.diagnostics = std::format(
        "Adaptive {} {} -> {} probes, {} receivers\n",
        job.path.filename().string(),
        result.density.original_probes,
        result.density.final_probes,
        result.density.receivers
    );
    return result;
}

// Retains the exact report schema and scalar formatting used by completed bake caches.
std::string output_report(
    const ProbeGridOutput& job,
    const ProbeGridOutputContext& context,
    const bake_detail::ProbeDensityReport& density
) {
    auto report = context.descriptions.describe(job.volume.grid) + job.density_identity;

    // Density-disabled and coarse reports omit adaptive fields, as before.
    if (context.verification.coarse && context.density.enabled && density.original_probes) {
        report += std::format(
            "adaptive_original_probes {}\nadaptive_final_probes {}\nadaptive_receivers {}\n"
            "adaptive_comparisons {}\nadaptive_accepted_axes {}\nadaptive_rejected_axes {}\n"
            "adaptive_maximum_error {}\n",
            density.original_probes,
            density.final_probes,
            density.receivers,
            density.comparisons,
            density.accepted_axes,
            density.rejected_axes,
            density.maximum_error
        );
    }

    return report;
}

// Captures output metadata before releasing expanded maps to keep queued memory bounded.
ProbeOutputCounts output_counts(const ProbeVolume& volume) {
    const auto valid =
        std::count_if(volume.probes.begin(), volume.probes.end(), [](const Probe& probe) {
            return probe.valid;
        });
    const auto relocated =
        std::count_if(volume.probes.begin(), volume.probes.end(), [](const Probe& probe) {
            return probe.relocated;
        });
    return {
        volume.probes.size(), static_cast<std::size_t>(valid), static_cast<std::size_t>(relocated)
    };
}
}  // namespace

Result<ProbeOutputCompletion> prepare_probe_grid_output(
    ProbeGridOutput& job, const ProbeGridOutputContext& context
) {
    try {
#ifdef OPENRAC_TRACY
        const profiling::Zone trace(job.path.filename().string());
#endif
        auto encoded = encode_output(job, context);

        // A cached codec payload must carry the same identity as a fresh final-grid encoding.
        if (job.volume.grid.settings_hash
            != probe_bake_settings_hash(job.volume.grid, context.options.settings)) {
            return unexpected(Error{"reused probe payload has stale grid settings"});
        }

        ProbeFileOutput file{
            job.path,
            job.volume.grid,
            std::move(encoded.bytes),
            output_report(job, context, encoded.density),
            output_counts(job.volume),
            job.started,
            bool(context.verification.coarse && context.density.enabled),
            std::move(encoded.reference)
        };
        job.volume = {};
        auto completed = write_probe_output(file, context.verification);

        // Verification failures cannot become ordered completion diagnostics or published grids.
        if (!completed) {
            return unexpected(completed.error());
        }

        completed->diagnostics = encoded.diagnostics + completed->diagnostics;
        return std::move(*completed);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"unknown probe grid output failure"});
    }
}
}  // namespace openrac::lighting
