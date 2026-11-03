// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native temporal graph boundaries: defined inputs, unique storage outputs and transactional
// validation.
#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/shader.h"
#include "renderer/vulkan/frame_capture.h"
#include "renderer/vulkan/temporal_graph.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;

// Owns independent single-sample input/output images for a synthetic native dispatch.
struct Fixture {
    renderer::FrameBuffer input, output;         // Only output is storage-capable.
    std::array<vkgl::NativeImageUse, 2> uses{};  // Stable names, not retained native handles.
    // Allocates the small image pair and initializes only the read-only source.
    Fixture();
    // Retires storage after the last synchronous callback.
    ~Fixture() = default;
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
};

Fixture::Fixture() {
    std::string error;
    CHECK(input.create(2, 2, error));
    CHECK(output.create(2, 2, error));
    CHECK(vkgl::allocate_storage_texture(output.colour_texture(), GL_RGBA8, 2, 2));
    glBindFramebuffer(GL_FRAMEBUFFER, input.id());
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    uses = {{{input.colour_texture()}, {output.colour_texture(), true}}};
}

// Returns the callback's borrowed identity and checks dimensions and untouched source colour
// outside callback nesting.
VkImage read_native_image(Fixture& fixture) {
    vkgl::NativeImage result{};
    // The provider borrows its image views only through this synchronous render-thread callback.
    const auto record = [&](auto, std::span<const vkgl::NativeImage> images) {
        // The test interface supplies exactly one read-only image.
        if (images.size() != 1) {
            return false;
        }

        result = images.front();
        return true;
    };
    CHECK(
        vkgl::temporal_compute("synthetic temporal reads", std::span(fixture.uses).first(1), record)
    );
    CHECK(result.image != VK_NULL_HANDLE && result.width == 2 && result.height == 2);
    const auto pixels = fixture.input.read_rgba();
    CHECK(pixels[0] == 0 && pixels[1] == 0 && pixels[2] == 0 && pixels[3] == 255);
    return result.image;
}

// Read-only providers borrow the same native identity across repeated graph imports without changing the known
// source pixels.
void test_native_reads(Fixture& fixture) {
    const auto reference = read_native_image(fixture);
    CHECK(reference == read_native_image(fixture));
}

// Invalid late imports and aliases must not flush pending drawing or invoke the provider.
void test_transactional_imports(Fixture& fixture) {
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.input.id());
    glClear(GL_COLOR_BUFFER_BIT);
    auto invalid = fixture.uses;
    invalid.back().texture = 0;
    const auto before = vkgl::submission_counters();
    bool called = false;
    // An incomplete provider list is rejected before an earlier valid image can change layout.
    const auto record = [&](auto, auto) {
        called = true;
        return true;
    };
    CHECK(!vkgl::temporal_compute("invalid last input", invalid, record));
    CHECK(!called);
    CHECK(vkgl::submission_counters().barriers == before.barriers);
    const std::array<vkgl::NativeImageUse, 2> alias{
        {{fixture.output.colour_texture()}, {fixture.output.colour_texture(), true}}
    };
    CHECK(!vkgl::temporal_compute("aliased output", alias, record));
    CHECK(!called);
    CHECK(vkgl::submission_counters().barriers == before.barriers);
}

// Undefined history cannot be sampled just because its texture name is valid, proving graph
// validation is active.
void test_unwritten_input() {
    unsigned texture = 0;
    glGenTextures(1, &texture);
    CHECK(vkgl::allocate_storage_texture(texture, GL_RGBA8, 2, 2));
    const std::array<vkgl::NativeImageUse, 1> uses{{{texture}}};
    bool called = false;
    CHECK(!vkgl::temporal_compute("unwritten history", uses, [&](auto, auto) {
        called = true;
        return true;
    }));
    CHECK(!called);
    glDeleteTextures(1, &texture);
}

