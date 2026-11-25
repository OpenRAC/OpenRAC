// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enumerates production recipes, compiles unique variants and records one build's work.
#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

#include "renderer/gl/shader_recipe.h"
#include "tools/shader_compiler/build_report.h"
#include "tools/shader_compiler/package_output.h"

namespace openrac::shader_compiler {
// Owns one build's source assemblies, deduplicated compilation results and publication report.
class Compiler {
public:
    // Keeps the output directory and its compilation cache for a synchronous invocation.
    explicit Compiler(std::filesystem::path output);
    // Generates and publishes the package; errors leave partial counts for summary().
    void run();
    // Returns the current report, including partial work when compilation failed.
    [[nodiscard]] std::string summary(bool success, std::string_view error = {}) const;

private:
    // Source choices whose combinations must be packaged before a device is known.
    struct Precision {
        bool half;  // FP16 arithmetic capability and preference.
        bool clip;  // Native vertex clip-distance output support.
    };

    // Owns the stages while source transforms build further variants.
    struct ProgramSources {
        std::string vertex;    // Graphics vertex text, empty for compute.
        std::string fragment;  // Graphics fragment text, empty for compute.
        std::string compute;   // Compute text, empty for graphics.
    };

    // Collects all enabled source families without windowing or a graphics context.
    void generate();
    // Attributes recipe requests and elapsed work to one source family.
    void family(
        const std::string& name,
        bool enhanced,
        const std::function<void(renderer::ShaderRecipes&)>& collect
    );
    // Rejects hash collisions before accepting an earlier compilation result.
    bool contains(vkgl::ShaderKey key) const;
    // Adds a shared vertex-layout alias without recompressing the existing result.
    void keep_vertex(std::string_view vertex, const CompiledVariant& compiled);
    // Records validation of an unmodified source stage for GL-style compile-status queries.
    void stage(std::string_view kind, const std::string& source);
    // Compiles or restores one complete program and retains its reflection with its bytecode.
    vkgl::CompiledProgram program(
        std::string_view name, ProgramSources sources, Precision precision
    );
    // Compiles explicit native Vulkan bindings using the pinned embedded SDK includes.
    void native(const renderer::ShaderRecipe& recipe);
    // Enumerates feature modes, colour clamps, material buffers and draw-uniform variants.
    void graphics(
        std::string_view name,
        ProgramSources sources,
        Precision precision,
        bool materials,
        bool clamp = true
    );
    // Enumerates the concrete programs for one arithmetic and clip-distance configuration.
    void precision_recipe(
        std::string_view name, const ProgramSources& sources, Precision precision
    );
    // Applies the shared preamble before enumerating supported precision/capability combinations.
    void recipe(const renderer::ShaderRecipe& recipe, bool enhanced);

    std::filesystem::path m_output;  // Generated output and cache directory.
    CompiledVariants m_results;      // Owned compressed results in key order.
    std::unordered_map<std::string, std::string> m_includes;  // Owned embedded native SDK headers.
    std::string m_variant;  // Capability label for build diagnostics.
    std::string m_family;   // Current report attribution.
    BuildReport m_report;   // Invocation timing and counts.
};
}  // namespace openrac::shader_compiler
