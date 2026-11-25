// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enumerates production recipes, compiles unique variants and records one build's work.
#pragma once
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

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

    // One unique program, compiled or read from the cache by the first worker that asks.
    struct Built {
        vkgl::ShaderKey key;      // Package identity of the final source.
        std::vector<char> raw;    // Encoded program, decoded again by later requests.
        std::vector<char> bytes;  // Compressed package entry.
        bool cache_hit = false;   // Read from the build cache instead of compiled.
    };

    // What one recipe asked for, in order; replayed serially so the package and report match a
    // one-thread run whatever order the workers finished in.
    struct Request {
        enum class Type {
            Stage,
            Program,
            Native
        } type;
        vkgl::ShaderKey key;                 // Package key of the stage marker or program.
        std::shared_ptr<const Built> built;  // Program result; empty for stage markers.
        std::string name;                    // Manifest name with its variant label.
        ProgramKind kind = ProgramKind::Graphics;
        std::vector<vkgl::ShaderKey> aliases;  // Vertex fallback, kept only by a first request.
        std::vector<vkgl::ShaderKey> stages;   // Stage markers, kept only by a first request.
    };

    // One recipe's requests and the labels its names carry.
    struct Job {
        std::string variant;            // Capability label for build diagnostics.
        std::vector<Request> requests;  // In the order a one-thread run would make them.
    };

    // Collects all enabled source families without windowing or a graphics context.
    void generate();
    // Attributes recipe requests and elapsed work to one source family.
    void family(
        const std::string& name,
        bool enhanced,
        const std::function<void(renderer::ShaderRecipes&)>& collect
    );
    // Returns the shared result for key, building it once across all workers; a compile error
    // carries the name of the request that sees it.
    std::shared_ptr<const Built> build_once(
        vkgl::ShaderKey key, const std::string& name, const std::function<Built()>& build
    );
    // Reads a cached program, remembering the entry as still in use.
    std::vector<char> read_cache(vkgl::ShaderKey key);
    // Deletes cache entries this run did not use, so the cache does not grow without end.
    void prune_cache() const;
    // Applies one recipe's requests to the package and the report.
    void replay(const std::string& family, const Job& job);
    // Rejects hash collisions before accepting an earlier compilation result.
    bool contains(vkgl::ShaderKey key) const;
    // Adds a shared vertex-layout alias without recompressing the existing result.
    void keep_vertex(vkgl::ShaderKey key, const CompiledVariant& compiled);
    // Records validation of an unmodified source stage for GL-style compile-status queries.
    void keep_stage(vkgl::ShaderKey key);
    // Requests a stage marker for GL-style compile-status queries.
    static void stage(Job& job, std::string_view kind, const std::string& source);
    // Compiles or restores one complete program and retains its reflection with its bytecode.
    vkgl::CompiledProgram program(
        Job& job, std::string_view name, ProgramSources sources, Precision precision
    );
    // Compiles explicit native Vulkan bindings using the pinned embedded SDK includes.
    void native(Job& job, const renderer::ShaderRecipe& recipe);
    // Enumerates feature modes, colour clamps, material buffers and draw-uniform variants.
    void graphics(
        Job& job,
        std::string_view name,
        ProgramSources sources,
        Precision precision,
        bool materials,
        bool clamp = true
    );
    // Enumerates the concrete programs for one arithmetic and clip-distance configuration.
    void precision_recipe(
        Job& job, std::string_view name, const ProgramSources& sources, Precision precision
    );
    // Applies the shared preamble before enumerating supported precision/capability combinations.
    void recipe(Job& job, const renderer::ShaderRecipe& recipe, bool enhanced);

    std::filesystem::path m_output;  // Generated output and cache directory.
    CompiledVariants m_results;      // Owned compressed results in key order.
    std::unordered_map<std::string, std::string> m_includes;  // Owned embedded native SDK headers.
    BuildReport m_report;                                     // Invocation timing and counts.
    std::mutex m_mutex;  // Guards m_built and m_cache_used across workers.
    std::unordered_map<std::uint64_t, std::shared_future<std::shared_ptr<const Built>>> m_built;
    std::set<std::string> m_cache_used;  // Cache file names this run read or wrote.
};
}  // namespace openrac::shader_compiler
