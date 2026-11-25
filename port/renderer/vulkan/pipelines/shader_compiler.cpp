// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Runtime package lookup, development compilation and local SPIR-V cache.
#include "renderer/vulkan/pipelines/shader_compiler.h"

#include <atomic>
#include <chrono>
#include <format>

#include "renderer/gl/shader_precision.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/cache/spirv_cache_file.h"
#include "renderer/vulkan/pipelines/error_shader.h"
#include "renderer/vulkan/pipelines/shader_backend.h"
#include "renderer/vulkan/pipelines/shader_package.h"
#include "renderer/vulkan/pipelines/spirv_precision.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace {
bool clip_distance() {
    return !detail::g
           || (detail::g->features.shaderClipDistance && detail::g->properties.vendorID != 0x13B5);
}

class CompileCount {
public:
    ~CompileCount() {
        if (detail::g) {
            ++detail::g->submission.shader_compiles;
            detail::g->submission.shader_compile_s += detail::seconds_since(m_start);
        }
    }

private:
    detail::Clock::time_point m_start = detail::Clock::now();
};

// ---- The SPIR-V cache (R-12) ----

// Returns the open context's SPIR-V cache when the cache is on and kept; null otherwise.
SpirvCache* spirv_cache() {
    // Without a context, or with the entry off, every compile runs glslang.
    if (detail::g == nullptr
        || !renderer::render_opt::enabled(renderer::render_opt::Opt::SpirvDiskCache)) {
        return nullptr;
    }

    SpirvCache& cache = detail::g->spirv_cache;
    return cache.active() ? &cache : nullptr;
}

// Returns whether the stage of `kind` and `source` compiled before: a kept stage is one that did,
// so its log was empty of errors and nothing is left to say.
bool stage_cached(std::string_view kind, std::string_view source) {
    if (!builtin_shader_package().find(shader_key(kind, {source})).empty()) {
        return true;
    }
    if (stats::probe("OPENRAC_SHADER_PACKAGE_STRICT", "Refuse runtime Vulkan shader compilation")) {
        return false;
    }
    const SpirvCache* cache = spirv_cache();

    // No cache: the stage is compiled.
    if (cache == nullptr) {
        return false;
    }

    const ShaderKey text = shader_key(kind, {source});
    return cache->find(text.key, text.check, text.length) != nullptr;
}

// Keeps that the stage of `kind` and `source` compiled.
void keep_stage(std::string_view kind, std::string_view source) {
    SpirvCache* cache = spirv_cache();

    // No cache to keep it in.
    if (cache == nullptr) {
        return;
    }

    const ShaderKey text = shader_key(kind, {source});
    cache->insert({text.key, text.check, text.length, {}});
}

}  // namespace

std::string spirv_compiler_version() {
    return shader_package_fingerprint();
}

bool check_stage(bool vertex, std::string_view source, std::string& log) {
    const auto precise_source = renderer::shader_precision_source(
        source,
        detail::g && detail::g->shader_float16
            && renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderFloat16)
    );
    const auto clipped_source =
        vertex ? compiler_detail::vertex_clip_distance(precise_source, clip_distance())
               : precise_source;
    source = clipped_source;
    const std::string_view kind = vertex ? "vertex stage" : "fragment stage";

    // Compiled before: it compiles.
    if (stage_cached(kind, source)) {
        return true;
    }

    if (stats::probe("OPENRAC_SHADER_PACKAGE_STRICT", "Refuse runtime Vulkan shader compilation")) {
        log = "missing packaged Vulkan shader stage";
        return false;
    }
    // Vulkan links complete programs; let an unknown graphics stage reach the error linker.
    if (detail::g && (!detail::g->compile_missing_shaders || !OPENRAC_SHADER_RUNTIME_COMPILER)
        && error_shader_available()) {
        log = "missing packaged Vulkan shader stage; the program will use the error checkerboard";
        return true;
    }
#if OPENRAC_SHADER_RUNTIME_COMPILER
    const CompileCount counted;
    const bool ok = compiler_detail::check_stage_fresh(vertex, source, log);
#else
    log = "missing packaged Vulkan shader stage";
    const bool ok = false;
#endif

    // Only a stage that compiled is kept; a failure is compiled again for its log.
    if (ok) {
        keep_stage(kind, source);
    }

    return ok;
}

