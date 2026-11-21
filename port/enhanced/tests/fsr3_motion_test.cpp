// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic rigid and skinned velocity checks use a known quarter-unit pose change in identity clip space.
#include <algorithm>
#include <iostream>
#include <vector>

#include "common/stats.h"
#include "enhanced/fsr3/frame_target.h"
#include "enhanced/fsr3/temporal_math.h"
#include "enhanced/viewer/scene_lighting.h"
#include "fsr3_shaders.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/gl_stats.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/graph/temporal_graph.h"
#include "renderer/vulkan/vkgl.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/graph_fixture.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;

// Float velocity is converted to UNORM only for readback; 16 pixels maps to one output colour unit.
constexpr const char* kReadMotion = R"glsl(#version 410 core
uniform sampler2D tex_T0;
uniform float motion_scale;
out vec4 colour;
void main() {
    vec4 motion = texelFetch(tex_T0, ivec2(gl_FragCoord.xy), 0);
    colour = vec4(motion.xy * motion_scale / 16.0 + .5, motion.z, 1);
}
)glsl";

// Builds three test models with optional cutouts and an overlapping two-material run.
scene::LevelData motion_level(bool cutout) {
    scene::LevelData level;
    level.vertices.resize(3);
    // An oversized triangle covers the centre without edge interpolation ambiguity.
    constexpr std::array<std::array<float, 3>, 3> kPositions{{{-1, -1, 0}, {3, -1, 0}, {-1, 3, 0}}};

    // Unit alpha and weight isolate pose motion from shading and material coverage.
    for (std::size_t n = 0; n < level.vertices.size(); ++n) {
        auto& vertex = level.vertices[n];
        std::copy(kPositions[n].begin(), kPositions[n].end(), vertex.position);
        std::fill_n(vertex.colour, 4, 1.0f);
        vertex.weights[0] = 1;
        vertex.light_slot = cutout ? -2.0f : -1.0f;
        vertex.uv[0] = (vertex.position[0] + 1.0f) * 0.5f;
    }

    level.indices = {0, 1, 2};
    level.materials.emplace_back();
    level.models.emplace_back();
    level.models[0].primitives.push_back({0, 3, 0});
    level.models[0].joints = 1;
    level.models.push_back(level.models[0]);
    level.models.push_back(level.models[0]);
    level.models[2].primitives.push_back({0, 3, 0});
    // Half of the alpha texture is transparent; the shader must retain its original coverage.
    if (cutout) {
        renderer::Rgba8Image image(2, 1);
        image.set_texel(0, 0, 0x00FFFFFFu);
        image.set_texel(1, 0, 0xFFFFFFFFu);
        level.images.push_back(image);
        level.image_paths.push_back("motion-cutout.png");
        level.translucent_images.push_back(true);
        level.materials[0].image = 0;
        level.materials[0].cutout = true;
    }
    return level;
}

// Geometry, shaders and targets owned by one render-thread fixture.
struct MotionFixture {
    MotionFixture() = default;
    MotionFixture(const MotionFixture&) = delete;
    MotionFixture& operator=(const MotionFixture&) = delete;
    MotionFixture(MotionFixture&&) = delete;
    MotionFixture& operator=(MotionFixture&&) = delete;
    renderer::Renderer renderer;                // Owns original alpha textures.
    scene::LevelScene scene;                    // Owns uploaded mesh and joint data.
    enhanced::fsr3_detail::FrameTarget motion;  // Signed RGBA16F velocity output.
    renderer::FrameBuffer output;               // UNORM readback target.
    renderer::Shader shader;                    // Production velocity shader.
    renderer::Shader read;                      // Test-only float-to-UNORM conversion.
    unsigned vao = 0;                           // Empty VAO for readback triangle.
    unsigned previous_palette = 0;              // Independent previous-frame joint image.

