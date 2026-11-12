// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks attachment contracts before native draws, rendering load operations and clears.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>

#include "renderer/frame/colour_graph.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace graph = renderer::graph;

// Blue contents make rejected writes visible; valid draws and clears produce green and red.
struct Fixture {
    renderer::FrameBuffer target;
    renderer::Shader shader;
    unsigned array = 0;

    Fixture() {
        std::string error;
        CHECK(target.create(4, 4, error));
        CHECK(shader.build(
            "attachment contract",
            R"(#version 410 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)",
            R"(#version 410 core
out vec4 result;
void main(){result=vec4(0,1,0,1);}
)",
            error
        ));
        glGenVertexArrays(1, &array);
        glBindVertexArray(array);
        shader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, target.width(), target.height());
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    ~Fixture() { glDeleteVertexArrays(1, &array); }

    // Rejected recording must preserve every destination pixel.
    void check_pixels(std::array<std::uint8_t, 4> colour) const {
        const auto pixels = target.read_rgba();
        CHECK(
            pixels.size()
            == static_cast<std::size_t>(target.width() * target.height()) * colour.size()
        );
        for (std::size_t offset = 0; offset + colour.size() <= pixels.size();
             offset += colour.size()) {
            CHECK(std::equal(colour.begin(), colour.end(), pixels.begin() + offset));
        }
    }
};

// Draws and clears exercise distinct native paths, including clear-as-load-op rendering.
enum class Operation {
    Draw,
    Clear
};

// Executes one known full-target writer.
void record(Operation operation) {
    if (operation == Operation::Draw) {
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return;
    }
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

// Both directions of the contract require an actual writable attachment, not just a work scope.
void test_undeclared_and_readonly(Fixture& fixture) {
    for (const auto operation : {Operation::Draw, Operation::Clear}) {
        vkgl::GraphExecution missing;
        graph::Pass pass{"missing attachment"};
        pass.execution = operation == Operation::Clear ? graph::Stage::ComputeShader
                                                       : graph::Stage::FragmentShader;
        missing.add_pass(std::move(pass));
        const auto result = missing.run([operation](unsigned) { record(operation); });
        CHECK(!result && result.error().message.find("attachment access") != std::string::npos);
        fixture.check_pixels({0, 0, 255, 255});

        vkgl::GraphExecution read_only;
        const auto image = read_only.import_attachment(
            "read-only target", fixture.target.id(), graph::Aspect::Colour
        );
        CHECK(image);
        read_only.add_pass(
            {"read-only attachment",
             {{*image,
               {},
               {graph::Stage::ColourOutput, graph::Access::Read, graph::Layout::ColourAttachment},
               graph::Load::Preserve}}}
        );
        CHECK(!read_only.run([operation](unsigned) { record(operation); }));
        fixture.check_pixels({0, 0, 255, 255});
    }
}

// A valid declaration still permits the same GL draw and clear, with their expected output.
void test_declared_writes(Fixture& fixture) {
    for (const auto operation : {Operation::Draw, Operation::Clear}) {
        vkgl::GraphExecution execution;
        const auto image = execution.import_attachment(
            "declared target", fixture.target.id(), graph::Aspect::Colour
        );
        CHECK(image);
        execution.add_pass(
            {"declared writer",
             {{*image,
               {},
               {graph::Stage::ColourOutput,
                graph::Access::ReadWrite,
                graph::Layout::ColourAttachment},
               graph::Load::Preserve}}}
        );
        CHECK(execution.run([operation](unsigned) { record(operation); }));
        fixture.check_pixels(
            operation == Operation::Draw ? std::array<std::uint8_t, 4>{0, 255, 0, 255}
                                         : std::array<std::uint8_t, 4>{255, 0, 0, 255}
        );
    }
}

// A masked clear touches no attachment and needs no write declaration.
void test_masked_clear(Fixture& fixture) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    vkgl::GraphExecution execution;
    execution.add_pass({"masked clear"});
    CHECK(execution.run([](unsigned) { record(Operation::Clear); }));
    fixture.check_pixels({255, 0, 0, 255});
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

// Borrowed depth permits testing, while enabling depth writes requires a writable declaration.
void test_readonly_depth(Fixture& fixture) {
    glDepthMask(GL_TRUE);
    glClearDepth(.75);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    for (const bool write : {false, true}) {
        glDepthMask(write ? GL_TRUE : GL_FALSE);
        vkgl::GraphExecution execution;
        const auto colour =
            execution.import_attachment("colour", fixture.target.id(), graph::Aspect::Colour);
        const auto depth =
            execution.import_attachment("tested depth", fixture.target.id(), graph::Aspect::Depth);
        CHECK(colour && depth);
        execution.add_pass(
            {"read-only depth",
             {{*colour,
               {},
               {graph::Stage::ColourOutput,
                graph::Access::ReadWrite,
                graph::Layout::ColourAttachment},
               graph::Load::Preserve},
              {*depth,
               {graph::Aspect::Depth},
               {graph::Stage::DepthTests, graph::Access::Read, graph::Layout::DepthAttachment},
               graph::Load::Preserve}}}
        );
        const auto result = execution.run([](unsigned) { record(Operation::Draw); });
        CHECK(static_cast<bool>(result) == !write);
        fixture.check_pixels({0, 255, 0, 255});
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

// Stencil clearing cannot borrow a depth-only declaration on the same native allocation.
void test_clear_aspect_declarations() {
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(4, 4, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    vkgl::GraphExecution execution;
    const auto image = execution.import_attachment("depth only", target.id(), graph::Aspect::Depth);
    CHECK(image);
    execution.add_pass(
        {"depth-only clear",
         {{*image,
           {graph::Aspect::Depth},
           {graph::Stage::DepthTests, graph::Access::Write, graph::Layout::DepthAttachment},
           graph::Load::Discard}}}
    );
    const auto result = execution.run([](unsigned) { glClear(GL_STENCIL_BUFFER_BIT); });
    CHECK(!result);
    if (!result) {
        CHECK(result.error().message.find("attachment access") != std::string::npos);
    }  // The legacy shadow volume pass explicitly owns stencil as well as depth and colour.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK(
        renderer::colour_pass(
            "declared stencil clear",
            {target.id(), 4, 4},
            [] { glClear(GL_STENCIL_BUFFER_BIT); },
            renderer::ColourAttachments::ColourDepthStencil
        )
    );
}
}  // namespace

int main() {
    try {
        test::GlContext context;
        std::string error;
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }
        Fixture fixture;
        test_undeclared_and_readonly(fixture);
        test_declared_writes(fixture);
        test_masked_clear(fixture);
        test_readonly_depth(fixture);
        test_clear_aspect_declarations();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "attachment graph work: %s\n", error.what());
        return 1;
    }
}
