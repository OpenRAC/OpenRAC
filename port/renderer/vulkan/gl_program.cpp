// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GL shader/program ownership, driver linking and reflected default-block uniform writes.
// The dispatch table and internal blits use the private gl_program.h entry points.

#include "renderer/vulkan/gl_program.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iterator>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
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

// The marker of a shader's mode uniforms (vk-mode-constants): a comment line
// "// openrac_mode: name name ...". A mode is a scalar `uniform int` that picks the code a draw
// runs and keeps its value over many draws (a pass, a setting, a level).
constexpr std::string_view kModeMarker = "// openrac_mode:";
constexpr std::string_view kModeUniformPrefix = "openrac_u_";

// Returns whether `c` may be part of a GLSL name.
bool glsl_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Adds the names `source` marks as modes to `names`, each once, in the order named.
void mode_names(const std::string& source, std::vector<std::string>& names) {
    for (std::size_t at = source.find(kModeMarker); at != std::string::npos;
         at = source.find(kModeMarker, at + 1)) {
        std::size_t i = at + kModeMarker.size();

        // Each name up to the line's end, between spaces or commas.
        while (i < source.size() && source[i] != '\n') {
            if (!glsl_name_char(source[i])) {
                ++i;
                continue;
            }

            const std::size_t from = i;
            while (i < source.size() && glsl_name_char(source[i])) {
                ++i;
            }

            std::string name = source.substr(from, i - from);
            if (std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(std::move(name));
            }
        }
    }
}

// Finds the declaration `uniform int <name>;` in `source` (any whitespace between); its start
// and the place past its semicolon, or npos twice.
std::pair<std::size_t, std::size_t> mode_declaration(
    const std::string& source, std::string_view name
) {
    constexpr std::string_view keyword = "uniform";
    const auto space = [&](std::size_t i) {
        return i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0;
    };

    for (std::size_t at = source.find(keyword); at != std::string::npos;
         at = source.find(keyword, at + 1)) {
        std::size_t i = at + keyword.size();
        if ((at > 0 && glsl_name_char(source[at - 1])) || !space(i)) {
            continue;
        }
        while (space(i)) {
            ++i;
        }
        if (source.compare(i, 3, "int") != 0 || !space(i + 3)) {
            continue;
        }
        i += 3;
        while (space(i)) {
            ++i;
        }
        if (source.compare(i, name.size(), name) != 0) {
            continue;
        }
        i += name.size();
        while (space(i)) {
            ++i;
        }
        if (i < source.size() && source[i] == ';') {
            return {at, i + 1};
        }
    }

    return {std::string::npos, std::string::npos};
}

// A program's sources with its mode uniforms also readable as specialisation constants
// (vk-mode-constants), and the modes' names by slot; no names when the sources mark none.
struct ModeSources {
    std::string vertex, fragment;
    std::vector<std::string> names;
};

// Rewrites each marked `uniform int name;` so that a pipeline may fix its value: the uniform is
// renamed, a specialisation constant is declared beside it, and `name` becomes the constant, or
// the uniform while the constant has its default (kModeDynamic). A pipeline made with the value
// lets the driver drop the code the mode leaves out; one made without reads the uniform as
// before. A mode the material path reads per draw is left alone.
ModeSources mode_sources(const std::string& vertex, const std::string& fragment) {
    ModeSources out;
    std::vector<std::string> marked;
    mode_names(vertex, marked);
    mode_names(fragment, marked);
    if (marked.empty()) {
        return out;
    }

    out.vertex = vertex;
    out.fragment = fragment;
    std::string macros;

    for (const std::string& name : marked) {
        if (out.names.size() == kModeConstants || material_takes(vertex, name)) {
            continue;
        }

        const std::string id = std::to_string(kModeConstantBase + out.names.size());
        const std::string dynamic = std::to_string(kModeDynamic);
        const std::string uniform = std::string(kModeUniformPrefix) + name;
        const std::string constant = "openrac_c_" + name;
        bool declared = false;

        for (std::string* source : {&out.vertex, &out.fragment}) {
            const auto [from, to] = mode_declaration(*source, name);
            if (from == std::string::npos) {
                continue;
            }
            // On the declaration's own line, so that the lines after it keep their numbers.
            source->replace(
                from,
                to - from,
                "uniform int " + uniform + "; layout(constant_id = " + id + ") const int "
                    + constant + " = " + dynamic + ";"
            );
            declared = true;
        }

        if (declared) {
            macros += "#define " + name + " (" + constant + " != " + dynamic + " ? " + constant
                      + " : " + uniform + ")\n";
            out.names.push_back(name);
        }
    }

    if (out.names.empty()) {
        return out;
    }

    // After the #version line, which stays first.
    for (std::string* source : {&out.vertex, &out.fragment}) {
        const std::size_t line = source->find('\n');
        if (line != std::string::npos) {
            source->insert(line + 1, macros);
        }
    }

    return out;
}