    // Creates a triangle whose first joint has unit weight at every vertex.
    bool create(std::string& error, bool cutout = false) {
        lighting::use_scene_lighting(scene);
        const auto level = motion_level(cutout);
        glGenVertexArrays(1, &vao);
        std::vector<scene::Instance> instances(3);
        instances[1].model = 1;
        instances[2].model = 2;

        // A fixture that cannot allocate its production pipeline cannot provide a meaningful sample.
        if (!renderer.init(error) || !scene.upload(level, renderer.textures(), error)
            || !motion.create(64, 64, 0x881A, 0, error) || !output.create(64, 64, error)
            || !shader.build(
                "motion fixture",
                scene.deformation_vertex(enhanced::shaders::motion_vert),
                scene.motion_fragment(enhanced::shaders::motion_frag),
                error
            )
            || !read.build(
                "read motion fixture", enhanced::shaders::fullscreen_vert, kReadMotion, error
            )) {
            return false;
        }

        scene.set_instances(scene::Layer::Mobys, instances);
        // A palette row contains 256 matrices; the first is an identity joint.
        std::vector<renderer::Mat4> joints(256, renderer::identity());
        glGenTextures(1, &previous_palette);
        glBindTexture(GL_TEXTURE_2D, previous_palette);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8814, 1024, 1, 0, GL_RGBA, GL_FLOAT, joints.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        scene.set_palette({renderer::identity()});
        return true;
    }

    // Samples either submission path. A supplied world depth uses zero-to-one rasterisation.
    std::vector<std::uint8_t> sample_many(
        std::span<const scene::MotionInstance> instances,
        scene::MotionSubmission submission,
        unsigned world_depth = 0
    ) {
        glBindFramebuffer(GL_FRAMEBUFFER, motion.framebuffer());
        glViewport(0, 0, 64, 64);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glClearColor(-8, -8, 0.25f, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        scene::MotionView view;
        view.raster = renderer::identity();
        view.current = renderer::identity();
        view.previous = renderer::identity();
        view.size = {64, 64};
        view.raster_zero_to_one = world_depth != 0;
        if (world_depth != 0) {
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, world_depth, 0
            );
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_GEQUAL);
        }
        // Both poses index separate matrices in the same synthetic palette image.
        view.previous_palette = previous_palette;
        // The owning render thread either records immediately or freezes this overlay until scope exit.
        const auto draw = [&] {
            scene.draw_motion(shader, instances, view, renderer.textures(), submission);
        };
        // Vulkan collects the production overlay; compatibility rendering keeps its direct draw.
        if (vkgl::active()) {
            CHECK(vkgl::temporal_chain([&] {
                const vkgl::TemporalTarget target{
                    motion.framebuffer(),
                    vkgl::TemporalContents::Preserve,
                    renderer::graph::Aspect::Colour,
                    world_depth != 0
                };
                CHECK(vkgl::temporal_raster("fixture object motion", target, {}, draw));
            }));
        } else {
            draw();
        }
        if (world_depth != 0) {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0
            );
        }
        return read_motion();
    }

    // Converts the signed motion target to bytes without changing the sampled velocity field.
    std::vector<std::uint8_t> read_motion(float scale = 1) {
        glBindFramebuffer(GL_FRAMEBUFFER, output.id());
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, motion.texture());
        read.use();
        glUniform1f(read.uniform("motion_scale"), scale);
        openrac::test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
        return output.read_rgba();
    }

    // Draws the production depth-derived field with explicit temporal graph dependencies.
    void draw_camera(
        renderer::Shader& camera,
        unsigned depth,
        const renderer::Mat4& raster,
        const renderer::Mat4& previous,
        float jitter
    ) {
        renderer::Mat4 inverse;
        renderer::Mat4 previous_clip;
        CHECK(enhanced::fsr3_detail::camera_transforms(raster, previous, inverse, previous_clip));
        glBindFramebuffer(GL_FRAMEBUFFER, motion.framebuffer());
        glViewport(0, 0, 64, 64);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(vao);
        camera.use();
        glUniformMatrix4fv(camera.uniform("inverse_raster"), 1, GL_FALSE, inverse.data());
        glUniformMatrix4fv(camera.uniform("previous_view"), 1, GL_FALSE, previous.data());
        glUniformMatrix4fv(camera.uniform("previous_clip"), 1, GL_FALSE, previous_clip.data());
        glUniform3f(camera.uniform("camera_position"), 0, 0, 0);
        glUniform2f(camera.uniform("jitter"), jitter, -jitter);
        glUniform1i(camera.uniform("depth_only"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth);
        const auto draw = [] {
            openrac::test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
        };

        // Vulkan needs declared image reads; compatibility GL draws immediately.
        if (vkgl::active()) {
            bool raster_recorded = false;
            const auto recorded = vkgl::temporal_chain([&] {
                const std::array inputs{depth};
                raster_recorded = static_cast<bool>(vkgl::temporal_raster(
                    "fixture camera motion", {motion.framebuffer()}, inputs, draw
                ));
            });
            CHECK(recorded);
            CHECK(raster_recorded);
        } else {
            draw();
        }
    }

    // Reads the signed centre velocity through the production batched path.
    std::array<int, 2> sample(scene::MotionInstance instance) {
        const auto pixels = sample_many(std::span(&instance, 1), scene::MotionSubmission::Batched);
        const std::size_t at = (std::size_t{32} * 64 + 32) * 4;
        return {pixels[at], pixels[at + 1]};
    }

    // Releases scene images before their pool and graphics context are destroyed.
    ~MotionFixture() {
        scene.release();
        motion.release();
        output.release();
        shader.release();
        read.release();
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &previous_palette);
        renderer.release();
    }
};

