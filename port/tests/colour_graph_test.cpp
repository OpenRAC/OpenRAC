// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic colour preservation, refraction resolves and graphics-input capture with arithmetic
// reference values.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "renderer/frame/colour_graph.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/renderer.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/diagnostics/frame_capture.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace graph = renderer::graph;
// One full-screen triangle covers every sample independently of camera or retail geometry.
constexpr auto kVertex = R"(#version 430 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)";
// Integer packing retains every sampled depth bit, so resolve differences cannot hide in colour
// tolerance.
constexpr auto kDepth = R"(#version 430 core
uniform sampler2D source_depth;
out vec4 colour;
void main(){uint bits=floatBitsToUint(texelFetch(source_depth,ivec2(gl_FragCoord.xy),0).r);
colour=vec4((uvec4(bits)>>uvec4(0,8,16,24))&255u)/255.;}
)";
// Slanted subpixel edges give MSAA colour resolves genuinely different covered samples.
constexpr auto kPartialVertex = R"(#version 430 core
void main(){vec2 p[3]=vec2[3](vec2(-.77,-.81),vec2(.83,-.67),vec2(-.29,.91));
gl_Position=vec4(p[gl_VertexID],0.,1.);}
)";
// Partial colour writes and blending must preserve the untouched pixels and destination colour.
constexpr auto kBlend = R"(#version 430 core
out vec4 colour;
void main(){colour=vec4(.75,.25,.5,.5);}
)";

// Owns targets and readback programs across sample-count changes and target recreation.
struct Fixture {
    renderer::FrameBuffer source, snapshot,
        output;                     // Actual MSAA source and independent sampled copies.
    renderer::Shader depth, blend;  // Known synthetic colour and exact depth packing.
    unsigned vao = 0;               // Vertex-ID generated triangle.
    Fixture();
    ~Fixture();
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
    // Replaces all allocations to exercise resize and stale-import lifetime handling.
    void resize(int side, int samples);
    // Draws one partial blended overlay and snapshots its complete colour/depth without aliasing.
    std::vector<std::uint8_t> render();
    // Copies/compares background colour and packs sampled depth after native snapshot completion.
    std::vector<std::uint8_t> read_snapshot(
        const renderer::RenderTarget& target,
        renderer::SnapshotAttachments attachments = renderer::SnapshotAttachments::ColourDepth
    );
};

Fixture::Fixture() {
    std::string error;
    CHECK(depth.build("snapshot depth bits", kVertex, kDepth, error));
    CHECK(blend.build("partial colour", kPartialVertex, kBlend, error));
    glGenVertexArrays(1, &vao);
}

Fixture::~Fixture() {
    glDeleteVertexArrays(1, &vao);
}

void Fixture::resize(int side, int samples) {
    std::string error;
    CHECK(source.create(side, side, error, samples, samples == 1));
    CHECK(snapshot.create(side, side, error, 1, true));
    CHECK(output.create(side, side, error));
}

std::vector<std::uint8_t> Fixture::render() {
    CHECK(renderer::colour_graph_enabled());
    const int side = source.width();
    const renderer::RenderTarget target{source.id(), side, side};
    glBindFramebuffer(GL_FRAMEBUFFER, source.id());
    glViewport(0, 0, side, side);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClearColor(.25f, .5f, .75f, 1);
    glClearDepth(.25);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, side / 2, side);
    blend.use();
    glBindVertexArray(vao);
    // The executor records the same partial draw synchronously on the test render thread.
    CHECK(renderer::colour_pass("partial blended colour", target, [&] {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }));
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    return read_snapshot(target);
}

std::vector<std::uint8_t> Fixture::read_snapshot(
    const renderer::RenderTarget& target, renderer::SnapshotAttachments attachments
) {
    const int side = source.width();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, snapshot.id());
    // The graph keeps native colour resolve and sample-zero depth resolve as separate work phases.
    std::array<unsigned, 2> copies{};
    const auto copied = renderer::colour_snapshot(
        target,
        snapshot.id(),
        source.samples(),
        [&](bool depth_copy) {
            ++copies[depth_copy];
            glBlitFramebuffer(
                0,
                0,
                side,
                side,
                0,
                0,
                side,
                side,
                depth_copy ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );
        },
        attachments
    );
    CHECK(copied);
    CHECK(copies[0] == unsigned(attachments != renderer::SnapshotAttachments::Depth));
    CHECK(copies[1] == unsigned(attachments != renderer::SnapshotAttachments::Colour));
    const auto colour = snapshot.read_rgba();
    CHECK(colour == source.read_rgba());
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    depth.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snapshot.depth_texture());
    glUniform1i(depth.uniform("source_depth"), 0);
    CHECK(renderer::graphics_shader_reads("snapshot depth reader"));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    auto result = output.read_rgba();
    const std::array<std::uint8_t, 4> packed{result[0], result[1], result[2], result[3]};
    CHECK(std::abs(std::bit_cast<float>(packed) - .25f) < 0.000001f);
    result.insert(result.end(), colour.begin(), colour.end());
    return result;
}

