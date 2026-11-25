// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Context-free GLSL translation and reflection shared by the build tool and development fallback.

#include "renderer/vulkan/pipelines/shader_backend.h"

#include <array>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <regex>
#include <string>

#include "renderer/gl/shader_precision.h"
#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "renderer/vulkan/pipelines/shader_compiler_internal.h"
#include "renderer/vulkan/pipelines/shader_specialization.h"
#include "renderer/vulkan/pipelines/spirv_precision.h"
#include "shader_backend_version.h"

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/build_info.h>
// After the public headers it needs: the GLSL resolver, which gives a uniform both stages use one
// binding (the default mapper numbers each stage's from zero).
// clang-format off

#include <glslang/MachineIndependent/localintermediate.h>

#include <glslang/MachineIndependent/iomapper.h>
// clang-format on

namespace openrac::vkgl {
namespace compiler_detail {

void initialise() {
    static std::once_flag once;
    std::call_once(once, [] { glslang::InitializeProcess(); });
}

std::string compiler_version() {
    return std::format(
               "glslang {}.{}.{}{} options 2",
               GLSLANG_VERSION_MAJOR,
               GLSLANG_VERSION_MINOR,
               GLSLANG_VERSION_PATCH,
               GLSLANG_VERSION_FLAVOR
           )
           + " " OPENRAC_SHADER_BACKEND_VERSION;
}

namespace {
// The vertex stage's depth from OpenGL's clip space (-w..w) to Vulkan's (0..w), after the
// shader's own main(), which may set its points' size over the default; left as it is where the
// pipeline says its z is Vulkan's already (glClipControl GL_ZERO_TO_ONE: the specialisation
// constant kClipZeroToOneConstant, vkgl_pipelines.cpp).
std::string vertex_source(std::string_view source) {
    static const std::regex main_decl(R"(\bvoid\s+main\s*\(\s*(void)?\s*\))");
    std::string out = std::regex_replace(
        std::string(source),
        main_decl,
        "void openrac_vk_main()",
        std::regex_constants::format_first_only
    );
    // A point's size is the shader's to say in Vulkan; OpenGL's is 1 unless the shader says.
    out += "\nlayout(constant_id = " + std::to_string(detail::kClipZeroToOneConstant)
           + ") const bool openrac_vk_clip_zero_to_one = false;\n"
             "void main() {\n    gl_PointSize = 1.0;\n    openrac_vk_main();\n"
             "    if (!openrac_vk_clip_zero_to_one) {\n"
             "        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;\n    }\n}\n";
    return out;
}

// A stage and its source: glslang keeps pointers to the text and to the array of its strings, not
// copies, until it is parsed; they live beside the shader, where moving the Stage does not move them.
struct Stage {
    struct Text {
        std::string source;
        const char* pointer = nullptr;
        int length = 0;
    };

    std::unique_ptr<Text> text;
    std::unique_ptr<glslang::TShader> shader;

    glslang::TShader* operator->() const { return shader.get(); }

    glslang::TShader& operator*() const { return *shader; }