// Separates identity clip space from the poorly conditioned distant perspective case.
enum class CameraDistance {
    Near,
    Distant
};

// Readback maps zero motion to 128 and four pixels to 191; opposite Y motion mirrors that value.
void check_camera_field(std::span<const std::uint8_t> pixels, int expected) {
    int largest_error = 0;

    // Every covered pixel must retain the same velocity and valid history.
    for (std::size_t at = 0; at < pixels.size(); at += 4) {
        largest_error = std::max(largest_error, std::abs(pixels[at] - expected));
        largest_error = std::max(largest_error, std::abs(pixels[at + 1] - (255 - expected)));
        CHECK(pixels[at + 2] == 0);
    }

    CHECK(largest_error <= 1);
}

// Static scenery must retain camera motion while removing the raster jitter from its velocity.
void test_static_camera_motion(CameraDistance distance) {
    const bool distant = distance == CameraDistance::Distant;
    MotionFixture fixture;
    renderer::FrameBuffer depth;
    renderer::Shader camera;
    std::string error;
    const bool ready = fixture.create(error) && depth.create(64, 64, error, 1, true)
                       && camera.build(
                           "camera motion fixture",
                           enhanced::shaders::fullscreen_vert,
                           enhanced::shaders::camera_motion_frag,
                           error
                       );
    CHECK(ready);

    // A failed setup cannot distinguish a missing field from an incorrect velocity.
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, depth.id());
    glDepthMask(GL_TRUE);
    // A reversed depth near .0005 places the surface about 100 units from the perspective camera.
    glClearDepth(distant ? .0005 : .5);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Both signs of subpixel jitter must cancel, including with a stationary camera.
    for (const float jitter : {-.375f, .25f}) {
        for (const float movement : {0.0f, .125f}) {
            // Magnify distant-camera errors to a 1920-pixel render width without a large target.
            if (distant && movement != 0) {
                continue;
            }

            const auto offset = renderer::translation({2 * jitter / 64, -2 * jitter / 64, 0});
            auto previous = renderer::translation({movement, -movement, 0});

            // Large camera translations expose precision loss hidden by identity clip space.
            if (distant) {
                previous = renderer::multiply(
                    renderer::perspective_reversed(1, 1.5f, .05f, 2000),
                    renderer::translation({-134, -234, -160})
                );
            }

            const auto raster = distant ? renderer::multiply(offset, previous) : offset;
            fixture.draw_camera(camera, depth.depth_texture(), raster, previous, jitter);

            const auto pixels = fixture.read_motion(distant ? 30.0f : 1.0f);
            check_camera_field(pixels, movement == 0 ? 128 : 191);
        }
    }
}

