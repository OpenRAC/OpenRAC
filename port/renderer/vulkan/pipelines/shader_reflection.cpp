// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GL-compatible buffer, uniform and attribute reflection from an already-linked glslang program.
#include <utility>

#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "renderer/vulkan/pipelines/shader_compiler_internal.h"

#include <glslang/Include/Types.h>

namespace openrac::vkgl::compiler_detail {
namespace {
// The GL_* types reflection answers with (glDefineType), the samplers among them.
constexpr int kSampler2D = 0x8B5E;
constexpr int kSamplerCube = 0x8B60;
constexpr int kSampler2DArray = 0x8DC1;
constexpr int kSamplerBuffer = 0x8DC2;
constexpr int kIntSampler2D = 0x8DCA;
constexpr int kIntSamplerCube = 0x8DCC;
constexpr int kIntSampler2DArray = 0x8DCF;
constexpr int kIntSamplerBuffer = 0x8DD0;
constexpr int kUnsignedSampler2D = 0x8DD2;
constexpr int kUnsignedSamplerCube = 0x8DD4;
constexpr int kUnsignedSampler2DArray = 0x8DD7;
constexpr int kUnsignedSamplerBuffer = 0x8DD8;

bool sampler_kind(int type, SamplerKind& kind) {
    switch (type) {
        case kSampler2D:
        case kIntSampler2D:
        case kUnsignedSampler2D:
            kind = SamplerKind::Texture2D;
            return true;
        case kSampler2DArray:
        case kIntSampler2DArray:
        case kUnsignedSampler2DArray:
            kind = SamplerKind::Texture2DArray;
            return true;
        case kSamplerCube:
        case kIntSamplerCube:
        case kUnsignedSamplerCube:
            kind = SamplerKind::Cube;
            return true;
        case kSamplerBuffer:
        case kIntSamplerBuffer:
        case kUnsignedSamplerBuffer:
            kind = SamplerKind::Buffer;
            return true;
        default:
            return false;
    }
}

// Bytes of one element of a non-opaque uniform type, for an array's std140 stride.
int element_bytes(int type) {
    switch (type) {
        case 0x8B5C:
            return 64;  // mat4
        case 0x8B5B:
            return 48;  // mat3, three padded columns
        case 0x8B5A:
            return 32;  // mat2
        case 0x8B52:
        case 0x8B55:
        case 0x8DC8:
            return 16;  // vec4, ivec4, uvec4
        case 0x8B51:
        case 0x8B54:
        case 0x8DC7:
            return 12;
        case 0x8B50:
        case 0x8B53:
        case 0x8DC6:
            return 8;
        default:
            return 4;
    }
}

// Retains named storage blocks and their read-only qualifiers.
void reflect_storage(glslang::TProgram& program, CompiledProgram& out) {
    for (int i = 0; i < program.getNumBufferBlocks(); ++i) {
        const auto& b = program.getBufferBlock(i);
        const auto* type = b.getType();
        out.storage.push_back(
            {b.name, b.getBinding(), b.size, type && type->getQualifier().isReadOnly()}
        );
    }
}

// Retains uniform-block bindings and returns the index of GL's default block.
int reflect_blocks(glslang::TProgram& program, CompiledProgram& out) {
    int default_index = -1;

    for (int i = 0; i < program.getNumUniformBlocks(); ++i) {
        const glslang::TObjectReflection& b = program.getUniformBlock(i);

        if (b.name == "gl_DefaultUniformBlock") {
            default_index = i;
            out.default_binding = b.getBinding();
            out.default_size = b.size;
        } else {
            std::string name = b.name;
            out.blocks.push_back(ProgramBlock{name, b.getBinding(), b.size});
        }
    }

    return default_index;
}

// Retains sampler locations and the original std140 offsets of loose uniforms.
void reflect_uniforms(glslang::TProgram& program, CompiledProgram& out, int default_index) {
    for (int i = 0; i < program.getNumUniformVariables(); ++i) {
        const glslang::TObjectReflection& u = program.getUniform(i);
        ProgramUniform pu;
        pu.name = u.name;

        if (const auto bracket = pu.name.find('['); bracket != std::string::npos) {
            pu.name.resize(bracket);
        }

        pu.gl_type = static_cast<std::uint32_t>(u.glDefineType);
        pu.array_size = u.size > 0 ? u.size : 1;

        if (sampler_kind(u.glDefineType, pu.kind)) {
            pu.sampler = true;
            pu.binding = u.getBinding();
        } else {
            if (u.index != default_index || default_index < 0) {
                continue;  // a member of a named block: read through the block's buffer
            }

            pu.offset = u.offset;
            const int bytes = element_bytes(u.glDefineType);
            pu.array_stride = u.arrayStride > 0 ? u.arrayStride : (bytes + 15) / 16 * 16;
        }

        out.uniforms.push_back(std::move(pu));
    }
}

// Expands matrix inputs into the per-column locations consumed by GL vertex arrays.
void reflect_attributes(glslang::TProgram& program, CompiledProgram& out) {
    for (int i = 0; i < program.getNumPipeInputs(); ++i) {
        const glslang::TObjectReflection& a = program.getPipeInput(i);
        // A matrix takes a location for each column, as GL's attributes do.
        int columns = 1;
        std::uint32_t type = static_cast<std::uint32_t>(a.glDefineType);

        switch (type) {
            case 0x8B5C:
                columns = 4;
                type = 0x8B52;
                break;  // mat4: four vec4
            case 0x8B5B:
                columns = 3;
                type = 0x8B51;
                break;  // mat3: three vec3
            case 0x8B5A:
                columns = 2;
                type = 0x8B50;
                break;  // mat2: two vec2
            default:
                break;
        }

        const int location = static_cast<int>(a.layoutLocation());

        for (int c = 0; c < columns; ++c) {
            out.attributes.push_back({a.name, location + c, type});
        }
    }
}
}  // namespace

void reflect_program(glslang::TProgram& program, CompiledProgram& out, EShLanguage stage) {
    reflect_storage(program, out);
    const int default_index = reflect_blocks(program, out);
    reflect_uniforms(program, out, default_index);

    if (stage == EShLangVertex) {
        reflect_attributes(program, out);
    }
}
}  // namespace openrac::vkgl::compiler_detail