    glslang::TShader* get() const { return shader.get(); }
};

Stage make_shader(EShLanguage stage, std::string source) {
    // Shared sources keep GL's 4.1 preamble; Vulkan accepts their 4.5 features, including
    // non-constant local const initializers used by water optics in FP32.
    if (source.starts_with("#version 410 core")) {
        source.replace(0, 17, "#version 450 core");
    }

    Stage out{std::make_unique<Stage::Text>(), std::make_unique<glslang::TShader>(stage)};
    out.text->source = std::move(source);
    out.text->pointer = out.text->source.c_str();
    out.text->length = static_cast<int>(out.text->source.size());
    glslang::TShader* shader = out.get();
    shader->setStringsWithLengths(&out.text->pointer, &out.text->length, 1);
    shader->setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    // Vertex viewport outputs use the Vulkan 1.2 / SPIR-V 1.5 core
    // capabilities, rather than requiring the older device extension.
    const bool viewport_output =
        out.text->source.find("GL_ARB_shader_viewport_layer_array") != std::string::npos;
    shader->setEnvClient(
        glslang::EShClientVulkan,
        viewport_output ? glslang::EShTargetVulkan_1_2 : glslang::EShTargetVulkan_1_1
    );
    shader->setEnvTarget(
        glslang::EShTargetSpv,
        viewport_output ? glslang::EShTargetSpv_1_5 : glslang::EShTargetSpv_1_3
    );
    shader->setEnvInputVulkanRulesRelaxed();
    shader->setGlobalUniformBlockName("gl_DefaultUniformBlock");
    shader->setGlobalUniformSet(0);
    shader->setAutoMapBindings(true);
    shader->setAutoMapLocations(true);
    return out;
}

bool parse(glslang::TShader& shader, std::string& log) {
    if (!shader.parse(
            GetDefaultResources(), 410, ECoreProfile, false, false, compiler_detail::kMessages
        )) {
        log += shader.getInfoLog();
        log += shader.getInfoDebugLog();
        return false;
    }

    return true;
}

// Links shared bindings and reflects only after all shader stages parse successfully.
bool link_program(glslang::TProgram& program, EShLanguage stage, std::string& log) {
    // Bindings across both stages: a uniform both use has one, the stages' others their own.
    if (!program.link(compiler_detail::kMessages)) {
        log += program.getInfoLog();
        log += program.getInfoDebugLog();
        return false;
    }

    const std::unique_ptr<glslang::TIoMapper> mapper(glslang::GetGlslIoMapper());
    glslang::TDefaultGlslIoResolver resolver(*program.getIntermediate(stage));

    if (!program.mapIO(&resolver, mapper.get())) {
        log += program.getInfoLog();
        log += program.getInfoDebugLog();
        return false;
    }

    if (!program.buildReflection(EShReflectionDefault | EShReflectionSeparateBuffers)) {
        log += "could not reflect the program's uniforms";
        return false;
    }

    return true;
}
}  // namespace

// check_stage without the cache: glslang parses the stage.
bool check_stage_fresh(bool vertex, std::string_view source, std::string& log) {
    compiler_detail::initialise();
    const EShLanguage stage = vertex ? EShLangVertex : EShLangFragment;
    auto shader = make_shader(stage, vertex ? vertex_source(source) : std::string(source));
    return parse(*shader, log);
}

// check_compute_stage without the cache.
bool check_compute_stage_fresh(std::string_view source, std::string& log) {
    compiler_detail::initialise();
    auto shader = make_shader(EShLangCompute, std::string(source));
    return parse(*shader, log);
}

// compile_program without the cache: glslang parses, links, maps and reflects the program.
bool compile_program_fresh(
    std::string_view vertex,
    std::string_view fragment,
    CompiledProgram& out,
    std::string& log,
    std::string_view compute
) {
    compiler_detail::initialise();
    Stage vs, fs, cs;
    glslang::TProgram program;

    if (compute.empty()) {
        vs = make_shader(EShLangVertex, vertex_source(vertex));
        fs = make_shader(EShLangFragment, std::string(fragment));

        if (!parse(*vs, log) || !parse(*fs, log)) {
            return false;
        }

        program.addShader(vs.get());
        program.addShader(fs.get());
    } else {
        cs = make_shader(EShLangCompute, std::string(compute));

        if (!parse(*cs, log)) {
            return false;
        }

        program.addShader(cs.get());
    }

    const EShLanguage stage = compute.empty() ? EShLangVertex : EShLangCompute;

    if (!link_program(program, stage, log)) {
        return false;
    }

    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    options.generateDebugInfo = false;
    options.disableOptimizer = true;
    out = CompiledProgram{};

    if (compute.empty()) {
        glslang::GlslangToSpv(
            *program.getIntermediate(EShLangVertex), out.vertex, &logger, &options
        );
        glslang::GlslangToSpv(
            *program.getIntermediate(EShLangFragment), out.fragment, &logger, &options
        );
    } else {
        glslang::GlslangToSpv(
            *program.getIntermediate(EShLangCompute), out.compute, &logger, &options
        );
    }

    reflect_program(program, out, compute.empty() ? EShLangVertex : EShLangCompute);

    return true;
}

}  // namespace compiler_detail
}  // namespace openrac::vkgl