// Gives the mode uniforms of `compiled` their GL names back, as the program's users know them.
void restore_mode_names(CompiledProgram& compiled) {
    for (ProgramUniform& u : compiled.uniforms) {
        if (u.name.starts_with(kModeUniformPrefix)) {
            u.name.erase(0, kModeUniformPrefix.size());
        }
    }
}

// Records where each mode of `names` (by slot) has its value in the program's uniforms.
void place_modes(Program& p, const std::vector<std::string>& names) {
    p.modes.assign(names.size(), Program::Mode{});

    for (std::size_t n = 0; n < names.size(); ++n) {
        const auto found = std::find_if(
            p.compiled.uniforms.begin(), p.compiled.uniforms.end(), [&](const ProgramUniform& u) {
                return u.name == names[n];
            }
        );
        // A mode no stage reads has no uniform, and its constant no reader.
        if (found != p.compiled.uniforms.end() && !found->sampler && found->gl_type == GL_INT
            && found->array_size == 1 && found->offset >= 0
            && (!p.gpu_materials || static_cast<unsigned>(found->offset) < p.material_start)) {
            p.modes[n].offset = found->offset;
        }
    }
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

// SPIR-V's OpDecorate and the decorations it carries here (documented: the SPIR-V
// specification's instruction and Decoration tables).
constexpr std::uint32_t kSpirvHeaderWords = 5;
constexpr std::uint32_t kOpDecorate = 71;
constexpr std::uint32_t kDecorationBinding = 33;
constexpr std::uint32_t kDecorationDescriptorSet = 34;

// Lists the bindings a stage's module declares in descriptor set 0, the program's own set.
std::vector<std::uint32_t> set0_bindings(std::span<const std::uint32_t> code) {
    std::unordered_map<std::uint32_t, std::uint32_t> binding_of;
    std::unordered_map<std::uint32_t, std::uint32_t> set_of;
    for (std::size_t at = kSpirvHeaderWords; at < code.size();) {
        const std::uint32_t count = code[at] >> 16;
        if (count == 0 || at + count > code.size()) {
            break;
        }
        if ((code[at] & 0xffffu) == kOpDecorate && count >= 4) {
            const std::uint32_t decoration = code[at + 2];
            if (decoration == kDecorationBinding) {
                binding_of[code[at + 1]] = code[at + 3];
            } else if (decoration == kDecorationDescriptorSet) {
                set_of[code[at + 1]] = code[at + 3];
            }
        }
        at += count;
    }
    std::vector<std::uint32_t> out;
    for (const auto& [id, binding] : binding_of) {
        const auto set = set_of.find(id);
        if (set == set_of.end() || set->second == 0) {
            out.push_back(binding);
        }
    }
    return out;
}

// Each binding is visible only to the stages that declare it. Vulkan counts a binding against
// every stage it names, so naming all stages put MoltenVK's mesh shader at 19 samplers for one
// stage, over its limit of 16, though its fragment stage uses 16 and its vertex stage 4.
std::unordered_map<std::uint32_t, VkShaderStageFlags> binding_stages(const CompiledProgram& c) {
    std::unordered_map<std::uint32_t, VkShaderStageFlags> stages;
    const std::pair<const std::vector<std::uint32_t>*, VkShaderStageFlagBits> modules[] =
        {{&c.vertex, VK_SHADER_STAGE_VERTEX_BIT},
         {&c.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
         {&c.compute, VK_SHADER_STAGE_COMPUTE_BIT}};
    for (const auto& [code, stage] : modules) {
        for (const std::uint32_t binding : set0_bindings(*code)) {
            stages[binding] |= stage;
        }
    }
    return stages;
}

// Reflects the program's complete descriptor interface before creating its layouts.
std::vector<VkDescriptorSetLayoutBinding> program_bindings(Program& program) {
    const auto& c = program.compiled;
    const auto stages = binding_stages(c);
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    auto add = [&](int binding, VkDescriptorType type) {
        if (binding < 0) {
            return;
        }
        const auto found = stages.find(static_cast<std::uint32_t>(binding));
        // A binding no module declares (never expected) keeps the old all-stage visibility.
        const VkShaderStageFlags flags = found != stages.end() ? found->second
                                                               : VK_SHADER_STAGE_VERTEX_BIT
                                                                     | VK_SHADER_STAGE_FRAGMENT_BIT
                                                                     | VK_SHADER_STAGE_COMPUTE_BIT;
        bindings.push_back({static_cast<std::uint32_t>(binding), type, 1, flags, nullptr});
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
    if (g->test_program_reuse || keeps_programs()) {
        program.test_layout_bindings = bindings;
    }
    return bindings;
}

// Warns at each link when a stage still names more samplers than the device allows; past
// the limit, Vulkan's behaviour is undefined (MoltenVK allows 16 per stage).
void check_sampler_limit(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    const std::uint32_t limit = g->properties.limits.maxPerStageDescriptorSamplers;
    for (const VkShaderStageFlagBits stage :
         {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_COMPUTE_BIT}) {
        const auto count = std::count_if(bindings.begin(), bindings.end(), [&](const auto& b) {
            return (b.stageFlags & stage) != 0
                   && b.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        });
        if (static_cast<std::uint32_t>(count) > limit) {
            log::warn(
                "vulkan: a program's stage {:#x} uses {} samplers, over the device's {}",
                static_cast<unsigned>(stage),
                count,
                limit
            );
        }
    }
}

// A failed descriptor layout must never be passed into pipeline layout creation.
bool make_program_layouts(Program& program) {
    const auto bindings = program_bindings(program);
    check_sampler_limit(bindings);
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
    if (program.compute && !program.compute_pipeline) {
        VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = program.compute;
        info.stage.pName = "main";
        info.layout = program.layout;
        if (g->pipeline_stats) {
            info.flags |= VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
        }
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
        if (g->pipeline_stats) {
            log_executable_statistics(
                program.compute_pipeline, std::format("compute program {}", program.serial)
            );
        }
    }
    program.linked =
        program.layout && ((program.vertex && program.fragment) || program.compute_pipeline);

    return program.linked;
}

// Whether two links of a program give its stages the same resources at the same places, so that
// one's fragment module can stand in for the other's in the same pipeline layout.
bool same_layout(const Program& a, const Program& b) {
    const CompiledProgram& x = a.compiled;
    const CompiledProgram& y = b.compiled;
    const auto same_uniform = [](const ProgramUniform& u, const ProgramUniform& v) {
        return u.name == v.name && u.gl_type == v.gl_type && u.offset == v.offset
               && u.array_size == v.array_size && u.array_stride == v.array_stride
               && u.sampler == v.sampler && u.binding == v.binding && u.kind == v.kind;
    };
    const auto same_block = [](const ProgramBlock& u, const ProgramBlock& v) {
        return u.name == v.name && u.binding == v.binding && u.size == v.size;
    };
    return a.gpu_materials == b.gpu_materials && a.draw_uniform_offset == b.draw_uniform_offset
           && x.default_binding == y.default_binding && x.default_size == y.default_size
           && std::equal(
               x.uniforms.begin(),
               x.uniforms.end(),
               y.uniforms.begin(),
               y.uniforms.end(),
               same_uniform
           )
           && std::equal(
               x.blocks.begin(), x.blocks.end(), y.blocks.begin(), y.blocks.end(), same_block
           )
           && std::equal(
               x.storage.begin(), x.storage.end(), y.storage.begin(), y.storage.end(), same_block
           );
}

}  // namespace

// Outside the anonymous namespace: vkgl_pipelines.cpp asks for it (vkgl_internal.h).
VkShaderModule clamped_fragment(Program& p) {
    if (p.clamped_tried) {
        return p.fragment_clamped;
    }
    p.clamped_tried = true;
    p.clamped_half_precision =
        g->shader_float16
        && renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderFloat16);
    static const std::regex output(
        R"(layout\s*\(\s*location\s*=\s*0\s*\)\s*out\s+vec4\s+([A-Za-z_][A-Za-z_0-9]*)\s*;)"
    );
    static const std::regex entry(R"(\bvoid\s+main\s*\(\s*(void)?\s*\))");
    // Or its one output without a location (location 0 then).
    static const std::regex lone(R"((^|\n)\s*out\s+vec4\s+([A-Za-z_][A-Za-z_0-9]*)\s*;)");
    const std::string& source = p.fragment_source;
    std::smatch match;
    std::string colour;
    if (std::regex_search(source, match, output)) {
        colour = match[1];
    } else {
        // Only when there is exactly one.
        const std::sregex_iterator first(source.begin(), source.end(), lone), end;
        if (first != end && std::next(first) == end) {
            colour = (*first)[2];
        }
    }
    if (colour.empty() || !std::regex_search(source, entry)) {
        log::warn("vulkan: a program's colour cannot be clamped (no colour output found)");
        return VK_NULL_HANDLE;
    }
    // The program's own main runs first, then its colour is clamped.
    std::string fs = std::regex_replace(source, entry, "void openrac_unclamped_main()");
    fs += "\nvoid main() {\n    openrac_unclamped_main();\n    " + colour + " = clamp(" + colour
          + ", 0.0, 1.0);\n}\n";
    // Linked as link_program links it, so that its resources land where the program's are.
    Program variant;
    std::string error;
    if (!compile_program(p.vertex_source, fs, variant.compiled, error)
        || !material_program(variant, p.vertex_source, fs)) {
        log::warn("vulkan: a program's colour cannot be clamped: {}", error);
        return VK_NULL_HANDLE;
    }
    separate_draw_uniform(variant, p.vertex_source, fs);
    restore_mode_names(variant.compiled);
    if (!same_layout(variant, p)) {
        log::warn("vulkan: a program's clamped colour stage has another layout; drawn unclamped");
        return VK_NULL_HANDLE;
    }
    p.fragment_clamped = module_of(variant, variant.compiled.fragment);
    return p.fragment_clamped;
}

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
    p->modes.clear();
    // A graphics program's marked modes become constants of its pipelines (vk-mode-constants).
    // An optional optimisation never prevents the original program from linking.
    std::vector<std::string> modes;
    if (cs.empty()) {
        ModeSources with_modes = mode_sources(vs, fs);
        if (!with_modes.names.empty()) {
            std::string error;
            if (compile_program(with_modes.vertex, with_modes.fragment, p->compiled, error)) {
                vs = std::move(with_modes.vertex);
                fs = std::move(with_modes.fragment);
                modes = std::move(with_modes.names);
            } else {
                log::warn("vulkan: a program's modes stay uniforms: {}", error);
            }
        }
    }
    if (modes.empty() && !compile_program(vs, fs, p->compiled, p->log, cs)) {
        return;
    }
    // Kept for the clamped fragment stage an HDR frame may ask for (clamped_fragment).
    if (cs.empty()) {
        p->vertex_source = vs;
        p->fragment_source = fs;
    }
    if (!material_program(*p, vs, fs)) {
        return;
    }
    separate_draw_uniform(*p, vs, fs);
    restore_mode_names(p->compiled);
    place_modes(*p, modes);
    p->key_modes_stale = true;
    // Descriptor mode is part of compatibility even when the same SPIR-V is used.
    if (g->test_program_reuse || keeps_programs()) {
        (void)program_bindings(*p);
    }
    const bool reused = reuse_test_program(*p);
    // Partial driver objects stay owned, but cannot become a linked executable.
    if ((!reused && (!make_program_modules(*p) || !make_program_layouts(*p)))
        || !finish_program_link(*p)) {
        return;
    }

    // Names the program for the pipeline statistics' lines (OPENRAC_VK_PIPELINE_STATS).
    if (g->pipeline_stats && !cs.empty()) {
        // A compute program by its first comment line past the licence header.
        auto comment = cs.find("\n//");
        while (comment != std::string::npos
               && (cs.compare(comment + 1, 7, "// SPDX") == 0
                   || cs.compare(comment + 1, 12, "// Copyright") == 0)) {
            comment = cs.find("\n//", comment + 1);
        }
        const auto line_end = comment == std::string::npos ? comment : cs.find('\n', comment + 1);
        log::info(
            "vulkan: pipeline stats compute program {} {} bytes: {}",
            p->serial,
            cs.size(),
            comment == std::string::npos
                ? std::string_view()
                : std::string_view(cs).substr(comment + 1, line_end - comment - 1)
        );
    }
    if (g->pipeline_stats && cs.empty()) {
        std::string defines;
        for (std::size_t at = fs.find("#define OPENRAC_"); at != std::string::npos;
             at = fs.find("#define OPENRAC_", at + 1)) {
            defines += ' ';
            defines += fs.substr(at + 8, fs.find_first_of(" \n", at + 8) - (at + 8));
        }
        std::string names;
        for (const auto& name : modes) {
            names += names.empty() ? name : "," + name;
        }
        log::info(
            "vulkan: pipeline stats program {} fragment {} bytes, modes [{}], defines:{}",
            p->serial,
            fs.size(),
            names,
            defines
        );
    }

    // A test's retained objects already own the graphics pipelines; fresh links keep normal
    // prewarming, and the game's kept ones queue what they still miss.
    if (!reused || keeps_programs()) {
        prewarm_program(*p);
    }
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
    // A mode is a scalar integer: its pipelines' keys are worked out again (vk-mode-constants).
    p->key_modes_stale = p->key_modes_stale || bytes == sizeof(std::int32_t);
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
    // No pre-warm job may use its modules or layout once they are handed to defer. What the
    // workers made for a kept program stays with it.
    prewarm_forget(p.serial, keeps_programs());
    // Only immutable objects are retained; a later link still starts with zero GL uniforms.
    if (retain_test_program(p)) {
        return;
    }
    destroy_program(p);
}

