// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shader source keys, bounded LZ4 blocks and native-compute reflection encoding.
#include <lz4.h>
#include <lz4hc.h>
#include <stdexcept>
#include <utility>

#include "renderer/vulkan/pipelines/shader_package.h"
#include "renderer/vulkan/pipelines/shader_package_internal.h"
#include "shader_package_version.h"

namespace openrac::vkgl {
using namespace package_detail;

std::string shader_package_fingerprint() {
    return OPENRAC_SHADER_PACKAGE_VERSION;
}

ShaderKey shader_key(
    std::string_view kind,
    std::initializer_list<std::string_view> parts,
    std::string_view fingerprint
) {
    std::string text =
        fingerprint.empty() ? shader_package_fingerprint() : std::string(fingerprint);
    text += '\0';
    text += kind;
    text += '\0';

    for (auto part : parts) {
        text += part;
        text += '\0';
    }

    ShaderKey out{kBasis, kBasis, text.size()};

    for (std::size_t n = 0; n < text.size(); ++n) {
        out.key = (out.key ^ static_cast<unsigned char>(text[n])) * kPrime;
        out.check = (out.check ^ static_cast<unsigned char>(text[text.size() - n - 1])) * kPrime;
    }

    return out;
}

std::vector<char> compress_shader_blob(std::span<const char> bytes) {
    if (bytes.empty() || bytes.size() > kMaxShaderBytes) {
        throw std::runtime_error("shader entry exceeds package limit");
    }

    const int size = static_cast<int>(bytes.size());
    std::vector<char> out(static_cast<std::size_t>(LZ4_compressBound(size)));
    const int written = LZ4_compress_HC(
        bytes.data(), out.data(), size, static_cast<int>(out.size()), LZ4HC_CLEVEL_MAX
    );

    if (written <= 0) {
        throw std::runtime_error("cannot compress shader entry");
    }

    out.resize(static_cast<std::size_t>(written));
    return out;
}

std::vector<char> decompress_shader_blob(std::span<const char> bytes, std::uint64_t size) {
    if (!size || size > kMaxShaderBytes || bytes.size() > kMaxShaderBytes) {
        return {};
    }

    std::vector<char> out(static_cast<std::size_t>(size));

    if (LZ4_decompress_safe(
            bytes.data(), out.data(), static_cast<int>(bytes.size()), static_cast<int>(out.size())
        )
        != static_cast<int>(out.size())) {
        return {};
    }

    return out;
}

std::vector<char> encode_native_shader(const NativeComputeShader& shader) {
    CompiledProgram encoded;
    encoded.compute = shader.code;

    for (const auto& binding : shader.bindings) {
        ProgramUniform uniform;
        uniform.name = binding.name;
        uniform.gl_type = static_cast<unsigned>(binding.kind);
        uniform.binding = static_cast<int>(binding.binding);
        uniform.array_size = static_cast<int>(binding.count);
        encoded.uniforms.push_back(std::move(uniform));
    }

    return encode_program(encoded);
}

bool decode_native_shader(std::span<const char> bytes, NativeComputeShader& shader) {
    CompiledProgram encoded;

    if (!decode_program(bytes, encoded) || encoded.compute.empty()) {
        return false;
    }

    NativeComputeShader out;
    out.code = std::move(encoded.compute);

    for (const auto& uniform : encoded.uniforms) {
        if (uniform.gl_type > static_cast<unsigned>(NativeBindingKind::Sampler)
            || uniform.array_size < 1) {
            return false;
        }

        // glslang uses UINT32_MAX for unbound push-constant blocks; preserve those binding bits.
        out.bindings.push_back(
            {uniform.name,
             static_cast<NativeBindingKind>(uniform.gl_type),
             static_cast<unsigned>(uniform.binding),
             static_cast<unsigned>(uniform.array_size)}
        );
    }

    shader = std::move(out);
    return true;
}
}  // namespace openrac::vkgl
