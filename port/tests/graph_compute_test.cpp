// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks reflected compute reads across shared ring allocations and storage promotion with synthetic colours.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <exception>

#include "enhanced/renderer/compute_graph.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/shader.h"
#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace graph = renderer::graph;
namespace wgl = renderer::world::gl;
// One dispatch adds independent uniform and texel-buffer inputs and packs a known RGBA result.
constexpr auto kKernel = R"(#version 430 core

layout(local_size_x=1) in;

layout(std140) uniform Inputs { vec4 parameters; };

uniform samplerBuffer tex_T0;

layout(std430,binding=11) writeonly buffer Output { uint colour; };

void main(){colour=packUnorm4x8((parameters+texelFetch(tex_T0,0))/16.0);}

)";

// Synthetic inputs produce (9, 11, 13, 15) / 16 after addition.
constexpr std::array<float, 4> kUniform{2, 3, 4, 5};
constexpr std::array<float, 4> kTexels{7, 8, 9, 10};

// Owns the input views and writable output for one synchronous regression.
struct Fixture {
    std::array<unsigned, 3> buffers{};  // Uniform, texel and output allocations.
    unsigned texture = 0;               // Texel view of the second buffer.
    renderer::Shader shader;            // Reflected compute descriptors.
    Fixture();
    ~Fixture();
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
};

// Uploads two small inputs that share one ring allocation at distinct byte ranges.
Fixture::Fixture() {
    glGenBuffers(static_cast<int>(buffers.size()), buffers.data());
    glBindBuffer(wgl::GL_UNIFORM_BUFFER, buffers[0]);
    glBufferData(wgl::GL_UNIFORM_BUFFER, sizeof(kUniform), kUniform.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(wgl::GL_UNIFORM_BUFFER, 3, buffers[0]);
    glBindBuffer(wgl::GL_TEXTURE_BUFFER, buffers[1]);
    glBufferData(wgl::GL_TEXTURE_BUFFER, sizeof(kTexels), kTexels.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, buffers[2]);
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, texture);
    glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32F, buffers[1]);
    std::string error;
    CHECK(shader.build_compute("graph input ranges", kKernel, error));
    shader.use();
    glUniformBlockBinding(shader.id(), glGetUniformBlockIndex(shader.id(), "Inputs"), 3);
}

// Retires the owned renderer handles after synchronous graph execution.
Fixture::~Fixture() {
    glDeleteTextures(1, &texture);
    glDeleteBuffers(static_cast<int>(buffers.size()), buffers.data());
}

// Confirms graph output ranges are validated before a dispatch callback can run.
void test_graph_rejects_invalid_output(Fixture& fixture) {
    fixture.shader.use();
    // The allocation holds one word; declaring two words must fail before any GPU write.
    const std::array outputs{enhanced::ComputeOutput{fixture.buffers[2], 2 * sizeof(unsigned)}};
    std::string error;
    bool called = false;
    CHECK(!enhanced::run_compute_graph("invalid output", outputs, [&] { called = true; }, error));
    CHECK(!called);
    CHECK(!error.empty());
}

// A failed sampler capture must not append its previously valid uniform reads to the caller's pass.
void test_input_capture_is_atomic(Fixture& fixture) {
    constexpr auto kImageKernel = R"(#version 430 core
layout(local_size_x=1) in;
layout(std140) uniform Inputs { vec4 parameters; };
uniform sampler2D tex_T0;
layout(std430,binding=11) writeonly buffer Output { uint colour; };
void main(){colour=packUnorm4x8(parameters+texelFetch(tex_T0,ivec2(0),0));}
)";
    renderer::Shader shader;
    std::string error;
    CHECK(shader.build_compute("capture transaction", kImageKernel, error));
    shader.use();
    glUniformBlockBinding(shader.id(), glGetUniformBlockIndex(shader.id(), "Inputs"), 3);
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Two mip levels exceed this executor's whole-single-level import contract.
    const std::array<float, 16> pixels{};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_FLOAT, pixels.data());
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_FLOAT, pixels.data());
    vkgl::GraphExecution execution;
    const auto existing =
        execution.read_buffer("existing", fixture.buffers[0], graph::Stage::ComputeShader);
    CHECK(existing);
    graph::Pass pass{"unchanged", {}, {*existing}};
    const auto failed = execution.capture_compute_reads(pass);
    CHECK(!failed);
    CHECK(failed.error().message.find("multiple mip levels") != std::string::npos);
    CHECK(pass.name == "unchanged" && pass.images.empty() && pass.buffers.size() == 1);
    CHECK(pass.buffers[0].buffer.index == existing->buffer.index);
    CHECK(pass.buffers[0].range == existing->range);
    CHECK(execution.capture_compute_reads(pass, vkgl::ComputeInputs::UniformBlocks));
    CHECK(pass.images.empty() && pass.buffers.size() == 2);
    glDeleteTextures(1, &texture);
    fixture.shader.use();
}

// Unsupported attachment aspects must fail instead of silently importing the colour attachment.
void test_attachment_aspects() {
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(1, 1, error));
    vkgl::GraphExecution execution;
    CHECK(!execution.import_attachment("stencil", target.id(), graph::Aspect::Stencil));
    CHECK(!execution.import_attachment(
        "both", target.id(), graph::Aspect::Colour | graph::Aspect::Depth
    ));
    CHECK(execution.import_attachment("colour", target.id(), graph::Aspect::Colour));
    target.release();
}