// Recreates one sample-count tier at odd extents and compares complete graph snapshots.
void check_sample_count(Fixture& fixture, int samples) {
    // Odd dimensions exercise partial rectangles without depending on a display size.
    for (int side : {7, 11}) {
        fixture.resize(side, samples);
        const auto reference = fixture.render();
        CHECK(reference == fixture.render());
        const renderer::RenderTarget target{fixture.source.id(), side, side};
        CHECK(reference == fixture.read_snapshot(target, renderer::SnapshotAttachments::Depth));
        CHECK(reference == fixture.read_snapshot(target, renderer::SnapshotAttachments::Colour));
    }
}

// Resizing, recreation and sample-count changes preserve exact graph colour and resolved
// depth.
void test_snapshots(Fixture& fixture) {
    const int multisample = std::min(4, renderer::FrameBuffer::max_samples());
    // Returning to single-sample targets checks resource recreation after an MSAA tier.
    for (int samples : {1, multisample, 1}) {
        check_sample_count(fixture, samples);
    }
}

// A source cannot serve as its own sampled background, and validation must run before any copy
// callback.
void test_snapshot_alias(Fixture& fixture) {
    bool called = false;
    const renderer::RenderTarget source{
        fixture.source.id(), fixture.source.width(), fixture.source.height()
    };
    const auto rejected =
        renderer::colour_snapshot(source, source.framebuffer, 1, [&](bool) { called = true; });
    CHECK(!rejected && !called);
}

// Graphics capture accepts complete mip chains and retains both graphics-stage readers.
void test_graphics_mips(Fixture& fixture) {
    constexpr auto kFragment = R"(#version 430 core
uniform sampler2D image;
out vec4 colour;
void main(){colour=textureLod(image,vec2(.5),1.);}
)";
    renderer::Shader shader;
    std::string error;
    CHECK(shader.build("graphics mip capture", kVertex, kFragment, error));
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Contrasting base and mip-one colours detect accidental base-level sampling.
    const std::array<std::uint32_t, 4> pixels{};
    constexpr std::uint32_t kWhite = 0xFFFFFFFFu;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &kWhite);
    shader.use();
    glUniform1i(shader.uniform("image"), 0);
    vkgl::GraphExecution execution;
    graph::Pass pass{"graphics mips"};
    const auto explicit_read = execution.read_image(
        "explicit mip input", texture, graph::Stage::VertexShader | graph::Stage::FragmentShader
    );
    CHECK(explicit_read);
    CHECK(explicit_read && explicit_read->range.mip_count == 2);
    CHECK(execution.capture_graphics_reads(pass));
    CHECK(pass.images.size() == 1);
    CHECK(pass.images[0].range.mip_count == 2);
    CHECK(
        pass.images[0].state.stages == (graph::Stage::VertexShader | graph::Stage::FragmentShader)
    );
    const auto output = execution.import_attachment(
        "mip sampling output",
        fixture.output.id(),
        graph::Aspect::Colour,
        vkgl::InitialContents::Discard
    );
    CHECK(output);
    pass.images.push_back(
        {*output,
         {},
         {graph::Stage::ColourOutput, graph::Access::Write, graph::Layout::ColourAttachment},
         graph::Load::Discard}
    );
    execution.add_pass(std::move(pass));
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.output.id());
    glViewport(0, 0, fixture.output.width(), fixture.output.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(fixture.vao);
    // The executor invokes the sampling draw synchronously after all mip-chain read barriers.
    CHECK(execution.run([](unsigned) { glDrawArrays(GL_TRIANGLES, 0, 3); }));
    const auto result = fixture.output.read_rgba();
    CHECK(std::all_of(result.begin(), result.end(), [](std::uint8_t channel) {
        return channel == 255;
    }));
    glDeleteTextures(1, &texture);
}

