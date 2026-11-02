// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GL shader/program ownership, driver linking and reflected default-block uniform writes.
// The dispatch table and internal blits use the private gl_program.h entry points.

#include "renderer/vulkan/gl_program.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <volk.h>

#include "renderer/render_opt.h"
#include "renderer/vulkan/shader_compiler.h"
#include "renderer/vulkan/vkgl_internal.h"
#include "renderer/world/gl_world.h"

namespace openrac::vkgl::detail {

using namespace openrac::gl;

#define VKGL OPENRAC_GLAPI

// Creates an owned stage whose source is supplied through the GL table.
GLuint VKGL create_shader(GLenum type) {
    const GLuint name = g->shaders.add();
    g->shaders.find(name)->type = type;
    return name;
}

// Keeps stage text until program linking compiles the complete interface.
void VKGL
shader_source(GLuint name, GLsizei count, const GLchar* const* strings, const GLint* lengths) {
    Shader* sh = g->shaders.find(name);
    if (sh == nullptr) {
        return;
    }
    sh->source.clear();
    for (GLsizei i = 0; i < count; ++i) {
        if (lengths != nullptr && lengths[i] >= 0) {
            sh->source.append(strings[i], static_cast<std::size_t>(lengths[i]));
        } else {
            sh->source.append(strings[i]);
        }
    }
}

// Checks one stage and retains diagnostics for GL shader queries.
void VKGL compile_shader(GLuint name) {
    if (Shader* sh = g->shaders.find(name)) {
        sh->log.clear();
        sh->compiled = sh->type == GL_COMPUTE_SHADER
                           ? check_compute_stage(sh->source, sh->log)
                           : check_stage(sh->type == GL_VERTEX_SHADER, sh->source, sh->log);
    }
}

namespace {

// Truncates diagnostics while preserving the GL-required trailing terminator.
void write_log(const std::string& log, GLsizei size, GLsizei* length, GLchar* out) {
    const GLsizei n = size > 0 ? std::min<GLsizei>(size - 1, static_cast<GLsizei>(log.size())) : 0;
    if (out != nullptr && size > 0) {
        std::memcpy(out, log.data(), static_cast<std::size_t>(n));
        out[n] = '\0';
    }
    if (length != nullptr) {
        *length = n;
    }
}

}  // namespace

// Reports stage compilation status and diagnostic length.
void VKGL get_shader_iv(GLuint name, GLenum what, GLint* out) {
    Shader* sh = g->shaders.find(name);
    *out = 0;
    if (sh == nullptr) {
        return;
    }
    if (what == GL_COMPILE_STATUS) {
        *out = sh->compiled ? GL_TRUE : GL_FALSE;
    } else if (what == GL_INFO_LOG_LENGTH) {
        *out = sh->log.empty() ? 0 : static_cast<GLint>(sh->log.size() + 1);
    }
}

// Copies stage diagnostics into the caller's bounded GL buffer.
void VKGL get_shader_info_log(GLuint name, GLsizei size, GLsizei* length, GLchar* log) {
    Shader* sh = g->shaders.find(name);
    write_log(sh != nullptr ? sh->log : std::string(), size, length, log);
}

// Releases the stage text after its owning program has linked.
void VKGL delete_shader(GLuint name) {
    g->shaders.objects.erase(name);
}

// Creates an owned program before attaching its stages.
GLuint VKGL create_program() {
    return g->programs.add();
}

// Associates an existing stage with the program that will link it.
void VKGL attach_shader(GLuint program, GLuint shader) {
    if (Program* p = g->programs.find(program)) {
        p->shaders.push_back(shader);
    }
}

// Accepts GL attribute bindings; shaders supply explicit locations.
void VKGL bind_attrib_location(GLuint, GLuint, const GLchar*) {
    // The renderer's shaders give their inputs locations of their own.
}

namespace {

// Leaves a readable program log when the driver cannot create one of its objects.
bool link_step(Program& program, VkResult result, const char* operation) {
    // A failed step must not feed null handles to subsequent Vulkan calls.
    if (!check(result, operation)) {
        program.log = std::format("vulkan: {} failed ({})", operation, int(result));
        return false;
    }

    return true;
}

// Makes one stage; partial programs retain ownership until relink or deletion.
VkShaderModule module_of(Program& program, const std::vector<std::uint32_t>& code) {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size() * sizeof(std::uint32_t);
    info.pCode = code.data();
    VkShaderModule module = VK_NULL_HANDLE;
    if (!link_step(
            program, vkCreateShaderModule(g->device, &info, nullptr, &module), "a shader module"
        )) {
        return VK_NULL_HANDLE;
    }
    return module;
}

// An explicit source marker opts one scalar int into the ordinary graphics draw's push constant.
// No shader arithmetic changes; unmarked, compute and native material programs keep their layouts.
void separate_draw_uniform(Program& p, const std::string& vertex, const std::string& fragment) {
    p.draw_uniform_offset = -1;
    if (p.gpu_materials || !p.compiled.compute.empty()
        || vertex.find("#define OPENRAC_DRAW_INT") == std::string::npos
        || !renderer::render_opt::enabled(renderer::render_opt::Opt::VkDrawUniform)) {
        return;
    }
    static const std::regex marker(R"(#define\s+OPENRAC_DRAW_INT\s+([A-Za-z_][A-Za-z_0-9]*))");
    std::smatch match;
    if (!std::regex_search(vertex, match, marker)) {
        return;
    }
    const std::string name = match[1];
    const auto uniform =
        std::find_if(p.compiled.uniforms.begin(), p.compiled.uniforms.end(), [&](const auto& u) {
            return u.name == name;
        });
    if (uniform == p.compiled.uniforms.end() || uniform->sampler || uniform->gl_type != GL_INT
        || uniform->array_size != 1) {
        return;
    }
    // A cross-stage uniform must retain its shared UBO. This opt-in is vertex-only.
    const std::regex declaration("uniform\\s+int\\s+" + name + R"(\s*;)");
    if (std::regex_search(fragment, declaration) || !std::regex_search(vertex, declaration)) {
        return;
    }
    const auto source = std::regex_replace(
        vertex, declaration, "layout(push_constant) uniform OpenracDrawInt { int " + name + "; };"
    );
    CompiledProgram compiled;
    std::string error;
    if (!compile_program(source, fragment, compiled, error)) {
        return;  // An optional optimisation never prevents the original program from linking.
    }
    if (std::any_of(compiled.uniforms.begin(), compiled.uniforms.end(), [&](const auto& u) {
            return u.name == name;
        })) {
        return;  // A surviving loose uniform means another stage also uses this name.
    }
    auto draw = *uniform;
    draw.offset = std::max(compiled.default_size, 0);
    p.draw_uniform_offset = draw.offset;
    compiled.uniforms.push_back(std::move(draw));
    std::erase_if(compiled.blocks, [](const auto& b) { return b.name == "OpenracDrawInt"; });
    p.compiled = std::move(compiled);
}

// Keeps successfully created modules owned by the program until deletion or relinking.
bool make_program_modules(Program& program) {
    const auto& compiled = program.compiled;
    // Compute programs have no graphics stages.
    if (!compiled.compute.empty()) {
        program.compute = module_of(program, compiled.compute);
        return program.compute != VK_NULL_HANDLE;
    }
    program.vertex = module_of(program, compiled.vertex);
    // The fragment module cannot rescue a failed vertex stage.
    if (!program.vertex) {
        return false;
    }
    program.fragment = module_of(program, compiled.fragment);
    return program.fragment != VK_NULL_HANDLE;
}

// Reflects the program's complete descriptor interface before creating its layouts.
std::vector<VkDescriptorSetLayoutBinding> program_bindings(Program& program) {
    const auto& c = program.compiled;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    auto add = [&](int binding, VkDescriptorType type) {
        if (binding < 0) {
            return;
        }
        bindings.push_back(
            {static_cast<std::uint32_t>(binding),
             type,
             1,
             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                 | VK_SHADER_STAGE_COMPUTE_BIT,
             nullptr}
        );
    };
    program.dynamic_uniforms = c.default_binding >= 0 && reuse_uniforms();
    add(c.default_binding,
        program.dynamic_uniforms ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                 : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (const ProgramBlock& b : c.blocks) {
        add(b.binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    for (const auto& b : c.storage) {
        add(b.binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    for (const ProgramUniform& u : c.uniforms) {
        if (u.sampler && u.binding >= 0) {
            add(u.binding,
                u.kind == SamplerKind::Buffer ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                                              : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
    }
    return bindings;
}

// A failed descriptor layout must never be passed into pipeline layout creation.
bool make_program_layouts(Program& program) {
    const auto bindings = program_bindings(program);
    VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    set_info.pBindings = bindings.data();
    if (!link_step(
            program,
            vkCreateDescriptorSetLayout(g->device, &set_info, nullptr, &program.set_layout),
            "a descriptor set layout"
        )) {
        return false;
    }
    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    const VkDescriptorSetLayout layouts[] = {program.set_layout, g->material_layout};
    const VkPushConstantRange draw_push{VK_SHADER_STAGE_VERTEX_BIT, 0, 4};
    layout_info.setLayoutCount = program.gpu_materials ? 2u : 1u;
    layout_info.pSetLayouts = layouts;
    if (program.gpu_materials || program.draw_uniform_offset >= 0) {
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &draw_push;
    }
    if (!link_step(
            program,
            vkCreatePipelineLayout(g->device, &layout_info, nullptr, &program.layout),
            "a pipeline layout"
        )) {
        return false;
    }
    return true;
}

// Initialises GL uniforms and creates the compute pipeline after every prerequisite exists.
bool finish_program_link(Program& program) {
    const auto& c = program.compiled;
    // Uniforms start at zero, as GL's do; a relink keeps nothing, as GL's does.
    program.uniforms.assign(
        program.gpu_materials ? program.material_start + kMaterialBytes
                              : static_cast<unsigned>(std::max(c.default_size, 0))
                                    + (program.draw_uniform_offset >= 0 ? 4u : 0u),
        0
    );
    program.units.assign(c.uniforms.size(), 0);
    program.block_bindings.assign(c.blocks.size(), 0);
    program.uniforms_dirty = true;
    program.last_set = VK_NULL_HANDLE;
    program.last_key.clear();
    program.descriptor_sets.clear();
    if (program.compute) {
        VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = program.compute;
        info.stage.pName = "main";
        info.layout = program.layout;
        if (!link_step(
                program,
                timed_pipeline_creation([&] {
                    return vkCreateComputePipelines(
                        g->device, g->pipeline_cache, 1, &info, nullptr, &program.compute_pipeline
                    );
                }),
                "a compute pipeline"
            )) {
            return false;
        }
        count_pipeline();
    }
    program.linked =
        program.layout && ((program.vertex && program.fragment) || program.compute_pipeline);

    return program.linked;
}

}  // namespace

// Builds the executable only after every driver prerequisite succeeds.
void VKGL link_program(GLuint name) {
    Program* p = g->programs.find(name);
    if (p == nullptr) {
        return;
    }
    std::string vs, fs, cs;
    for (GLuint s : p->shaders) {
        if (Shader* sh = g->shaders.find(s)) {
            // The attached stage determines which source the compiler receives.
            switch (sh->type) {
                case GL_VERTEX_SHADER:
                    vs = sh->source;
                    break;
                case GL_COMPUTE_SHADER:
                    cs = sh->source;
                    break;
                default:
                    fs = sh->source;
                    break;
            }
        }
    }
    if (p->serial != 0) {
        release_program(*p);
    }
    p->log.clear();
    p->linked = false;
    p->serial = ++g->program_serial;
    if (!compile_program(vs, fs, p->compiled, p->log, cs)) {
        return;
    }
    if (!material_program(*p, vs, fs)) {
        return;
    }
    separate_draw_uniform(*p, vs, fs);
    // Partial driver objects stay owned, but cannot become a linked executable.
    if (!make_program_modules(*p) || !make_program_layouts(*p) || !finish_program_link(*p)) {
        return;
    }

    // Its pipelines of earlier runs are made on the pre-warm worker before its first draw.
    prewarm_program(*p);
}

// Reports link status and diagnostic length through the GL table.
void VKGL get_program_iv(GLuint name, GLenum what, GLint* out) {
    Program* p = g->programs.find(name);
    *out = 0;
    if (p == nullptr) {
        return;
    }
    if (what == GL_LINK_STATUS) {
        *out = p->linked ? GL_TRUE : GL_FALSE;
    } else if (what == GL_INFO_LOG_LENGTH) {
        *out = p->log.empty() ? 0 : static_cast<GLint>(p->log.size() + 1);
    } else if (what == kGlActiveUniforms) {
        *out = 0;  // not listed (gl_stats' uniform listing and shader reloading are GL's alone)
    }
}

// Copies link diagnostics into the caller's bounded GL buffer.
void VKGL get_program_info_log(GLuint name, GLsizei size, GLsizei* length, GLchar* log) {
    Program* p = g->programs.find(name);
    write_log(p != nullptr ? p->log : std::string(), size, length, log);
}

// Retires linked resources after the frames that can reference them.
void VKGL delete_program(GLuint name) {
    auto it = g->programs.objects.find(name);
    if (it == g->programs.objects.end()) {
        return;
    }
    if (it->second.serial != 0) {
        release_program(it->second);
    }
    g->programs.objects.erase(it);
    if (g->state.program == name) {
        g->state.program = 0;
    }
}

// Selects the program that subsequent draws and uniform writes consume.
void VKGL use_program(GLuint name) {
    g->state.program = name;
}

namespace {

// A uniform's location: its entry in the program's list, and the element of an array.
constexpr int kElementBits = 12;

}  // namespace

// Encodes the reflected uniform and array element in a GL location.
GLint VKGL get_uniform_location(GLuint program, const GLchar* name) {
    Program* p = g->programs.find(program);
    if (p == nullptr || !p->linked || name == nullptr) {
        return -1;
    }
    std::string_view n(name);
    int element = 0;
    if (const std::size_t bracket = n.find('['); bracket != std::string_view::npos) {
        element = std::atoi(std::string(n.substr(bracket + 1)).c_str());
        n = n.substr(0, bracket);
    }
    const auto& list = p->compiled.uniforms;
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].name == n && element >= 0 && element < list[i].array_size) {
            return static_cast<GLint>(i << kElementBits | static_cast<std::size_t>(element));
        }
    }
    return -1;
}

namespace {

// Where a location's value goes in the current program's default block; null for none.
std::uint8_t* uniform_at(
    GLint location,
    int count_bytes,
    const ProgramUniform** out_uniform = nullptr,
    int* out_index = nullptr
) {
    Program* p = g->programs.find(g->state.program);
    if (p == nullptr || !p->linked || location < 0) {
        return nullptr;
    }
    const std::size_t index = static_cast<std::size_t>(location) >> kElementBits;
    const int element = location & ((1 << kElementBits) - 1);
    if (index >= p->compiled.uniforms.size()) {
        return nullptr;
    }
    const ProgramUniform& u = p->compiled.uniforms[index];
    if (out_uniform != nullptr) {
        *out_uniform = &u;
    }
    if (out_index != nullptr) {
        *out_index = static_cast<int>(index);
    }
    if (u.sampler || u.offset < 0) {
        return nullptr;
    }
    const std::size_t at =
        static_cast<std::size_t>(u.offset)
        + static_cast<std::size_t>(element) * static_cast<std::size_t>(u.array_stride);
    if (at + static_cast<std::size_t>(count_bytes) > p->uniforms.size()) {
        return nullptr;
    }
    if (element >= u.array_size) {
        return nullptr;
    }
    return p->uniforms.data() + at;
}

// Skips identical default-block uploads without changing float bit patterns.
void write_uniform(std::uint8_t* destination, const void* source, std::size_t bytes) {
    ++g->submission.uniform_writes;
    // Compare bytes, preserving signed zero and NaN payloads exactly.
    if (reuse_uniforms() && std::memcmp(destination, source, bytes) == 0) {
        ++g->submission.uniform_writes_skipped;
        return;
    }
    std::memcpy(destination, source, bytes);
    auto* p = g->programs.find(g->state.program);
    const auto offset = static_cast<std::size_t>(destination - p->uniforms.data());
    if ((!p->gpu_materials || offset < p->material_start)
        && (p->draw_uniform_offset < 0 || offset < static_cast<unsigned>(p->draw_uniform_offset))) {
        p->uniforms_dirty = true;
    }
}

}  // namespace

// Writes an integer uniform or selects a sampler's texture unit.
void VKGL uniform_1i(GLint location, GLint v) {
    const ProgramUniform* u = nullptr;
    int index = -1;
    if (std::uint8_t* at = uniform_at(location, 4, &u, &index)) {
        write_uniform(at, &v, 4);
    } else if (u != nullptr && u->sampler) {
        g->programs.find(g->state.program)->units[static_cast<std::size_t>(index)] = v;
    }
}

// Writes an unsigned integer into the reflected default block.
void VKGL uniform_1ui(GLint location, GLuint v) {
    if (std::uint8_t* at = uniform_at(location, 4)) {
        write_uniform(at, &v, 4);
    }
}

// Writes one float into the reflected default block.
void VKGL uniform_1f(GLint location, GLfloat v) {
    if (std::uint8_t* at = uniform_at(location, 4)) {
        write_uniform(at, &v, 4);
    }
}

// Writes two floats into the reflected default block.
void VKGL uniform_2f(GLint location, GLfloat a, GLfloat b) {
    const float v[] = {a, b};
    if (std::uint8_t* at = uniform_at(location, 8)) {
        write_uniform(at, v, 8);
    }
}

// Writes three floats into the reflected default block.
void VKGL uniform_3f(GLint location, GLfloat a, GLfloat b, GLfloat c) {
    const float v[] = {a, b, c};
    if (std::uint8_t* at = uniform_at(location, 12)) {
        write_uniform(at, v, 12);
    }
}

// Writes four floats into the reflected default block.
void VKGL uniform_4f(GLint location, GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    const float v[] = {a, b, c, d};
    if (std::uint8_t* at = uniform_at(location, 16)) {
        write_uniform(at, v, 16);
    }
}

// Writes vector array elements using the reflected stride.
void VKGL uniform_4fv(GLint location, GLsizei count, const GLfloat* v) {
    if (location < 0 || count <= 0 || !v) {
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        if (std::uint8_t* at = uniform_at(location + i, 16)) {
            write_uniform(at, v + static_cast<std::size_t>(i) * 4, 16);
        }
    }
}

namespace {

// Keeps matrix transposition outside the array loop without changing its float bit patterns.
void write_matrix(std::uint8_t* destination, const GLfloat* matrix, GLboolean transpose) {
    // Ordinary GL matrices already have the reflected block's column-major order.
    if (transpose == GL_FALSE) {
        write_uniform(destination, matrix, 64);
        return;
    }

    std::array<GLfloat, 16> transposed;
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            transposed[column * 4 + row] = matrix[row * 4 + column];
        }
    }
    write_uniform(destination, transposed.data(), 64);
}

}  // namespace

// Writes matrix array elements, transposing when requested by GL.
void VKGL uniform_matrix_4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v) {
    if (location < 0 || count <= 0 || !v) {
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        std::uint8_t* at = uniform_at(location + i, 64);
        if (at == nullptr) {
            continue;
        }
        write_matrix(at, v + static_cast<std::size_t>(i) * 16, transpose);
    }
}

// Finds a named reflected block or returns the GL invalid index.
GLuint VKGL get_uniform_block_index(GLuint program, const GLchar* name) {
    Program* p = g->programs.find(program);
    if (p == nullptr || !p->linked || name == nullptr) {
        return wgl::GL_INVALID_INDEX;
    }
    for (std::size_t i = 0; i < p->compiled.blocks.size(); ++i) {
        if (p->compiled.blocks[i].name == name) {
            return static_cast<GLuint>(i);
        }
    }
    return wgl::GL_INVALID_INDEX;
}

// Assigns the GL buffer binding consumed by a reflected block.
void VKGL uniform_block_binding(GLuint program, GLuint block, GLuint binding) {
    Program* p = g->programs.find(program);
    if (p != nullptr && block < p->block_bindings.size()) {
        p->block_bindings[block] = binding;
    }
}

#undef VKGL

// Retires a program's driver objects after all referencing frames finish.
void release_program(Program& p) {
    // No pre-warm job may use its modules or layout once they are handed to defer.
    prewarm_forget(p.serial);
    // Relinking and deletion retire every pipeline referenced by the program's fast lookup.
    p.last_pipeline = VK_NULL_HANDLE;
    p.last_pipeline_key.clear();
    defer([vertex = p.vertex,
           fragment = p.fragment,
           compute = p.compute,
           pipeline = p.compute_pipeline,
           set_layout = p.set_layout,
           layout = p.layout] {
        vkDestroyShaderModule(g->device, vertex, nullptr);
        vkDestroyShaderModule(g->device, fragment, nullptr);
        vkDestroyShaderModule(g->device, compute, nullptr);
        vkDestroyPipeline(g->device, pipeline, nullptr);
        vkDestroyPipelineLayout(g->device, layout, nullptr);
        vkDestroyDescriptorSetLayout(g->device, set_layout, nullptr);
    });
    // Its pipelines go with it.
    // Called by erase_program, on this thread, for each pipeline of the program: destroyed once
    // the frames that may still draw with it are done.
    g->pipelines.erase_program(p.serial, [](VkPipeline pipeline) {
        defer([pipeline] { vkDestroyPipeline(g->device, pipeline, nullptr); });
    });
    p.vertex = p.fragment = VK_NULL_HANDLE;
    p.compute = VK_NULL_HANDLE;
    p.compute_pipeline = VK_NULL_HANDLE;
    p.set_layout = VK_NULL_HANDLE;
    p.layout = VK_NULL_HANDLE;
}

}  // namespace openrac::vkgl::detail