// Callback failures consume the recording and return an error rather than escaping the render thread.
void test_callback_failure() {
    vkgl::GraphExecution execution;
    execution.add_pass({"throwing callback"});
    unsigned calls = 0;
    // The synchronous executor must contain even exceptions that do not derive from std::exception.
    const auto failed = execution.run([&](unsigned) {
        ++calls;
        throw 0;
    });
    CHECK(!failed);
    CHECK(failed.error().message.find("unknown exception") != std::string::npos);
    CHECK(calls == 1);
    CHECK(!execution.run([&](unsigned) { ++calls; }));
    CHECK(calls == 1);
}

// Reads a packed colour whose arithmetic is independent of either synchronisation path.
void check_colour(unsigned buffer) {
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(1, 1, error));
    CHECK(vkgl::copy_buffer_to_texture(buffer, target.colour_texture(), 1, 1));
    const auto pixels = target.read_rgba();

    // Each channel is the independently added synthetic inputs, normalized and packed to eight bits.
    for (std::size_t channel = 0; channel < kUniform.size(); ++channel) {
        CHECK(pixels[channel] == std::lround((kUniform[channel] + kTexels[channel]) * 255 / 16));
    }

    target.release();
}

// Sampling first must not prevent a later same-frame SSBO binding from acquiring stable storage.
void test_storage_promotion(Fixture& fixture) {
    vkgl::GraphExecution sampled;
    CHECK(sampled.read_buffer("sampled first", fixture.buffers[0], graph::Stage::ComputeShader));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, fixture.buffers[0]);
    vkgl::GraphExecution promoted;
    CHECK(promoted.import_buffer("promoted storage", fixture.buffers[0]));
}

// Typed graph copies retain a ring view's byte offset and may run only inside a declared transfer callback.
void test_graph_copy_range() {
    constexpr std::array<std::uint32_t, 2> kSource{0xDEADBEEFu, 0xF0C08040u};
    unsigned buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(wgl::GL_UNIFORM_BUFFER, buffer);
    glBufferData(wgl::GL_UNIFORM_BUFFER, sizeof(kSource), kSource.data(), GL_STREAM_DRAW);
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(1, 1, error));
    vkgl::GraphExecution execution;
    auto source = execution.read_buffer("copy view", buffer, graph::Stage::Transfer);
    const auto image = execution.import_image(
        "copy target", target.colour_texture(), vkgl::InitialContents::Discard
    );
    CHECK(source && image);
    source->range.offset += sizeof(std::uint32_t);
    source->range.size = sizeof(std::uint32_t);
    CHECK(!execution.copy_buffer_to_image(*source, *image));
    execution.add_pass(
        {"copy exact word",
         {{*image,
           {},
           {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}},
         {*source}}
    );
    execution.add_pass(
        {"sample copied word",
         {{*image,
           {},
           {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled}}},
         {},
         graph::PassKind::Export}
    );
    // The callback tries a short range first, then records the exact aligned source word.
    const auto copied = execution.run([&](unsigned) {
        auto short_read = *source;
        short_read.range.size -= 1;
        CHECK(!execution.copy_buffer_to_image(short_read, *image));
        auto misaligned = *source;
        ++misaligned.range.offset;
        CHECK(!execution.copy_buffer_to_image(misaligned, *image));
        CHECK(execution.copy_buffer_to_image(*source, *image));
    });
    CHECK(copied);
    CHECK(!execution.copy_buffer_to_image(*source, *image));
    const auto pixels = target.read_rgba();
    const auto expected = std::bit_cast<std::array<std::uint8_t, 4>>(kSource[1]);
    CHECK(std::equal(pixels.begin(), pixels.end(), expected.begin(), expected.end()));
    target.release();
    glDeleteBuffers(1, &buffer);
}

// Shared upload allocations retain disjoint graph ranges and produce the expected GPU arithmetic.
void test_reflected_reads(Fixture& fixture) {
    const auto& buffers = fixture.buffers;
    auto& shader = fixture.shader;
    vkgl::GraphExecution execution;
    const auto uniforms =
        execution.read_buffer("uniforms", buffers[0], graph::Stage::ComputeShader);
    const auto texels = execution.read_buffer("texels", buffers[1], graph::Stage::ComputeShader);
    CHECK(uniforms && texels);
    CHECK(uniforms->buffer.index == texels->buffer.index);
    CHECK(uniforms->range.offset != texels->range.offset);
    const auto output =
        execution.import_buffer("result", buffers[2], vkgl::InitialContents::Discard);
    CHECK(output.has_value());
    graph::Pass pass{
        "add inputs",
        {},
        {{*output,
          {0, sizeof(unsigned)},
          {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}}
    };
    CHECK(execution.capture_compute_reads(pass).has_value());
    execution.add_pass(std::move(pass));
    execution.add_pass(
        {"copy result",
         {},
         {{*output,
           {0, sizeof(unsigned)},
           {graph::Stage::Transfer, graph::Access::Read, graph::Layout::Buffer}}},
         graph::PassKind::Export}
    );
    CHECK(execution
              .run([&](unsigned) {
                  shader.use();
                  glDispatchCompute(1, 1, 1);
              })
              .has_value());
    check_colour(buffers[2]);
}
}  // namespace

// Keeps optional Vulkan setup and unexpected failures inside the regression executable.
int main() {
    // Setup and execution failures stay within the test process.
    try {
        test::GlContext context;
        std::string error;

        // Hosts without a Vulkan device cannot execute this GPU regression.
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }

        Fixture fixture;
        test_graph_rejects_invalid_output(fixture);
        test_reflected_reads(fixture);
        test_input_capture_is_atomic(fixture);
        test_attachment_aspects();
        test_callback_failure();
        test_storage_promotion(fixture);
        test_graph_copy_range();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "graph compute: %s\n", failure.what());
        return 1;
    }
}
