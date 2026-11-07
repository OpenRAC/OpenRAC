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
            texture(
                binding_of(compiler, r), compiler.get_type(r.type_id).image.dim != spv::DimBuffer
            );
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
            return std::format(
                "{} blocks, more than Metal's {} below the vertex buffers",
                m_out.buffers.size(),
                kVertexBufferBase
            );
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
//
// Apple's OpenGL fuses a multiply into a later add, except in a vertex stage whose position is
// invariant: there only a matrix times a vector is fused, column by column from the first.
class Translator : public CompilerMSL {
public:
    Translator(const std::vector<std::uint32_t>& code, bool apple, bool exact)
        : CompilerMSL(code),
          m_apple(apple),
          m_exact(exact) {}

protected:
    void emit_instruction(const spirv_cross::Instruction& instr) override {
        const std::uint32_t* ops = stream(instr);
        if (m_exact && static_cast<spv::Op>(instr.op) == spv::OpMatrixTimesVector) {
            const std::string m = to_enclosed_unpacked_expression(ops[2]);
            const std::string v = to_enclosed_unpacked_expression(ops[3]);
            std::string sum = spirv_cross::join(m, "[0] * ", v, "[0]");
            for (std::uint32_t c = 1; c < expression_type(ops[2]).columns; ++c) {
                sum = spirv_cross::join("fma(", m, "[", c, "], ", v, "[", c, "], ", sum, ")");
            }
            emit_op(ops[0], ops[1], sum, should_forward(ops[2]) && should_forward(ops[3]));
            inherit_expression_dependencies(ops[1], ops[2]);
            inherit_expression_dependencies(ops[1], ops[3]);
            return;
        }
        if (static_cast<spv::Op>(instr.op) != spv::OpFDiv) {
            CompilerMSL::emit_instruction(instr);
            return;
        }
        if (m_apple || !maybe_get<spirv_cross::SPIRConstant>(ops[3])) {
            emit_binary_func_op(ops[0], ops[1], ops[2], ops[3], "fast::divide");
            return;
        }
        emit_op(
            ops[0],
            ops[1],
            spirv_cross::join(
                to_enclosed_unpacked_expression(ops[2]),
                " * (1.0 / ",
                to_enclosed_unpacked_expression(ops[3]),
                ")"
            ),
            should_forward(ops[2])
        );
        inherit_expression_dependencies(ops[1], ops[2]);
    }

private:
    bool m_apple;
    bool m_exact;
};

