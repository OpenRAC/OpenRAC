// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU shader reflection round trips and archive validation using synthetic GLSL and corrupt inputs.

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <span>
#include <vector>

#include "renderer/vulkan/pipelines/shader_backend.h"
#include "renderer/vulkan/pipelines/shader_package.h"
#include "tests/check.h"

using namespace openrac::vkgl;

namespace {
// Reads a little-endian archive word from a valid encoder-produced test fixture.
std::uint64_t archive_word(std::span<const char> bytes, std::size_t at) {
    std::uint64_t value = 0;

    // Archive header and index fields are unsigned 64-bit words.
    for (unsigned n = 0; n < sizeof(value); ++n) {
        value |= std::uint64_t(static_cast<unsigned char>(bytes[at + n])) << (n * CHAR_BIT);
    }

    return value;
}

// Corrupts one known index field without changing the fixture's remaining bytes.
void overwrite_archive_word(std::span<char> bytes, std::size_t at, std::uint64_t value) {
    // Each byte receives the corresponding low eight bits of the little-endian word.
    for (unsigned n = 0; n < sizeof(value); ++n) {
        bytes[at + n] = static_cast<char>(static_cast<unsigned char>(value >> (n * CHAR_BIT)));
    }
}

// Synthetic SPIR-V-shaped streams exercise chunk storage without asserting instruction semantics.
CompiledProgram chunking_program() {
    // A five-word SPIR-V 1.0 header; the remaining fixture words are opaque test data.
    constexpr std::array<std::uint32_t, 5> kHeader{0x07230203, 0x00010000, 0, 1, 0};
    // The encoded count makes this fragment occupy three full 16 KiB blocks and one partial block.
    constexpr std::size_t kFragmentBytes = std::size_t{48} * 1024;
    CompiledProgram program;
    program.vertex.assign(kHeader.begin(), kHeader.end());
    program.fragment.resize(kFragmentBytes / sizeof(std::uint32_t));
    // Fixed xorshift32 seed and 13/17/5 shifts make the fixture repeatable and hard to compress.
    std::uint32_t state = 0x9e3779b9;

    for (auto& word : program.fragment) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        word = state;
    }

    std::copy(kHeader.begin(), kHeader.end(), program.fragment.begin());
    return program;
}

// Different vertex lengths share all fragment blocks and still reconstruct the original programs.
void test_stage_block_sharing() {
    const auto first = chunking_program();
    auto second = first;
    second.vertex.push_back(0);
    const auto a = encode_program(first), b = encode_program(second);
    const auto ka = shader_key("test", {"vertex a"}), kb = shader_key("test", {"vertex b"});
    const ShaderPackageEntry entries[] = {{ka, a}, {kb, b}};
    const auto bytes = encode_shader_package("chunks", entries);
    const auto fingerprint_size = archive_word(bytes, sizeof(std::uint64_t));
    // Magic, fingerprint length and entry count precede the chunk count.
    constexpr std::size_t kChunkCountWord = 3;
    // Two vertices, four shared fragment blocks, shared empty compute and shared reflection.
    constexpr std::uint64_t kExpectedChunks = 2 + 4 + 1 + 1;
    CHECK(
        archive_word(bytes, kChunkCountWord * sizeof(std::uint64_t) + fingerprint_size)
        == kExpectedChunks
    );
    ShaderPackage package;
    std::string error;
    CHECK(package.load(bytes, "chunks", error));
    CHECK(package.find(ka) == a);
    CHECK(package.find(kb) == b);
}

