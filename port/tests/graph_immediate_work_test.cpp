// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks compatibility transfer contracts, draw batching and compute mip-chain sampling.
#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <utility>
#include <vector>
#include <volk.h>

#include "common/stats.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/graph_immediate_work.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace graph = renderer::graph;

void clear_target(const renderer::FrameBuffer& target, std::array<float, 4> colour) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, target.width(), target.height());
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(colour[0], colour[1], colour[2], colour[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

void check_pixels(const renderer::FrameBuffer& target, std::array<std::uint8_t, 4> colour) {
    const auto pixels = target.read_rgba();
    CHECK(pixels.size() == static_cast<std::size_t>(target.width() * target.height()) * 4);
    for (std::size_t offset = 0; offset + 4 <= pixels.size(); offset += 4) {
        CHECK(std::equal(colour.begin(), colour.end(), pixels.begin() + offset));
    }
}

void add_full_copy(
    vkgl::GraphExecution& execution,
    const char* name,
    graph::ImageHandle from,
    graph::ImageHandle to,
    bool cull_if_unused = false
) {
    graph::CommandBuffer commands;
    commands.transfer_image(from, to, graph::Aspect::Colour, graph::ImageTransferKind::Copy);
    auto pass = commands.declarations(name);
    pass.cull_if_unused = cull_if_unused;
    CHECK(execution.add_pass(std::move(pass), std::move(commands)));
}

void test_culled_export_consumer() {
    renderer::FrameBuffer source, output;
    std::string error;
    CHECK(source.create_colour(4, 4, error));
    CHECK(output.create_colour(4, 4, error));
    for (const bool cull_reader : {false, true}) {
        clear_target(source, {1, 0, 0, 1});
        clear_target(output, {0, 0, 1, 1});
        bool read = false;
        CHECK(renderer::run_surface_chain([&] {
            vkgl::GraphExecution producer;
            const auto from = producer.import_image("source", source.colour_texture()).value();
            const auto to = producer.import_image("output", output.colour_texture()).value();
            add_full_copy(producer, "optional producer", from, to, true);
            graph::ImageUse sample{
                to, {}, {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled}
            };
            sample.continuation = true;
            producer.add_pass({"export output", {sample}, {}, graph::PassKind::Export});
            CHECK(producer.run());

            vkgl::GraphExecution consumer;
            sample.image = consumer.import_image("output", output.colour_texture()).value();
            sample.continuation = false;
            graph::Pass pass{"later reader", {sample}};
            pass.cull_if_unused = cull_reader;
            consumer.add_pass(std::move(pass));
            CHECK(consumer.run_prepared([&] { read = true; }));
        }));
        CHECK(read == !cull_reader);
        check_pixels(output, {255, 0, 0, 255});
    }
}

void test_transfer_export_keeps_producer() {
    renderer::FrameBuffer source, output;
    std::string error;
    CHECK(source.create_colour(4, 4, error));
    CHECK(output.create_colour(4, 4, error));
    clear_target(source, {1, 0, 0, 1});
    clear_target(output, {0, 0, 1, 1});

    CHECK(renderer::run_surface_chain([&] {
        vkgl::GraphExecution producer;
        const auto from = producer.import_image("source", source.colour_texture()).value();
        const auto to = producer.import_image("output", output.colour_texture()).value();
        add_full_copy(producer, "optional producer", from, to, true);
        producer.add_pass(
            {"export transfer source",
             {{to,
               {},
               {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}}},
             {},
             graph::PassKind::Export}
        );
        CHECK(producer.run());
    }));
    const auto& image = vkgl::detail::g->textures.find(output.colour_texture())->image;
    CHECK(image->layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    check_pixels(output, {255, 0, 0, 255});
}

// Two disjoint lifetimes reuse one device image; a copy between them needs two at its boundary.
void test_transient_colour_reuse() {
    constexpr std::array extent{512, 512};
    renderer::FrameBuffer red, first;
    std::string error;
    for (auto* target : {&red, &first}) {
        CHECK(target->create_colour(extent[0], extent[1], error));
    }
    clear_target(red, {1, 0, 0, 1});
    stats::set_enabled(stats::Client::Dump, true);
    std::uint64_t tick = 0;
    stats::begin_frame(stats::FrameKind::Tick, tick);
    double shared_bytes = 0;
    for (const auto& [overlap, same_extent] :
         {std::pair{false, true}, std::pair{true, true}, std::pair{false, false}}) {
        const auto second_extent = same_extent ? extent : std::array{128, 256};
        renderer::FrameBuffer blue, second;
        CHECK(blue.create_colour(second_extent[0], second_extent[1], error));
        CHECK(second.create_colour(second_extent[0], second_extent[1], error));
        clear_target(blue, {0, 0, 1, 1});
        vkgl::GraphExecution execution;
        const auto r = execution.import_image("red", red.colour_texture()).value();
        const auto b = execution.import_image("blue", blue.colour_texture()).value();
        const auto dst1 = execution.import_image("first output", first.colour_texture()).value();
        const auto dst2 = execution.import_image("second output", second.colour_texture()).value();
        // Register in the opposite order to prove placement follows pass order.
        const auto temp2 =
            execution.create_transient_colour("second temporary", second_extent).value();
        const auto temp1 = execution.create_transient_colour("first temporary", extent).value();
        CHECK(temp1 != temp2);
        const auto before = vkgl::submission_counters().texture_creations;
        add_full_copy(execution, "write first temporary", r, temp1);
        add_full_copy(execution, "read first temporary", temp1, dst1);
        add_full_copy(execution, "write second temporary", overlap ? temp1 : b, temp2);
        graph::CommandBuffer blit;
        blit.blit(temp2, dst2, {{}, second_extent, {}, second_extent});
        auto pass = blit.declarations("read second temporary");
        CHECK(execution.add_pass(std::move(pass), std::move(blit)));
        CHECK(vkgl::submission_counters().texture_creations == before);
        CHECK(execution.run());
        CHECK(
            vkgl::submission_counters().texture_creations - before
            == (overlap || !same_extent ? 2u : 1u)
        );
        stats::begin_frame(stats::FrameKind::Tick, ++tick);
        const auto* measured = stats::history(0);
        CHECK(measured);
        if (measured && same_extent) {
            const auto bytes = std::ranges::find_if(measured->gauges, [](const auto& item) {
                return item.first == "graph transient image bytes";
            });
            CHECK(bytes != measured->gauges.end());
            if (bytes != measured->gauges.end()) {
                if (overlap) {
                    CHECK(shared_bytes > 0 && bytes->second == 2 * shared_bytes);
                    std::printf(
                        "Transient backing allocation bytes: %.0f -> %.0f with reuse\n",
                        bytes->second,
                        shared_bytes
                    );
                } else {
                    shared_bytes = bytes->second;
                }
            }
        }
        check_pixels(first, {255, 0, 0, 255});
        check_pixels(
            second,
            overlap ? std::array<std::uint8_t, 4>{255, 0, 0, 255}
                    : std::array<std::uint8_t, 4>{0, 0, 255, 255}
        );
    }
    stats::set_enabled(stats::Client::Dump, false);
}

// Undefined contents cannot escape through a reused slot, and dead work allocates nothing.
void test_transient_colour_validation() {
    constexpr std::array extent{7, 5};
    renderer::FrameBuffer source, output;
    std::string error;
    CHECK(source.create_colour(extent[0], extent[1], error));
    CHECK(output.create_colour(extent[0], extent[1], error));
    clear_target(source, {1, 0, 0, 1});
    clear_target(output, {0, 0, 1, 1});
    const auto before = vkgl::submission_counters().texture_creations;
    vkgl::GraphExecution invalid;
    const auto unwritten = invalid.create_transient_colour("unwritten", extent).value();
    const auto destination = invalid.import_image("output", output.colour_texture()).value();
    add_full_copy(invalid, "read before initialization", unwritten, destination);
    CHECK(!invalid.run());
    check_pixels(output, {0, 0, 255, 255});

    vkgl::GraphExecution exported;
    const auto external = exported.import_image("source", source.colour_texture()).value();
    const auto temporary = exported.create_transient_colour("graph-local", extent).value();
    add_full_copy(exported, "initialize graph-local image", external, temporary);
    exported.add_pass(
        {"export transient",
         {{temporary,
           {},
           {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled}}},
         {},
         graph::PassKind::Export}
    );
    CHECK(!exported.run());

    vkgl::GraphExecution dead;
    const auto input = dead.import_image("source", source.colour_texture()).value();
    const auto unused = dead.create_transient_colour("unused", extent).value();
    graph::CommandBuffer commands;
    commands.transfer_image(input, unused, graph::Aspect::Colour, graph::ImageTransferKind::Copy);
    auto pass = commands.declarations("dead copy");
    pass.cull_if_unused = true;
    CHECK(dead.add_pass(std::move(pass), std::move(commands)));
    CHECK(dead.run());
    CHECK(vkgl::submission_counters().texture_creations == before);
}

// A surrounding work scope is not permission to copy resources it never declared.
void test_transfer_declarations() {
    renderer::FrameBuffer source, destination;
    std::string error;
    CHECK(source.create_colour(4, 4, error));
    CHECK(destination.create_colour(4, 4, error));
    clear_target(source, {1, 0, 0, 1});
    clear_target(destination, {0, 0, 1, 1});
    const auto copy = [&] {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source.id());
        glBindTexture(GL_TEXTURE_2D, destination.colour_texture());
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 4, 4);
    };
    vkgl::GraphExecution empty;
    graph::Pass unrelated{"no transfer resources"};
    unrelated.execution = graph::Stage::ComputeShader;
    empty.add_pass(std::move(unrelated));
    const auto undeclared = empty.run([&](unsigned) { copy(); });
    CHECK(!undeclared);
    if (!undeclared) {
        CHECK(undeclared.error().message.find("declaration") != std::string::npos);
    }
    check_pixels(destination, {0, 0, 255, 255});

    // Importing an allocation alone does not declare permission to write it.
    vkgl::GraphExecution missing_write;
    const auto read = missing_write.import_image("source", source.colour_texture());
    CHECK(read);
    CHECK(missing_write.import_image("unused destination", destination.colour_texture()));
    missing_write.add_pass(
        {"missing transfer write",
         {{*read,
           {},
           {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}}}}
    );
    CHECK(!missing_write.run([&](unsigned) { copy(); }));
    check_pixels(destination, {0, 0, 255, 255});

    vkgl::GraphExecution wrong_stage;
    const auto sampled = wrong_stage.read_image(
        "sampled source", source.colour_texture(), graph::Stage::ComputeShader
    );
    const auto written = wrong_stage.import_image("destination", destination.colour_texture());
    CHECK(sampled && written);
    wrong_stage.add_pass(
        {"wrong transfer source state",
         {*sampled,
          {*written,
           {},
           {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}}}
    );
    CHECK(!wrong_stage.run([&](unsigned) { copy(); }));
    check_pixels(destination, {0, 0, 255, 255});

    // A declared source and destination must still execute the ordinary GL copy.
    vkgl::GraphExecution valid;
    const auto from = valid.import_image("source", source.colour_texture());
    const auto to = valid.import_image(
        "destination", destination.colour_texture(), vkgl::InitialContents::Discard
    );
    CHECK(from && to);
    valid.add_pass(
        {"declared transfer",
         {{*from, {}, {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}},
          {*to,
           {},
           {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}}}
    );
    CHECK(valid.run([&](unsigned) { copy(); }));
    check_pixels(destination, {255, 0, 0, 255});
}

void test_transfer_buffer_range() {
    renderer::FrameBuffer destination;
    std::string error;
    CHECK(destination.create_colour(2, 2, error));
    clear_target(destination, {0, 0, 1, 1});
    unsigned buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    const std::array<unsigned, 4> words{0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff};
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(words), words.data(), GL_DYNAMIC_DRAW);
    vkgl::GraphExecution execution;
    auto source = execution.read_buffer("upload", buffer, graph::Stage::Transfer);
    const auto image = execution.import_image("destination", destination.colour_texture());
    CHECK(source && image);
    source->range.size = sizeof(unsigned);
    execution.add_pass(
        {"short upload declaration",
         {{*image,
           {},
           {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}},
         {*source}}
    );
    const auto result = execution.run([&](unsigned) {
        CHECK(vkgl::copy_buffer_to_texture(buffer, destination.colour_texture(), 2, 2));
    });
    CHECK(!result);
    if (!result) {
        CHECK(result.error().message.find("buffer access") != std::string::npos);
    }
    check_pixels(destination, {0, 0, 255, 255});
    glDeleteBuffers(1, &buffer);
}

// Read-only geometry must preserve one native attachment pass across repeated draws.
void test_repeated_vertex_reads(bool resident) {
    renderer::FrameBuffer target;
    renderer::Shader shader;
    std::string error;
    CHECK(target.create_colour(4, 4, error));
    CHECK(shader.build(
        "immediate vertex reads",
        R"(#version 410 core
layout(location=0) in vec2 position;
void main(){gl_Position=vec4(position,0,1);}
)",
        R"(#version 410 core
layout(location=0) out vec4 result;
void main(){result=vec4(0,1,0,1);}
)",
        error
    ));
    unsigned array = 0, buffer = 0;
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    std::vector<float> vertices(resident ? 32 * 1024 : 6);
    const float triangle[]{-1, -1, 3, -1, -1, 3};
    std::copy(std::begin(triangle), std::end(triangle), vertices.begin());
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        resident ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    shader.use();
    clear_target(target, {0, 0, 0, 1});
    const auto before = vkgl::submission_counters();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    const auto first = vkgl::submission_counters();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    const auto second = vkgl::submission_counters();
    CHECK(second.barriers == first.barriers);
    glFinish();
    CHECK(vkgl::submission_counters().render_passes - before.render_passes == 1);
    check_pixels(target, {0, 255, 0, 255});
    glDeleteBuffers(1, &buffer);
    glDeleteVertexArrays(1, &array);
}

// Stencil-only copies/resolves must leave depth unchanged, including combined allocations.
void test_stencil_only_copy(int samples, bool discard_depth = false) {
    renderer::FrameBuffer source, destination;
    renderer::Shader shader;
    std::string error;
    CHECK(source.create(4, 4, error, samples));
    CHECK(destination.create(4, 4, error));
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    for (const auto& entry : {std::pair{source.id(), 7}, std::pair{destination.id(), 3}}) {
        glBindFramebuffer(GL_FRAMEBUFFER, entry.first);
        glClearDepth(entry.second == 7 ? .25 : .75);
        glClearStencil(entry.second);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    const auto blit = [&] {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source.id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.id());
        glBlitFramebuffer(0, 0, 4, 4, 0, 0, 4, 4, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    };
    if (discard_depth) {
        glBindFramebuffer(GL_FRAMEBUFFER, source.id());
        constexpr GLenum depth = GL_DEPTH_ATTACHMENT;
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &depth);
        if (samples > 1) {
            // A stencil clear in an open pass must leave discarded depth undefined.
            glClearStencil(7);
            glClear(GL_STENCIL_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_STENCIL_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
        }
        blit();
        vkgl::GraphExecution execution;
        const auto from =
            execution.import_attachment("discarded depth", source.id(), graph::Aspect::Depth);
        CHECK(from);
        execution.add_pass(
            {"read discarded depth",
             {{*from,
               {graph::Aspect::Depth},
               {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}}}}
        );
        CHECK(!execution.run([](unsigned) {}));
    } else {
        vkgl::GraphExecution execution;
        const auto from =
            execution.import_attachment("stencil source", source.id(), graph::Aspect::Depth);
        const auto to = execution.import_attachment(
            "stencil destination", destination.id(), graph::Aspect::Depth
        );
        CHECK(from && to);
        const bool resolve = source.samples() > 1;
        const auto stages = resolve ? graph::Stage::DepthTests | graph::Stage::ColourOutput
                                    : graph::Stage::Transfer;
        execution.add_pass(
            {"stencil-only transfer",
             {{*from,
               {graph::Aspect::Stencil},
               {stages,
                graph::Access::Read,
                resolve ? graph::Layout::DepthAttachment : graph::Layout::TransferSource}},
              {*to,
               {graph::Aspect::Stencil},
               {stages,
                graph::Access::Write,
                resolve ? graph::Layout::DepthAttachment : graph::Layout::TransferDestination}}}}
        );
        CHECK(execution.run([&](unsigned) { blit(); }));
    }
    CHECK(shader.build(
        "stencil copy reader",
        R"(#version 410 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)",
        R"(#version 410 core
out vec4 result;
void main(){result=vec4(0,1,0,1);}
)",
        error
    ));
    clear_target(destination, {0, 0, 0, 1});
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);  // .5 passes only if the destination kept its original .75 depth.
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 7, 0xff);
    glStencilMask(0);
    glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
    unsigned array = 0;
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    shader.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    check_pixels(destination, {0, 255, 0, 255});
    glDeleteVertexArrays(1, &array);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
}