// A previous quarter-unit translation yields eight render pixels of current-to-previous motion.
void test_rigid_motion_and_new_object_rejection() {
    MotionFixture fixture;
    std::string error;
    const bool ready = fixture.create(error);
    CHECK(ready);

    // Setup diagnostics identify shader or allocation failures before any readback is attempted.
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }

    scene::MotionInstance instance;
    instance.current = renderer::identity();
    instance.previous = renderer::translation({.25f, 0, 0});
    auto velocity = fixture.sample(instance);
    // Eight pixels / sixteen plus 0.5 is 1.0; stationary Y remains 0.5.
    CHECK(velocity[0] >= 253 && std::abs(velocity[1] - 128) <= 1);
    instance.reactive = true;
    instance.palettes = {-1, 100000};
    velocity = fixture.sample(instance);
    CHECK(std::abs(velocity[0] - 128) <= 1);
    CHECK(std::abs(velocity[1] - 128) <= 1);
}

// Skinning motion follows displayed joint deformation even when the object's world matrix stays still.
void test_skinned_motion() {
    MotionFixture fixture;
    std::string error;
    const bool ready = fixture.create(error);
    CHECK(ready);

    // Setup diagnostics identify shader or allocation failures before any readback is attempted.
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }

    constexpr std::size_t kSecondRow = 256;
    std::vector palette(kSecondRow + 1, renderer::identity());
    fixture.scene.set_palette(palette);
    palette[0] = renderer::translation({0, .25f, 0});
    fixture.scene.set_palette(palette);
    scene::MotionInstance instance;
    instance.current = renderer::identity();
    instance.previous = renderer::identity();
    instance.palettes = {0, 0};
    const auto velocity = fixture.sample(instance);
    CHECK(std::abs(velocity[0] - 128) <= 1);
    CHECK(velocity[1] <= 2);
    instance.palettes[0] = static_cast<int>(kSecondRow);
    const auto unchanged_row = fixture.sample(instance);
    CHECK(std::abs(unchanged_row[0] - 128) <= 1 && std::abs(unchanged_row[1] - 128) <= 1);
}

// Creates overlapping rigid/skinned objects, model boundaries and distinct coverage/history flags.
std::vector<scene::MotionInstance> mixed_motion_instances() {
    std::vector<scene::MotionInstance> instances(12);

    // Consecutive runs mix rigid and independently skinned poses with distinct history flags.
    for (std::size_t n = 0; n < instances.size(); ++n) {
        auto& instance = instances[n];
        const std::size_t row = n / 4;
        instance.current = renderer::multiply(
            renderer::translation(
                {static_cast<float>(n % 4) * .4f - .6f, static_cast<float>(row) * .4f - .6f, 0}
            ),
            renderer::scale(.3f)
        );
        instance.previous =
            renderer::multiply(renderer::translation({.0625f, -.03125f, 0}), instance.current);
        instance.palettes = {static_cast<int>(n % 2), 0};
        instance.reactive = n % 3 == 0;
        instance.exclude_glow = n % 5 == 0;
    }
    instances[4].model = 1;
    instances[7].alpha = .5f;
    instances[8].model = 99;
    instances[9].model = 2;
    instances[10].model = 2;
    instances[0].palettes[1] = 100000;
    return instances;
}