// Partial section blocks do not relax entry-size, reference-count or archive-version validation.
void test_stage_block_validation() {
    const auto program = chunking_program();
    const auto value = encode_program(program);
    const auto key = shader_key("test", {"chunk bounds"});
    const ShaderPackageEntry entry{key, value};
    const auto bytes = encode_shader_package("chunks", {&entry, 1});
    // Four header words surround the fingerprint; each index starts with three key words.
    constexpr std::size_t kHeaderWords = 4, kSizeWord = 3, kPartsWord = 4;
    const auto index =
        kHeaderWords * sizeof(std::uint64_t) + archive_word(bytes, sizeof(std::uint64_t));
    const auto parts_at = index + kPartsWord * sizeof(std::uint64_t);
    const auto parts = archive_word(bytes, parts_at);
    ShaderPackage package;
    std::string error;

    // The fixture uses the maximum permitted partial blocks, so one more reference is invalid.
    for (const auto invalid : {std::uint64_t{0}, parts + 1}) {
        auto corrupt = bytes;
        overwrite_archive_word(corrupt, parts_at, invalid);
        CHECK(!package.load(corrupt, "chunks", error));
        CHECK(package.find(key).empty());
    }

    auto corrupt = bytes;
    overwrite_archive_word(corrupt, index + kSizeWord * sizeof(std::uint64_t), value.size() + 1);
    CHECK(!package.load(corrupt, "chunks", error));
    corrupt = bytes;
    // The last of eight magic bytes selects the layout version.
    constexpr std::size_t kFormatByte = 7;
    corrupt[kFormatByte] = '1';
    CHECK(!package.load(corrupt, "chunks", error));
}

// Opaque markers and impossible vector prefixes retain their original bytes without stage parsing.
void test_opaque_entries() {
    // Every byte set makes the first encoded word count exceed this bounded opaque payload.
    const std::vector<char> opaque(sizeof(std::uint32_t), static_cast<char>(-1));
    const std::vector<char> marker{1};
    const auto data_key = shader_key("test", {"opaque"});
    const auto marker_key = shader_key("test", {"marker"});
    const ShaderPackageEntry entries[] = {{data_key, opaque}, {marker_key, marker}};
    const auto bytes = encode_shader_package("opaque", entries);
    ShaderPackage package;
    std::string error;
    CHECK(package.load(bytes, "opaque", error));
    CHECK(package.find(data_key) == opaque);
    CHECK(package.find(marker_key) == marker);
}

// Counts instructions in compiler-produced SPIR-V using the specification's word-count encoding.
std::size_t instruction_count(std::span<const std::uint32_t> code, std::uint32_t opcode) {
    // SPIR-V starts with five header words; each instruction packs its length above its opcode.
    constexpr std::size_t kHeaderWords = 5;
    constexpr std::uint32_t kOpcodeMask = 0xffff, kWordCountShift = 16;
    std::size_t count = 0;

    // Compiler output is already validated, so each instruction has a nonzero bounded length.
    for (std::size_t at = kHeaderWords; at < code.size(); at += code[at] >> kWordCountShift) {
        count += (code[at] & kOpcodeMask) == opcode;
    }

    return count;
}

// Performance passes inline helper calls while retaining shader specialization constants.
void test_performance_optimization() {
    // SPIR-V's OpFunctionCall and OpSpecConstant instruction numbers.
    constexpr std::uint32_t kFunctionCall = 57, kSpecConstant = 50;
    const std::string vertex = "#version 410 core\nlayout(location=0) in vec3 position; "
                               "vec4 transform(vec3 p){return vec4(p,1);} "
                               "void main(){gl_Position=transform(position);}";
    const std::string fragment =
        "#version 450 core\nlayout(constant_id=7) const float gain=1; "
        "layout(location=0) out vec4 colour; float shade(float x){return x*x;} "
        "void main(){colour=vec4(shade(gain));}";
    CompiledProgram compiled;
    std::string error;
    CHECK(compiler_detail::compile_program_fresh(vertex, fragment, compiled, error));
    CHECK(instruction_count(compiled.vertex, kFunctionCall) == 0);
    CHECK(instruction_count(compiled.fragment, kFunctionCall) == 0);
    CHECK(instruction_count(compiled.fragment, kSpecConstant) == 1);

    const std::string compute =
        "#version 450\nlayout(local_size_x=1) in; "
        "layout(binding=0,rgba32f) uniform image2D output_image; "
        "float shade(float x){return x*x;} "
        "void "
        "main(){imageStore(output_image,ivec2(0),vec4(shade(float(gl_GlobalInvocationID.x))));}";
    std::vector<std::uint32_t> unoptimized;
    CHECK(compiler_detail::compile_compute_for_analysis(compute, unoptimized, error));
    CHECK(instruction_count(unoptimized, kFunctionCall) > 0);
    NativeComputeShader native;
    CHECK(compiler_detail::compile_native_fresh(compute, {}, {}, native, error));
    CHECK(!native.code.empty());
    CHECK(instruction_count(native.code, kFunctionCall) == 0);
}

