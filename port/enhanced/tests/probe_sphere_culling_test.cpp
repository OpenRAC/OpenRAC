// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Novalis-scale synthetic records verify compaction tails, reset counts and bounded host uploads.

#include <cstdio>
#include <exception>
#include <vector>

#include "enhanced/viewer/probe_sphere_culling.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac::gl;
namespace renderer = openrac::renderer;
namespace lighting = openrac::lighting;
namespace wgl = renderer::world::gl;

// A fullscreen diagnostic reads only the generated instance count, never the instance records.
constexpr const char* kVertex = R"(#version 410 core
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2 - 1, 0, 1);
})";
constexpr const char* kFragment = R"(#version 410 core
uniform usamplerBuffer commands;
layout(location = 0) out vec4 colour;
void main() { colour = vec4(float(texelFetch(commands, 1).r) / 255, 0, 0, 1); }
)";

// Owns the framebuffer and texture-buffer view used solely by this readback fixture.
struct CommandReadback {
    CommandReadback() = default;
    CommandReadback(const CommandReadback&) = delete;
    CommandReadback& operator=(const CommandReadback&) = delete;
    CommandReadback(CommandReadback&&) = delete;
    CommandReadback& operator=(CommandReadback&&) = delete;
    renderer::Shader shader;      // Fullscreen integer-buffer reader.
    renderer::FrameBuffer frame;  // One RGBA8 pixel stores a count below 256.
    unsigned texture = 0;         // Borrowed argument-buffer view, owned GL name.
    unsigned vao = 0;             // Attribute-free fullscreen draw.

    // Releases view names while the fixture's context is still current.
    ~CommandReadback() {
        glDeleteTextures(1, &texture);
        glDeleteVertexArrays(1, &vao);
    }

    // Creates the one-pixel reader; false reports a shader or framebuffer error.
    bool open(std::string& error) {
        glGenTextures(1, &texture);
        glGenVertexArrays(1, &vao);
        return frame.create(1, 1, error)
               && shader.build("sphere count readback", kVertex, kFragment, error);
    }

    // Reads the device-generated count through an RGBA8 pixel, for verification only.
    unsigned count(unsigned arguments) {
        glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
        glViewport(0, 0, 1, 1);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        shader.use();
        glUniform1i(shader.uniform("commands"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, texture);
        glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_R32UI, arguments);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return frame.read_rgba()[0];
    }
};

// A non-multiple of the compute group size retains the final record and resets empty counts next frame.
void test_resident_compaction_and_uploads() {
    constexpr std::size_t kCount =
        237'107;  // Novalis-scale workload, including an incomplete final group.
    std::vector<lighting::ProbeSphere> spheres(kCount, {{1000, 0, 0, .1f}, {1, 0, 1}, -1, 1});
    spheres.front().position_radius = {0, 0, 0, .1f};
    spheres.back().position_radius = {0, 0, 0, .1f};
    lighting::ProbeSphereCulling culling;
    std::string error;
    CHECK(culling.prepare(spheres, error));
    CommandReadback readback;
    CHECK(readback.open(error));
    renderer::Camera camera;
    const std::array<lighting::ProbeRegionSlot, lighting::kDenseProbeSlots> slots{};
    culling.dispatch(camera, 10, slots);
    CHECK(readback.count(culling.arguments()) == 2);
    const auto before = openrac::vkgl::submission_counters();
    camera.position = {5000, 0, 0};
    culling.dispatch(camera, 10, slots);
    CHECK(readback.count(culling.arguments()) == 0);
    camera.position = {};
    culling.dispatch(camera, 10, slots);
    CHECK(readback.count(culling.arguments()) == 2);
    const auto after = openrac::vkgl::submission_counters();
    // Later dispatches upload uniforms only, rather than another multi-megabyte sphere list.
    CHECK(after.ring_bytes - before.ring_bytes < 64ull * 1024);
    CHECK(after.validation_errors == before.validation_errors);
    CHECK(glGetError() == GL_NO_ERROR);
    std::printf(
        "Sphere stress: %zu records; repeat host upload %llu bytes\n",
        kCount,
        static_cast<unsigned long long>(after.ring_bytes - before.ring_bytes)
    );
}
}  // namespace

int main() {
    // Traversal and allocation failures must be reported as failed tests.
    try {
        openrac::test::GlContext context;
        std::string error;

        // This fixture exercises the Vulkan-only enhanced diagnostic path.
        if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
            return openrac::test::kSkip;
        }

        test_resident_compaction_and_uploads();
        return openrac::test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "%s\n", failure.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected sphere culling fixture failure\n");
        return 1;
    }
}
