// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/metal/shader_translate.h"

#include <algorithm>
#include <format>
#include <memory>

#include <spirv_msl.hpp>

namespace openrac::mtlgl {

namespace {

using spirv_cross::CompilerMSL;
using spirv_cross::MSLResourceBinding;
using spirv_cross::Resource;

class Slots {
public:
    // Gives the program's resources in `compiler` their slots: those met in an earlier stage keep
    // theirs.
    void assign(const CompilerMSL& compiler) {
        const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        for (const auto* list : {&resources.uniform_buffers, &resources.storage_buffers}) {
            for (const Resource& r : *list) {
                const int binding = binding_of(compiler, r);
                if (std::ranges::find(m_out.buffers, binding) == m_out.buffers.end()) {
                    m_out.buffers.push_back(binding);
                }
            }
        }
        for (const Resource& r : resources.sampled_images) {
            texture(binding_of(compiler, r), compiler.get_type(r.type_id).image.dim != spv::DimBuffer);
        }
        for (const Resource& r : resources.storage_images) {
            texture(binding_of(compiler, r), false);
        }
    }

    // Tells `compiler`, of stage `stage`, the slots assigned.
    void bind(CompilerMSL& compiler, spv::ExecutionModel stage) const {
        for (std::size_t slot = 0; slot < m_out.buffers.size(); ++slot) {
            MSLResourceBinding b;
            b.stage = stage;
            b.binding = static_cast<std::uint32_t>(m_out.buffers[slot]);
            b.msl_buffer = static_cast<std::uint32_t>(slot);
            compiler.add_msl_resource_binding(b);
        }
        for (std::size_t slot = 0; slot < m_out.textures.size(); ++slot) {
            const MslTexture& t = m_out.textures[slot];
            MSLResourceBinding b;
            b.stage = stage;
            b.binding = static_cast<std::uint32_t>(t.binding);
            b.msl_texture = static_cast<std::uint32_t>(slot);
            b.msl_sampler = static_cast<std::uint32_t>(std::max(t.sampler, 0));
            compiler.add_msl_resource_binding(b);
        }
    }

    // Returns why the slots do not fit Metal's, or nothing.
    std::string overflow() const {
        if (m_out.buffers.size() > kVertexBufferBase) {
            return std::format("{} blocks, more than Metal's {} below the vertex buffers",
                               m_out.buffers.size(), kVertexBufferBase);
        }
        if (m_samplers > kMaxSamplers) {
            return std::format("{} samplers, more than Metal's {}", m_samplers, kMaxSamplers);
        }
        return {};
    }

    void into(MslProgram& out) const {
        out.buffers = m_out.buffers;
        out.textures = m_out.textures;
    }

private:
    static int binding_of(const CompilerMSL& compiler, const Resource& r) {
        return static_cast<int>(compiler.get_decoration(r.id, spv::DecorationBinding));
    }

    void texture(int binding, bool sampled) {
        const auto found = std::ranges::find(m_out.textures, binding, &MslTexture::binding);
        if (found == m_out.textures.end()) {
            m_out.textures.push_back({binding, sampled ? static_cast<int>(m_samplers++) : -1});
        }
    }

    MslProgram m_out;
    std::uint32_t m_samplers = 0;
};

// Apple's OpenGL divides as fast::divide does, not exactly as `/` does without fast math. AMD's
// multiplies by a constant's rounded reciprocal, which fast::divide there folds into an earlier
// multiply (README.md).
class Translator : public CompilerMSL {
public:
    Translator(const std::vector<std::uint32_t>& code, bool apple) : CompilerMSL(code), m_apple(apple) {}

protected:
    void emit_instruction(const spirv_cross::Instruction& instr) override {
        if (static_cast<spv::Op>(instr.op) != spv::OpFDiv) {
            CompilerMSL::emit_instruction(instr);
            return;
        }
        const std::uint32_t* ops = stream(instr);
        if (m_apple || !maybe_get<spirv_cross::SPIRConstant>(ops[3])) {
            emit_binary_func_op(ops[0], ops[1], ops[2], ops[3], "fast::divide");
            return;
        }
        emit_op(ops[0], ops[1],
                spirv_cross::join(to_enclosed_unpacked_expression(ops[2]), " * (1.0 / ",
                                  to_enclosed_unpacked_expression(ops[3]), ")"),
                should_forward(ops[2]));
        inherit_expression_dependencies(ops[1], ops[2]);
    }

private:
    bool m_apple;
};

std::unique_ptr<CompilerMSL> make_compiler(const std::vector<std::uint32_t>& code, bool apple) {
    auto compiler = std::make_unique<Translator>(code, apple);
    CompilerMSL::Options options;
    options.platform = CompilerMSL::Options::macOS;
    options.set_msl_version(kMslMajor, kMslMinor);
    options.texture_buffer_native = true;
    compiler->set_msl_options(options);
    spirv_cross::CompilerGLSL::Options common = compiler->get_common_options();
    common.vertex.flip_vert_y = true;
    compiler->set_common_options(common);
    return compiler;
}

}  // namespace

bool translate_program(const vkgl::CompiledProgram& spirv, bool apple, MslProgram& out, std::string& log) {
    out = MslProgram{};
    try {
        struct Stage {
            const std::vector<std::uint32_t>& code;
            spv::ExecutionModel model;
            std::string& source;
            std::string& entry;
            std::unique_ptr<CompilerMSL> compiler;
        };
        std::vector<Stage> stages;
        if (!spirv.compute.empty()) {
            stages.push_back({spirv.compute, spv::ExecutionModelGLCompute, out.compute, out.compute_entry, {}});
        } else {
            stages.push_back({spirv.vertex, spv::ExecutionModelVertex, out.vertex, out.vertex_entry, {}});
            stages.push_back({spirv.fragment, spv::ExecutionModelFragment, out.fragment, out.fragment_entry, {}});
        }
        Slots slots;
        for (Stage& stage : stages) {
            stage.compiler = make_compiler(stage.code, apple);
            slots.assign(*stage.compiler);
        }
        if (const std::string why = slots.overflow(); !why.empty()) {
            log += why;
            return false;
        }
        for (Stage& stage : stages) {
            slots.bind(*stage.compiler, stage.model);
            stage.source = stage.compiler->compile();
            stage.entry = stage.compiler->get_cleansed_entry_point_name("main", stage.model);
        }
        if (!spirv.compute.empty()) {
            for (std::uint32_t i = 0; i < 3; ++i) {
                out.workgroup[i] = stages[0].compiler->get_execution_mode_argument(spv::ExecutionModeLocalSize, i);
            }
        }
        slots.into(out);
        return true;
    } catch (const spirv_cross::CompilerError& e) {
        log += std::format("SPIRV-Cross: {}", e.what());
        return false;
    }
}

}  // namespace openrac::mtlgl