void test_compute_mip_chain() {
    renderer::Shader shader;
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create_colour(1, 1, error));
    CHECK(shader.build_compute(
        "compute samples mip one",
        R"(#version 430 core
layout(local_size_x=1) in;
uniform sampler2D tex_T0;
layout(std430,binding=11) writeonly buffer Output {uint colour;};
void main(){colour=packUnorm4x8(textureLod(tex_T0,vec2(.5),1.0));}
)",
        error
    ));
    unsigned texture = 0, buffer = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    const std::array<std::uint8_t, 16> base{
        0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255
    };
    const std::array<std::uint8_t, 4> mip{255, 0, 0, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, base.data());
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, mip.data());
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, buffer);
    shader.use();
    vkgl::GraphExecution capture;
    graph::Pass pass{"mip-chain input"};
    CHECK(capture.capture_compute_reads(pass));
    CHECK(pass.images.size() == 1 && pass.images[0].range.mip_count == 2);
    glDispatchCompute(1, 1, 1);
    CHECK(vkgl::copy_buffer_to_texture(buffer, target.colour_texture(), 1, 1));
    check_pixels(target, mip);
    glDeleteBuffers(1, &buffer);
    glDeleteTextures(1, &texture);
}

void test_independent_image_barriers() {
    renderer::FrameBuffer sampled, copied;
    std::string error;
    CHECK(sampled.create_colour(4, 4, error));
    CHECK(copied.create_colour(4, 4, error));
    clear_target(sampled, {1, 0, 0, 1});
    clear_target(copied, {0, 1, 0, 1});

    VkPhysicalDeviceVulkan13Features features13{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features.pNext = &features13;
    vkGetPhysicalDeviceFeatures2(
        static_cast<VkPhysicalDevice>(vkgl::native().physical_device), &features
    );

    vkgl::GraphExecution execution;
    const auto colour = execution.import_image("sampled colour", sampled.colour_texture()).value();
    const auto source = execution.import_image("copy source", copied.colour_texture()).value();
    execution.add_pass(
        {"export independent images",
         {{colour, {}, {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled}},
          {source,
           {},
           {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}}},
         {},
         graph::PassKind::Export}
    );
    const auto before = vkgl::submission_counters().barriers;
    CHECK(execution.run());
    CHECK(vkgl::submission_counters().barriers - before == (features13.synchronization2 ? 1 : 2));
    check_pixels(sampled, {255, 0, 0, 255});
    check_pixels(copied, {0, 255, 0, 255});
}

void test_mip_state_between_scopes() {
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    const std::array<std::uint8_t, 64> base{};
    const std::array<std::uint8_t, 16> texels{};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, base.data());
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());

    constexpr graph::State sampled{
        graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled
    };
    constexpr graph::State copy{
        graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource
    };
    constexpr graph::ImageRange last{graph::Aspect::Colour, 2};
    constexpr graph::ImageRange missing{graph::Aspect::Colour, 1};
    vkgl::GraphExecution first;
    const auto original = first.import_image("mip chain", texture).value();
    first.add_pass({"transfer mip two", {{original, last, copy}}});
    CHECK(first.run([](unsigned) {}));

    vkgl::GraphExecution next;
    const auto imported = next.import_image("mip chain", texture).value();
    next.add_pass({"reuse both layouts", {{imported, {}, sampled}, {imported, last, copy}}});
    const auto before = vkgl::submission_counters().barriers;
    CHECK(next.run([](unsigned) {}));
    CHECK(vkgl::submission_counters().barriers == before);

    const auto sample_missing = [&] {
        vkgl::GraphExecution scope;
        const auto image = scope.import_image("mip chain", texture).value();
        scope.add_pass({"sample mip one", {{image, missing, sampled}}});
        return scope.run([](unsigned) {});
    };
    CHECK(!sample_missing());
    glTexSubImage2D(GL_TEXTURE_2D, 1, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
    CHECK(!sample_missing());
    glTexSubImage2D(GL_TEXTURE_2D, 1, 0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
    CHECK(sample_missing());

    vkgl::GraphExecution failed;
    const auto target = failed.import_image("mip chain", texture).value();
    constexpr graph::State write{
        graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination
    };
    failed.add_pass({"interrupted mip update", {{target, missing, write}}});
    CHECK(!failed.run([&](unsigned) {
        namespace native = vkgl::detail;
        auto& image = *native::g->textures.find(texture)->image;
        native::GraphImmediateWork::clear_image(
            native::frame().main, image, {VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, 0, 1}, {}
        );
        throw std::runtime_error("incomplete update");
    }));
    CHECK(!sample_missing());

    unsigned framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    constexpr GLenum colour = GL_COLOR_ATTACHMENT0;
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &colour);
    vkgl::GraphExecution invalidated;
    const auto after_discard = invalidated.import_image("mip chain", texture).value();
    invalidated.add_pass({"sample invalidated base", {{after_discard, {}, sampled}}});
    CHECK(!invalidated.run([](unsigned) {}));
    vkgl::GraphExecution other_mip;
    const auto preserved = other_mip.import_image("mip chain", texture).value();
    other_mip.add_pass({"read unaffected mip", {{preserved, last, copy}}});
    CHECK(other_mip.run([](unsigned) {}));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);
}

