// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic constant fields exercise reference swapping, restoration and returned reflection inputs.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "enhanced/tools/probes/density_stage.h"
#include "enhanced/tools/probes/report.h"
#include "tests/check.h"

namespace {
using namespace openrac::lighting;
namespace fs = std::filesystem;

// One isolated scratch directory; only the test controller touches its files.
class TestDirectory {
public:
    // Uses a unique suffix so concurrent test processes cannot share reference files.
    TestDirectory() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_path = fs::temp_directory_path() / ("openrac-density-stage-" + std::to_string(stamp));
        fs::create_directories(m_path / "adaptive-reference");
    }

    // Cleans up test-owned files without throwing during failure unwinding.
    ~TestDirectory() {
        std::error_code error;
        fs::remove_all(m_path, error);
    }

    TestDirectory(const TestDirectory&) = delete;
    TestDirectory& operator=(const TestDirectory&) = delete;
    TestDirectory(TestDirectory&&) = delete;
    TestDirectory& operator=(TestDirectory&&) = delete;

    // Returns the directory owned until this fixture is destroyed.
    const fs::path& path() const { return m_path; }

private:
    fs::path m_path;  // Scratch outputs owned by this fixture.
};

// Fills linear irradiance and unoccluded moments with analytically constant values.
ProbeVolume constant_field(const ProbeGrid& grid, float irradiance) {
    auto volume = ProbeVolume::allocate(grid);

    // Every capture supports the same field; only the test's reduced value can create an error.
    for (auto& probe : volume.probes) {
        probe.valid = true;
    }

    std::fill(
        volume.irradiance.begin(),
        volume.irradiance.end(),
        Float3{irradiance, irradiance, irradiance}
    );
    return volume;
}

// Writes the same encoded fields and fine-reference report that the batch stage owns.
std::vector<fs::path> write_fields(const fs::path& output, float candidate) {
    ProbeGrid coarse;
    coarse.origin = {-16, -16, -16};
    coarse.spacing = {32, 32, 32};
    coarse.dimensions = {3, 3, 3};
    coarse.map_resolution = 2;
    ProbeGrid fine = coarse;
    fine.origin = {};
    fine.spacing = {4, 4, 4};
    fine.dimensions = {5, 5, 5};
    ProbeGrid reduced = fine;
    reduced.spacing[0] *= 2;
    reduced.dimensions[0] = 3;
    std::vector<fs::path> paths{output / "sparse.orpg", output / "region-000.orpg"};
    write_bake_file(paths[0], encode_probes(constant_field(coarse, 1)));
    write_bake_file(paths[1], encode_probes(constant_field(reduced, candidate)));
    const auto reference = output / "adaptive-reference" / paths[1].filename();
    write_bake_file(reference, encode_probes(constant_field(fine, 1)));
    std::ofstream report(reference.string() + ".txt");
    report << "fine reference\n";
    CHECK(bool(report));
    return paths;
}

// A valid reduction keeps its reduced grid and returns precisely the field used by validation.
void test_reuses_candidate_field() {
    TestDirectory directory;
    const auto paths = write_fields(directory.path(), 1);
    const std::array<bake_detail::ProbeDensityReceiver, 1> receivers{{{{8, 8, 5}, {0, 0, 1}}}};
    std::ostringstream report;
    const auto volumes = verify_probe_density_field(paths, receivers, {}, report, {1});
    CHECK(volumes.size() == 2);
    CHECK(volumes[1].grid.dimensions[0] == 3);
    CHECK(volumes[1].gpu_bytes.capacity() == 0);
    CHECK(volumes[1].irradiance[0] == Float3({1, 1, 1}));
    CHECK(report.str().find("0 reductions restored") != std::string::npos);
    CHECK(fs::exists(directory.path() / "adaptive-reference" / paths[1].filename()));
}

// A rejected candidate returns the restored fine field, including its original bounds and texels.
void test_returns_restored_field() {
    TestDirectory directory;
    const auto paths = write_fields(directory.path(), 8);
    const std::array<bake_detail::ProbeDensityReceiver, 1> receivers{{{{8, 8, 5}, {0, 0, 1}}}};
    std::ostringstream report;
    const auto volumes = verify_probe_density_field(paths, receivers, {}, report, {1});
    CHECK(volumes.size() == 2);
    CHECK(volumes[1].grid.dimensions[0] == 5);
    CHECK(volumes[1].irradiance[0] == Float3({1, 1, 1}));
    CHECK(report.str().find("1 reductions restored") != std::string::npos);
    CHECK(!fs::exists(directory.path() / "adaptive-reference" / paths[1].filename()));
}
}  // namespace

int main() {
    // Scratch-file and format failures are reported after fixture owners clean up their files.
    try {
        test_reuses_candidate_field();
        test_returns_restored_field();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
