// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exercises complete shadow depth phases with exact float-bit readback and synthetic cache contents.
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <vector>

#include "renderer/frame/surface_chain.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/passes/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#include "scene/shadow_graph.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
// A tiny target suffices to expose dirty strips, wrapped reads and retained neighbouring depth.
constexpr int kSize = 8;
// The comparison shader packs all 32 float bits into four exact RGBA8 channels.
constexpr auto kVertex = R"(#version 410 core

void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}

)";
constexpr auto kFragment = R"(#version 410 core

uniform sampler2D tex_T0;

out vec4 colour;

void main(){uint bits=floatBitsToUint(texelFetch(tex_T0,ivec2(gl_FragCoord.xy),0).r);

colour=vec4((uvec4(bits)>>uvec4(0,8,16,24))&255u)/255.;}

)";

// Owns the real cache, dynamic atlas, combined output and exact readback shader.
struct Fixture {
    renderer::FrameBuffer cache, atlas, output;  // GPU-owned depth and readback targets.
    renderer::Shader sample;                     // Packs sampled depth without colour tolerance.
    unsigned combined = 0;                       // Storage texture written by native composition.
    unsigned vao = 0;                            // Empty full-screen triangle vertex array.
    Fixture();
    ~Fixture();
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
    // Restarts cache ownership so a recreated allocation must receive a complete first write.
    void recreate_cache();
    // Reads a sampled depth image through the same shader path used by later lighting consumers.
    std::vector<std::uint8_t> read(unsigned texture);
};

Fixture::Fixture() {
    std::string error;
    CHECK(cache.create_depth(kSize, kSize, error));
    CHECK(atlas.create_depth(kSize, kSize, error));
    CHECK(output.create(kSize, kSize, error));
    CHECK(sample.build("shadow depth bits", kVertex, kFragment, error));
    glGenVertexArrays(1, &vao);
    glGenTextures(1, &combined);
    // Native shadow composition writes one R32F value per shadow texel.
    CHECK(vkgl::allocate_storage_texture(combined, 0x822E, kSize, kSize));
}

Fixture::~Fixture() {
    cache.release();
    atlas.release();
    output.release();
    sample.release();
    glDeleteTextures(1, &combined);
    glDeleteVertexArrays(1, &vao);
}

void Fixture::recreate_cache() {
    cache.release();
    std::string error;
    CHECK(cache.create_depth(kSize, kSize, error));
}

