// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Recipe enumeration, source transforms and deduplicated CPU compilation.
#include "tools/shader_compiler/compiler.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <format>
#include <stdexcept>
#include <thread>
#include <utility>

#include "renderer/gl/shader_precision.h"
#include "renderer/gl/shader_sources.h"
#include "renderer/vulkan/pipelines/clamped_shader_source.h"
#include "renderer/vulkan/pipelines/draw_uniform_source.h"
#include "renderer/vulkan/pipelines/mode_shader_sources.h"
#include "renderer/vulkan/pipelines/shader_backend.h"
#ifdef OPENRAC_SHADER_FSR3
#include "fsr3_sources.h"
#endif

namespace openrac::shader_compiler {
using namespace vkgl;

namespace {
// Accumulates phase time during both normal completion and exception unwinding.
class ScopedTimer {
public:
    explicit ScopedTimer(double& seconds) : m_seconds(seconds) {}

    ~ScopedTimer() {
        m_seconds += std::chrono::duration<double>(BuildReport::Clock::now() - m_start).count();
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    double& m_seconds;  // The compiler's report outlives its scoped timer.
    BuildReport::Clock::time_point m_start = BuildReport::Clock::now();
};

bool has_colour_output(std::span<const std::uint32_t> code) {
    std::vector<std::uint32_t> located, outputs;

    for (std::size_t at = 5; at < code.size();) {
        const auto words = code[at] >> 16;

        if (!words || words > code.size() - at) {
            return false;
        }

        const auto op = code[at] & 0xffffu;

        if (op == 71 && words >= 4 && code[at + 2] == 30) {
            located.push_back(code[at + 1]);
        }

        if (op == 59 && words >= 4 && code[at + 3] == 3) {
            outputs.push_back(code[at + 2]);
        }

        at += words;
    }

    return std::any_of(outputs.begin(), outputs.end(), [&](auto id) {
        return std::find(located.begin(), located.end(), id) != located.end();
    });
}

}  // namespace

Compiler::Compiler(std::filesystem::path output) : m_output(std::move(output)) {}

void Compiler::graphics(
    Job& job,
    std::string_view name,
    ProgramSources sources,
    Precision precision,
    bool materials,
    bool clamp
) {
    auto& vertex = sources.vertex;
    auto& fragment = sources.fragment;
    auto modes = shader_transforms::mode_sources(vertex, fragment, materials);

    if (!modes.error.empty()) {
        throw std::runtime_error(modes.error);
    }

    if (!modes.names.empty()) {
        vertex = std::move(modes.vertex);
        fragment = std::move(modes.fragment);
    }

    const auto compiled = program(job, name, {vertex, fragment, {}}, precision);

    if (clamp && has_colour_output(compiled.fragment)) {
        const auto clamped = shader_transforms::clamped_fragment_source(fragment);

        if (clamped) {
            graphics(
                job,
                std::string(name) + " / clamped",
                {vertex, *clamped, {}},
                precision,
                materials,
                false
            );
        }
    }

    if (materials && vertex.find("in mat4 model;") != std::string::npos
        && std::any_of(compiled.uniforms.begin(), compiled.uniforms.end(), [](const auto& u) {
               return u.name == "material_colour" || u.name == "shadow_view";
           })) {
        std::string vs, fs, error;

        if (!shader_transforms::material_sources(compiled, vertex, fragment, vs, fs, error)) {
            throw std::runtime_error(error);
        }

        program(job, std::string(name) + " / material-buffer", {vs, fs, {}}, precision);
        return;
    }

    const auto draw_name = shader_transforms::draw_uniform_name(vertex);

    if (const auto draw_vertex =
            shader_transforms::draw_uniform_source(vertex, fragment, draw_name)) {
        program(
            job, std::string(name) + " / draw-uniform", {*draw_vertex, fragment, {}}, precision
        );
    }
}

// Enumerates all material paths for one arithmetic and clip-distance configuration.
void Compiler::precision_recipe(
    Job& job, std::string_view name, const ProgramSources& sources, Precision precision
) {
    const auto& [vs, fs, cs] = sources;
    // Check-stage requests use the original source, before program feature rewriting.
    stage(
        job,
        "vertex stage",
        compiler_detail::vertex_clip_distance(
            renderer::shader_precision_source(vs, precision.half), precision.clip
        )
    );
    stage(job, "fragment stage", renderer::shader_precision_source(fs, precision.half));
    stage(job, "compute stage", renderer::shader_precision_source(cs, precision.half));

    if (!cs.empty()) {
        program(job, name, {{}, {}, cs}, precision);
        return;
    }

    for (bool materials : {false, true}) {
        if (materials && vs.find("in mat4 model;") == std::string::npos) {
            continue;
        }

        graphics(job, name, {vs, fs, {}}, precision, materials);
    }
}

void Compiler::recipe(Job& job, const renderer::ShaderRecipe& recipe, bool enhanced) {
    if (recipe.native) {
        native(job, recipe);
        return;
    }

    const auto source = [&](std::string_view text) {
        return recipe.raw ? std::string(text) : renderer::shader_enhanced_source(text, enhanced);
    };
    const std::string vs = source(recipe.vertex);
    const std::string fs = source(renderer::shader_fragment_source(recipe.fragment, enhanced));
    const std::string cs = source(recipe.compute);

    for (bool half : {false, true}) {
        for (bool clip : {true, false}) {
            job.variant = std::format("enhanced={} fp16={} clip={}", enhanced, half, clip);
            precision_recipe(job, recipe.name, {vs, fs, cs}, {half, clip});
        }
    }
}

std::string Compiler::summary(bool success, std::string_view error) const {
    auto summary = m_report.format(success, error);
    summary += std::format(
        "Compiler: {}\nFingerprint: {}\n",
        compiler_detail::compiler_version(),
        shader_package_fingerprint()
    );
#ifdef OPENRAC_ENHANCED_RENDERING
    summary += "Enhanced rendering: enabled\n";
#else
    summary += "Enhanced rendering: disabled\n";
#endif
#ifdef OPENRAC_SHADER_FSR3
    summary += "FSR: enabled\n";
#else
    summary += "FSR: disabled\n";
#endif
    if (!m_output.empty()) {
        const auto path = m_output / "shader-compiler.log";
        summary += "Log: " + path.string() + "\n";
    }

    return summary;
}

void Compiler::generate() {
#ifdef OPENRAC_SHADER_FSR3
    for (const auto& entry : enhanced::fsr3_detail::kSdkSources) {
        auto& text = m_includes[std::string(entry.name)];

        for (auto chunk : entry.chunks) {
            text += chunk;
        }
    }
#endif

    for (bool enhanced : {false, true}) {
#ifndef OPENRAC_ENHANCED_RENDERING
        if (enhanced) {
            continue;
        }
#endif
        family("Renderer", enhanced, [enhanced](auto& recipes) {
            renderer::renderer_shader_recipes(recipes, enhanced);
        });
        family("Scene", enhanced, [enhanced](auto& recipes) {
            renderer::scene_shader_recipes(recipes, enhanced);
        });

        if (enhanced) {
            family("Enhanced", enhanced, renderer::enhanced_shader_recipes);
        }

        family("Native", enhanced, renderer::native_shader_recipes);

        if (enhanced) {
            family("FSR", enhanced, renderer::fsr_shader_recipes);
        }
    }
}

void Compiler::family(
    const std::string& name,
    bool enhanced,
    const std::function<void(renderer::ShaderRecipes&)>& collect
) {
    auto& counts = m_report.families[name];
    const ScopedTimer timer(counts.seconds);
    renderer::ShaderRecipes recipes;
    collect(recipes);
    const auto& entries = recipes.entries;
    counts.recipes += entries.size();

    // Workers take recipes in turn; each unique program compiles once (build_once). The requests
    // are then applied in recipe order, so the output is the same for any number of threads.
    std::vector<Job> jobs(entries.size());
    std::vector<std::exception_ptr> errors(entries.size());
    std::atomic<std::size_t> next = 0;
    const auto work = [&] {
        for (std::size_t at; (at = next++) < entries.size();) {
            try {
                recipe(jobs[at], entries[at], enhanced);
            } catch (...) {
                errors[at] = std::current_exception();
            }
        }
    };
    std::vector<std::thread> workers;

    for (unsigned n = 1; n < std::max(1u, std::thread::hardware_concurrency()); ++n) {
        workers.emplace_back(work);
    }

    work();

    for (auto& worker : workers) {
        worker.join();
    }

    for (std::size_t at = 0; at < entries.size(); ++at) {
        replay(name, jobs[at]);

        if (errors[at]) {
            std::rethrow_exception(errors[at]);
        }
    }
}

void Compiler::run() {
    try {
        generate();
    } catch (...) {
        m_report.generation_seconds = m_report.elapsed_seconds();
        throw;
    }

    m_report.generation_seconds = m_report.elapsed_seconds();
    prune_cache();
    const ScopedTimer timer(m_report.packaging_seconds);
    m_report.package_bytes = write_package(m_output, m_results);
}
}  // namespace openrac::shader_compiler