bool can_sample_colour(unsigned texture) {
    vkgl::GraphExecution scope;
    const auto image = scope.import_image("colour texture", texture).value();
    scope.add_pass(
        {"sample colour texture",
         {{image, {}, {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled}}}}
    );
    const auto result = scope.run([](unsigned) {});
    if (!result) {
        CHECK(result.error().message.find("read of unwritten contents") != std::string::npos);
    }
    return static_cast<bool>(result);
}

void test_partial_copy_requires_partial_declaration() {
    renderer::FrameBuffer source;
    std::string error;
    CHECK(source.create_colour(4, 4, error));
    clear_target(source, {1, 0, 0, 1});
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (const bool partial : {false, true}) {
        vkgl::GraphExecution execution;
        const auto from = execution.import_image("source", source.colour_texture()).value();
        const auto to = execution.import_image("destination", texture).value();
        execution.add_pass(
            {"copy one texel",
             {{from,
               {},
               {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}},
              {to,
               {},
               {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination},
               graph::Load::None,
               graph::Store::Preserve,
               partial}}}
        );
        const auto result = execution.run([&](unsigned) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, source.id());
            glBindTexture(GL_TEXTURE_2D, texture);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1, 1);
        });
        CHECK(static_cast<bool>(result) == partial);
        if (!partial && !result) {
            CHECK(result.error().message.find("partial declaration") != std::string::npos);
        }
        CHECK(!can_sample_colour(texture));
    }
    glDeleteTextures(1, &texture);
}