std::vector<std::uint8_t> Fixture::read(unsigned texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    glViewport(0, 0, kSize, kSize);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    sample.use();
    glUniform1i(sample.uniform("tex_T0"), 0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    return output.read_rgba();
}

// Starts each clear with deterministic attachment state, without changing the graph's allocation names.
void begin_depth(unsigned target) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glViewport(0, 0, kSize, kSize);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

// A failed declaration is a regression failure rather than an incomplete image comparison.
void require(Result<void> result) {
    // A rejected phase must fail the regression instead of comparing incomplete images.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Retained cache texels have a known binary-exact base value, with one optional dirty strip.
float cached_depth(unsigned frame, int x) {
    // A complete refresh and a later recreated cache exercise both ways of replacing old contents.
    const float base = frame == 3 ? .125f : .25f;
    return (frame == 1 || frame == 2) && x < 2 ? .5f : base;
}

// Compares one packed floating-point value without nesting assertions inside the image traversal.
void check_bits(const std::vector<std::uint8_t>& pixels, std::size_t offset, std::uint32_t bits) {
    // The shader stores the float's four bytes from least to most significant.
    for (unsigned channel = 0; channel < 4; ++channel) {
        CHECK(pixels[offset + channel] == ((bits >> (channel * 8)) & 255));
    }
}

// Checks composition against arithmetic, independently of graph/reference image comparisons.
void check_composed(const std::vector<std::uint8_t>& pixels, unsigned frame) {
    // Every texel participates, including retained neighbours of both dirty regions.
    for (int y = 0; y < kSize; ++y) {
        // Wrapping shifts only the static sample; the dynamic sample stays in atlas coordinates.
        for (int x = 0; x < kSize; ++x) {
            // The two-by-two dynamic patch is nearer than every static depth in reversed-Z.
            const float moving = x >= 3 && x < 5 && y >= 3 && y < 5 ? .75f : cached_depth(frame, x);
            // Composition uses a toroidal static offset of (+2,-1), and max for reversed-Z depth.
            const float fixed = cached_depth(frame, (x + 2) % kSize);
            const auto bits = std::bit_cast<std::uint32_t>(std::max(fixed, moving));
            check_bits(pixels, static_cast<std::size_t>(y * kSize + x) * 4, bits);
        }
    }
}

// ---- Prepared shadow phases ----

// A hit skips the writer; all other frames either initialise every pixel or preserve a dirty strip's neighbours.
void update_cache(Fixture& fixture, unsigned frame) {
    // The third frame is a warm cache hit with no writer or clear.
    if (frame == 2) {
        return;
    }

    const bool partial = frame == 1;
    // The callback runs synchronously between the cache's attachment and copy-source transitions.
    require(
        scene::shadow_depth_pass(
            "test static cache",
            fixture.cache.id(),
            partial ? scene::ShadowDepthContents::Preserve : scene::ShadowDepthContents::Clear,
            [&fixture, partial, frame] {
                begin_depth(fixture.cache.id());

                // Only the two-column dirty strip may overwrite retained cache depth.
                if (partial) {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(0, 0, 2, kSize);
                }
                glClearDepth(cached_depth(frame, 0));
                glClear(GL_DEPTH_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);
            },
            scene::ShadowDepthConsumer::Copy
        )
    );
}

// Copies cached depth, overlays a dynamic patch, combines scrolling depth, then samples the result.
void prepare_chain(Fixture& fixture, unsigned frame) {
    update_cache(fixture, frame);
    // The transfer callback retains the production full-image depth copy.
    require(scene::shadow_depth_copy(fixture.cache.id(), fixture.atlas.id(), [&fixture] {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fixture.cache.id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fixture.atlas.id());
        glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }));
    // Dynamic casters preserve all copied depth outside their written region.
    require(
        scene::shadow_depth_pass(
            "test dynamic atlas",
            fixture.atlas.id(),
            scene::ShadowDepthContents::Preserve,
            [&fixture] {
                begin_depth(fixture.atlas.id());
                glEnable(GL_SCISSOR_TEST);
                glScissor(3, 3, 2, 2);
                glClearDepth(.75);
                glClear(GL_DEPTH_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);
            }
        )
    );
    CHECK(
        vkgl::combine_shadow_depth(
            fixture.cache.depth_texture(),
            fixture.atlas.depth_texture(),
            fixture.combined,
            {2, -1},
            {0, 0, kSize, kSize}
        )
    );
}

// Executes the prepared chain before reading any output, including standalone and joined plans.
std::vector<std::uint8_t> render(Fixture& fixture, unsigned frame) {
    const auto before = vkgl::submission_counters();
    require(renderer::run_surface_chain([&fixture, frame] { prepare_chain(fixture, frame); }));
    const auto after = vkgl::submission_counters();
    CHECK(
        after.surface_chains - before.surface_chains
        == (renderer::surface_chain_enabled() ? 1u : 0u)
    );
    auto pixels = fixture.read(fixture.combined);
    check_composed(pixels, frame);
    return pixels;
}

// ---- Validation and equivalence ----

// Graph mode must validate attachment imports before allowing a draw callback to execute.
void test_rejects_invalid_target() {
    bool called = false;
    const auto result =
        scene::shadow_depth_pass("invalid", ~0u, scene::ShadowDepthContents::Clear, [&] {
            called = true;
        });
    CHECK(!result);
    CHECK(!called);
}

// Rejects a later undefined read before queued cache updates or native composition can execute.
void test_rejects_incomplete_chain() {
    Fixture fixture;
    renderer::FrameBuffer undefined;
    std::string error;
    CHECK(undefined.create_depth(kSize, kSize, error));
    bool published = false;
    const auto before = vkgl::submission_counters();
    const auto rejected = renderer::run_surface_chain([&fixture, &undefined, &published] {
        prepare_chain(fixture, 0);
        require(scene::shadow_depth_ready(fixture.atlas.id(), [&published] { published = true; }));
        require(
            scene::shadow_depth_pass(
                "undefined cache", undefined.id(), scene::ShadowDepthContents::Preserve, [] {}
            )
        );
    });
    CHECK(!rejected);
    CHECK(!published);
    const auto after = vkgl::submission_counters();
    CHECK(
        after.commands[static_cast<unsigned>(vkgl::Command::Dispatch)]
        == before.commands[static_cast<unsigned>(vkgl::Command::Dispatch)]
    );
    CHECK(after.surface_chains == before.surface_chains);
    // A rejected plan leaves the fresh allocations eligible for a complete retry.
    render(fixture, 0);
}

// Captures the same cache lifetime sequence through graph recording.
std::array<std::vector<std::uint8_t>, 5> capture_chain() {
    Fixture fixture;
    std::array<std::vector<std::uint8_t>, 5> frames;

    // The final frame replaces the allocation rather than only clearing its previous contents.
    for (unsigned frame = 0; frame < frames.size(); ++frame) {
        // Replacing the image invalidates all old contents even though the cascade size stays fixed.
        if (frame == 4) {
            fixture.recreate_cache();
        }

        frames[frame] = render(fixture, frame);
    }

    return frames;
}

// Repeated graphs retain identical bits through fresh, partial, hit, refresh and recreated cache frames.
void test_complete_chain() {
    const auto reference = capture_chain();
    const auto graph = capture_chain();
    const auto shared = capture_chain();

    // All samples include later shader reads, not only attachment contents at the end of a pass.
    for (unsigned frame = 0; frame < reference.size(); ++frame) {
        CHECK(reference[frame] == graph[frame]);
        CHECK(reference[frame] == shared[frame]);
    }
}

// Two partial phases on one atlas, as a frame that keeps far cascades draws them: each is a render
// pass of its own that covers only its rectangle, so nothing is loaded and the rest is kept.
void test_separate_partial_passes() {
    Fixture fixture;
    require(
        scene::shadow_depth_pass(
            "test atlas", fixture.atlas.id(), scene::ShadowDepthContents::Clear, [&fixture] {
                begin_depth(fixture.atlas.id());
                glClearDepth(.25);
                glClear(GL_DEPTH_BUFFER_BIT);
            }
        )
    );
    const auto before = vkgl::submission_counters();
    require(renderer::run_surface_chain([&fixture] {
        // The left columns, then the last two; the two between keep the first clear.
        for (const int part : {0, 1}) {
            require(
                scene::shadow_depth_pass(
                    "test atlas part",
                    fixture.atlas.id(),
                    scene::ShadowDepthContents::PreserveSeparate,
                    [&fixture, part] {
                        begin_depth(fixture.atlas.id());
                        glEnable(GL_SCISSOR_TEST);
                        glScissor(part ? 6 : 0, 0, part ? 2 : 4, kSize);
                        glClearDepth(part ? .75 : .5);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        glDisable(GL_SCISSOR_TEST);
                    }
                )
            );
        }
    }));
    const auto after = vkgl::submission_counters();
    CHECK(after.render_passes - before.render_passes == 2);
    CHECK(after.attachment_load_bytes == before.attachment_load_bytes);
    const auto pixels = fixture.read(fixture.atlas.depth_texture());

    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float depth = x < 4 ? .5f : x < 6 ? .25f : .75f;
            check_bits(
                pixels,
                static_cast<std::size_t>(y * kSize + x) * 4,
                std::bit_cast<std::uint32_t>(depth)
            );
        }
    }
}
}  // namespace

// Uses a tiny Vulkan context; every expected value comes from binary-exact synthetic depth arithmetic.
int main() {
    // Context and recording errors are reported within this test executable.
    try {
        test::GlContext context;
        std::string error;

        // A host without Vulkan cannot exercise this native shadow chain.
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }

        test_rejects_invalid_target();
        test_complete_chain();
        test_rejects_incomplete_chain();
        test_separate_partial_passes();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "shadow graph: %s\n", failure.what());
        return 1;
    }
}
