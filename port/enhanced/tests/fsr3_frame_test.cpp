// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic temporal frame handoff: Quality renders two-thirds size, while a one-pixel HUD stays native.
#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <memory>

#include "enhanced/fsr3/fsr3.h"
#include "enhanced/fsr3/object_history.h"
#include "enhanced/fsr3/palette_history.h"
#include "enhanced/fsr3/temporal_math.h"
#include "enhanced/renderer/agx.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/vkgl.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;
std::vector<std::vector<unsigned char>> reference_frames;
std::size_t compared_frames = 0;
bool compare_direct = false;
bool with_agx = false;

void test_object_history() {
    namespace opt = renderer::render_opt;
    enhanced::fsr3_detail::ObjectHistory reference, retained;
    std::vector<scene::Instance> instances(256);
    for (std::size_t i = 0; i < instances.size(); ++i) {
        instances[i].temporal_id = i + 1;
        instances[i].model = std::uint32_t(i % 3);
        instances[i].palette = i % 2 ? int(i) : -1;
        instances[i].floats_on_water = i % 7 == 0;
    }
    const auto update = [&](bool deformation) {
        opt::set(opt::Opt::FsrObjectHistoryReuse, false);
        const auto rebuilt = reference.update(instances, deformation);
        opt::set(opt::Opt::FsrObjectHistoryReuse, true);
        const auto reused = retained.update(instances, deformation);
        CHECK(reference.size() == retained.size());
        for (const auto& instance : instances) {
            const auto* a = reference.find(instance.temporal_id);
            const auto* b = retained.find(instance.temporal_id);
            CHECK(a && b);
            if (a && b) {
                CHECK(a->model == b->model && a->matrix == b->matrix);
                CHECK(a->palette == b->palette && a->tint == b->tint);
                CHECK(a->floats_on_water == b->floats_on_water);
                CHECK(a->moby_light == b->moby_light && a->moby_ambient == b->moby_ambient);
                CHECK(a->blend == b->blend);
            }
        }
        return std::pair{rebuilt, reused};
    };
    update(true);
    const auto* stable = retained.find(17);
    std::size_t reference_inserts = 0, reused_inserts = 0;
    for (int frame = 0; frame < 60; ++frame) {
        std::reverse(instances.begin(), instances.end());
        for (auto& instance : instances) {
            instance.matrix[12] = float(frame) * .125f;
            instance.model = std::uint32_t(frame % 3);
        }
        const auto [rebuilt, reused] = update(frame % 2 == 0);
        reference_inserts += rebuilt.inserted;
        reused_inserts += reused.inserted;
        CHECK(reused.inserted == 0 && reused.erased == 0);
        CHECK(retained.find(17) == stable);
    }
    CHECK(reference_inserts == 60 * 256 && reused_inserts == 0);
    std::cout << "FSR history node allocations across 60 stable frames: " << reference_inserts
              << " -> " << reused_inserts << '\n';
    const auto removed = instances[16];
    instances.erase(instances.begin() + 16);
    CHECK(update(true).second.erased == 1);
    CHECK(!retained.find(removed.temporal_id));
    instances.push_back(removed);
    CHECK(update(true).second.inserted == 1);
    instances = {removed, removed};
    instances.back().matrix[12] = 99;
    update(false);
    CHECK(retained.size() == 1 && retained.find(removed.temporal_id)->matrix[12] == 99);
    CHECK(!retained.find(removed.temporal_id)->floats_on_water);
    instances.clear();
    CHECK(update(true).second.erased == 1);
    CHECK(retained.size() == 0 && !retained.find(removed.temporal_id));
    retained.clear();
    opt::reset();
}

