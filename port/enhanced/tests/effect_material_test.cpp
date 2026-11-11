// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Mixed production water/particle shaders and per-pass material setup must preserve every byte.
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <numbers>
#include <string_view>
#include <vector>

#include "common/stats.h"
#include "enhanced/renderer/atmosphere.h"
#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_internal.h"
#include "enhanced/tests/probe_fixture.h"
#include "enhanced/tests/water_test_bake.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/effects.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/surface_chain.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/temporal_graph.h"
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

namespace {
// Deferring real water/particle work must retain metadata until a following temporal consumer executes.
void draw_joined_effects(
    renderer::EffectRenderer& effects,
    const renderer::FrameInput& input,
    renderer::RenderState& state,
    renderer::FrameBuffer& target
) {
#ifdef OPENRAC_VULKAN
    renderer::FrameBuffer copied;
    std::string error;
    CHECK(copied.create(state.target.width, state.target.height, error));
    bool copied_before_history = false;
    const auto recorded = renderer::run_surface_chain([&] {
        CHECK(effects.record_colour(input, state));
        const auto completed = vkgl::temporal_chain([&] {
            CHECK(
                vkgl::temporal_blit(
                    "effect temporal consumer",
                    target.id(),
                    copied.id(),
                    vkgl::TemporalConsumer::Shader,
                    [&] {
                        target.blit_to(copied.id(), state.target.width, state.target.height);
                        copied_before_history = true;
                    }
                )
            );
        });
        CHECK(completed);
        CHECK(copied_before_history);
    });
    CHECK(recorded);
    CHECK(copied.read_rgba() == target.read_rgba());
#else
    (void)target;
    effects.render(input, state);
#endif
}
}  // namespace

// Compare the separable FFT against direct synthesis at an off-axis grid point.
void check_ocean_fft(bool fine) {
    using namespace enhanced::water_detail;
    auto spectrum = std::make_unique<OceanSpectrum>(12., fine);
    constexpr double time = 1.25;
    constexpr int x = 13, y = 21;
    spectrum->evaluate(time);
    double reference = 0.;
    for (int ky = 0; ky < kOceanSide; ++ky) {
        for (int kx = 0; kx < kOceanSide; ++kx) {
            const int i = ky * kOceanSide + kx;
            const double phase = 2 * std::numbers::pi * (kx * x + ky * y) / kOceanSide
                                 - spectrum->frequency[i] * time;
            reference += 2 * (spectrum->seed[i] * std::polar(1., phase)).real();
        }
    }
    CHECK(std::abs(spectrum->pixels[y * kOceanSide + x][0] - reference * spectrum->gain) < 1e-6);
    const float difference =
        spectrum->pixels[y * kOceanSide + x + 1][0] - spectrum->pixels[y * kOceanSide + x - 1][0];
    CHECK(
        std::abs(
            spectrum->pixels[y * kOceanSide + x][1] - difference * kOceanSide / (2 * spectrum->tile)
        )
        < 1e-6
    );
    for (const auto& pixel : spectrum->pixels) {
        CHECK(std::abs(pixel[0]) < (fine ? .06f : .5f));
    }
}

