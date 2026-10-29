// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/tools/probes/density_stage.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <set>

#include "enhanced/lighting/environment_gi.h"
#include "enhanced/tools/probes/report.h"

namespace openrac::lighting {
namespace {
namespace fs = std::filesystem;
using namespace bake_detail;

struct FieldSample {
    Float3 irradiance{};
    bool supported = false;
};

// Only batch-owned fixed filenames reach this reader.
ProbeVolume read_volume(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("cannot read adaptive probe volume");
    }

    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
    return decode_probes(bytes);
}

fs::path reference_path(const fs::path& path) {
    return path.parent_path() / "adaptive-reference" / path.filename();
}

// Hold one field at a time. Only sampled RGB values survive loading the reference field.
std::vector<std::array<FieldSample, 5>> reference_samples(
    std::span<const fs::path> paths,
    std::span<const ProbeDensityReceiver> receivers,
    std::size_t& probes
) {
    std::vector<ProbeVolume> volumes;

    for (const auto& path : paths) {
        const auto reference = reference_path(path);
        volumes.push_back(read_volume(fs::exists(reference) ? reference : path));
        probes += volumes.back().probes.size();
    }

    const CaptureGi field(volumes);
    EnvironmentBakeSettings bias;
    bias.normal_bias = bias.view_bias = .5f;
    std::vector<std::array<FieldSample, 5>> values(receivers.size());

    for (std::size_t i = 0; i < receivers.size(); ++i) {
        const auto& receiver = receivers[i];
        const auto views = probe_density_views(receiver.normal);

        for (unsigned view = 0; view < views.size(); ++view) {
            const auto sample =
                field.sample_result(receiver.position, receiver.normal, views[view], bias);
            values[i][view] = {sample ? sample->irradiance : Float3{}, bool(sample)};
        }
    }

    return values;
}

// A failed receiver can depend on several overlapping reductions; restore all of those together.
std::set<unsigned> failed_regions(
    std::span<const ProbeVolume> volumes,
    const std::set<unsigned>& changed,
    std::span<const ProbeDensityReceiver> receivers,
    std::span<const std::array<FieldSample, 5>> expected,
    const ProbeDensitySettings& settings,
    float& maximum,
    std::size_t& failures
) {
    const CaptureGi field(volumes);
    EnvironmentBakeSettings bias;
    bias.normal_bias = bias.view_bias = .5f;
    std::set<unsigned> rejected;

    for (std::size_t i = 0; i < receivers.size(); ++i) {
        const auto& receiver = receivers[i];
        const auto views = probe_density_views(receiver.normal);

        for (unsigned view = 0; view < views.size(); ++view) {
            const auto sample =
                field.sample_result(receiver.position, receiver.normal, views[view], bias);
            const auto value = sample ? sample->irradiance : Float3{};

            if (expected[i][view].supported == bool(sample)
                && probe_density_matches(expected[i][view].irradiance, value, settings, maximum)) {
                continue;
            }

            ++failures;
            bool found = false;

            for (auto id : changed) {
                if (probe_boundary_weight(volumes[id].grid, receiver.position) > 0) {
                    rejected.insert(id);
                    found = true;
                }
            }

            if (!found) {
                throw std::runtime_error(
                    "adaptive lighting changed outside reduced probe coverage"
                );
            }
        }
    }

    return rejected;
}
}  // namespace

void verify_probe_density_field(
    std::span<const fs::path> paths,
    std::span<const ProbeDensityReceiver> receivers,
    const ProbeDensitySettings& settings,
    std::ostream& output
) {
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
    const auto expected = reference_samples(paths, receivers, original_probes);
    std::vector<ProbeVolume> volumes;

    for (const auto& path : paths) {
        volumes.push_back(read_volume(path));
    }

    const auto candidates = changed.size();
    std::size_t failed = 0, restored = 0;
    float maximum = 0;

    for (;;) {
        maximum = 0;
        const auto rejected =
            failed_regions(volumes, changed, receivers, expected, settings, maximum, failed);

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
        failed,
        maximum,
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
           << restored << " reductions restored, max RGB error " << maximum << '\n'
           << std::flush;
}
}  // namespace openrac::lighting
