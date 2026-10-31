// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

#include "common/stats.h"
#include "enhanced/renderer/gtao.h"
#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_float.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/tests/prepass_capture.h"
#include "enhanced/tests/probe_fixture.h"
#include "enhanced/tests/water_test_bake.h"
#include "enhanced/viewer/early_lighting.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif
#include "scene/level_renderer.h"
#include "scene_shaders.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

using namespace openrac;

// Real controller + GPU lookup: native lily movement must retain history by object identity.
void check_moving_lily(scene::LevelScene& scene) {
    using namespace gl;
    enhanced::WaterFloat attachment;
    attachment.load();
    renderer::Shader read;
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(1, 1, error));
    CHECK(read.build(
        "moving lily momentum",
        enhanced::WaterFloat::vertex_source(R"(#version 410 core
uniform vec3 anchor;
uniform int raw_pose;
flat out float height;
void main() {
    bool matched;
    height=raw_pose!=0 ? water_float_target(anchor,matched).w
        : (water_float_pose(anchor)*vec4(anchor,1)).z;
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0,1);
})"),
        R"(#version 430 core
flat in float height;
layout(location=0) out vec4 colour;
void main(){uint b=floatBitsToUint(height);
colour=vec4(float(b&255u),float((b>>8)&255u),float((b>>16)&255u),float(b>>24))/255.;}
)",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    scene::Instance lily;
    lily.floats_on_water = true;
    lily.temporal_id = 7;
    lily.matrix[12] = lily.matrix[13] = .25f;
    lily.matrix[14] = 1;
    renderer::FrameInput input;
    input.frame = 20;
    input.enhanced_water = true;
    input.lily_momentum = 1;
    const auto sample = [&] {
        scene.set_instances(scene::Layer::Ties, {lily});
        attachment.prepare(input, &scene);
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, 1, 1);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBindVertexArray(vao);
        read.use();
        attachment.bind(read, true);
        glUniform3f(read.uniform("anchor"), lily.matrix[12], lily.matrix[13], lily.matrix[14]);
        std::array<float, 2> heights{};
        for (int raw = 0; raw < 2; ++raw) {
            glUniform1i(read.uniform("raw_pose"), raw);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            const auto bytes = target.read_rgba();
            const auto bits = std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8)
                              | (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
            heights[raw] = std::bit_cast<float>(bits);
        }
        return heights;
    };
    const auto initial = sample();
    CHECK(std::abs(initial[0] - initial[1]) < 2e-6f);
    ++input.frame;
    lily.matrix[12] += .01f;
    lily.matrix[14] += .2f;
    const auto moving = sample();
    const float expected = initial[0] + (moving[1] - initial[0]) * float(-std::expm1(-1.5 / 60.));
    CHECK(std::abs(moving[0] - expected) < 3e-6f);
    CHECK(std::abs(moving[0] - moving[1]) > .1f);
    // Replacing an object at the same position must seed a fresh pose, even in-place.
    ++input.frame;
    ++lily.temporal_id;
    const auto replaced = sample();
    CHECK(std::abs(replaced[0] - replaced[1]) < 2e-6f);
    // The slider's zero endpoint must change live, including a repeated game tick.
    input.lily_momentum = 0;
    lily.matrix[14] += .1f;
    const auto immediate = sample();
    CHECK(std::abs(immediate[0] - immediate[1]) < 2e-6f);
    glDeleteVertexArrays(1, &vao);
}

// --expect-off: switched off at start (OPENRAC_ENHANCED=0, the setting's default) or on OpenGL,
// nothing of enhanced rendering may be made, so the game draws as a build without it.
// --expect-build: switched on, Vulkan has it when the build compiled it in.
int main(int argc, char** argv) {
    const bool expect_off = argc > 1 && std::string_view(argv[1]) == "--expect-off";
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        return test::kSkip;
    }
#if defined(OPENRAC_ENHANCED_RENDERING) && OPENRAC_ENHANCED_RENDERING
    const bool expected = !expect_off && gl::is_vulkan();
#else
    const bool expected = false;