void destroy_program(Program& p) {
    // Relinking and deletion retire every pipeline referenced by the program's fast lookup.
    p.last_pipeline = VK_NULL_HANDLE;
    p.last_pipeline_key.clear();
    defer([vertex = p.vertex,
           fragment = p.fragment,
           clamped = p.fragment_clamped,
           compute = p.compute,
           pipeline = p.compute_pipeline,
           set_layout = p.set_layout,
           layout = p.layout] {
        vkDestroyShaderModule(g->device, vertex, nullptr);
        vkDestroyShaderModule(g->device, fragment, nullptr);
        vkDestroyShaderModule(g->device, clamped, nullptr);
        vkDestroyShaderModule(g->device, compute, nullptr);
        vkDestroyPipeline(g->device, pipeline, nullptr);
        vkDestroyPipelineLayout(g->device, layout, nullptr);
        vkDestroyDescriptorSetLayout(g->device, set_layout, nullptr);
    });
    // Its pipelines go with it.
    // Called by erase_program, on this thread, for each pipeline of the program: destroyed once
    // the frames that may still draw with it are done.
    g->pipelines.erase_program(p.serial, [](VkPipeline pipeline) {
        g->libraries.linked.erase(pipeline);
        defer([pipeline] { vkDestroyPipeline(g->device, pipeline, nullptr); });
    });
    // And the parts its pipelines were linked from (vk-pipeline-library).
    release_program_libraries(p.serial);
    p.vertex = p.fragment = VK_NULL_HANDLE;
    p.fragment_clamped = VK_NULL_HANDLE;
    p.clamped_tried = false;
    p.compute = VK_NULL_HANDLE;
    p.compute_pipeline = VK_NULL_HANDLE;
    p.set_layout = VK_NULL_HANDLE;
    p.layout = VK_NULL_HANDLE;
}

}  // namespace openrac::vkgl::detail