// A fixed camera must see a correction toggle without changing the FFT clock.
void check_ocean_material(renderer::Renderer& renderer, scene::LevelScene& scene, bool spectrum) {
    test::WaterTestBake fixture;
    const auto path = fixture.folder / "ocean.wave";
    renderer::Rgba8Image ocean_colour(1, 1);
    ocean_colour.set_texel(0, 0, 0xffc08040);
    renderer.textures().place(
        1050, renderer.textures().add(std::move(ocean_colour), renderer::AlphaScale::Full, "ocean")
    );
    {
        std::ofstream out(path);
        out << "{\"version\":" << (spectrum ? 3 : 1);
        out << R"(,"heading":180,"period":3.2,"wavenumber":0.4,"offshore_depth":12,
                  "patch":{"size":64,"cell":0.5,"mean_height":223,"origin":[0,0],"bank":[)";
        for (int i = 0; i < 4096; ++i) {
            out << (i ? ",false" : "false");
        }
        out << "],\"depth\":[";
        for (int i = 0; i < 4096; ++i) {
            out << (i ? ",12" : "12");
        }
        out << "]}";
        if (spectrum) {
            // Put the only correction in BA, leaving the dominant mode's RG clear.
            out << R"(,"modes":[
                {"heading":180,"period":3.2,"wavenumber":0.4,"amplitude":0.075},
                {"heading":155,"period":2.65,"wavenumber":0.57,"amplitude":0.045)";
        }
        out << ",\"correction\":[";
        for (int i = 0; i < 4096; ++i) {
            if (spectrum && i == 0) {
                out << "null";
                continue;
            }
            const auto ramp = 4.f * std::sin(float(i % 64) * .36f);
            out << (i ? ",[" : "[") << (spectrum ? 0.f : ramp) << ',' << (spectrum ? ramp : 0.f)
                << ']';
        }
        out << "]}";
        if (spectrum) {
            out << "]}";
        }
    }
    if (spectrum) {
        std::ofstream profile(fixture.folder / "ocean-profile.json");
        profile << R"({"near_radius":24,"fine":{"rms":0.004},"material":{
            "colour":[0.08,0.16,0.32],"absorption":[0,0.5,1],
            "roughness":0.3,"reflectivity":0.04,"reflection_strength":1.5}})";
    }
    auto material = enhanced::make_water(scene, fixture.folder);
    std::filesystem::remove(path);
    CHECK(bool(material));
    renderer::EffectRenderer effects(
        "ocean correction",
        renderer::EffectPart::Rest,
        renderer::Bucket::Particles,
        std::move(material)
    );
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(64, 64, error));
    renderer::RenderState state{renderer.textures(), {target.id(), 64, 64}, {}};
    CHECK(effects.init(state, error));
    // The spectrum fixture publishes its profile and resets live edits back to that profile.
    if (spectrum) {
        const auto authored = enhanced::ocean_material();
        CHECK((authored.colour == std::array{.08f, .16f, .32f}));
        CHECK((authored.absorption == std::array{0.f, .5f, 1.f}));
        CHECK(authored.roughness == .3f);
        CHECK(authored.reflectivity == .04f);
        CHECK(authored.reflection_strength == 1.5f);
        enhanced::set_ocean_material({});
        CHECK(enhanced::ocean_material().colour != authored.colour);
        enhanced::reset_ocean_material();
        CHECK(enhanced::ocean_material().colour == authored.colour);
    }
    const std::array<renderer::EffectVertex, 4> vertices{{
        {0, 0, 223, 0, 0, 0x80808080},
        {50, 0, 223, 1, 0, 0x80808080},
        {0, 50, 223, 0, 1, 0x80808080},
        {50, 50, 223, 1, 1, 0x80808080},
    }};
    renderer::EffectStrip strip{};
    strip.count = 4;
    strip.surface = 1;
    strip.tex0 = 1050ull | (2ull << 26) | (2ull << 30) | (1ull << 34);
    strip.alpha = 0x44;
    renderer::FrameInput input;
    input.enhanced_water = true;
    input.camera.position = {16, 9, 228};
    input.camera.view = renderer::look_along(input.camera.position, {0, .7f, -.7f}, {0, 1, 0});
    input.camera.projection = renderer::perspective_reversed(1, 1, .1f, 100);
    input.strip_vertices = vertices;
    input.strips = {&strip, 1};
    const auto saved = enhanced::water_debug_settings();
    auto settings = saved;
    settings.paused = true;
    const auto draw = [&](bool correction) {
        settings.coastal_correction = correction;
        enhanced::set_water_debug_settings(settings);
        ++input.frame;
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, 64, 64);
        glDepthMask(GL_TRUE);
        glClearDepth(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        effects.render(input, state);
        return target.read_rgba();
    };
    const auto offshore = draw(false);
    CHECK(draw(true) != offshore);
    CHECK(draw(false) == offshore);
    if (spectrum) {
        settings.ocean_chop = 0;
        settings.ocean_ripples = 0;
        CHECK(draw(false) != offshore);
        settings.ocean_chop = saved.ocean_chop;
        settings.ocean_ripples = saved.ocean_ripples;
        CHECK(draw(false) == offshore);
    }
    enhanced::set_water_debug_settings(saved);
    effects.release();
}