#endif
    CHECK(gl::enhanced_rendering_available() == expected);
    if (expect_off) {
        CHECK(!gl::enhanced_rendering_available());
    }
    // Live fur controls cannot opt the original renderer into enhanced shaders.
    {
        const auto saved = enhanced::fur_material_settings();
        enhanced::set_fur_material_settings({});
        enhanced::set_fur_debug_view(enhanced::FurDebug::Mask);
        lighting::SceneLighting lighting;
        const auto vertex = lighting.mesh_vertex(scene::shaders::mesh_vert);
        const auto fragment = lighting.mesh_fragment(scene::shaders::mesh_frag);
        const auto early = lighting.early_fragment(scene::shaders::mesh_frag);
        for (const auto* source : {&vertex, &fragment, &early}) {
            CHECK((source->find("#define OPENRAC_HERO_FUR 1") != std::string::npos) == expected);
        }
        enhanced::set_fur_debug_view(enhanced::FurDebug::Off);
        enhanced::set_fur_material_settings(saved);
    }
    if (!expected) {
        // Explicit API requests must not bypass the build/backend boundary.
        auto volume = lighting::ProbeVolume::allocate({});
        lighting::ProbeLighting probes;
        CHECK(!probes.upload(openrac::test::gpu_probes(volume), error));
        CHECK(!probes.active());
        CHECK(!probes.compute_selection());
        scene::LevelScene scene;
        lighting::use_scene_lighting(scene);
        CHECK(!scene.enable_sun_shadow(true, error));
        CHECK(!scene.enable_hero_shadow(true, error));
        CHECK(!scene.sun_shadow_enabled() && !scene.hero_shadow_enabled());
        CHECK(!lighting::EarlyLighting::supported());
        renderer::Gtao ao;
        CHECK(!ao.render({}, {}));
        CHECK(ao.visibility_texture() == 0 && ao.depth_texture() == 0);
        // No atlas/shadow source is injected into the compatibility shader.
        const auto fragment = lighting::probe_mesh_fragment(scene::shaders::mesh_frag);
        CHECK(fragment.find("uniform sampler2D probe_irradiance") == std::string::npos);
        CHECK(fragment.find("uniform sampler2D sun_shadow_map0") == std::string::npos);
    }
    if (expected) {
        lighting::ProbeLighting probes;
        CHECK(
            !probes.upload(lighting::ProbeVolume::allocate({}), error)
        );  // No runtime conversion fallback.
        renderer::Renderer renderer;
        renderer.set_ambient_occlusion(std::make_unique<renderer::Gtao>());
        scene::LevelScene scene;
        lighting::use_scene_lighting(scene);
        scene::LevelData level;
        for (const auto& p :
             {renderer::Vec3{-1, -1, 0}, renderer::Vec3{1, -1, 0}, renderer::Vec3{0, 1, 0}}) {
            scene::Vertex vertex{};
            std::copy(p.begin(), p.end(), vertex.position);
            vertex.normal[2] = 1;
            std::fill(std::begin(vertex.colour), std::end(vertex.colour), 1.0f);
            level.vertices.push_back(vertex);
        }
        // Distinct normal/UV vertices retain the original shadow triangle order.
        for (unsigned i = 0; i < 3; ++i) {
            auto vertex = level.vertices[i];
            vertex.normal[0] = .5f;
            vertex.uv[0] = 2;
            level.vertices.push_back(vertex);
        }
        level.indices = {0, 1, 2, 3, 4, 5};
        // A single terrain instance spans visible geometry and hundreds of off-map triangles.
        // Instance culling alone cannot reject these; chunk culling must preserve full depth.
        for (unsigned triangle = 0; triangle < 512; ++triangle) {
            for (unsigned corner = 0; corner < 3; ++corner) {
                auto vertex = level.vertices[corner];
                vertex.position[0] += triangle >= 256 && triangle < 384
                                          ? .001f * float(triangle - 256)
                                          : 1000.f + float(triangle);
                level.indices.push_back(static_cast<std::uint32_t>(level.vertices.size()));
                level.vertices.push_back(vertex);
            }
        }
        scene::Model model;
        model.min = {-1, -1, 0};
        model.max = {1512, 1, 0};
        model.primitives.push_back({0, static_cast<std::uint32_t>(level.indices.size()), 0});
        level.models.push_back(model);
        level.materials.push_back({});
        // A rigid alpha-tested model has separate visible and off-map primitive ranges.
        auto alpha_model = model;
        alpha_model.primitives =
            {{0, 6, 1}, {6, 256 * 3, 1}, {258 * 3, 128 * 3, 1}, {386 * 3, 128 * 3, 1}};
        level.models.push_back(alpha_model);
        scene::Material alpha_material;
        alpha_material.cutout = true;
        level.materials.push_back(alpha_material);
        level.instances[static_cast<std::size_t>(scene::Layer::Terrain)].push_back({});
        const bool uploaded = scene.upload(level, renderer.textures(), error);
        if (!uploaded) {
            std::fprintf(stderr, "%s\n", error.c_str());
        }
        CHECK(uploaded);
        CHECK(!scene.probe_data_available());
        {
            // A level without a bake under the player's probe folder keeps the original lighting.
            lighting::set_probe_root(std::filesystem::temp_directory_path() / "openrac-no-probes");
            level.dir = "level_05";
            lighting::ProbeLighting unbaked;
            unbaked.load(level);
            CHECK(!unbaked.has_data() && !unbaked.active());
            lighting::set_probe_root({});
        }
        CHECK(!scene.enable_sun_shadow(true, error));
        CHECK(!scene.enable_hero_shadow(true, error));
        CHECK(
            renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Sky), error)
        );
        CHECK(renderer.add(
            std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Terrain), error
        ));
        CHECK(
            renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Mobys), error)
        );
        CHECK(renderer.init(error));
        renderer::FrameBuffer target;
        CHECK(target.create(32, 32, error));
        renderer::FrameInput frame;
        frame.camera.position = {0, 0, 4};
        frame.camera.view = renderer::look_along(frame.camera.position, {0, 0, -1}, {0, 1, 0});
        frame.camera.projection = renderer::perspective_reversed(1, 1, .1f, 100);
        const renderer::RenderTarget rt{target.id(), 32, 32};
        renderer.render(frame, rt);
        const auto plain = target.read_rgba();
        const auto draws = renderer.last_stats().draw_calls;
        frame.gtao = true;
        renderer.render(frame, rt);
        CHECK(target.read_rgba() == plain);
        CHECK(renderer.last_stats().draw_calls == draws);  // No AO passes without probe data.

        CHECK(test::shadow_probe_fixture(scene, error));
        CHECK(scene.probe_data_available());
        CHECK(scene.enable_sun_shadow(true, error));
        CHECK(!scene.needs_early_lighting(frame));
        frame.sun_pixel_shadow = true;
        CHECK(scene.needs_early_lighting(frame));
        frame.sun_pixel_shadow = false;
        scene.set_probe_lighting(false, 1);
        CHECK(!scene.needs_early_lighting(frame));  // Allocated maps do not request a pass.
        CHECK(scene.enable_hero_shadow(true, error));
        frame.hero_pixel_shadow = true;
        CHECK(scene.needs_early_lighting(frame));  // Hero-only shadows still use compute.
        frame.hero_pixel_shadow = false;
        CHECK(!scene.needs_early_lighting(frame));
        // Sky atmosphere and early lighting consume identical current-frame shadows.
        // Exercise enabled/off atmosphere, camera changes and map resolution changes.
        scene.set_probe_lighting(true, 1);
        scene.set_probe_view(1);
        frame.sun_pixel_shadow = true;
        frame.hero_pixel_shadow = true;
        stats::set_enabled(stats::Client::Dump, true);
        stats::begin_frame(stats::FrameKind::Tick, 0);
        for (unsigned tick = 0; tick < 6; ++tick) {
            frame.seconds = 20 + tick / 60.;
            scene::Instance hero;
            hero.hero_shadow_anchor = hero.hero_shadow_caster = true;
            hero.matrix[12] = float(tick) * .15f;
            hero.matrix[14] = .5f;
            scene.set_instances(scene::Layer::Mobys, {hero});
            frame.hero_airborne = tick == 1 || tick == 2;
            frame.camera.position[0] = float(tick) * .1f;
            frame.camera.view = renderer::look_along(frame.camera.position, {0, 0, -1}, {0, 1, 0});
            frame.volumetric_quality =
                tick % 2 ? renderer::VolumetricQuality::Off : renderer::VolumetricQuality::Medium;
            frame.shadow_quality =
                tick < 2 ? renderer::ShadowQuality::Low : renderer::ShadowQuality::Medium;
            std::vector<std::uint8_t> reference;
            std::uint64_t reference_draws = 0;
            for (const bool reuse : {false, true}) {
                renderer::render_opt::set(renderer::render_opt::Opt::ShadowFrameReuse, reuse);
                renderer.render(frame, rt);
                const auto pixels = target.read_rgba();
                stats::begin_frame(stats::FrameKind::Tick, tick + 1);
                const auto* measured = stats::history(0);
                CHECK(measured != nullptr);
                if (measured) {
                    const auto maps = std::count_if(
                        measured->sections.begin(),
                        measured->sections.end(),
                        [](const auto& section) { return section.name == "sun0 map"; }
                    );
                    CHECK(maps == (!reuse && tick % 2 == 0 ? 2 : 1));
                }
                if (!reuse) {
                    reference = pixels;
                    reference_draws = renderer.last_stats().draw_calls;
                } else {
                    CHECK(pixels == reference);
                    if (tick % 2 == 0) {
                        CHECK(renderer.last_stats().draw_calls < reference_draws);
                    } else {
                        CHECK(renderer.last_stats().draw_calls == reference_draws);
                    }
                }
            }
        }
        {
            // Mixed dry/wet buckets must keep real wave deformation on floating casters.
            test::WaterTestBake fixture;
            auto water = enhanced::make_water(scene, fixture.folder);
            CHECK(bool(water));
            CHECK(water && water->init(error));
            check_moving_lily(scene);
            frame.enhanced_water = true;
            frame.water_interactor = renderer::Vec3{.5f, .5f, 1};
            frame.camera.position = {.5f, .5f, 4};
            frame.camera.view = renderer::look_along(frame.camera.position, {0, 0, -1}, {0, 1, 0});
            frame.volumetric_quality = renderer::VolumetricQuality::Off;
            frame.shadow_quality = renderer::ShadowQuality::Low;
            scene::Instance lily;
            lily.floats_on_water = true;
            lily.matrix[0] = lily.matrix[5] = .2f;
            lily.matrix[12] = lily.matrix[13] = .25f;
            lily.matrix[14] = 1;
            auto dry = lily;
            dry.floats_on_water = false;
            dry.model = 1;
            dry.matrix[12] = .6f;
            auto floating_alpha = dry;
            floating_alpha.floats_on_water = true;
            floating_alpha.matrix[12] = .45f;
            scene.set_instances(scene::Layer::Ties, {lily, dry, floating_alpha});
            CHECK(renderer.add(
                std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Ties), error
            ));
            auto observer = std::make_unique<test::PrepassCapture>();
            auto* capture = observer.get();
            CHECK(renderer.add(std::move(observer), error));
            renderer::Shader read_depth;
            renderer::FrameBuffer depth_pixels;
            CHECK(depth_pixels.create(768, 768, error));
            CHECK(read_depth.build(
                "shadow depth bits",
                R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
                R"(#version 430 core
uniform sampler2D sun_shadow_map0;
layout(location=0) out vec4 result;
void main(){
    ivec2 p=ivec2(gl_FragCoord.xy);
    uint bits=any(greaterThanEqual(p,textureSize(sun_shadow_map0,0)))?0u:
        floatBitsToUint(texelFetch(sun_shadow_map0,p,0).r);
    result=vec4(float(bits&255u),float((bits>>8)&255u),float((bits>>16)&255u),float(bits>>24))/255.;
})",
                error
            ));
            unsigned read_vao = 0;
            gl::glGenVertexArrays(1, &read_vao);
            const auto shadow_bits = [&] {
                read_depth.use();
                scene.bind_surface_lighting(read_depth, frame.camera);
                gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, depth_pixels.id());
                gl::glViewport(0, 0, 768, 768);
                gl::glDisable(gl::GL_DEPTH_TEST);
                gl::glDisable(gl::GL_STENCIL_TEST);
                gl::glDisable(gl::GL_BLEND);
                gl::glDisable(gl::GL_SCISSOR_TEST);
                gl::glColorMask(gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
                gl::glBindVertexArray(read_vao);
                gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
                return depth_pixels.read_rgba();
            };
            // Two renders per simulation frame: first verify the existing terrain path,
            // then isolate the static alpha extension in a separate sequence of frames.
            for (unsigned update = 0; update < 12; ++update) {
                const unsigned tick = update % 4;
                const bool extended = update >= 4;
                const bool rigid_test = update >= 8;
                const unsigned baseline = rigid_test ? 7u : extended ? 3u : 0u;
                frame.frame = 100 + update;
                frame.seconds = 2 + update / 60.;
                scene::Instance ground;
                ground.matrix[0] = tick == 1 ? -1.f : 1.f;
                ground.matrix[5] = 1.f + .1f * float(tick);
                ground.matrix[12] = .1f * float(tick);
                // Non-affine transforms cannot use affine chunk bounds and must retain all faces.
                ground.matrix[3] = tick == 3 ? .0001f : 0.f;
                scene.set_instances(scene::Layer::Terrain, {ground});
                std::vector<std::uint8_t> reference, reference_depth, reference_normals,
                    reference_prepass_depth;
                dry.matrix[0] = tick == 1 ? -.2f : .2f;
                dry.matrix[12] = .6f + .02f * float(tick);
                dry.tint[3] = tick == 2 ? .4f : 1.f;
                scene.set_instances(scene::Layer::Ties, {lily, dry, floating_alpha});
                std::uint64_t triangles = 0;
                for (const unsigned path : {baseline, rigid_test ? 15u : extended ? 7u : 3u}) {
                    renderer::render_opt::set(
                        renderer::render_opt::Opt::ShadowDryShaders, (path & 1) != 0
                    );
                    renderer::render_opt::set(
                        renderer::render_opt::Opt::ShadowGeometryCull, (path & 2) != 0
                    );
                    renderer::render_opt::set(
                        renderer::render_opt::Opt::ShadowStaticGeometryCull, (path & 4) != 0
                    );
                    renderer::render_opt::set(
                        renderer::render_opt::Opt::PrepassRigid, (path & 8) != 0
                    );
                    renderer.render(frame, rt);
                    const auto pixels = target.read_rgba();
                    const auto depth = shadow_bits();
                    CHECK(enhanced::water_available() && scene.deformation_active());
                    if (path == baseline) {
                        reference = pixels;
                        reference_depth = depth;
                        reference_normals = capture->normals;
                        reference_prepass_depth = capture->depths;
                        CHECK(std::any_of(depth.begin(), depth.end(), [](auto byte) {
                            return byte != 0;
                        }));
                        triangles = renderer.last_stats().triangles;
                    } else {
                        CHECK(pixels == reference);
                        if (depth != reference_depth) {
                            std::fprintf(stderr, "shadow mismatch tick %u path %u\n", tick, path);
                        }
                        CHECK(depth == reference_depth);
                        CHECK(capture->normals == reference_normals);
                        CHECK(capture->depths == reference_prepass_depth);
                        CHECK(!capture->normals.empty());
                        CHECK(
                            rigid_test || (!extended && tick == 3)
                                ? renderer.last_stats().triangles == triangles
                                : renderer.last_stats().triangles < triangles
                        );
                        stats::begin_frame(stats::FrameKind::Tick, 200 + tick);
                        const auto* measured = stats::history(0);
                        CHECK(measured != nullptr);
                        if (measured) {
                            if (rigid_test) {
                                const auto used = std::find_if(
                                    measured->gauges.begin(),
                                    measured->gauges.end(),
                                    [](const auto& gauge) {
                                        return gauge.first == "prepass rigid draws ties";
                                    }
                                );
                                CHECK(used != measured->gauges.end() && used->second > 0);
                            }
                            const auto rejected = std::find_if(
                                measured->gauges.begin(),
                                measured->gauges.end(),
                                [](const auto& gauge) {
                                    return gauge.first == "shadow geometry rejected indices";
                                }
                            );
                            CHECK(rejected != measured->gauges.end());
                            if (rejected != measured->gauges.end()) {
                                CHECK(
                                    !extended && tick == 3 ? rejected->second == 0
                                                           : rejected->second > 0
                                );
                            }
#if defined(OPENRAC_VULKAN)
                            if (vkgl::gpu_driven_active() && tick != 3) {
                                const auto bulk = std::find_if(
                                    measured->gauges.begin(),
                                    measured->gauges.end(),
                                    [](const auto& gauge) {
                                        return gauge.first == "shadow opaque bulk ranges";
                                    }
                                );
                                CHECK(bulk != measured->gauges.end() && bulk->second > 1);
                            }
#endif
                        }
                    }
                }
            }
            gl::glDeleteVertexArrays(1, &read_vao);
            if (water) {
                water->release();
            }
        }
        renderer::render_opt::reset();
        stats::set_enabled(stats::Client::Dump, false);
#if defined(OPENRAC_VULKAN)
        CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
        scene.release();
        CHECK(!scene.probe_data_available());
        CHECK(!scene.enable_sun_shadow(true, error));
        renderer.release();
        target.release();
    }
    return test::result();
}
