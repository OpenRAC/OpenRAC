// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Build timing and deduplicated variant counts for the compiler's console and persistent log.
#pragma once
#include <chrono>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>

namespace openrac::shader_compiler {
// Executable program categories, counted separately from archive aliases and validation markers.
enum class ProgramKind {
    Graphics,
    Compute,
    NativeCompute
};

// Work attributed to the first source family requesting each distinct program.
struct FamilySummary {
    std::size_t recipes = 0;          // Source requests before variant enumeration.
    std::size_t graphics = 0;         // Unique linked graphics programs.
    std::size_t compute = 0;          // Unique GL-shaped compute programs.
    std::size_t native = 0;           // Unique native Vulkan programs.
    std::size_t cache_hits = 0;       // Programs restored from the build cache.
    std::size_t fresh_compiles = 0;   // Programs compiled with glslang this invocation.
    std::size_t reused_requests = 0;  // Duplicate variant requests served by earlier results.
    double seconds = 0;               // Wall time spent enumerating and compiling this family.

    // Counts executable programs, excluding validation markers and duplicate requests.
    std::size_t variants() const { return graphics + compute + native; }
};

// One invocation's mutable counts and steady-clock timing, owned by its compiler.
class BuildReport {
public:
    using Clock = std::chrono::steady_clock;
    std::map<std::string, FamilySummary> families;  // Deterministically ordered source families.
    double generation_seconds = 0;                  // Enumeration and compilation wall time.
    double packaging_seconds = 0;                   // Serialization and publication wall time.
    std::size_t stage_records = 0;                  // Non-executable stage-validation markers.
    std::size_t package_bytes = 0;                  // Serialized binary size.
    std::size_t vertex_fallback_records = 0;        // Aliases preserving original vertex layouts.

    // Records one successful unique program and whether glslang ran for it.
    void program(const std::string& family, ProgramKind kind, bool cache_hit);
    // Aggregates all family rows without counting validation markers as shader variants.
    FamilySummary total() const;
    // Formats the same summary for stdout and shader-compiler.log, including partial failures.
    std::string format(bool success, std::string_view error = {}) const;
    // Returns wall time since this invocation began.
    double elapsed_seconds() const;

private:
    Clock::time_point m_start = Clock::now();  // Invocation start, excluding building this tool.
};
}  // namespace openrac::shader_compiler