void test_indirect_requires_arguments() {
    renderer::FrameBuffer target;
    renderer::Shader shader;
    std::string error;
    CHECK(target.create_colour(4, 4, error));
    CHECK(shader.build(
        "indirect declarations",
        R"(#version 410 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);})",
        R"(#version 410 core
out vec4 result;
void main(){result=vec4(0,1,0,1);})",
        error
    ));
    shader.use();
    unsigned array = 0, index = 0, args = 0;
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    glGenBuffers(1, &index);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index);
    constexpr std::array<unsigned, 3> indices{0, 1, 2};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &args);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, args);
    constexpr VkDrawIndexedIndirectCommand draw{3, 1, 0, 0, 0};
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(draw), &draw, GL_DYNAMIC_DRAW);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    for (const bool declare : {false, true}) {
        clear_target(target, {0, 0, 1, 1});
        vkgl::GraphExecution execution;
        const auto colour =
            execution.import_attachment("colour", target.id(), graph::Aspect::Colour).value();
        graph::Pass pass{
            "indirect draw",
            {{colour,
              {},
              {graph::Stage::ColourOutput,
               graph::Access::ReadWrite,
               graph::Layout::ColourAttachment},
              graph::Load::Preserve}}
        };
        pass.buffers.push_back(
            execution.read_buffer("indices", index, graph::Stage::VertexInput).value()
        );
        if (declare) {
            const auto command = execution.import_buffer("draw arguments", args).value();
            pass.buffers.push_back(
                {command,
                 {0, sizeof(draw)},
                 {graph::Stage::DrawIndirect, graph::Access::Read, graph::Layout::Buffer}}
            );
        }
        execution.add_pass(std::move(pass));
        const auto result =
            execution.run([&](unsigned) { CHECK(vkgl::draw_indexed_indirect(args, 3)); });
        CHECK(static_cast<bool>(result) == declare);
        check_pixels(
            target,
            declare ? std::array<std::uint8_t, 4>{0, 255, 0, 255}
                    : std::array<std::uint8_t, 4>{0, 0, 255, 255}
        );
    }
    glDeleteBuffers(1, &index);
    glDeleteBuffers(1, &args);
    glDeleteVertexArrays(1, &array);
}

