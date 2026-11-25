// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compiles and caches unique programs while retaining their package entries and vertex aliases.
#include <stdexcept>
#include <utility>

#include "renderer/gl/shader_precision.h"
#include "renderer/vulkan/pipelines/shader_backend.h"
#include "tools/shader_compiler/compiler.h"
#include "tools/shader_compiler/file_io.h"

namespace openrac::shader_compiler {
using namespace vkgl;

bool Compiler::contains(ShaderKey key) const {
    const auto found = m_results.find(key.key);

    if (found == m_results.end()) {
        return false;
    }

    if (found->second.key != key) {
        throw std::runtime_error("shader package key collision");
    }

    return true;
}

void Compiler::stage(std::string_view kind, const std::string& source) {
    if (source.empty()) {
        return;
    }

    const auto key = shader_key(kind, {source});

    if (!contains(key)) {
        const char valid = 1;
        m_results.emplace(key.key, CompiledVariant{key, {}, compress_shader_blob({&valid, 1}), 1});
        ++m_report.stage_records;
    }
}

CompiledProgram Compiler::program(
    std::string_view name, ProgramSources sources, Precision precision
) {
    auto& [vertex, fragment, compute] = sources;
    const auto [half, clip] = precision;
    vertex = compiler_detail::vertex_clip_distance(
        renderer::shader_precision_source(vertex, half), clip
    );
    fragment = renderer::shader_precision_source(fragment, half);
    compute = renderer::shader_precision_source(compute, half);
    const auto key = shader_key("program", {vertex, fragment, compute});
    CompiledProgram compiled;

    if (contains(key)) {
        ++m_report.families[m_family].reused_requests;
        const auto& kept = m_results.at(key.key);

        if (!decode_program(decompress_shader_blob(kept.bytes, kept.raw_size), compiled)) {
            throw std::runtime_error("invalid generated program");
        }

        return compiled;
    }

    std::string error;
    const auto cache_key =
        shader_key("program", {vertex, fragment, compute}, compiler_detail::compiler_version());
    const auto cached_bytes = read_cached_program(m_output, cache_key);
    const bool cache_hit = decode_program(cached_bytes, compiled);

    if (!cache_hit
        && !compiler_detail::compile_program_fresh(vertex, fragment, compiled, error, compute)) {
        throw std::runtime_error(std::string(name) + " / " + m_variant + ": " + error);
    }

    const auto bytes = encode_program(compiled);

    if (!cache_hit) {
        cache_program(m_output, cache_key, bytes);
    }

    m_results.emplace(
        key.key,
        CompiledVariant{
            key, std::string(name) + " / " + m_variant, compress_shader_blob(bytes), bytes.size()
        }
    );
    m_report.program(
        m_family, compute.empty() ? ProgramKind::Graphics : ProgramKind::Compute, cache_hit
    );

    // Alias references share the already-compressed program rather than compiling another vertex.
    if (compute.empty()) {
        keep_vertex(vertex, m_results.at(key.key));
    }

    stage("vertex stage", vertex);
    stage("fragment stage", fragment);
    stage("compute stage", compute);
    return compiled;
}

void Compiler::native(const renderer::ShaderRecipe& recipe) {
    const auto key = shader_key("native", {recipe.compute, recipe.definitions});

    if (contains(key)) {
        ++m_report.families[m_family].reused_requests;
        return;
    }

    NativeComputeShader compiled;
    std::string error;
    const auto include = [&](std::string_view name) -> std::string_view {
        const auto found = m_includes.find(std::string(name));
        return found == m_includes.end() ? std::string_view{} : found->second;
    };
    const auto cache_key = shader_key(
        "native", {recipe.compute, recipe.definitions}, compiler_detail::compiler_version()
    );
    const auto cached_bytes = read_cached_program(m_output, cache_key);
    const bool cache_hit = decode_native_shader(cached_bytes, compiled);

    if (!cache_hit
        && !compiler_detail::compile_native_fresh(
            recipe.compute, recipe.definitions, include, compiled, error
        )) {
        throw std::runtime_error(recipe.name + ": " + error);
    }

    const auto bytes = encode_native_shader(compiled);

    if (!cache_hit) {
        cache_program(m_output, cache_key, bytes);
    }

    m_results.emplace(
        key.key, CompiledVariant{key, recipe.name, compress_shader_blob(bytes), bytes.size()}
    );
    m_report.program(m_family, ProgramKind::NativeCompute, cache_hit);
}

// Keeps one complete vertex interface for substitution when a graphics variant is absent.
void Compiler::keep_vertex(std::string_view vertex, const CompiledVariant& compiled) {
    const auto key = shader_key("error vertex", {vertex});

    if (contains(key)) {
        return;
    }

    m_results.emplace(key.key, CompiledVariant{key, {}, compiled.bytes, compiled.raw_size});
    ++m_report.vertex_fallback_records;
}

}  // namespace openrac::shader_compiler