// Read the actual history texture, including padding and exact FP32 matrix bits.
void test_palette_history() {
    renderer::FrameBuffer target;
    renderer::Shader shader;
    std::string error;
    CHECK(target.create(4, 4, error));
    CHECK(shader.build(
        "palette history readback",
        R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
        R"(#version 430 core
uniform sampler2D palette;
uniform int matrix_index,component;
layout(location=0) out vec4 result;
void main(){
    ivec2 xy=ivec2((matrix_index%256)*4+component/4,matrix_index/256);
    uint bits=floatBitsToUint(texelFetch(palette,xy,0)[component%4]);
    result=vec4(float(bits&255u),float((bits>>8)&255u),float((bits>>16)&255u),float(bits>>24))/255.;
})",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    for (bool reuse : {false, true}) {
        renderer::render_opt::set(renderer::render_opt::Opt::FsrPaletteReuse, reuse);
        enhanced::fsr3_detail::PaletteHistory history;
        const auto sample = [&](int matrix, int component) {
            glBindFramebuffer(GL_FRAMEBUFFER, target.id());
            glViewport(0, 0, 4, 4);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_CULL_FACE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            shader.use();
            glUniform1i(shader.uniform("palette"), 0);
            glUniform1i(shader.uniform("matrix_index"), matrix);
            glUniform1i(shader.uniform("component"), component);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, history.texture());
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            const auto pixels = target.read_rgba();
            CHECK(pixels.size() >= 4);
            return pixels.size() < 4
                       ? std::uint32_t{0}
                       : std::uint32_t(pixels[0]) | (std::uint32_t(pixels[1]) << 8)
                             | (std::uint32_t(pixels[2]) << 16) | (std::uint32_t(pixels[3]) << 24);
        };
        std::vector<renderer::Mat4> joints(1, renderer::identity());
        joints[0][0] = 1e-8f;
        history.update(joints);
        CHECK(sample(0, 0) == std::bit_cast<std::uint32_t>(joints[0][0]));
        CHECK(sample(255, 15) == 0);
        auto before = vkgl::submission_counters();
        history.update(joints);
        auto after = vkgl::submission_counters();
        CHECK(after.texture_upload_bytes - before.texture_upload_bytes == (reuse ? 0 : 16384));
        std::cout << "Unchanged FSR palette upload bytes (reuse=" << reuse
                  << "): " << after.texture_upload_bytes - before.texture_upload_bytes << '\n';
        joints[0][0] = -0.f;
        before = vkgl::submission_counters();
        history.update(joints);
        after = vkgl::submission_counters();
        CHECK(after.texture_upload_bytes - before.texture_upload_bytes == 16384);
        CHECK(sample(0, 0) == std::bit_cast<std::uint32_t>(-0.f));
        joints.resize(300, renderer::identity());
        joints[299][15] = 100000.f;
        history.update(joints);
        CHECK(sample(299, 15) == std::bit_cast<std::uint32_t>(100000.f));
        CHECK(sample(511, 15) == 0);
        joints.resize(257);
        history.update(joints);
        CHECK(sample(299, 15) == 0);  // shrink within the same image height clears stale joints
        joints.resize(1);
        history.update(joints);
        CHECK(sample(255, 15) == 0);
        history.update({});
        CHECK(sample(0, 0) == 0 && sample(255, 15) == 0);
        history.release();
        history.update(joints);
        CHECK(sample(0, 0) == std::bit_cast<std::uint32_t>(-0.f));
    }
    renderer::render_opt::reset();
    glDeleteVertexArrays(1, &vao);
    CHECK(vkgl::submission_counters().validation_errors == 0);
}

// Procedural world/HUD buckets keep expected colours independent of game assets.
class TestBucket final : public renderer::BucketRenderer {
public:
    // Chooses an opaque world or final HUD draw for this synthetic frame.
    explicit TestBucket(bool hud, const std::array<int, 2>& world_size)
        : BucketRenderer(
              "temporal fixture", hud ? renderer::Bucket::Hud : renderer::Bucket::Terrain
          ),
          m_hud(hud),
          m_world_size(world_size) {}

    ~TestBucket() override {
        if (m_vao) {
            glDeleteVertexArrays(1, &m_vao);
        }
    }

    void render(const renderer::FrameInput&, renderer::RenderState& state) override {
        // A single display pixel reveals accidental low-resolution HUD rendering or later upscaling.
        if (m_hud) {
            CHECK(state.target.width == 96 && state.target.height == 72);
            if (!m_glimmer.id()) {
                std::string error;
                CHECK(m_glimmer.build(
                    "late depth-tested world effect",
                    R"(#version 410 core
uniform float depth;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,depth*2.-1.,1);})",
                    R"(#version 410 core
layout(location=0) out vec4 result;
void main(){result=vec4(0,1,0,1);})",
                    error
                ));
                glGenVertexArrays(1, &m_vao);
            }
            // The native HUD packet stream also contains depth-tested world glimmers.
            m_glimmer.use();
            glBindVertexArray(m_vao);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_GEQUAL);
            glDepthMask(GL_FALSE);
            glDisable(GL_BLEND);
            glDisable(GL_CULL_FACE);
            glEnable(GL_SCISSOR_TEST);
            glScissor(40, 30, 1, 1);
            glUniform1f(m_glimmer.uniform("depth"), .25f);
            glDrawArrays(GL_TRIANGLES, 0, 3);  // behind the wall: must remain hidden
            glScissor(43, 30, 1, 1);
            glUniform1f(m_glimmer.uniform("depth"), .75f);
            glDrawArrays(GL_TRIANGLES, 0, 3);  // in front: must remain visible
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_SCISSOR_TEST);
            glScissor(48, 36, 1, 1);
            glClearColor(1, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);
            return;
        }

        CHECK(state.target.width == m_world_size[0] && state.target.height == m_world_size[1]);
        glClearColor(.25f, .5f, .75f, 1);
        glClearDepth(.5);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