int main(int argc, char** argv) {
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    const std::string_view mode = argc == 2 ? argv[1] : "";
    if (mode == "--ocean-only" || mode == "--ocean-spectrum") {
        if (mode == "--ocean-spectrum") {
            check_ocean_fft(false);
            check_ocean_fft(true);
        }
        renderer::Renderer renderer;
        scene::LevelScene scene;
        lighting::use_scene_lighting(scene);
        CHECK(test::shadow_probe_fixture(scene, error));
        scene.set_probe_lighting(true, 1);
        check_ocean_material(renderer, scene, mode == "--ocean-spectrum");
#ifdef OPENRAC_VULKAN
        CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
        scene.release();
        renderer.release();
        return test::result();
    }
    std::vector<std::vector<std::uint8_t>> variant_reference;
    std::size_t compared_variants = 0;
    for (const bool variants : {false, true}) {
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
        // Effects need the lighting metadata, but never draw this fixture's scene meshes.
        lighting::scene_lighting(scene)->load(level);
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
        // Consecutive quads can share a draw when the renderer ignores their register differences.
        quads[8].tex0 = quads[7].tex0;
        quads[8].tex1 = quads[7].tex1 | (1ull << 32);
        quads[8].clamp = quads[7].clamp | (1ull << 16);
        quads[8].alpha = quads[7].alpha | (0x70ull << 32);
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
            // Authored fog must survive shader specialisation and per-strip program switches.
            for (auto& strip : strips) {
                strip.fogged = frame == 2 || frame == 3 || frame == 4;
                strip.fog = {-.01f, 200, 0, 255};
                strip.fog_colour = 0x00a08050;
            }
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
                // Shader variants retain identical output under shared snapshot/draw recording.

                glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);
                glDisable(GL_SCISSOR_TEST);
                glClearColor(.04f, .07f, .11f, .3f);
                glClearDepth(0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                state.stats = {};
                // One path retains effects through a nested temporal consumer after preparation returns.
                if (path == 15) {
                    draw_joined_effects(effects, input, state, target);
                } else {
                    effects.render(input, state);
                }
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
                    draw_joined_effects(effects, input, state, target);
                    images[colour] = target.read_rgba();
                    // Repeating the same input preserves simulation state while comparing MSAA refraction scheduling.

                    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glDepthMask(GL_TRUE);
                    glDisable(GL_SCISSOR_TEST);
                    glClearColor(colour == 0 ? .6f : .05f, .1f, colour == 0 ? .05f : .6f, .3f);
                    glClearDepth(screen_depth);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    effects.render(input, state);
                    CHECK(images[colour] == target.read_rgba());

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
        // Authored point/spot lights must reach the real water material, even over cleared sky depth.
        CHECK(target.create(64, 64, error));
        state.target = {target.id(), 64, 64};
        auto saved_water_debug = enhanced::water_debug_settings();
        auto paused_water_debug = saved_water_debug;
        paused_water_debug.paused = true;
        enhanced::set_water_debug_settings(paused_water_debug);
        auto* provider = lighting::scene_lighting(scene);
        const auto draw_water = [&] {
            glBindFramebuffer(GL_FRAMEBUFFER, target.id());
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glDisable(GL_SCISSOR_TEST);
            glClearColor(.05f, .05f, .05f, 1);
            glClearDepth(0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            effects.render(input, state);
            return target.read_rgba();
        };
        provider->prepare_deformation(input, scene);
        const auto unlit_water = draw_water();
        std::vector<enhanced::LocalLight> authored(64);
        for (std::size_t i = 0; i < authored.size(); ++i) {
            auto& light = authored[i];
            light.position = {.4f, .4f, 2};
            light.range = 4;
            light.colour = {.01f, .003f, .001f};
            light.spot = i % 2 != 0;
        }
        CHECK(provider->set_local_lights(authored));
        ++input.frame;
        provider->prepare_deformation(input, scene);
        provider->set_local_light_reference(true);
        const auto all_light_water = draw_water();
        CHECK(all_light_water != unlit_water);
        provider->set_local_light_reference(false);
        provider->select_visible_regions(target.depth_texture(), input.camera, 64, 64);
        CHECK(draw_water() == all_light_water);
        CHECK(provider->set_local_lights({}));
        ++input.frame;
        provider->prepare_deformation(input, scene);
        CHECK(draw_water() == unlit_water);
        enhanced::set_water_debug_settings(saved_water_debug);
        CHECK(target.create(79, 53, error));
        state.target = {target.id(), 79, 53};
        glClearDepth(0);
        for (const int path : {0, 1, 2}) {
            const bool classic = path == 2;
            renderer::EffectRenderer fallback(
                "effect fallback",
                renderer::EffectPart::Rest,
                renderer::Bucket::Particles,
                classic ? nullptr : std::make_unique<FallbackMaterial>(path == 1, variants)
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
                const auto before_draws = state.stats.draw_calls;
#ifdef OPENRAC_VULKAN
                const auto before_fallback = vkgl::submission_counters().surface_chains;
#endif
                fallback.render(input, state);
                CHECK(
                    state.stats.draw_calls - before_draws
                    == quads.size() - (gl::is_vulkan() ? 1 : 0)
                );
#ifdef OPENRAC_VULKAN
                if (!classic) {
                    CHECK(vkgl::submission_counters().surface_chains == before_fallback);
                }
#endif
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