bool invariant(const std::vector<std::uint32_t>& code) {
    for (std::size_t i = 5; i < code.size(); i += code[i] >> 16) {
        const auto op = static_cast<spv::Op>(code[i] & 0xffff);
        if ((op == spv::OpDecorate && code[i + 2] == spv::DecorationInvariant)
            || (op == spv::OpMemberDecorate && code[i + 3] == spv::DecorationInvariant)) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<CompilerMSL> make_compiler(
    const std::vector<std::uint32_t>& code, bool apple, bool exact
) {
    auto compiler = std::make_unique<Translator>(code, apple, exact);
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

std::string constant(const char* name, std::uint32_t index) {
    return std::format(
        "constant bool {0}_ [[function_constant({1})]];\n"
        "constant bool {0} = is_function_constant_defined({0}_) && {0}_;\n",
        name,
        index
    );
}

void declare(std::string& source, const std::string& what) {
    const std::string metal = "using namespace metal;\n";
    source.insert(source.find(metal) + metal.size(), what);
}

// Adds the far half's depth after SPIRV-Cross's flip of y, the stage's last word, and
// the z / w by which a clamped half's fragments are kept.
void far_half(std::string& source) {
    const std::string flip = ".gl_Position.y);    // Invert Y-axis for Metal";
    const std::size_t end = source.find(flip);
    if (end == std::string::npos) {
        return;
    }
    const std::size_t line = source.rfind('\n', end) + 1;
    const std::size_t start = source.find_first_not_of(' ', line);
    const std::string indent = source.substr(line, start - line);
    const std::string out = source.substr(start, source.find(".gl_Position.y = -(", start) - start);
    const std::string p = out + ".gl_Position";
    source.insert(
        end + flip.size(),
        std::format(
            "\n{0}{1}.openrac_z = {2}.z / {2}.w;\n"
            "{0}{2}.z = openrac_far_half ? -{2}.z : {2}.z;",
            indent,
            out,
            p
        )
    );
    const std::size_t member = source.find('\n', source.find("gl_Position [[position")) + 1;
    source.insert(member, "    float openrac_z [[user(openrac_z)]];\n");
    declare(source, constant("openrac_far_half", kFarHalfConstant));
}

// Clamped depth clips neither half away, so each half's draw keeps only its own fragments: both
// interpolate the same z / w.
void clamped_half(std::string& source) {
    const std::string input = "    float openrac_z [[user(openrac_z), center_no_perspective]];\n";
    if (const std::size_t in = source.find("struct main0_in\n{\n"); in != std::string::npos) {
        source.insert(in + 18, input);
    } else {
        source.insert(source.find("\nfragment ") + 1, "struct main0_in\n{\n" + input + "};\n\n");
        const std::size_t parameters = source.find('(', source.find("\nfragment ")) + 1;
        source.insert(
            parameters,
            source[parameters] == ')' ? "main0_in in [[stage_in]]" : "main0_in in [[stage_in]], "
        );
    }
    const std::size_t body = source.find("\n{\n", source.find("\nfragment ")) + 3;
    source.insert(
        body,
        "    if (openrac_clamped && (openrac_far_half ? in.openrac_z > 0.0 : in.openrac_z <= "
        "0.0))\n"
        "    {\n"
        "        discard_fragment();\n"
        "    }\n"
    );
    declare(
        source,
        constant("openrac_far_half", kFarHalfConstant)
            + constant("openrac_clamped", kClampedConstant)
    );
}

}  // namespace

bool translate_program(
    const vkgl::CompiledProgram& spirv, bool apple, MslProgram& out, std::string& log
) {
    out = MslProgram{};
    try {
        struct Stage {
            const std::vector<std::uint32_t>& code;
            spv::ExecutionModel model;
            std::string& source;
            std::string& entry;
            std::unique_ptr<CompilerMSL> compiler;
            bool exact = false;
        };

        std::vector<Stage> stages;
        if (!spirv.compute.empty()) {
            stages.push_back(
                {spirv.compute, spv::ExecutionModelGLCompute, out.compute, out.compute_entry, {}}
            );
        } else {
            stages.push_back(
                {spirv.vertex, spv::ExecutionModelVertex, out.vertex, out.vertex_entry, {}}
            );
            stages.push_back(
                {spirv.fragment, spv::ExecutionModelFragment, out.fragment, out.fragment_entry, {}}
            );
        }
        Slots slots;
        for (Stage& stage : stages) {
            stage.exact =
                apple && stage.model == spv::ExecutionModelVertex && invariant(stage.code);
            stage.compiler = make_compiler(stage.code, apple, stage.exact);
            slots.assign(*stage.compiler);
        }
        if (const std::string why = slots.overflow(); !why.empty()) {
            log += why;
            return false;
        }
        for (Stage& stage : stages) {
            slots.bind(*stage.compiler, stage.model);
            stage.source = std::string(
                               stage.exact ? "#pragma clang fp contract(off)\n"
                                           : "#pragma clang fp contract(fast)\n"
                           )
                           + stage.compiler->compile();
            stage.entry = stage.compiler->get_cleansed_entry_point_name("main", stage.model);
            if (stage.model == spv::ExecutionModelVertex) {
                far_half(stage.source);
            } else if (stage.model == spv::ExecutionModelFragment) {
                clamped_half(stage.source);
            }
        }
        if (!spirv.compute.empty()) {
            for (std::uint32_t i = 0; i < 3; ++i) {
                out.workgroup[i] =
                    stages[0].compiler->get_execution_mode_argument(spv::ExecutionModeLocalSize, i);
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