// Compiles synthetic sources, round-trips their complete reflection and rejects corrupt archives.
void test_archive() {
    // Compile and reflect on the CPU, then retrieve exactly that result from the binary archive.
    const std::string vertex = "#version 410 core\nlayout(location=0) in vec3 position; uniform "
                               "mat4 camera; void main(){gl_Position=camera*vec4(position,1);}";
    const std::string fragment =
        "#version 410 core\nuniform sampler2D tex; uniform vec4 tint; layout(location=0) out vec4 "
        "colour; void main(){colour=texture(tex,vec2(0))*tint;}";
    CompiledProgram compiled;
    std::string error;
    CHECK(compiler_detail::compile_program_fresh(vertex, fragment, compiled, error));
    CHECK(!compiled.vertex.empty() && !compiled.fragment.empty());
    CHECK(!compiled.uniforms.empty() && !compiled.attributes.empty());
    const auto value = encode_program(compiled);
    const auto key = shader_key("program", {vertex, fragment, {}});
    const ShaderPackageEntry entry{key, value};
    const auto bytes = encode_shader_package("test compiler", {&entry, 1});
    ShaderPackage package;
    CHECK(package.load(bytes, "test compiler", error));
    CompiledProgram restored;
    CHECK(decode_program(package.find(key), restored));
    CHECK(encode_program(restored) == value);
    auto wrong = key;
    ++wrong.check;
    CHECK(package.find(wrong).empty());
    CHECK(shader_key("program", {"ab", "c"}) != shader_key("program", {"a", "bc"}));
    CHECK(!package.load(bytes, "other compiler", error));
    CHECK(package.find(key).empty());

    auto corrupt = bytes;
    corrupt.back() ^= 1;
    CHECK(!package.load(corrupt, "test compiler", error));

    for (std::size_t cut : {std::size_t{0}, std::size_t{7}, bytes.size() - 1}) {
        CHECK(!package.load(std::span(bytes).first(cut), "test compiler", error));
    }

    corrupt = bytes;
    corrupt.push_back(0);
    CHECK(!package.load(corrupt, "test compiler", error));
    const ShaderPackageEntry duplicates[] = {entry, entry};
    CHECK(
        !package.load(encode_shader_package("test compiler", duplicates), "test compiler", error)
    );
}

// Preserves explicit native descriptors through the same bytecode/reflection codec.
void test_native_reflection() {
    std::string error;
    NativeComputeShader native;
    CHECK(
        compiler_detail::compile_native_fresh(
            "#version 450\nlayout(local_size_x=1) in; layout(binding=0,rgba16f) uniform image2D "
            "output_image; layout(push_constant) uniform Parameters { float value; } params; void "
            "main(){imageStore(output_image,ivec2(0),vec4(params.value));}",
            {},
            {},
            native,
            error
        )
    );
    CHECK(native.bindings.size() == 2);
    NativeComputeShader native_restored;
    CHECK(decode_native_shader(encode_native_shader(native), native_restored));
    CHECK(encode_native_shader(native_restored) == encode_native_shader(native));
}

// Loads the shipped archive without a window or Vulkan device.
void test_embedded_package() {
    std::string error;
    // The shipped package can be loaded without creating a window, Vulkan instance or GPU device.
    ShaderPackage builtin;
    CHECK(builtin.load(embedded_shader_package(), shader_package_fingerprint(), error));
}

// Runs the invocation while the executable boundary retains exception reporting.
int run_tests() {
    test_stage_block_sharing();
    test_stage_block_validation();
    test_opaque_entries();
    test_performance_optimization();
    test_archive();
    test_native_reflection();
    test_embedded_package();
    return openrac::test::result();
}
}  // namespace

int main() {
    try {
        return run_tests();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "unexpected shader test exception: %s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected shader test exception\n");
        return 1;
    }
}