// Rejects partial outputs from failing providers while allowing later valid reads.
void test_provider_failure(Fixture& fixture) {
    bool called = false;
    CHECK(!vkgl::temporal_compute("provider rejected", fixture.uses, [&](auto, auto) {
        called = true;
        return false;
    }));
    CHECK(called);
    called = false;
    CHECK(!vkgl::temporal_compute("provider threw", fixture.uses, [&](auto, auto) -> bool {
        called = true;
        throw 0;
    }));
    CHECK(called);
    const std::array<vkgl::NativeImageUse, 1> output{{{fixture.output.colour_texture()}}};
    called = false;
    // Failed writes invalidate the whole output, preventing later phases from sampling partial
    // results.
    CHECK(!vkgl::temporal_compute("rejected partial output", output, [&](auto, auto) {
        called = true;
        return true;
    }));
    CHECK(!called);
    // A subsequent valid read-only provider can still use the current recording.
    CHECK(
        vkgl::temporal_compute(
            "provider recovered", std::span(fixture.uses).first(1), [](auto, auto) { return true; }
        )
    );
}

// Reusing one shader must retain each pass's uniform values across collection and the SDK decision.
void test_temporal_chain_uniforms(Fixture& fixture) {
    renderer::Shader shader;
    std::string error;
    CHECK(shader.build(
        "temporal uniform snapshots",
        R"(#version 430 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);})",
        R"(#version 430 core
uniform float value;out vec4 colour;void main(){colour=vec4(value,0,0,1);})",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    bool called = false;
    const auto result = vkgl::temporal_chain([&] {
        glBindVertexArray(vao);
        glViewport(0, 0, 2, 2);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        shader.use();
        // Two writes use the same program and different values before either draw executes.
        for (unsigned n = 0; n < 2; ++n) {
            const unsigned target = n == 0 ? fixture.input.id() : fixture.output.id();
            glBindFramebuffer(GL_FRAMEBUFFER, target);
            glUniform1f(shader.uniform("value"), n == 0 ? .25f : .75f);
            CHECK(vkgl::temporal_raster("uniform snapshot", {target}, {}, [] {
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }));
        }
        const std::array<vkgl::NativeImageUse, 2> reads{
            {{fixture.input.colour_texture()}, {fixture.output.colour_texture()}}
        };
        CHECK(vkgl::temporal_compute("synchronous temporal decision", reads, [&](auto, auto) {
            called = true;
            return true;
        }));
        CHECK(called);
    });
    CHECK(result && called);
    CHECK(fixture.input.read_rgba()[0] == 64);
    CHECK(fixture.output.read_rgba()[0] == 191);
    glDeleteVertexArrays(1, &vao);
}

// SDK rejection still allows a complete fallback copy within the same temporal recording scope.
void test_temporal_chain_fallback(Fixture& fixture) {
    const auto source = fixture.input.read_rgba();
    // Both rejection and an SDK exception retain the same complete-world fallback.
    for (bool throwing : {false, true}) {
        const auto result = vkgl::temporal_chain([&] {
            CHECK(!vkgl::temporal_compute(
                "reject temporal dispatch", fixture.uses, [throwing](auto, auto) -> bool {
                    // Provider exceptions are recording failures, not partial history commits.
                    if (throwing) {
                        throw std::runtime_error("synthetic provider failure");
                    }
                    return false;
                }
            ));
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fixture.input.id());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fixture.output.id());
            CHECK(
                vkgl::temporal_blit(
                    "complete temporal fallback",
                    fixture.input.id(),
                    fixture.output.id(),
                    vkgl::TemporalConsumer::Attachment,
                    [] {
                        glBlitFramebuffer(0, 0, 2, 2, 0, 0, 2, 2, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                    }
                )
            );
        });
        CHECK(result);
        CHECK(fixture.output.read_rgba() == source);
    }
}

// ---- Frame-wide capture ----