private:
    renderer::Shader m_glimmer;
    unsigned m_vao = 0;
    bool m_hud;                              // Only the final bucket marks a display pixel.
    const std::array<int, 2>& m_world_size;  // Expected extent owned by the enclosing test.
};

// Preset extents and matrix inversion are checked against arithmetic and an identity product.
void test_temporal_math() {
    CHECK(
        (enhanced::fsr3_detail::render_extent({1920, 1080}, renderer::TemporalQuality::Quality)
         == std::array<int, 2>{1280, 720})
    );
    CHECK(
        (enhanced::fsr3_detail::render_extent({1920, 1080}, renderer::TemporalQuality::Performance)
         == std::array<int, 2>{960, 540})
    );
    renderer::Mat4 matrix = renderer::multiply(
        renderer::perspective_reversed(1, 1.5f, .05f, 2000), renderer::translation({-3, 4, 2})
    );
    renderer::Mat4 inverse;
    CHECK(enhanced::fsr3_detail::invert(matrix, inverse));
    const auto product = renderer::multiply(matrix, inverse);

    // A matrix times its computed inverse must reproduce the identity in every component.
    for (std::size_t n = 0; n < product.size(); ++n) {
        CHECK(std::abs(product[n] - renderer::identity()[n]) < .001f);
    }

    CHECK(!enhanced::fsr3_detail::invert({}, inverse));
}

// Checks one resolved frame against constant world RGB and an exact one-pixel HUD marker.
void check_temporal_frame(
    renderer::Renderer& renderer,
    scene::LevelScene& scene,
    enhanced::Fsr3& upscaler,
    enhanced::AgxPreview& agx,
    renderer::FrameBuffer& display,
    renderer::TemporalQuality quality,
    bool blur = false,
    bool cinematic = false
) {
    std::string error;
    renderer::FrameInput input;
    input.volumetric_quality = renderer::VolumetricQuality::Medium;
    input.camera.projection = renderer::perspective_reversed(1, 1.5f, .05f, 2000);
    renderer::RenderTarget target{display.id(), 96, 72};
    bool before_hud_called = false;
    input.before_hud = [&] {
        before_hud_called = true;
    };
    if (!cinematic) {
        input.camera.target = renderer::Vec3{};
        input.camera.target_id = 1;
    }
    upscaler.set_scene({}, {});
    const enhanced::TemporalFrame frame{
        input, target, display, scene, renderer.textures(), 0, blur, .5f, cinematic
    };
    const bool prepared = upscaler.begin(frame, quality, 0, error);
    CHECK(prepared);

    // An incomplete setup must not be rendered as if reconstruction succeeded.
    if (!prepared) {
        std::cerr << error << '\n';
        return;
    }

    if (with_agx) {
        auto before_hud = std::move(input.before_hud);
        input.before_hud = [&agx, &display, before_hud = std::move(before_hud)] {
            const auto texture = display.colour_texture();
            agx.apply(display, 0);
            CHECK(display.colour_texture() == texture);
            before_hud();
        };
    }
    renderer.render(input, target);
    CHECK(before_hud_called);
    const auto pixels = display.read_rgba();
    if (compare_direct) {
        CHECK(compared_frames < reference_frames.size());
        if (compared_frames < reference_frames.size()) {
            if (with_agx) {
                // Different shader stages can round the same FP32 curve to adjacent UNORM8 bytes.
                // Alpha and the independent HUD/depth assertions remain exact.
                const auto& reference = reference_frames[compared_frames];
                for (std::size_t i = 0; i < pixels.size(); ++i) {
                    CHECK(std::abs(int(pixels[i]) - int(reference[i])) <= (i % 4 == 3 ? 0 : 1));
                }
            } else {
                CHECK(pixels == reference_frames[compared_frames]);
            }
        }
        ++compared_frames;
    } else {
        reference_frames.push_back(pixels);
    }
    // read_rgba returns top row first; GL pixel (48,36) is row 35.
    const std::size_t at = (std::size_t{35} * 96 + 48) * 4;
    CHECK(pixels[at] == 255 && pixels[at + 1] == 0 && pixels[at + 2] == 0);
    const std::array expected = with_agx ? std::array{76, 132, 176} : std::array{64, 128, 191};
    for (unsigned c = 0; c < 3; ++c) {
        CHECK(std::abs(int(pixels[at + 4 + c]) - expected[c]) <= 2);
    }
    const auto behind = (std::size_t{41} * 96 + 40) * 4;
    const auto front = (std::size_t{41} * 96 + 43) * 4;
    CHECK(
        std::abs(int(pixels[behind]) - expected[0]) <= 2
        && std::abs(int(pixels[behind + 1]) - expected[1]) <= 2
    );
    CHECK(pixels[front] == 0 && pixels[front + 1] == 255 && pixels[front + 2] == 0);
}