// Graphics UBO and read-only storage capture use the same allocation ranges as native descriptors.
void test_graphics_buffer_ranges() {
    constexpr auto kFragment = R"(#version 430 core
layout(std140) uniform Inputs { vec4 tint; };
layout(std430,binding=2) readonly buffer Samples { vec4 sample_colour; };
out vec4 colour;
void main(){colour=tint+sample_colour;}
)";
    renderer::Shader shader;
    std::string error;
    CHECK(shader.build("graphics buffer ranges", kVertex, kFragment, error));
    constexpr std::array<float, 4> kValues{.25f, .25f, .25f, .25f};
    std::array<unsigned, 2> buffers{};
    glGenBuffers(2, buffers.data());
    // The uniform block uses a small ring upload; storage owns a stable allocation.
    for (unsigned index = 0; index < buffers.size(); ++index) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[index]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(kValues), kValues.data(), GL_STREAM_DRAW);
    }
    shader.use();
    constexpr unsigned kUniformBuffer = 0x8A11;
    glUniformBlockBinding(shader.id(), glGetUniformBlockIndex(shader.id(), "Inputs"), 3);
    glBindBufferBase(kUniformBuffer, 3, buffers[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers[1]);
    vkgl::GraphExecution execution;
    graph::Pass pass{"graphics buffers"};
    CHECK(execution.capture_graphics_reads(pass));
    CHECK(pass.buffers.size() == 2);
    // Reflected UBO size and complete SSBO size are each one vec4.
    for (const auto& input : pass.buffers) {
        CHECK(input.range.size == sizeof(kValues));
        CHECK(input.state.access == graph::Access::Read);
        CHECK(input.state.stages == (graph::Stage::VertexShader | graph::Stage::FragmentShader));
    }
    execution.add_pass(std::move(pass));
    CHECK(execution.run([](unsigned) {}));
    glDeleteBuffers(2, buffers.data());
}

// Executes unchanged snapshot copies before a draw that explicitly reads both produced images.
std::vector<std::uint8_t> snapshot_chain(Fixture& fixture, bool pooled = false) {
    const int side = fixture.source.width();
    const renderer::RenderTarget source{fixture.source.id(), side, side};
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.output.id());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, source.framebuffer);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(.25f, .5f, .75f, 1);
    glClearDepth(.25);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // The owning fixture and frame parameters outlive every callback in this render-thread scope.
    const auto recorded = renderer::run_surface_chain([&fixture, source, side, pooled] {
        const auto scratch =
            pooled ? renderer::lease_surface_refraction(source.framebuffer, {side, side})
                   : renderer::SurfaceRefractionScratch{};
        CHECK(static_cast<bool>(scratch) == pooled);
        const auto destination = scratch ? scratch.framebuffer : fixture.snapshot.id();
        CHECK(
            renderer::colour_snapshot(
                source,
                destination,
                fixture.source.samples(),
                [source, side, destination](bool depth) {
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.framebuffer);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination);
                    glBlitFramebuffer(
                        0,
                        0,
                        side,
                        side,
                        0,
                        0,
                        side,
                        side,
                        depth ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT,
                        GL_NEAREST
                    );
                }
            )
        );
        const auto textures =
            scratch
                ? scratch.textures
                : std::array{fixture.snapshot.colour_texture(), fixture.snapshot.depth_texture()};
        CHECK(
            renderer::colour_pass(
                "shared refraction colour",
                {fixture.output.id(), side, side},
                [&fixture, side, depth_texture = textures[1]] {
                    glBindFramebuffer(GL_FRAMEBUFFER, fixture.output.id());
                    glViewport(0, 0, side, side);
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_BLEND);
                    fixture.depth.use();
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, depth_texture);
                    glUniform1i(fixture.depth.uniform("source_depth"), 0);
                    glBindVertexArray(fixture.vao);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                },
                renderer::ColourAttachments::Colour,
                {{}, textures}
            )
        );
    });
    CHECK(recorded);
    return fixture.output.read_rgba();
}

