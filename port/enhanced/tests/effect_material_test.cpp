// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Mixed production water/particle shaders and per-pass material setup must preserve every byte.
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#include "common/stats.h"
#include "enhanced/renderer/atmosphere.h"
#include "enhanced/renderer/water.h"
#include "enhanced/tests/probe_fixture.h"
#include "enhanced/tests/water_test_bake.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/effects.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif

using namespace openrac;
using namespace gl;

// Unknown providers opt out; a failed optional shader also retains the complete material.
class FallbackMaterial final : public renderer::EffectMaterial {
public:
    FallbackMaterial(bool fail, bool rebind) : m_fail(fail), m_rebind(rebind) {}

    std::string fragment_source() const override {
        return "vec3 effect_material(vec3 rgb){return rgb;}\n";
    }

    std::string vertex_source() const override {
        return "void effect_vertex(inout vec3 p,inout vec2 uv,inout vec4 rgba){p.x+=.1;}\n";
    }

    std::optional<std::string> quad_fragment_source() const override {
        return m_fail ? std::optional<std::string>{"invalid optional shader"} : std::nullopt;
    }

    bool init(std::string&) override {
        glGenBuffers(1, &m_buffer);
        return true;
    }

    void upload_meshes() override {
        if (m_rebind) {
            // A material may upload another stream before the core quad upload.
            glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
            const float scratch[4]{};
            glBufferData(GL_ARRAY_BUFFER, sizeof(scratch), scratch, GL_STREAM_DRAW);
        }
    }

    void prepare(const renderer::FrameInput&) override {}

    void bind(
        const renderer::Shader&,
        std::uint32_t,
        const renderer::FrameInput&,
        const renderer::EffectUvMotion&
    ) override {}

    void release() override {
        glDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }

private:
    bool m_fail;
    bool m_rebind;
    unsigned m_buffer = 0;
};