// Resolving before the final HUD bucket retains world RGB across quality and output-image changes.
void test_world_resolve_and_native_hud() {
    renderer::Renderer renderer;
    scene::LevelScene scene;
    enhanced::Fsr3 upscaler;
    enhanced::AgxPreview agx;
    renderer::FrameBuffer display;
    std::string error;
    std::array<int, 2> world_size{64, 48};
    CHECK(renderer.init(error));
    CHECK(renderer.add(std::make_unique<TestBucket>(false, world_size), error));
    CHECK(renderer.add(std::make_unique<TestBucket>(true, world_size), error));
    CHECK(display.create(96, 72, error));

    // Preset switches resize history; replacing the output at the same extent tests recycled texture names.
    for (int n = 0; n < 13; ++n) {
        auto quality = renderer::TemporalQuality::Quality;
        world_size = {64, 48};

        // Performance halves both display dimensions.
        if (n == 2) {
            quality = renderer::TemporalQuality::Performance;
            world_size = {48, 36};
        }

        // Native AA reconstructs at display resolution.
        if (n == 3) {
            quality = renderer::TemporalQuality::NativeAA;
            world_size = {96, 72};
        }

        if (n >= 7) {
            quality = renderer::TemporalQuality::Off;
            world_size = {96, 72};
        }

        bool recreated = true;

        // A recreated framebuffer can reuse its previous texture name without retaining storage usage.
        if (n == 5) {
            recreated = display.create(96, 72, error);
        }

        CHECK(recreated);
        check_temporal_frame(
            renderer, scene, upscaler, agx, display, quality, n >= 6, n == 9 || n == 10
        );
    }

    upscaler.release();
    agx.release();
    display.release();
    renderer.release();
    CHECK(vkgl::submission_counters().validation_errors == 0);
}
}  // namespace

int main() {
    try {
        // Construction, repeated release and destruction cannot require a graphics context.
        {
            enhanced::Fsr3 unused;
            unused.release();
            unused.release();
            enhanced::AgxPreview tone;
            tone.release();
        }
        test_temporal_math();
        test_object_history();
        test::GlContext context;
        std::string error;

        // Only unavailable graphics hardware may skip the synthetic GPU part.
        if (!context.open(error)) {
            std::cerr << error << '\n';
            return test::kSkip;
        }

        for (bool tone : {false, true}) {
            with_agx = tone;
            for (bool direct : {false, true}) {
                renderer::render_opt::set(
                    renderer::render_opt::Opt::MotionBlurFastPath, tone || direct
                );
                renderer::render_opt::set(
                    renderer::render_opt::Opt::FsrPaletteReuse, tone || direct
                );
                renderer::render_opt::set(
                    renderer::render_opt::Opt::FsrObjectHistoryReuse, tone || direct
                );
                renderer::render_opt::set(renderer::render_opt::Opt::AgxDirectOutput, direct);
                compare_direct = direct;
                test_world_resolve_and_native_hud();
            }
        }
        CHECK(compared_frames == reference_frames.size());
        test_palette_history();
        std::cout << "Compared " << compared_frames
                  << " complete FSR/HUD frames (plain exact, AgX RGB within one UNORM8 step)\n";
        renderer::render_opt::reset();
        return test::result();
    } catch (...) {
        return 1;
    }
}
