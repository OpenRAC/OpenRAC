// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Source composition inputs shared by runtime lighting and the CPU-only shader compiler.

#pragma once
#include <string>

namespace openrac::lighting::shader_sources {
// Explicit source choices; build recipes enumerate these without reading runtime settings.
struct Options {
    bool enhanced = true;                // Enhanced material declarations.
    bool vulkan = true;                  // Vulkan-compatible probe selection code.
    bool water_local = true;             // Local water displacement and lighting support.
    bool gather = true;                  // Probe texture-gather support.
    bool filter_reference = false;       // Reference probe filtering.
    bool zero_weight_reference = false;  // Reference handling for zero probe weights.
};

// Reads the active renderer's supported options and opt-in diagnostic references.
Options runtime_options();
// Returns immutable shared local-light declarations, retained for the process lifetime.
const std::string& local_light_source();
// Returns probe, local-light and environment sampling declarations.
std::string sampling_source();
// Adds capability-selected probe declarations and helper implementations.
std::string probe_shader_source(std::string source, const Options& options);
// Assembles mesh, early-lighting, atmosphere, water and motion entry points from shared sources.
std::string probe_mesh_fragment(std::string source, const Options& options);
std::string probe_early_fragment(std::string source, const Options& options);
std::string atmosphere_fragment(std::string source, const Options& options);
std::string water_vertex(std::string source, const Options& options);
std::string mesh_vertex(std::string source, const Options& options);
std::string mesh_fragment(std::string source, const Options& options);
std::string motion_fragment(std::string source, const Options& options);
std::string early_fragment(std::string source, const Options& options);
}  // namespace openrac::lighting::shader_sources