bool check_compute_stage(std::string_view source, std::string& log) {
    const auto precise_source = renderer::shader_precision_source(
        source,
        detail::g && detail::g->shader_float16
            && renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderFloat16)
    );
    source = precise_source;
    constexpr std::string_view kKind = "compute stage";

    // Compiled before: it compiles.
    if (stage_cached(kKind, source)) {
        return true;
    }

    if (stats::probe("OPENRAC_SHADER_PACKAGE_STRICT", "Refuse runtime Vulkan shader compilation")) {
        log = "missing packaged Vulkan shader stage";
        return false;
    }
#if OPENRAC_SHADER_RUNTIME_COMPILER
    const CompileCount counted;
    const bool ok = compiler_detail::check_compute_stage_fresh(source, log);
#else
    log = "missing packaged Vulkan shader stage";
    const bool ok = false;
#endif

    // Only a stage that compiled is kept.
    if (ok) {
        keep_stage(kKind, source);
    }

    return ok;
}

bool compile_program(
    std::string_view vertex,
    std::string_view fragment,
    CompiledProgram& out,
    std::string& log,
    std::string_view compute
) {
    const bool half = detail::g && detail::g->shader_float16
                      && renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderFloat16);
    const auto precise_vertex = compiler_detail::vertex_clip_distance(
        renderer::shader_precision_source(vertex, half), clip_distance()
    );
    const auto precise_fragment = renderer::shader_precision_source(fragment, half);
    const auto precise_compute = renderer::shader_precision_source(compute, half);
    vertex = precise_vertex;
    fragment = precise_fragment;
    compute = precise_compute;
    const auto audit = [&] {
        if (!detail::g) {
            return;
        }
        for (const auto* module : {&out.vertex, &out.fragment, &out.compute}) {
            const auto precision = spirv_precision(*module);
            detail::g->submission.shader_fp16_ops += precision.fp16;
            detail::g->submission.shader_fp32_ops += precision.fp32;
            detail::g->submission.shader_fp16_modules += precision.fp16 != 0;
        }
    };
    SpirvCache* cache = spirv_cache();
    const ShaderKey text = shader_key("program", {vertex, fragment, compute});

    {
        const auto packaged = builtin_shader_package().find(text);
        if (!packaged.empty() && decode_program(packaged, out)) {
            if (detail::g) {
                std::atomic_ref(detail::g->submission.shader_package_hits).fetch_add(1);
            }
            audit();
            return true;
        }
    }
    if (detail::g) {
        std::atomic_ref(detail::g->submission.shader_package_misses).fetch_add(1);
    }
    if (stats::probe("OPENRAC_SHADER_PACKAGE_STRICT", "Refuse runtime Vulkan shader compilation")) {
        log = std::format("missing packaged Vulkan shader variant {:016x}", text.key);
        openrac::log::error("{}", log);
        return false;
    }
    // Missing graphics stay visible even in development builds, before either writable cache.
    if (compute.empty() && detail::g
        && (!detail::g->compile_missing_shaders || !OPENRAC_SHADER_RUNTIME_COMPILER)
        && load_error_shader(vertex, out)) {
        if (detail::g->missing_shader_keys.insert(text.key).second) {
            log = std::format(
                "missing packaged Vulkan shader variant {:016x}; using magenta/black error "
                "shader{}",
                text.key,
                out.fallback == GraphicsFallback::Fullscreen
                    ? " (unknown vertex shader; viewport fallback)"
                    : ""
            );
            openrac::log::error("{}", log);
        } else {
            log.clear();
        }
        audit();
        return true;
    }
    // Compiled before, and its encoding whole: glslang does not run.
    if (cache != nullptr) {
        // The kept program, unless its bytes are not one (it is then compiled and replaced).
        if (const std::vector<char>* kept = cache->find(text.key, text.check, text.length);
            kept != nullptr && decode_program(*kept, out)) {
            audit();
            return true;
        }
    }

#if OPENRAC_SHADER_RUNTIME_COMPILER
    const CompileCount counted;
    const bool ok = compiler_detail::compile_program_fresh(vertex, fragment, out, log, compute);

    // Kept for the next link of the same program, here or in a later run.
    if (ok && cache != nullptr) {
        cache->insert({text.key, text.check, text.length, encode_program(out)});
    }

    if (ok) {
        audit();
    }
    return ok;
#else
    log = std::format(
        "missing packaged Vulkan shader variant {:016x} ({} source bytes)", text.key, text.length
    );
    return false;
#endif
}

}  // namespace openrac::vkgl
