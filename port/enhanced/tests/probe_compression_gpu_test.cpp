// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compressed Vulkan atlas allocation and subimage copies must match the CPU HDR decoder.
#include <bit>
#include <cmath>
#include <cstdio>

#include "enhanced/lighting/probe_texture_encode.h"
#include "enhanced/viewer/probe_texture_upload.h"
#include "renderer/framebuffer.h"
#include "renderer/shader.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
constexpr unsigned kSide = 8;  // One bordered probe fills an ASTC block or four BC6H blocks.
constexpr const char* kVertex = R"(#version 410 core




void main() {




 vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));




 gl_Position=vec4(p*2.-1.,0,1);




})";
constexpr const char* kFragment = R"(#version 410 core




uniform sampler2D image;




layout(location=0) out vec4 result;




void main() {




 ivec2 at=ivec2(gl_FragCoord.xy);




 float value=texelFetch(image,ivec2(at.x/3,at.y+8),0)[at.x%3];




 uint bits=floatBitsToUint(value);




 result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;




})";

// Own all GPU objects until readback completes, including failures during fixture setup.
struct UploadFixture {
    unsigned texture = 0, vao = 0;
    renderer::FrameBuffer target;
    renderer::Shader shader;
    std::string error;

    // Object ownership cannot be copied to another GL context or duplicated.
    UploadFixture() = default;
    UploadFixture(const UploadFixture&) = delete;
    UploadFixture& operator=(const UploadFixture&) = delete;

    // GPU objects are released while the hidden test context is still alive.
    ~UploadFixture() {
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &texture);
    }

    // A full allocation followed by a distinct block-aligned row copy exercises both upload paths.
    void upload(lighting::ProbeTextureFormat format, std::span<const std::uint8_t> bytes) {
        const auto internal = lighting::probe_texture_internal(format);
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        std::vector<std::uint8_t> empty(lighting::probe_texture_bytes(format, kSide, kSide * 2));
        glCompressedTexImage2D(
            GL_TEXTURE_2D,
            0,
            internal,
            kSide,
            kSide * 2,
            0,
            static_cast<int>(empty.size()),
            empty.data()
        );
        glCompressedTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            kSide,
            kSide,
            kSide,
            internal,
            static_cast<int>(bytes.size()),
            bytes.data()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Encode float bits into RGBA8 so readback compares HDR values without display conversion.
    std::vector<std::uint8_t> draw() {
        CHECK(target.create(kSide * 3, kSide, error));
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, kSide * 3, kSide);
        CHECK(shader.build("compressed atlas upload", kVertex, kFragment, error));
        shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(shader.uniform("image"), 0);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::vector<std::uint8_t> output(std::size_t{kSide} * kSide * 3 * 4);
        glReadPixels(0, 0, kSide * 3, kSide, GL_RGBA, GL_UNSIGNED_BYTE, output.data());
        return output;
    }
};

// CPU reference decoding must match the independent Vulkan implementation for the same physical blocks.
void check_upload(lighting::ProbeTextureFormat format, std::span<const std::uint8_t> bytes) {
    const auto expected = lighting::decode_probe_texture(format, kSide, kSide, bytes);
    UploadFixture fixture;
    fixture.upload(format, bytes);
    const auto output = fixture.draw();

    for (unsigned i = 0; i < kSide * kSide * 3; ++i) {
        std::uint32_t bits = 0;

        for (unsigned c = 0; c < 4; ++c) {
            bits |= static_cast<unsigned>(output[i * 4 + c]) << (c * 8);
        }

        CHECK(std::abs(std::bit_cast<float>(bits) - expected[i / 3][i % 3]) < .0001f);
    }
}

// Sample every texel after a complete upload and an independent block-aligned replacement.
void test_compressed_upload(lighting::ProbeTextureFormat format) {
    std::vector<lighting::Float3> source(std::size_t{kSide} * kSide);

    for (unsigned i = 0; i < source.size(); ++i) {
        source[i] = {.2f + static_cast<float>(i % kSide) * .02f, .4f, .8f};
    }

    const auto bytes = lighting::encode_probe_texture(format, kSide, kSide, source, {true});
    check_upload(format, bytes);
}

// Exercise BC6H mode 5 with blue-Y's high delta bit set, independent of encoder mode selection.
void test_bc6_mode_five() {
    // Mode 0x0A, blue base 512 at bit 25, blue-Y delta -16 via bit 40, partition zero and indices zero.
    const std::array<std::uint8_t, 16> block{0x0A, 0, 0, 0, 4, 1};
    std::vector<std::uint8_t> bytes;
    for (unsigned i = 0; i < 4; ++i) {
        bytes.insert(bytes.end(), block.begin(), block.end());
    }
    check_upload(lighting::ProbeTextureFormat::Bc6h, bytes);
}

}  // namespace

// Unsupported hardware skips only its codec; the ordinary CPU tests cover both encoders.
int main() {
    try {
        openrac::test::GlContext context;
        std::string error;

        // These uploads use the production Vulkan backend, not a system OpenGL implementation.
        if (!context.open(error) || !openrac::gl::is_vulkan()) {
            return openrac::test::kSkip;
        }

        // BC6H is available on this test's desktop adapter when advertised by the backend.
        if (openrac::gl::has_extension("GL_ARB_texture_compression_bptc")) {
            test_compressed_upload(openrac::lighting::ProbeTextureFormat::Bc6h);
            test_bc6_mode_five();
        }

        // ASTC HDR is tested on devices that expose the actual HDR feature, not just ASTC LDR.
        if (openrac::gl::has_extension("GL_KHR_texture_compression_astc_hdr")) {
            test_compressed_upload(openrac::lighting::ProbeTextureFormat::Astc8x8);
        }

        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