// Shared colour/depth resolves retain exact depth bytes and export snapshots after their real consumer.
void test_snapshot_chain(Fixture& fixture, test::GlContext& context) {
    // Odd resized targets exercise both complete native resolve paths and allocation recreation.
    for (int samples : {1, std::min(4, renderer::FrameBuffer::max_samples())}) {
        fixture.resize(samples == 1 ? 7 : 11, samples);
        const auto reference = snapshot_chain(fixture);
        CHECK(vkgl::capture_frame_graph());
        const auto before = vkgl::submission_counters();
        CHECK(snapshot_chain(fixture, true) == reference);
        CHECK(vkgl::submission_counters().surface_chains == before.surface_chains + 1);
        fixture.output.blit_to(0, 64, 64);
        context.window().swap();
        const auto description = vkgl::frame_graph_description();
        CHECK(description);
        // Missing captures must not hide a producer or export ordering failure.
        if (!description) {
            continue;
        }
        const auto colour = description->find(" work refraction colour snapshot\n");
        const auto depth = description->find(" work refraction depth snapshot\n");
        const auto draw = description->find(" work shared refraction colour\n");
        const auto exported = description->find(" export refraction snapshot consumers\n");
        CHECK(colour != std::string::npos && depth != std::string::npos);
        CHECK(draw != std::string::npos && exported != std::string::npos);
        CHECK(colour < depth && depth < draw && draw < exported);
    }
}

// Sampling the colour attachment being written is rejected before any graph callback draws.
void test_colour_feedback(Fixture& fixture) {
    fixture.resize(7, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.source.id());
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const renderer::RenderTarget target{
        fixture.source.id(), fixture.source.width(), fixture.source.height()
    };
    const std::array feedback{fixture.source.colour_texture()};
    CHECK(feedback[0] != 0);
    bool called = false;
    const auto result = renderer::colour_pass(
        "invalid colour feedback",
        target,
        [&called] { called = true; },
        renderer::ColourAttachments::ColourDepth,
        {{}, feedback}
    );
    CHECK(!result && !called);
    const std::array missing{0u};
    const auto invalid = renderer::colour_pass(
        "missing sampled input",
        target,
        [&called] { called = true; },
        renderer::ColourAttachments::ColourDepth,
        {{}, missing}
    );
    CHECK(!invalid && !called);
}

// Consecutive bucket draws and the following immediate draw inherit blending, masks and scissor state.
std::vector<std::uint8_t> ordered_blending(Fixture& fixture) {
    const int side = fixture.output.width();
    const renderer::RenderTarget target{fixture.output.id(), side, side};
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, side, side);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(.25f, .5f, .75f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    fixture.blend.use();
    glBindVertexArray(fixture.vao);
    const renderer::ColourInputs inputs{{}, {}, renderer::ColourRecording::Ordered};
    // The synchronous graph keeps frame-owned state alive across both ordered draw callbacks.
    CHECK(renderer::run_surface_chain([target, side, inputs] {
        CHECK(
            renderer::colour_pass(
                "ordered blend setup",
                target,
                [side] {
                    glEnable(GL_BLEND);
                    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
                    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(0, 0, side / 2, side);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                },
                renderer::ColourAttachments::Colour,
                inputs
            )
        );
        CHECK(
            renderer::colour_pass(
                "ordered blend continuation",
                target,
                [side] {
                    glScissor(side / 2, 0, side - side / 2, side);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                },
                renderer::ColourAttachments::Colour,
                inputs
            )
        );
    }));
    // Outgoing bucket state must also reach work beyond the shared scope.
    glScissor(0, 0, side, side);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    const auto pixels = fixture.output.read_rgba();
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    return pixels;
}

// Ordered callbacks retain the same inherited graphics state across repeated graph recording.
void test_ordered_blending(Fixture& fixture) {
    fixture.resize(11, 1);
    const auto reference = ordered_blending(fixture);
    CHECK(ordered_blending(fixture) == reference);
}

// Disabling every optional optimisation cannot bypass Vulkan graph validation or composition.
void test_graph_is_required(Fixture& fixture) {
    for (const auto& entry : renderer::render_opt::entries()) {
        renderer::render_opt::set(entry.opt, false);
    }

    CHECK(renderer::colour_graph_enabled());
    CHECK(renderer::surface_chain_enabled());
    test_snapshot_alias(fixture);
    test_colour_feedback(fixture);
    renderer::render_opt::reset();
}
}  // namespace

// Keeps unexpected driver errors inside this standalone regression process.
int main() {
    // Unexpected driver errors must not escape the regression process.
    try {
        test::GlContext context;
        std::string error;

        // A host without Vulkan cannot execute the native dependency regression.
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }

        Fixture fixture;
        test_snapshots(fixture);
        test_snapshot_alias(fixture);
        test_graphics_mips(fixture);
        test_graphics_buffer_ranges();
        test_snapshot_chain(fixture, context);
        test_colour_feedback(fixture);
        test_ordered_blending(fixture);
        test_graph_is_required(fixture);
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "colour graph: %s\n", failure.what());
        return 1;
    }
}
