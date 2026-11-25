// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU shader reflection round trips and archive validation using synthetic GLSL and corrupt inputs.

#include <cstdio>
#include <exception>

#include "renderer/vulkan/pipelines/shader_backend.h"
#include "renderer/vulkan/pipelines/shader_package.h"
#include "tests/check.h"

using namespace openrac::vkgl;

namespace {
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