int main() {
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    std::vector<std::vector<std::uint8_t>> variant_reference;
    std::size_t compared_variants = 0;
    for (const bool variants : {false, true}) {
        renderer::render_opt::set(renderer::render_opt::Opt::ShaderVariants, variants);
        renderer::render_opt::set(renderer::render_opt::Opt::VkDrawUniform, variants);
        renderer::render_opt::set(renderer::render_opt::Opt::EffectStaticControls, variants);
        renderer::render_opt::set(renderer::render_opt::Opt::WaterLocalShader, variants);
        renderer::Renderer renderer;
        scene::LevelScene scene;
        lighting::use_scene_lighting(scene);
        scene::LevelData level;
        for (const auto p :
             {renderer::Vec3{0, 0, 0}, renderer::Vec3{1, 0, 0}, renderer::Vec3{0, 1, 0}}) {
            scene::Vertex vertex{};
            std::copy(p.begin(), p.end(), vertex.position);
            vertex.normal[2] = 1;
            std::fill(std::begin(vertex.colour), std::end(vertex.colour), 1.f);
            level.vertices.push_back(vertex);
        }
        level.indices = {0, 1, 2};
        scene::Model model;
        model.min = {0, 0, 0};
        model.max = {1, 1, 0};
        model.primitives.push_back({0, 3, 0});
        level.models.push_back(model);
        level.materials.push_back({});
        level.instances[std::size_t(scene::Layer::Terrain)].push_back({});
        CHECK(scene.upload(level, renderer.textures(), error));
        CHECK(test::shadow_probe_fixture(scene, error));
        scene.set_probe_lighting(true, 1);
        scene.set_probe_view(1);

        test::WaterTestBake water;
        renderer::EffectRenderer effects(
            "effect material equivalence",
            renderer::EffectPart::Rest,
            renderer::Bucket::Particles,
            enhanced::atmosphere_material(scene, enhanced::make_water(scene, water.folder))
        );
        renderer::FrameBuffer target;
        CHECK(target.create(64, 64, error));
        renderer::RenderState state{renderer.textures(), {target.id(), 64, 64}, {}};
        const bool initialized = effects.init(state, error);
        if (!initialized) {
            std::fprintf(stderr, "%s\n", error.c_str());
            CHECK(false);
            return test::result();
        }
        for (unsigned texture = 0; texture < 2; ++texture) {
            renderer::Rgba8Image image(4, 4);
            for (unsigned y = 0; y < 4; ++y) {
                for (unsigned x = 0; x < 4; ++x) {
                    const unsigned alpha = ((x + y) % 3) * 127;
                    image.set_texel(x, y, (alpha << 24) | (texture ? 0x00d08030u : 0x003080d0u));
                }
            }
            const auto handle = renderer.textures().add(
                std::move(image), renderer::AlphaScale::Full, "effect fixture"
            );
            renderer.textures().place(1000 + texture, handle);
        }
        std::array<renderer::EffectQuad, 9> quads{};
        for (unsigned i = 0; i < quads.size(); ++i) {
            auto& q = quads[i];
            const float offset = .03f * float(i);
            constexpr float xy[4][2] = {{0, 0}, {.8f, 0}, {0, .8f}, {.8f, .8f}};
            for (unsigned corner = 0; corner < 4; ++corner) {
                q.corner[corner][0] = xy[corner][0] + offset;
                q.corner[corner][1] = xy[corner][1];
                q.corner[corner][2] = 1.2f + .01f * float(i);
                q.corner[corner][3] = 1;
                q.st[corner][0] = xy[corner][0] * 2 - .2f;
                q.st[corner][1] = xy[corner][1] * 2;
                q.rgba[corner] = ((i % 3 == 0 ? 0u : 64u) << 24) | 0x00806080u;
            }
            q.tex0 =
                (1000 + i % 2) | (2ull << 26) | (2ull << 30) | (1ull << 34) | ((i % 4ull) << 35);
            q.tex1 = i % 2 ? 0x60 : 0;
            q.clamp = i % 2 ? 5 : 0;
            q.alpha = i % 3 == 0 ? 0x44 : i % 3 == 1 ? 0x48 : 0x42;
        }
        const auto strip_tex0 = 1000ull | (2ull << 26) | (2ull << 30) | (1ull << 34);
        std::array<renderer::EffectVertex, 6> vertices{
            {{0, 0, 1, 0, 0, 0x80808080},
             {1, 0, 1, 1, 0, 0x80808080},
             {0, 1, 1, 0, 1, 0x80808080},
             {0, 0, 1.05f, 0, 0, 0x40808080},
             {1, 0, 1.05f, 1, 0, 0x40808080},
             {0, 1, 1.05f, 0, 1, 0x40808080}}
        };
        std::array<renderer::EffectStrip, 2> strips{};
        for (unsigned i = 0; i < strips.size(); ++i) {
            strips[i].first = i * 3;
            strips[i].count = 3;
            strips[i].surface = i == 0 ? 1 : 0;
            strips[i].tex0 = strip_tex0;
            strips[i].tex1 = 0x60;
            strips[i].alpha = 0x44;
        }
        renderer::FrameInput input;
        input.camera.position = {.5f, .5f, 4};
        input.camera.view = renderer::look_along(input.camera.position, {0, 0, -1}, {0, 1, 0});
        input.camera.projection = renderer::perspective_reversed(1, 1, .1f, 100);
        input.water_interactor = renderer::Vec3{.4f, .4f, 1};
        input.strip_vertices = vertices;
        stats::set_enabled(stats::Client::Dump, true);
        stats::begin_frame(stats::FrameKind::Tick, 0);
        const auto gauge = [](const char* name) {
            const auto* measured = stats::history(0);
            if (measured) {
                const auto at = std::find_if(
                    measured->gauges.begin(), measured->gauges.end(), [&](const auto& item) {
                        return item.first == name;
                    }
                );
                if (at != measured->gauges.end()) {
                    return at->second;
                }
            }
            return -1.;
        };
        unsigned comparisons = 0;
        std::size_t variant_frame = 0;
        for (unsigned frame = 0; frame < 7; ++frame) {
            input.frame = 10 + frame;
            input.seconds = 1 + double(frame) / 60;
            input.enhanced_water = frame != 2;
            std::array<float, 64> lights{};
            if (frame % 2 == 0) {
                lights[0] = .4f;
                lights[1] = .2f;
                lights[2] = .1f;
                lights[4] = .3f;
                lights[5] = .4f;
                lights[6] = 3;
                lights[7] = 10;
                lights[8] = .1f;
                lights[9] = .2f;
                lights[10] = .3f;
                lights[12] = 20;
                lights[13] = 20;
                lights[14] = 20;
                lights[15] = .1f;
            }
            scene.set_point_lights(lights);
            std::vector<float> sunlight(256, 0);
            sunlight[0] = .3f;
            sunlight[1] = .4f;
            sunlight[2] = .2f;
            sunlight[4] = .2f;
            sunlight[5] = .1f;
            sunlight[6] = -1;
            scene.set_light_sets(sunlight);
            input.texture_filter =
                frame % 2 ? renderer::TextureFilter::Normal : renderer::TextureFilter::EdgeAware;
            input.volumetric_quality =
                frame == 3 ? renderer::VolumetricQuality::Off : renderer::VolumetricQuality::Medium;
            input.strips =
                frame == 1 || frame == 5 ? std::span<const renderer::EffectStrip>{} : strips;
            input.effects =
                frame == 4 || frame == 5 ? std::span<const renderer::EffectQuad>{} : quads;
            strips[1]
                .uv_motion = {{.1f, -.2f}, frame % 2 ? -.03f : .03f, frame * 103u, frame % 2 != 0};
            if (frame == 6) {
                CHECK(target.create(79, 53, error));
                state.target = {target.id(), 79, 53};
                for (unsigned i = 0; i < 3; ++i) {
                    vertices[i].x += 7.5f;
                }
                input.camera.position[0] = 7.5f;
                input.camera.view =
                    renderer::look_along(input.camera.position, {0, 0, -1}, {0, 1, 0});
            }
            lighting::scene_lighting(scene)
                ->prepare_atmosphere(scene, input, state.target, {.1f, .2f, .3f, 1}, {0, 0, 10, 1});
            std::vector<std::uint8_t> reference;
            std::uint64_t draws = 0, triangles = 0;
            for (const unsigned path : {0u, 1u, 2u, 3u, 4u, 8u, 15u, 0u}) {
                renderer::render_opt::set(
                    renderer::render_opt::Opt::EffectQuadShader, (path & 1) != 0
                );
                renderer::render_opt::set(
                    renderer::render_opt::Opt::EffectSetupReuse, (path & 2) != 0
                );
                renderer::render_opt::set(
                    renderer::render_opt::Opt::EffectStripShader, (path & 4) != 0
                );
                renderer::render_opt::set(
                    renderer::render_opt::Opt::WaterMeshReuse, (path & 8) != 0
                );
                glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);
                glDisable(GL_SCISSOR_TEST);
                glClearColor(.04f, .07f, .11f, .3f);
                glClearDepth(0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                state.stats = {};
                effects.render(input, state);
                const auto pixels = target.read_rgba();
                if (!variants) {
                    variant_reference.push_back(pixels);
                } else {
                    CHECK(pixels == variant_reference[variant_frame]);
                    ++compared_variants;
                }
                ++variant_frame;
                stats::begin_frame(stats::FrameKind::Tick, 100 + comparisons);
                if (reference.empty()) {
                    reference = pixels;
                    draws = state.stats.draw_calls;
                    triangles = state.stats.triangles;
                } else {
                    CHECK(pixels == reference);
                    CHECK(
                        std::uint64_t(state.stats.draw_calls) == draws
                        && state.stats.triangles == triangles
                    );
                    ++comparisons;
                }
                if (!input.effects.empty()) {
                    CHECK(gauge("effect quad shader") == ((path & 1) ? 1 : 0));
                    const bool full = !input.strips.empty() || !(path & 1);
                    const bool plain =
                        (path & 1)
                        || (!input.strips.empty() && !strips[1].uv_motion.enabled && (path & 4));
                    CHECK(
                        gauge("effect atmosphere setups")
                        == ((path & 2) ? int(full) + int(plain) : double(state.stats.draw_calls))
                    );
                }
                if (!input.strips.empty()) {
                    CHECK(
                        gauge("effect plain strips")
                        == ((!strips[1].uv_motion.enabled && (path & 4)) ? 1 : 0)
                    );
                }
                CHECK(glGetError() == GL_NO_ERROR);
            }
        }
        std::printf("Compared %u mixed water/particle frames byte-for-byte\n", comparisons);
        // The last mixed frame moved the camera away from these quads.
        input.camera.position = {.5f, .5f, 4};
        input.camera.view = renderer::look_along(input.camera.position, {0, 0, -1}, {0, 1, 0});
        // Real material snapshots must transmit scene colour once, including MSAA and resize.
        vertices[0].x = vertices[2].x = 0;
        vertices[1].x = 1;
        input.strips = std::span<const renderer::EffectStrip>(strips.data(), 1);
        input.effects = {};
        input.enhanced_water = true;
        input.water_interactor.reset();
        input.frame = 1000;
        input.seconds = 2;
        for (const int samples : {1, 4}) {
            CHECK(target.create(samples == 1 ? 64 : 79, samples == 1 ? 64 : 53, error, samples));
            state.target = {target.id(), target.width(), target.height()};
            lighting::scene_lighting(scene)
                ->prepare_atmosphere(scene, input, state.target, {.1f, .2f, .3f, 1}, {0, 0, 10, 1});
            std::array<double, 2> transmitted_difference{};
            for (unsigned depth = 0; depth < 3; ++depth) {
                const float distance = depth == 0 ? 3.06f : depth == 1 ? 12.f : 2.f;
                const auto& p = input.camera.projection;
                const float screen_depth = .5f * ((-p[10] * distance + p[14]) / distance + 1);
                std::array<std::vector<std::uint8_t>, 2> images;
                for (unsigned colour = 0; colour < 2; ++colour) {
                    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glDepthMask(GL_TRUE);
                    glDisable(GL_SCISSOR_TEST);
                    glClearColor(colour == 0 ? .6f : .05f, .1f, colour == 0 ? .05f : .6f, .3f);
                    glClearDepth(screen_depth);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    const auto cleared = target.read_rgba();
                    effects.render(input, state);
                    images[colour] = target.read_rgba();
                    if (depth < 2) {
                        auto debug = enhanced::water_debug_settings();
                        debug.view = 17;
                        enhanced::set_water_debug_settings(debug);
                        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        glDepthMask(GL_TRUE);
                        glDisable(GL_SCISSOR_TEST);
                        glClearColor(colour == 0 ? .6f : .05f, .1f, colour == 0 ? .05f : .6f, .3f);
                        glClearDepth(screen_depth);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        CHECK(target.read_rgba() == cleared);
                        effects.render(input, state);
                        const auto reference = target.read_rgba();
                        debug.view = 0;
                        enhanced::set_water_debug_settings(debug);
                        if (depth == 1) {
                            CHECK(images[colour] == reference);
                        } else {
                            double blended_distance = 0, reference_distance = 0;
                            for (std::size_t at = 0; at < reference.size(); at += 4) {
                                if (reference[at + 3] != 255 || images[colour][at + 3] != 255) {
                                    continue;
                                }
                                for (unsigned c = 0; c < 3; ++c) {
                                    const int blended =
                                        int(images[colour][at + c]) - int(cleared[at + c]);
                                    const int original =
                                        int(reference[at + c]) - int(cleared[at + c]);
                                    CHECK(std::abs(blended) <= std::abs(original) + 1);
                                    blended_distance += std::abs(blended);
                                    reference_distance += std::abs(original);
                                }
                            }
                            CHECK(reference_distance > 0);
                            CHECK(blended_distance < reference_distance * .95);
                        }
                    }
                    if (depth == 2) {
                        CHECK(
                            images[colour] == cleared
                        );  // Above-water foreground remains untouched.
                    }
                }
                if (depth < 2) {
                    unsigned wet_pixels = 0;
                    for (std::size_t at = 0; at < images[0].size(); at += 4) {
                        if (images[0][at + 3] == 255 && images[1][at + 3] == 255) {
                            ++wet_pixels;
                            for (unsigned c = 0; c < 3; ++c) {
                                transmitted_difference[depth] +=
                                    std::abs(int(images[0][at + c]) - int(images[1][at + c]));
                            }
                        }
                    }
                    CHECK(wet_pixels > 20);
                    transmitted_difference[depth] /= std::max(1u, wet_pixels);
                }
                CHECK(glGetError() == GL_NO_ERROR);
            }
            CHECK(transmitted_difference[0] > 30);
            CHECK(transmitted_difference[1] < transmitted_difference[0] * .75);
            std::printf(
                "Water scene colour difference, %dx MSAA: shallow %.3f, deep %.3f bytes/pixel\n",
                target.samples(),
                transmitted_difference[0],
                transmitted_difference[1]
            );
        }
        CHECK(target.create(79, 53, error));
        state.target = {target.id(), 79, 53};
        glClearDepth(0);
        for (const bool fail : {false, true}) {
            renderer::EffectRenderer fallback(
                "effect fallback",
                renderer::EffectPart::Rest,
                renderer::Bucket::Particles,
                std::make_unique<FallbackMaterial>(fail, variants)
            );
            CHECK(fallback.init(state, error) && error.empty());
            input.strips = {};
            input.effects = quads;
            std::vector<std::uint8_t> reference;
            for (const bool specialised : {false, true}) {
                renderer::render_opt::set(renderer::render_opt::Opt::EffectQuadShader, specialised);
                glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                const auto cleared = target.read_rgba();
                fallback.render(input, state);
                const auto pixels = target.read_rgba();
                CHECK(pixels != cleared);
                if (!variants) {
                    variant_reference.push_back(pixels);
                } else {
                    CHECK(pixels == variant_reference[variant_frame]);
                    ++compared_variants;
                }
                ++variant_frame;
                stats::begin_frame(stats::FrameKind::Tick, 300);
                CHECK(gauge("effect quad shader") == 0);
                if (!specialised) {
                    reference = pixels;
                } else {
                    CHECK(pixels == reference);
                }
            }
            fallback.release();
        }
        stats::set_enabled(stats::Client::Dump, false);
        renderer::render_opt::reset();
        effects.release();
        scene.release();
        renderer.release();
        target.release();
    }
    CHECK(compared_variants == variant_reference.size());
    std::printf("Compared %zu specialised effect frames byte-for-byte\n", compared_variants);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
