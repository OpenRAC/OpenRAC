// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks reflected compute reads across shared ring allocations and storage promotion with synthetic colours.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <exception>

#include "common/breadcrumbs.h"
#include "enhanced/renderer/compute_graph.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/graph/graph_execution.h"
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

// A bound output cannot escape dependency tracking by omitting its declaration.
void test_dispatch_rejects_undeclared_output(Fixture& fixture) {
    fixture.shader.use();
    vkgl::GraphExecution execution;
    graph::Pass pass{"undeclared dispatch output"};
    pass.execution = graph::Stage::ComputeShader;
    CHECK(execution.capture_compute_reads(pass));
    execution.add_pass(std::move(pass));
    const auto result = execution.run([](unsigned) { glDispatchCompute(1, 1, 1); });
    CHECK(!result);
    if (!result) {
        CHECK(result.error().message.find("undeclared storage buffer") != std::string::npos);
    }
}

// Rebinding a declared output must fail before the dispatch is encoded.
void test_dispatch_rejects_rebound_output(Fixture& fixture) {
    unsigned other = 0;
    glGenBuffers(1, &other);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, other);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned), nullptr, GL_DYNAMIC_DRAW);

    fixture.shader.use();
    const std::array outputs{enhanced::ComputeOutput{fixture.buffers[2], sizeof(unsigned)}};
    std::string error;
    CHECK(!enhanced::run_compute_graph(
        "changed output binding",
        outputs,
        [&] {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, other);
            glDispatchCompute(1, 1, 1);
        },
        error
    ));
    CHECK(error.find("declared storage output") != std::string::npos);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, fixture.buffers[2]);
    glDeleteBuffers(1, &other);
}