void test_attachment_validity_between_scopes() {
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create_colour(4, 4, error));
    for (const auto [store, fail] :
         {std::pair{graph::Store::Preserve, false},
          std::pair{graph::Store::Discard, false},
          std::pair{graph::Store::Preserve, true}}) {
        clear_target(target, {0, 0, 1, 1});
        CHECK(can_sample_colour(target.colour_texture()));
        vkgl::GraphExecution write;
        const auto image =
            write.import_attachment("output", target.id(), graph::Aspect::Colour).value();
        write.add_pass(
            {"update output",
             {{image,
               {},
               {graph::Stage::ColourOutput, graph::Access::Write, graph::Layout::ColourAttachment},
               graph::Load::Clear,
               store}}}
        );
        const auto recorded = write.run([&](unsigned) {
            clear_target(target, {1, 0, 0, 1});
            if (fail) {
                throw std::runtime_error("interrupted output");
            }
        });
        CHECK(static_cast<bool>(recorded) == !fail);
        CHECK(
            can_sample_colour(target.colour_texture()) == (!fail && store == graph::Store::Preserve)
        );
    }
    clear_target(target, {0, 1, 0, 1});
    CHECK(can_sample_colour(target.colour_texture()));
    check_pixels(target, {0, 255, 0, 255});
}

