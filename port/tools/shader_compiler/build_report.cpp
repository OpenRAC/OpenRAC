// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The compiler's summary, with program kinds, cache work and timing attributed to source families.
#include "tools/shader_compiler/build_report.h"

#include <format>

namespace openrac::shader_compiler {
namespace {
// Formats one row per source family with the same columns as the invocation totals.
std::string family_table(const std::map<std::string, FamilySummary>& families) {
    std::string out = std::format(
        "{:<12} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>10}\n",
        "Family",
        "Recipes",
        "Variants",
        "Graphics",
        "Compute",
        "Native",
        "Cached",
        "Fresh",
        "Seconds"
    );

    for (const auto& [name, row] : families) {
        out += std::format(
            "{:<12} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>10.3f}\n",
            name,
            row.recipes,
            row.variants(),
            row.graphics,
            row.compute,
            row.native,
            row.cache_hits,
            row.fresh_compiles,
            row.seconds
        );
    }

    return out;
}
}  // namespace

void BuildReport::program(const std::string& family, ProgramKind kind, bool cache_hit) {
    auto& row = families[family];

    switch (kind) {
        case ProgramKind::Graphics:
            ++row.graphics;
            break;
        case ProgramKind::Compute:
            ++row.compute;
            break;
        case ProgramKind::NativeCompute:
            ++row.native;
            break;
    }

    if (cache_hit) {
        ++row.cache_hits;
    } else {
        ++row.fresh_compiles;
    }
}

FamilySummary BuildReport::total() const {
    FamilySummary out;

    for (const auto& [name, row] : families) {
        out.recipes += row.recipes;
        out.graphics += row.graphics;
        out.compute += row.compute;
        out.native += row.native;
        out.cache_hits += row.cache_hits;
        out.fresh_compiles += row.fresh_compiles;
        out.reused_requests += row.reused_requests;
        out.seconds += row.seconds;
    }

    return out;
}

double BuildReport::elapsed_seconds() const {
    return std::chrono::duration<double>(Clock::now() - m_start).count();
}

std::string BuildReport::format(bool success, std::string_view error) const {
    const auto counts = total();
    std::string out = std::format(
        "OpenRAC shader compiler - {}\n"
        "Elapsed: {:.3f} s\n"
        "  Shader generation: {:.3f} s\n"
        "  Packaging and output: {:.3f} s\n"
        "Recipe requests: {}\n"
        "Unique program variants: {} (graphics {}, compute {}, native compute {})\n"
        "Stage validation records: {}\n"
        "Vertex fallback records: {}\n"
        "Cache hits: {}; freshly compiled: {}; duplicate program requests reused: {}\n\n",
        success ? "SUCCEEDED" : "FAILED",
        elapsed_seconds(),
        generation_seconds,
        packaging_seconds,
        counts.recipes,
        counts.variants(),
        counts.graphics,
        counts.compute,
        counts.native,
        stage_records,
        vertex_fallback_records,
        counts.cache_hits,
        counts.fresh_compiles,
        counts.reused_requests
    );
    out += family_table(families);

    out += "\nShared programs are counted once, under the first family that requests them.\n";

    if (success) {
        out += std::format(
            "Package: {} entries, {} bytes ({:.2f} MiB)\n",
            counts.variants() + stage_records + vertex_fallback_records,
            package_bytes,
            static_cast<double>(package_bytes) / (1024 * 1024)
        );
    } else {
        out += "Error: ";
        out += error;
        out += '\n';
    }

    return out;
}
}  // namespace openrac::shader_compiler