// Checks exact velocities, opacity, reactive/glow flags and ordering against individual draws.
void test_batched_motion_matches_reference(bool cutout) {
    MotionFixture fixture;
    std::string error;
    const bool ready = fixture.create(error, cutout);
    CHECK(ready);

    // Failed allocation or shader setup cannot provide a meaningful comparison.
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }

    fixture.scene.set_palette(
        {renderer::translation({0, .125f, 0}), renderer::translation({.125f, 0, 0})}
    );
    auto instances = mixed_motion_instances();
    const std::size_t counter = static_cast<std::size_t>(stats::Counter::DrawCalls);
    const std::uint64_t before = stats::g_counters[counter];
    const auto reference = fixture.sample_many(instances, scene::MotionSubmission::Individual);
    const std::uint64_t reference_draws = stats::g_counters[counter] - before;
    const std::uint64_t middle = stats::g_counters[counter];
    CHECK(fixture.sample_many(instances, scene::MotionSubmission::Batched) == reference);
    CHECK(stats::g_counters[counter] - middle < reference_draws);
    std::reverse(instances.begin(), instances.end());
    const auto reversed = fixture.sample_many(instances, scene::MotionSubmission::Individual);
    CHECK(fixture.sample_many(instances, scene::MotionSubmission::Batched) == reversed);
    instances.resize(2);
    const auto smaller = fixture.sample_many(instances, scene::MotionSubmission::Individual);
    CHECK(fixture.sample_many(instances, scene::MotionSubmission::Batched) == smaller);
    // The same rigid, skinned and cutout coverage must match across repeated graph recording.

    CHECK(fixture.sample_many(instances, scene::MotionSubmission::Batched) == smaller);
    renderer::render_opt::reset();
    CHECK(glGetError() == GL_NO_ERROR);
}

void test_precise_depth_occludes_motion() {
    if (glClipControl == nullptr) {
        return;
    }

    MotionFixture fixture;
    renderer::FrameBuffer world;
    std::string error;
    const bool ready = fixture.create(error) && world.create(64, 64, error, 1, true);
    CHECK(ready);
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }

    constexpr float kWorldDepth = .75f;
    glBindFramebuffer(GL_FRAMEBUFFER, world.id());
    glDepthMask(GL_TRUE);
    glClearDepth(kWorldDepth);
    glClear(GL_DEPTH_BUFFER_BIT);

    scene::MotionInstance instance;
    instance.current = renderer::translation({0, 0, .625f});
    instance.previous = renderer::translation({.25f, 0, .625f});
    const auto sample = [&](scene::MotionSubmission submission) {
        const auto pixels =
            fixture.sample_many(std::span(&instance, 1), submission, world.depth_texture());
        return pixels[(32 * 64 + 32) * 4];
    };
    // The old [-1, 1] convention remaps .625 to .8125 and wrongly passes the .75 depth test.
    CHECK(sample(scene::MotionSubmission::Batched) <= 1);
    CHECK(sample(scene::MotionSubmission::Individual) <= 1);
    // Matching world depth still writes the full eight-pixel horizontal velocity.
    instance.current[14] = instance.previous[14] = kWorldDepth;
    CHECK(sample(scene::MotionSubmission::Batched) >= 254);
    CHECK(sample(scene::MotionSubmission::Individual) >= 254);
}

}  // namespace

int main() {
    try {
        test::GlContext context;
        std::string error;

        // Only an unavailable graphics context may skip the GPU fixture.
        if (!context.open(error)) {
            std::cerr << error << '\n';
            return test::kSkip;
        }

        // Count native draw submissions as well as comparing the complete output images.
        renderer::gl_stats::install(platform::Window::gl_loader());
        test_static_camera_motion(CameraDistance::Near);
        test_static_camera_motion(CameraDistance::Distant);
        test_rigid_motion_and_new_object_rejection();
        test_skinned_motion();
        test_batched_motion_matches_reference(false);
        test_batched_motion_matches_reference(true);
        test_precise_depth_occludes_motion();
        // Nested material reads must not change layouts behind the temporal graph's attachment declarations.
        if (vkgl::active()) {
            CHECK(vkgl::submission_counters().validation_errors == 0);
        }
        return test::result();
    } catch (...) {
        return 1;
    }
}