void test_dispatch_resource_contract() {
    unsigned storage = 0, texture = 0;
    glGenBuffers(1, &storage);
    glGenTextures(1, &texture);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, storage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, storage);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    constexpr std::array<std::uint8_t, 4> red{255, 0, 0, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, red.data());
    renderer::Shader shader;
    renderer::FrameBuffer output;
    std::string error;
    CHECK(output.create_colour(4, 1, error));
    CHECK(shader.build_compute("storage contract", R"(#version 430 core
layout(local_size_x=1) in;
uniform sampler2D tex_T0;
layout(std430,binding=11) writeonly buffer Output { uint colour[4]; };
void main(){colour[3]=packUnorm4x8(texelFetch(tex_T0,ivec2(0),0));}
)", error));
    shader.use();
    enum class Contract { Complete, MissingSampler, ReadOnlyStorage, ShortStorage };
    for (const auto contract : {Contract::Complete, Contract::MissingSampler,
                                Contract::ReadOnlyStorage, Contract::ShortStorage}) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, storage);
        constexpr std::array<unsigned, 4> empty{};
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(empty), empty.data(), GL_DYNAMIC_DRAW);
        vkgl::GraphExecution execution;
        graph::Pass pass{"dispatch contract"};
        pass.execution = graph::Stage::ComputeShader;
        if (contract != Contract::MissingSampler) {
            CHECK(execution.capture_compute_reads(pass));
        }
        const auto buffer = execution.import_buffer("output", storage).value();
        pass.buffers.push_back({buffer, {0, contract == Contract::ShortStorage ? sizeof(unsigned) : sizeof(empty)},
            {graph::Stage::ComputeShader, contract == Contract::ReadOnlyStorage ? graph::Access::Read : graph::Access::Write,
             graph::Layout::Buffer}});
        execution.add_pass(std::move(pass));
        const auto result = execution.run([](unsigned) { glDispatchCompute(1, 1, 1); });
        CHECK(static_cast<bool>(result) == (contract == Contract::Complete));
        CHECK(vkgl::copy_buffer_to_texture(storage, output.colour_texture(), 4, 1));
        const auto pixels = output.read_rgba();
        CHECK(pixels.size() == 16);
        if (pixels.size() == 16) {
            const auto expected = contract == Contract::Complete ? red : std::array<std::uint8_t, 4>{};
            CHECK(std::equal(expected.begin(), expected.end(), pixels.begin() + 12));
        }
    }

    CHECK(shader.build_compute("storage descriptor range", R"(#version 430 core
layout(local_size_x=1) in;
layout(std430,binding=11) writeonly buffer Output { uint words[]; };
void main(){words[0]=uint(words.length());}
)", error));
    shader.use();
    vkgl::GraphExecution execution;
    const auto buffer = execution.import_buffer("output", storage).value();
    execution.add_pass({"one-word descriptor",
                        {},
                        {{buffer, {0, sizeof(unsigned)},
                          {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}}});
    CHECK(execution.run([](unsigned) { glDispatchCompute(1, 1, 1); }));
    CHECK(vkgl::copy_buffer_to_texture(storage, output.colour_texture(), 4, 1));
    const auto pixels = output.read_rgba();
    CHECK(pixels.size() == 16 && pixels[0] == 1);
    glDeleteBuffers(1, &storage);
    glDeleteTextures(1, &texture);
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
    // A preserving sampler import conflicts with this graph's discarded entry contract.
    const std::array<float, 16> pixels{};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_FLOAT, pixels.data());
    vkgl::GraphExecution execution;
    CHECK(execution.import_image("discarded sampler", texture, vkgl::InitialContents::Discard));
    const auto existing =
        execution.read_buffer("existing", fixture.buffers[0], graph::Stage::ComputeShader);
    CHECK(existing);
    graph::Pass pass{"unchanged", {}, {*existing}};
    const auto failed = execution.capture_compute_reads(pass);
    CHECK(!failed);
    CHECK(failed.error().message.find("inconsistent alias import") != std::string::npos);
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
    constexpr std::int64_t kFrame = 4;  // Isolates the callback failure's captured history.
    const crash::BreadcrumbScope context{crash::BreadcrumbThread::Render, kFrame};
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
    crash::Breadcrumbs::Snapshot history{};
    const auto count = crash::breadcrumbs().snapshot(history);
    CHECK(std::any_of(history.begin(), history.begin() + count, [](const auto& entry) {
        return entry.value == kFrame
               && std::string_view(entry.message) == "failed graph pass: throwing callback";
    }));
    CHECK(std::none_of(history.begin(), history.begin() + count, [](const auto& entry) {
        return entry.value == kFrame
               && std::string_view(entry.message) == "complete graph pass: throwing callback";
    }));
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
    graph::CommandBuffer commands;
    commands.copy_buffer_to_image(source->buffer, source->range, *image);
    const graph::Pass pass{
        "copy exact word",
        {{*image,
          {},
          {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}},
        {*source}
    };
    graph::CommandBuffer invalid;
    invalid.copy_buffer_to_image(
        source->buffer, {source->range.offset, source->range.size - 1}, *image
    );
    CHECK(!execution.add_pass(pass, invalid));  // Wrong declaration.
    auto short_pass = pass;
    --short_pass.buffers.front().range.size;
    CHECK(!execution.add_pass(short_pass, std::move(invalid)));  // Declared but too small.
    graph::CommandBuffer misaligned;
    misaligned.copy_buffer_to_image(
        source->buffer, {source->range.offset + 1, source->range.size}, *image
    );
    CHECK(!execution.add_pass(pass, std::move(misaligned)));
    CHECK(execution.add_pass(pass, std::move(commands)));
    execution.add_pass(
        {"sample copied word",
         {{*image,
           {},
           {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled}}},
         {},
         graph::PassKind::Export}
    );
    const auto copied = execution.run();
    CHECK(copied);
    const auto pixels = target.read_rgba();
    const auto expected = std::bit_cast<std::array<std::uint8_t, 4>>(kSource[1]);
    CHECK(std::equal(pixels.begin(), pixels.end(), expected.begin(), expected.end()));
    target.release();
    glDeleteBuffers(1, &buffer);
}

void test_buffer_visibility_across_scopes() {
    constexpr auto fragment = graph::Stage::FragmentShader;
    constexpr auto compute = graph::Stage::ComputeShader;
    constexpr auto read = graph::Access::Read;
    constexpr auto write = graph::Access::Write;
    constexpr auto layout = graph::Layout::Buffer;
    constexpr graph::BufferRange whole{0, 12};
    constexpr graph::BufferRange middle{4, 4};
    unsigned name = 0;
    glGenBuffers(1, &name);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, name);
    glBufferData(GL_SHADER_STORAGE_BUFFER, whole.size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, name);
    renderer::Shader shader;
    std::string error;
    CHECK(shader.build_compute("visibility writer", R"(#version 430 core
layout(local_size_x=1) in;
layout(std430,binding=11) writeonly buffer Output { uint words[3]; };
void main(){words[gl_GlobalInvocationID.x]=1u;}
)", error));
    shader.use();

    const auto record_write = [&](graph::BufferRange bytes, bool dispatch) {
        vkgl::GraphExecution execution;
        const auto target = execution.import_buffer(
            "visibility destination",
            name,
            vkgl::InitialContents::Discard,
            vkgl::BufferVisibility::TrackShaderReads
        );
        CHECK(target);
        execution.add_pass({"write visibility bytes", {}, {{*target, bytes, {compute, write, layout}}}});
        if (dispatch) {
            execution.add_pass(
                {"publish visibility bytes",
                 {},
                 {{*target, bytes, {fragment, read, layout}}},
                 graph::PassKind::Export}
            );
        }
        CHECK(execution.run([&](unsigned) {
            if (dispatch) {
                glDispatchCompute(3, 1, 1);
            }
        }));
    };
    const auto barriers_for = [&](graph::BufferRange bytes, graph::Stage stages) {
        vkgl::GraphExecution execution;
        const auto target = execution.import_buffer("visibility read", name);
        CHECK(target);
        execution.add_pass(
            {"read visibility bytes",
             {},
             {{*target, bytes, {stages, read, layout}}},
             graph::PassKind::Export}
        );
        const auto before = vkgl::submission_counters().barriers;
        CHECK(execution.run());
        return vkgl::submission_counters().barriers - before;
    };

    record_write(whole, true);
    CHECK(barriers_for(whole, fragment) == 0);
    CHECK(barriers_for(whole, compute) == 1);
    CHECK(barriers_for(whole, compute | fragment) == 0);

    // Even a callback that records no work invalidates its declared write range conservatively.
    record_write(middle, false);
    CHECK(barriers_for({0, 4}, fragment) == 0);
    CHECK(barriers_for({8, 4}, fragment) == 0);
    CHECK(barriers_for(middle, fragment) == 1);
    CHECK(barriers_for(middle, fragment) == 0);
    glDeleteBuffers(1, &name);
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
        test_dispatch_rejects_undeclared_output(fixture);
        test_dispatch_rejects_rebound_output(fixture);
        test_reflected_reads(fixture);
        test_input_capture_is_atomic(fixture);
        test_attachment_aspects();
        test_callback_failure();
        test_storage_promotion(fixture);
        test_graph_copy_range();
        test_buffer_visibility_across_scopes();
        test_dispatch_resource_contract();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "graph compute: %s\n", failure.what());
        return 1;
    }
}
