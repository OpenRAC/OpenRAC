// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Build summaries count unique executable variants independently of recipe requests and stage markers.
#include <cstdio>
#include <exception>

#include "tests/check.h"
#include "tools/shader_compiler/build_report.h"

namespace {
// Runs the invocation while the executable boundary retains exception reporting.
int run_tests() {
    using namespace openrac::shader_compiler;
    BuildReport report;
    report.families["Renderer"].recipes = 4;
    report.families["Renderer"].reused_requests = 2;
    report.program("Renderer", ProgramKind::Graphics, true);
    report.program("Renderer", ProgramKind::Compute, false);
    report.families["FSR"].recipes = 1;
    report.program("FSR", ProgramKind::NativeCompute, true);
    report.stage_records = 7;
    report.vertex_fallback_records = 2;
    report.package_bytes = 1024;
    const auto counts = report.total();
    CHECK(counts.variants() == 3);
    CHECK(counts.recipes == 5);
    CHECK(counts.cache_hits == 2 && counts.fresh_compiles == 1);
    CHECK(counts.reused_requests == 2);
    const auto success = report.format(true);
    CHECK(success.find("SUCCEEDED") != std::string::npos);
    CHECK(success.find("Unique program variants: 3") != std::string::npos);
    CHECK(success.find("12 entries, 1024 bytes") != std::string::npos);
    CHECK(success.find("Elapsed:") != std::string::npos);
    const auto failure = report.format(false, "mesh variant failed");
    CHECK(failure.find("FAILED") != std::string::npos);
    CHECK(failure.find("mesh variant failed") != std::string::npos);
    CHECK(failure.find("Package:") == std::string::npos);
    return openrac::test::result();
}
}  // namespace

int main() {
    try {
        return run_tests();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "unexpected shader test exception: %s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected shader test exception\n");
        return 1;
    }
}