void test_single_mip_partial_upload() {
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    CHECK(!can_sample_colour(texture));
    const std::array<std::uint8_t, 4> pixel{255, 0, 0, 255};
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    CHECK(!can_sample_colour(texture));

    const std::array<std::uint8_t, 64> full{};
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, full.data());
    CHECK(can_sample_colour(texture));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    CHECK(can_sample_colour(texture));
    glDeleteTextures(1, &texture);
}

void test_packed_depth_stencil_state(unsigned layers) {
    namespace native = vkgl::detail;
    unsigned texture = 0;
    glGenTextures(1, &texture);
    native::prepare_frame();
    const auto image = native::make_image(
        native::format_of(GL_DEPTH24_STENCIL8), 4, 4, 1, VK_SAMPLE_COUNT_1_BIT, 0, layers
    );
    CHECK(image);
    native::g->textures.find(texture)->image = image;
    const unsigned layer = layers - 1;
    const auto clear = [&](VkImageAspectFlags aspect) {
        native::GraphImmediateWork::clear_image(
            native::cmd_for(image.get()), *image, {aspect, 0, 1, layer, 1}, {}, {0.5f, 5}
        );
    };
    clear(VK_IMAGE_ASPECT_DEPTH_BIT);

    constexpr graph::State transfer{
        graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource
    };
    constexpr graph::State depth_test{
        graph::Stage::DepthTests, graph::Access::Read, graph::Layout::DepthReadOnly
    };
    const graph::ImageRange depth{graph::Aspect::Depth, 0, 1, layer};
    const graph::ImageRange stencil{graph::Aspect::Stencil, 0, 1, layer};
    const auto read = [&](graph::ImageRange range) {
        vkgl::GraphExecution scope;
        const auto handle = scope.import_image("packed image", texture).value();
        scope.add_pass({"read packed image", {{handle, range, transfer}}});
        return scope.run([](unsigned) {});
    };
    if (layers > 1) {
        CHECK(!read({graph::Aspect::Depth}));
    }
    CHECK(!read(stencil));
    CHECK(read(depth));
    clear(VK_IMAGE_ASPECT_STENCIL_BIT);

    vkgl::GraphExecution conflict;
    const auto conflicting = conflict.import_image("packed image", texture).value();
    conflict.add_pass(
        {"incompatible layouts",
         {{conflicting, depth, transfer}, {conflicting, stencil, depth_test}}}
    );
    CHECK(!conflict.run([](unsigned) {}));

    vkgl::GraphExecution scope;
    const auto handle = scope.import_image("packed image", texture).value();
    scope.add_pass({"depth copy", {{handle, depth, transfer}}});
    scope.add_pass({"stencil test", {{handle, stencil, depth_test}}});
    scope.add_pass({"depth copy again", {{handle, depth, transfer}}});
    const auto before = vkgl::submission_counters().barriers;
    CHECK(scope.run([&](unsigned pass) {
        const auto expected = pass == 1 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                        : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        CHECK(native::image_has_layout(*image, depth, expected));
    }));
    CHECK(vkgl::submission_counters().barriers == before + 3);
    CHECK(read(stencil));
    glDeleteTextures(1, &texture);
}
}  // namespace

int main() {
    try {
        test::GlContext context;
        std::string error;
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }
        test_transfer_declarations();
        test_transient_colour_reuse();
        test_transient_colour_validation();
        test_culled_export_consumer();
        test_transfer_export_keeps_producer();
        test_transfer_buffer_range();
        test_repeated_vertex_reads(false);
        renderer::render_opt::set(renderer::render_opt::Opt::StaticBuffersDeviceLocal, true);
        renderer::render_opt::set(renderer::render_opt::Opt::StaticBuffersForceStaging, true);
        test_repeated_vertex_reads(true);
        renderer::render_opt::reset();
        test_stencil_only_copy(1);
        test_stencil_only_copy(4);
        test_stencil_only_copy(1, true);
        test_stencil_only_copy(4, true);
        test_compute_mip_chain();
        test_independent_image_barriers();
        test_mip_state_between_scopes();
        test_packed_depth_stencil_state(1);
        test_packed_depth_stencil_state(2);
        test_attachment_validity_between_scopes();
        test_single_mip_partial_upload();
        test_partial_copy_requires_partial_declaration();
        test_indirect_requires_arguments();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "immediate graph work: %s\n", error.what());
        return 1;
    }
}