// Records nested raster/copy declarations while preserving the original complete-image commands.
void capture_scene(renderer::FrameBuffer& source, renderer::FrameBuffer& output) {
    const auto produce = vkgl::temporal_raster("capture producer", {source.id()}, {}, [&] {
        glBindFramebuffer(GL_FRAMEBUFFER, source.id());
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(.25f, .5f, .75f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    });
    CHECK(produce);
    const std::array inputs{source.colour_texture()};
    const auto parent = vkgl::temporal_raster("capture parent", {output.id()}, inputs, [&] {
        const auto nested = vkgl::temporal_blit(
            "capture nested copy",
            source.id(),
            output.id(),
            vkgl::TemporalConsumer::Attachment,
            [&] { source.blit_to(output.id(), 2, 2); }
        );
        CHECK(nested);
    });
    CHECK(parent);
}

// A failed callback invalidates capture while leaving later rendering and capture requests usable.
void test_failed_capture(test::GlContext& context) {
    renderer::FrameBuffer output;
    std::string error;
    CHECK(output.create(2, 2, error));
    CHECK(vkgl::capture_frame_graph());
    const auto result =
        vkgl::temporal_raster("aborted capture", {output.id()}, {}, [] { throw 0; });
    CHECK(!result);
    context.window().swap();
    CHECK(!vkgl::frame_graph_description());
}

// Capturing declarations must not change image bytes, native barrier counts or nested recording
// order.
void test_frame_capture(test::GlContext& context) {
    renderer::FrameBuffer source, output;
    std::string error;
    CHECK(source.create(2, 2, error));
    CHECK(output.create(2, 2, error));
    std::vector<std::uint8_t> reference;
    std::uint64_t reference_barriers = 0;
    // Warmup normalizes native image entry layouts before the measured reference and captured
    // frames.
    for (int iteration : {0, 1, 2}) {
        // Only the final iteration captures; the warm reference retains identical native entry
        // states.
        if (iteration == 2) {
            CHECK(vkgl::capture_frame_graph());
            CHECK(!vkgl::capture_frame_graph());
            CHECK(!vkgl::frame_graph_description());
        }
        const auto before = vkgl::submission_counters();
        capture_scene(source, output);
        context.window().swap();
        const auto barriers = vkgl::submission_counters().barriers - before.barriers;
        const auto pixels = output.read_rgba();
        // Keep exact reference bytes and barrier counts from the warmed recording.
        if (iteration == 1) {
            reference = pixels;
            reference_barriers = barriers;
        } else if (iteration == 2) {
            CHECK(reference == pixels);
            CHECK(reference_barriers == barriers);
            const auto description = vkgl::frame_graph_description();
            // Surface collection failures directly rather than hiding them behind image equality.
            if (!description) {
                std::fprintf(stderr, "%s\n", description.error().message.c_str());
            }
            CHECK(description);
            // The dump must include nested work and its enclosing callback completion.
            if (description) {
                CHECK(description->find("capture producer") != std::string::npos);
                CHECK(description->find("capture nested copy") != std::string::npos);
                CHECK(
                    description->find("capture parent (callback completion)") != std::string::npos
                );
                CHECK(description->find("dependency") != std::string::npos);
            }
        }
    }
}

// The environment request survives empty presentations, completes one renderer frame, then stays dormant.
void test_environment_capture(test::GlContext& context) {
    CHECK(!vkgl::capture_frame_graph());
    context.window().swap();
    CHECK(!vkgl::frame_graph_description());
    CHECK(!vkgl::capture_frame_graph());
    renderer::FrameBuffer source, output;
    std::string error;
    CHECK(source.create(2, 2, error));
    CHECK(output.create(2, 2, error));
    capture_scene(source, output);
    context.window().swap();
    const auto description = vkgl::frame_graph_description();
    // The environment option must arm a real capture rather than merely emit a placeholder.
    if (!description) {
        std::fprintf(stderr, "%s\n", description.error().message.c_str());
    }
    CHECK(description);
    // Later empty frames must not replace the completed renderer capture.
    if (description) {
        CHECK(description->find("capture nested copy") != std::string::npos);
        context.window().swap();
        CHECK(vkgl::frame_graph_description());
        CHECK(*description == *vkgl::frame_graph_description());
    }
}
}  // namespace

// Runs without game data or a visible window; unavailable native graphics is the only skip
// condition.
int main(int argc, char**) {
    // Unexpected native failures remain inside this standalone regression process.
    try {
        test::GlContext context;
        std::string error;

        // Only an unavailable Vulkan context may skip this native regression.
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }

        // The companion invocation checks environment activation in a fresh context.
        if (argc > 1) {
            test_environment_capture(context);
            CHECK(vkgl::submission_counters().validation_errors == 0);
            return test::result();
        }

        Fixture fixture;
        test_native_reads(fixture);
        test_transactional_imports(fixture);
        test_unwritten_input();
        test_provider_failure(fixture);
        test_temporal_chain_uniforms(fixture);
        test_temporal_chain_fallback(fixture);
        test_failed_capture(context);
        test_frame_capture(context);
        CHECK(vkgl::submission_counters().validation_errors == 0);
        renderer::render_opt::reset();
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "temporal graph: %s\n", failure.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
