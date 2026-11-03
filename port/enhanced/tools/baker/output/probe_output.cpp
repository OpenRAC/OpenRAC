// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe output writes and readback checks, with diagnostics buffered until ordered retirement.
#include "enhanced/tools/baker/output/probe_output.h"

#include <algorithm>
#include <format>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "common/profiler.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/probe_rounding.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"

namespace openrac::lighting {
namespace {
// Applies the same fine-reference cleanup before publishing a reduced or unchanged local grid.
void write_reference(const ProbeFileOutput& file) {
    // Cached references are retained when this output does not replace a local grid.
    if (!file.reset_reference) {
        return;
    }

    const auto reference = file.path.parent_path() / "adaptive-reference" / file.path.filename();
    std::filesystem::remove(reference);
    std::filesystem::remove(reference.string() + ".txt");

    // Only reduced grids retain a fine field for later density verification.
    if (file.reference) {
        std::filesystem::create_directories(reference.parent_path());
        write_bake_file(reference, file.reference->bytes);
        const auto& report = file.reference->report;
        write_bake_file(
            reference.string() + ".txt",
            std::span(reinterpret_cast<const std::uint8_t*>(report.data()), report.size())
        );
    }
}

// Retains runtime arithmetic and geometric diagnostics before any output is reported as verified.
std::optional<ProbeVolume> verify_output(
    const ProbeFileOutput& file, const ProbeOutputContext& context, std::ostream& diagnostics
) {
    RuntimeProbeRounding rounding;
    auto checked = verify_probe_file(file.path, file.grid, file.bytes);

    // Invalid readback cannot participate in coverage or publication.
    if (!checked) {
        throw std::runtime_error(checked.error().message);
    }

    // Runtime rounding must preserve the settings identity used during baking.
    if (checked->grid.settings_hash != probe_bake_settings_hash(checked->grid, context.settings)) {
        throw std::runtime_error("probe batch settings differ under runtime rounding");
    }

    // Local fields must remain compatible with their verified coarse field.
    if (context.coarse) {
        validate_probe_pair(*context.coarse, *checked);
    }

    report_terrain_coverage(context.level, *checked, context.coarse, diagnostics);

    // The first decoded field is needed by all subsequent local-grid bakes.
    if (!context.coarse) {
        return std::move(*checked);
    }

    return std::nullopt;
}

// Writes the original metadata after verification, using completion time rather than submission time.
void write_report(const ProbeFileOutput& file) {
    std::string report = file.report;
    report += std::format(
        "relocated_probes {}\nvalid_probes {}\ninvalid_probes {}\n"
        "bake_seconds {}\nasset_bytes {}\nverified_runtime_rounding 1\n",
        file.counts.relocated,
        file.counts.valid,
        file.counts.total - file.counts.valid,
        std::chrono::duration<double>(std::chrono::steady_clock::now() - file.started).count(),
        file.bytes.size()
    );
    write_bake_file(
        file.path.string() + ".txt",
        std::span(reinterpret_cast<const std::uint8_t*>(report.data()), report.size())
    );
}

}  // namespace

Result<ProbeVolume> verify_probe_file(
    const std::filesystem::path& path, const ProbeGrid& grid, std::span<const std::uint8_t> encoded
) {
    // File and format errors return to the owning batch controller.
    try {
        const profiling::Zone timing("Verify written probe asset");
        const auto checksum = bake_json::verify_file_checksum(path, encoded);
        // Only an exact saved-file checksum can authorize the in-memory format checks.
        if (!checksum) {
            return unexpected(checksum.error());
        }
        auto volume = decode_probes(encoded);

        // The file must describe the grid that submitted this output.
        if (!probe_identity_matches(
                volume, grid.game_version, grid.level, grid.scene_hash, grid.settings_hash
            )) {
            return unexpected(Error{"probe batch identity mismatch"});
        }

        return volume;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<ProbeOutputCompletion> write_probe_output(
    const ProbeFileOutput& file, const ProbeOutputContext& context
) {
    // File and format errors return to the owning batch controller.
    try {
        const profiling::Zone timing("Write and verify probe output");
        write_reference(file);
        write_bake_file(file.path, file.bytes);
        std::ostringstream diagnostics;
        ProbeOutputCompletion completed;
        completed.coarse = verify_output(file, context, diagnostics);
        write_report(file);
        diagnostics << "Verified " << file.path.filename().string() << '\n';
        completed.diagnostics = diagnostics.str();
        return completed;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
