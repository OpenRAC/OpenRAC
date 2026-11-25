// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compiles and caches unique programs while retaining their package entries and vertex aliases.
#include <stdexcept>
#include <system_error>
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

std::shared_ptr<const Compiler::Built> Compiler::build_once(
    ShaderKey key, const std::function<Built()>& build
) {
    std::promise<std::shared_ptr<const Built>> promise;
    std::shared_future<std::shared_ptr<const Built>> result;
    bool first = false;
    {
        const std::lock_guard lock(m_mutex);
        auto [found, inserted] = m_built.try_emplace(key.key);

        if (inserted) {
            found->second = promise.get_future().share();
            first = true;
        }

        result = found->second;
    }

    if (first) {
        try {
            promise.set_value(std::make_shared<const Built>(build()));
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    }

    auto built = result.get();

    if (built->key != key) {
        throw std::runtime_error("shader package key collision");
    }

    return built;
}

std::vector<char> Compiler::read_cache(ShaderKey key) {
    {
        const std::lock_guard lock(m_mutex);
        m_cache_used.insert(cache_name(key));
    }
    return read_cached_program(m_output, key);
}

void Compiler::prune_cache() const {
    std::error_code error;

    for (const auto& entry : std::filesystem::directory_iterator(m_output / ".cache", error)) {
        if (!m_cache_used.contains(entry.path().filename().string())) {
            std::filesystem::remove(entry.path(), error);
        }
    }
}

void Compiler::stage(Job& job, std::string_view kind, const std::string& source) {
    if (!source.empty()) {
        job.requests.push_back(
            {Request::Type::Stage, shader_key(kind, {source}), {}, {}, {}, {}, {}}
        );
    }
}

void Compiler::keep_stage(ShaderKey key) {
    if (!contains(key)) {
        const char valid = 1;
        m_results.emplace(key.key, CompiledVariant{key, {}, compress_shader_blob({&valid, 1}), 1});
        ++m_report.stage_records;
    }
}

CompiledProgram Compiler::program(
    Job& job, std::string_view name, ProgramSources sources, Precision precision
) {
    auto& [vertex, fragment, compute] = sources;
    const auto [half, clip] = precision;
    vertex = compiler_detail::vertex_clip_distance(
        renderer::shader_precision_source(vertex, half), clip
    );
    fragment = renderer::shader_precision_source(fragment, half);
    compute = renderer::shader_precision_source(compute, half);
    const auto key = shader_key("program", {vertex, fragment, compute});
    const auto label = std::string(name) + " / " + job.variant;
    const auto built = build_once(key, [&] {
        CompiledProgram compiled;
        std::string error;
        const auto cache_key =
            shader_key("program", {vertex, fragment, compute}, compiler_detail::compiler_version());
        const bool cache_hit = decode_program(read_cache(cache_key), compiled);

        if (!cache_hit
            && !compiler_detail::compile_program_fresh(
                vertex, fragment, compiled, error, compute
            )) {
            throw std::runtime_error(label + ": " + error);
        }

        auto bytes = encode_program(compiled);

        if (!cache_hit) {
            cache_program(m_output, cache_key, bytes);
        }

        auto packed = compress_shader_blob(bytes);
        return Built{key, std::move(bytes), std::move(packed), cache_hit};
    });

    Request request{
        Request::Type::Program,
        key,
        built,
        label,
        compute.empty() ? ProgramKind::Graphics : ProgramKind::Compute,
        {},
        {}
    };

    // Alias references share the already-compressed program rather than compiling another vertex.
    if (compute.empty()) {
        request.aliases.push_back(shader_key("error vertex", {vertex}));
    }

    for (const auto& [kind, source] :
         {std::pair{"vertex stage", &vertex},
          {"fragment stage", &fragment},
          {"compute stage", &compute}}) {
        if (!source->empty()) {
            request.stages.push_back(shader_key(kind, {*source}));
        }
    }

    job.requests.push_back(std::move(request));
    CompiledProgram compiled;

    if (!decode_program(built->raw, compiled)) {
        throw std::runtime_error("invalid generated program");
    }

    return compiled;
}

void Compiler::native(Job& job, const renderer::ShaderRecipe& recipe) {
    const auto key = shader_key("native", {recipe.compute, recipe.definitions});
    const auto built = build_once(key, [&] {
        NativeComputeShader compiled;
        std::string error;
        const auto include = [&](std::string_view name) -> std::string_view {
            const auto found = m_includes.find(std::string(name));
            return found == m_includes.end() ? std::string_view{} : found->second;
        };
        const auto cache_key = shader_key(
            "native", {recipe.compute, recipe.definitions}, compiler_detail::compiler_version()
        );
        const bool cache_hit = decode_native_shader(read_cache(cache_key), compiled);

        if (!cache_hit
            && !compiler_detail::compile_native_fresh(
                recipe.compute, recipe.definitions, include, compiled, error
            )) {
            throw std::runtime_error(recipe.name + ": " + error);
        }

        auto bytes = encode_native_shader(compiled);

        if (!cache_hit) {
            cache_program(m_output, cache_key, bytes);
        }

        auto packed = compress_shader_blob(bytes);
        return Built{key, std::move(bytes), std::move(packed), cache_hit};
    });
    job.requests.push_back(
        {Request::Type::Native, key, built, recipe.name, ProgramKind::NativeCompute, {}, {}}
    );
}

void Compiler::replay(const std::string& family, const Job& job) {
    for (const auto& request : job.requests) {
        if (request.type == Request::Type::Stage) {
            keep_stage(request.key);
            continue;
        }

        if (contains(request.key)) {
            ++m_report.families[family].reused_requests;
            continue;
        }

        const auto& built = *request.built;
        m_results.emplace(
            request.key.key,
            CompiledVariant{request.key, request.name, built.bytes, built.raw.size()}
        );
        m_report.program(family, request.kind, built.cache_hit);

        for (const auto& alias : request.aliases) {
            keep_vertex(alias, m_results.at(request.key.key));
        }

        for (const auto& stage : request.stages) {
            keep_stage(stage);
        }
    }
}

// Keeps one complete vertex interface for substitution when a graphics variant is absent.
void Compiler::keep_vertex(ShaderKey key, const CompiledVariant& compiled) {
    if (contains(key)) {
        return;
    }

    m_results.emplace(key.key, CompiledVariant{key, {}, compiled.bytes, compiled.raw_size});
    ++m_report.vertex_fallback_records;
}

}  // namespace openrac::shader_compiler
