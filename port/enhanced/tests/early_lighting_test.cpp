// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/viewer/early_lighting.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"
#include "tests/check.h"
#include "tests/graph_fixture.h"
#include "tests/renderer_gl_context.h"
using namespace openrac;
using namespace openrac::gl;

namespace {
constexpr auto vertex = R"glsl(#version 410 core
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);
}
)glsl";
constexpr auto depth_fragment = R"glsl(#version 410 core
uniform mat4 projection;
uniform int fixture;
out vec4 colour;
void main() {
    float x = gl_FragCoord.x;
    float distance = 8.0;
    // Parallel surfaces separated by a thin wall, a large step, and a
    // one-pixel foreground strip. Covered pixels should favour their own surface.
    if (fixture == 1 && x < 18.0) {
        distance = 7.95;
    }
    if (fixture == 2 && x < 18.0) {
        distance = 4.0;
    }
    if (fixture == 3 && int(x) == 18) {
        distance = 4.0;
    }
    if (fixture == 4) {
        discard;
    }
    vec4 clip = projection * vec4(0, 0, -distance, 1);
    gl_FragDepth = clip.z / clip.w * 0.5 + 0.5;
    vec3 normal = vec3(0, 0, 1);
    if (fixture == 5 || fixture == 6) {
        // Tilt only the shading normals; the depth remains a flat plane.
        normal = normalize(vec3((x / 37.0 - 0.5) * 1.2, 0, 1));
        if (fixture == 6) normal = -normal;
    }
    colour = fixture == 7 ? vec4(0) : vec4(normal * 0.5 + 0.5, 1);
}
)glsl";
constexpr auto fragment = R"glsl(#version 410 core
uniform int translucent_pass;
uniform int effect_pass;
uniform int use_cache;
uniform int show_hits;
uniform int show_hero;
uniform int show_cloud_edge;
uniform int empty_guides;
uniform sampler2D source_depth;
uniform mat4 view;
out vec4 colour;
)glsl";
constexpr auto fragment_main = R"glsl(
void main() {
    if (show_hits == 2) {
        colour = vec4(float(early_scale) * 0.25);
        return;
    }
    if (show_cloud_edge != 0) {
        float visibility = clamp((gl_FragCoord.x - 0.5) * 0.25, 0.0, 1.0);
        vec3 cloud_loss = vec3(1.0, 0.75, 0.0);
        colour = vec4(probe_shadowed_directional(vec3(1), cloud_loss, visibility, 0.1), 1.0);
        return;
    }
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(source_depth, pixel, 0).r;
    if (depth <= 0.0) {
        if (empty_guides == 0) {
            colour = vec4(0);
            return;
        }
        // Draw a receiver after the depth prepass to exercise genuinely empty guides.
        vec4 clip = early_projection * vec4(0, 0, -8, 1);
        depth = clip.z / clip.w * 0.5 + 0.5;
    }
    float distance = early_projection[3][2] / (depth * 2.0 + (early_projection[2][2] - 1.0));
    vec2 ndc = gl_FragCoord.xy / early_extent * 2.0 - 1.0;
    vec3 eye =
        vec3(
            (ndc + early_projection[2].xy) / vec2(early_projection[0][0], early_projection[1][1]),
            -1
        )
        * distance;
    mat3 inverse_view = transpose(mat3(view));
    vec3 normal_view = empty_guides != 0 ? vec3(0, 0, 1) : smooth_normal_at(pixel, -eye);
    vec3 world = inverse_view * (eye - view[3].xyz), normal = inverse_view * normal_view;
    vec4 light = vec4(0);
    float hero_visibility = 1.0;
    float cloudy_sun = 1.0;
    bool hit = use_cache != 0
        && early_surface(eye, normal_view, light, hero_visibility, cloudy_sun);
    bool covered = hit;
    if (show_hits != 0) {
        colour = vec4(hit ? 1 : 0, covered ? 1 : 0, 0, 1);
        return;
    }
    if (!hit) {
        covered = sample_probe_irradiance(
            world, normal, probe_surface_bias(normal, inverse_view * (-eye)), light.rgb
        );
        light.a = sun_shadow_visibility(world, normal, normal);
        hero_visibility = hero_shadow_visibility(world, normal);
    }
    if (hit && !hero_shadow_applies(normal)) {
        hero_visibility = 1.0;
    }
    if (empty_guides != 0 && !hit) {
        cloud_sun_attenuation = cloud_shadow_visibility(world, light.a);
        float visibility = min(light.a, hero_visibility);
        colour = vec4(vec3(probe_fallback_diffuse_shadow(visibility)), light.a * cloud_sun_attenuation);
        return;
    }
    if (cloud_shadow_shape.w > 0.0) {
        light.a = hit ? cloudy_sun : light.a * cloud_shadow_visibility(world, light.a);
    }
    colour =
        show_hero != 0 ? vec4(hero_visibility) : vec4(covered ? light.rgb / 4.0 : vec3(0), light.a);
}
)glsl";

// Gives the fixture direction-dependent irradiance so flat reconstructed normals cannot pass.
void set_directional_irradiance(lighting::ProbeVolume& volume) {
    const auto& grid = volume.grid;
    for (std::size_t i = 0; i < volume.probes.size(); ++i) {
        // Directional irradiance exposes a regression to depth-derived flat normals.
        for (unsigned y = 0; y < grid.map_resolution; ++y) {
            for (unsigned x = 0; x < grid.map_resolution; ++x) {
                const auto n = lighting::oct_decode(
                    {(static_cast<float>(x) + .5f) / static_cast<float>(grid.map_resolution),
                     (static_cast<float>(y) + .5f) / static_cast<float>(grid.map_resolution)}
                );
                const float factor = .4f + .6f * std::max(0.0f, (n[0] + n[2]) * .70710678f);
                const auto base =
                    i / 9 < 2 ? lighting::Float3{.25f, 1, 3} : lighting::Float3{3, 1, .25f};
                volume.irradiance
                    [i * grid.texels_per_probe()
                     + (static_cast<std::size_t>(y) + 1) * (grid.map_resolution + 2) + x + 1] =
                    {base[0] * factor, base[1] * factor, base[2] * factor};
            }
        }
    }
}

// Compares cached and reference lighting across receiver edges, smooth normals, AO and shadow states.
void check() {
    std::string error;
    renderer::Shader fill, sample;
    CHECK(fill.build("early lighting fixture depth", vertex, depth_fragment, error));
    std::string source = lighting::probe_shader_source(fragment);
    source.insert(source.find('\n') + 1, renderer::shaders::surface_normal_glsl);
    source.insert(source.find('\n') + 1, lighting::shaders::cloud_shadows_glsl);
    source += lighting::shaders::early_lighting_glsl;
    source += fragment_main;
    CHECK(sample.build("early lighting fixture sample", vertex, source, error));
    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    lighting::ProbeLighting probes;
    lighting::ProbeGrid grid;
    grid.origin = {-16, -16, -.025f};
    grid.spacing = {32, 32, .05f};
    grid.dimensions = {3, 3, 4};
    auto volume = lighting::ProbeVolume::allocate(grid);
    for (auto& p : volume.probes) {
        p.valid = true;
    }
    set_directional_irradiance(volume);
    lighting::update_probe_gutters(volume);
    for (auto& m : volume.distance_moments) {
        m = {1024, 1024 * 1024};
    }
    CHECK(probes.upload(openrac::test::gpu_probes(volume), error));
    probes.normal_bias = probes.view_bias = 0;
    renderer::FrameBuffer depth, output, shadow;
    CHECK(shadow.create(4, 4, error, 1, true));
    unsigned ao_texture = 0, ao_buffer = 0;
    glGenTextures(1, &ao_texture);
    glActiveTexture(GL_TEXTURE0 + 18);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, ao_texture);
    glGenBuffers(1, &ao_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ao_buffer);
    const std::vector<unsigned> ao_samples(19 * 12, 0x80800080u);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        ao_samples.size() * sizeof(unsigned),
        ao_samples.data(),
        GL_DYNAMIC_DRAW
    );
    glTexBuffer(renderer::world::gl::GL_TEXTURE_BUFFER, GL_RGBA8, ao_buffer);
    glActiveTexture(GL_TEXTURE0);
    float ao_strength = 0;
    int hero_mode = 0;
    lighting::EarlyLighting early;
    renderer::TexturePool textures;
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    renderer::FrameInput input;
    input.early_lighting_quality = renderer::EarlyLightingQuality::High;
    input.camera.position = {0, 0, 8};
    input.camera.view = renderer::look_along(input.camera.position, {0, 0, -1}, {0, 1, 0});
    float cloud_time = 0.f;
    const auto bind = [&](const renderer::Shader& shader, bool shadows) {
        probes.bind(shader, scene::Layer::Terrain, true, &input.camera);
        glUniform4f(
            shader.uniform("cloud_shadow_shape"),
            input.cloud_shadows.minimum,
            input.cloud_shadows.maximum,
            input.cloud_shadows.scale,
            input.cloud_shadows.effective_strength()
        );
        glUniform3f(
            shader.uniform("cloud_shadow_motion"),
            cloud_time * input.cloud_shadows.speed,
            0.f,
            cloud_time * input.cloud_shadows.undulation
        );
        glUniform3f(shader.uniform("cloud_shadow_to_sun"), 0, 0, 1);
        glUniform1i(shader.uniform("early_ao_enabled"), ao_strength > 0 ? 1 : 0);
        glUniform1i(shader.uniform("early_ao"), 18);
        glUniform1i(shader.uniform("early_ao_scale"), 2);
        glUniform1f(shader.uniform("early_ao_strength"), ao_strength);
        glActiveTexture(GL_TEXTURE0 + 18);
        glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, ao_texture);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(shader.uniform("hero_shadow_enabled"), hero_mode != 0 ? 1 : 0);
        glUniform1i(shader.uniform("hero_shadow_airborne"), hero_mode == 2 ? 1 : 0);
        glUniform1i(shader.uniform("hero_shadow_receiver"), 0);
        glUniform1i(shader.uniform("hero_shadow_map"), 6);
        const auto hero_matrix = renderer::scale(.01f);
        glUniformMatrix4fv(shader.uniform("hero_shadow_matrix"), 1, GL_FALSE, hero_matrix.data());
        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(GL_TEXTURE_2D, shadow.depth_texture());
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(shader.uniform("sun_shadow_enabled"), shadows ? 1 : 0);
        glUniform1i(
            shader.uniform("sun_shadow_fast"), input.shadow_filter == renderer::ShadowFilter::Fast
        );
        glUniformMatrix4fv(
            shader.uniform("sun_shadow_view"), 1, GL_FALSE, input.camera.view.data()
        );
        glUniform4f(shader.uniform("sun_shadow_ends"), 16, 32, 64, 128);
        glUniform1f(shader.uniform("sun_shadow_near"), .1f);
        auto matrix = renderer::scale(.01f);
        glUniformMatrix4fv(shader.uniform("sun_shadow_matrix[0]"), 1, GL_FALSE, matrix.data());
        glUniform1i(shader.uniform("sun_shadow_map0"), 7);
        glActiveTexture(GL_TEXTURE0 + 7);
        glBindTexture(GL_TEXTURE_2D, shadow.depth_texture());
        glActiveTexture(GL_TEXTURE0);
    };
    for (const auto size : {std::array{37, 23}, std::array{19, 17}}) {
        const auto [width, height] = size;
        input.camera.projection = renderer::perspective_reversed(
            std::numbers::pi_v<float> / 2, float(width) / height, .1f, 128
        );
        CHECK(depth.create(width, height, error, 1, true));
        CHECK(output.create(width, height, error));
        renderer::RenderState state{textures, {output.id(), width, height}, {}};
        state.target.normal_texture = depth.colour_texture();
        for (int fixture = 0; fixture < 8; ++fixture) {
            for (bool shadows : {false, true}) {
                // Tilted fixtures isolate GI; the original fixtures retain shadow-edge equivalence checks.
                if (fixture >= 5 && shadows) {
                    continue;
                }
                ++input.frame;
                glBindFramebuffer(GL_FRAMEBUFFER, shadow.id());
                glDepthMask(GL_TRUE);
                glClearDepth(.75);
                glClear(GL_DEPTH_BUFFER_BIT);
                glBindFramebuffer(GL_FRAMEBUFFER, depth.id());
                glViewport(0, 0, width, height);
                glDepthMask(GL_TRUE);
                glClearDepth(0);
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_ALWAYS);
                glDisable(GL_BLEND);
                glDisable(GL_CULL_FACE);
                fill.use();
                glUniformMatrix4fv(
                    fill.uniform("projection"), 1, GL_FALSE, input.camera.projection.data()
                );
                glUniform1i(fill.uniform("fixture"), fixture);
                openrac::test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
                const auto render_cache = [&] {
                    CHECK(early.render(
                        depth.depth_texture(),
                        input,
                        state.target,
                        [&](const auto& shader) { bind(shader, shadows); },
                        error
                    ));
                };
                render_cache();
                state.early_lighting = true;
                const auto draw = [&](bool cache,
                                      int hits,
                                      int translucent = 0,
                                      bool hero_view = false,
                                      bool self = false,
                                      bool cloud_edge = false) {
                    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
                    glViewport(0, 0, width, height);
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    sample.use();
                    bind(sample, shadows);
                    early.bind(sample, input, state);
                    glUniform1i(sample.uniform("use_cache"), cache ? 1 : 0);
                    glUniform1i(sample.uniform("show_hits"), hits);
                    glUniform1i(sample.uniform("show_hero"), hero_view ? 1 : 0);
                    glUniform1i(sample.uniform("show_cloud_edge"), cloud_edge ? 1 : 0);
                    glUniform1i(sample.uniform("empty_guides"), fixture == 4 ? 1 : 0);
                    glUniform1i(sample.uniform("hero_shadow_receiver"), self ? 1 : 0);
                    glUniform1i(sample.uniform("translucent_pass"), translucent);
                    glUniform1i(sample.uniform("effect_pass"), 0);
                    glUniformMatrix4fv(
                        sample.uniform("view"), 1, GL_FALSE, input.camera.view.data()
                    );
                    glUniformMatrix4fv(
                        sample.uniform("early_projection"),
                        1,
                        GL_FALSE,
                        input.camera.projection.data()
                    );
                    glUniform2f(sample.uniform("early_extent"), float(width), float(height));
                    glUniform1i(sample.uniform("source_depth"), 0);
                    glUniform1i(sample.uniform("surface_normals"), 19);
                    glUniform1i(sample.uniform("surface_normals_enabled"), 1);
                    glActiveTexture(GL_TEXTURE0 + 19);
                    glBindTexture(GL_TEXTURE_2D, depth.colour_texture());
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, depth.depth_texture());
                    glBindVertexArray(vao);
                    openrac::test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
                    return output.read_rgba();
                };
                const auto reference = draw(false, false), actual = draw(true, false);
                // Identical cache bytes must survive repeated graph recording at every fixture and size.

                render_cache();
                CHECK(draw(true, false) == actual);

                unsigned differences = 0;
                for (std::size_t i = 0; fixture != 4 && i < actual.size(); ++i) {
                    const auto x = i / 4 % width;
                    const bool uncovered = (fixture == 2 && x < 18) || (fixture == 3 && x == 18);
                    if (uncovered && i % 4 != 3) {
                        // A correct cache deliberately supplies ambient where full probe sampling
                        // returns no irradiance. Keep the shadow comparison below.
                        CHECK(reference[i] == 0 && actual[i] > 0);
                    } else if (std::abs(int(actual[i]) - int(reference[i]))
                               > (i % 4 == 3 ? 1 : 6)) {
                        if (differences < 4) {
                            std::fprintf(
                                stderr,
                                "pixel %zu,%zu ch%zu: %u vs %u\n",
                                i / 4 % width,
                                i / 4 / width,
                                i % 4,
                                unsigned(actual[i]),
                                unsigned(reference[i])
                            );
                        }
                        ++differences;
                    }
                }
                if (differences) {
                    std::fprintf(
                        stderr,
                        "fixture %d, shadows %d, %dx%d: %u mismatched channels\n",
                        fixture,
                        int(shadows),
                        width,
                        height,
                        differences
                    );
                }
                CHECK(differences == 0);
                if (fixture == 0) {
                    const auto draw_cached = [&] {
                        render_cache();
                        return draw(true, false);
                    };

                    // Fixed noise remapping gives 55% sunlight at 90% cloud strength.
                    // Exercise the extra guide plane and both directions of live resizing.
                    input.cloud_shadows.enabled = true;
                    input.cloud_shadows.minimum = input.cloud_shadows.maximum = .5f;
                    const auto cloudy = draw_cached();
                    for (std::size_t i = 0; i < actual.size(); ++i) {
                        const float expected = actual[i] * (i % 4 == 3 ? .55f : 1.f);
                        CHECK(std::abs(float(cloudy[i]) - expected) <= 1);
                    }
                    if (shadows) {
                        // Cover half the shadow map to produce fractional visibility under PCF.
                        glBindFramebuffer(GL_FRAMEBUFFER, shadow.id());
                        glDepthMask(GL_TRUE);
                        glClearDepth(1.0);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        glEnable(GL_SCISSOR_TEST);
                        glScissor(0, 0, 2, 4);
                        glClearDepth(.25);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        glDisable(GL_SCISSOR_TEST);
                        const auto alpha = std::size_t(height - 1) * width * 4 + 3;
                        int soft_visibility = 0;
                        for (auto filter :
                             {renderer::ShadowFilter::Soft, renderer::ShadowFilter::Fast}) {
                            input.shadow_filter = filter;
                            input.cloud_shadows.enabled = true;
                            const auto penumbra = draw_cached();
                            const auto full_penumbra = draw(false, false);
                            input.cloud_shadows.enabled = false;
                            const int unclouded = draw(false, false)[alpha];
                            CHECK(unclouded > 0 && unclouded < 255);
                            CHECK(std::abs(float(full_penumbra[alpha]) - .55f * unclouded) <= 1);
                            CHECK(std::abs(int(penumbra[alpha]) - int(full_penumbra[alpha])) <= 1);
                            if (filter == renderer::ShadowFilter::Soft) {
                                soft_visibility = unclouded;
                            } else {
                                CHECK(std::abs(unclouded - soft_visibility) > 1);
                            }
                        }
                        input.shadow_filter = renderer::ShadowFilter::Soft;
                        glBindFramebuffer(GL_FRAMEBUFFER, shadow.id());
                        glDepthMask(GL_TRUE);
                        glClearDepth(.75);
                        glClear(GL_DEPTH_BUFFER_BIT);
                    }
                    if (!shadows) {
                        // Full and partial cloud cover both retain the cascade floor.
                        const auto edge = draw(false, false, 0, false, false, true);
                        for (std::size_t x = 0; x <= 4; ++x) {
                            const float visible = float(x) / 4.f;
                            CHECK(std::abs(float(edge[x * 4]) - 25.5f) <= 1);
                            CHECK(
                                std::abs(float(edge[x * 4 + 1]) - 255.f * (.1f + .225f * visible))
                                <= 1
                            );
                            CHECK(
                                std::abs(float(edge[x * 4 + 2]) - 255.f * (.1f + .9f * visible))
                                <= 1
                            );
                        }
                        input.cloud_shadows.minimum = 0.f;
                        input.cloud_shadows.maximum = 1.f;
                        const auto first_phase = draw_cached();
                        const auto first_full = draw(false, false);
                        ++input.frame;
                        cloud_time = 4.f;
                        const auto later_phase = draw_cached();
                        const auto later_full = draw(false, false);
                        CHECK(first_phase != later_phase);
                        CHECK(first_full != later_full);
                        // The bottom-left pixel reconstructs one cell at the exact position
                        // sampled by the fragment shader. Readback starts at the top.
                        const auto alpha = std::size_t(height - 1) * width * 4 + 3;
                        CHECK(std::abs(int(first_phase[alpha]) - int(first_full[alpha])) <= 1);
                        CHECK(std::abs(int(later_phase[alpha]) - int(later_full[alpha])) <= 1);
                        for (std::size_t i = 0; i < actual.size(); ++i) {
                            if (i % 4 != 3) {
                                CHECK(first_phase[i] == later_phase[i]);
                            }
                        }
                        cloud_time = 0.f;
                    }
                    input.cloud_shadows.enabled = false;
                    CHECK(draw_cached() == actual);
                    if (!shadows) {
                        CHECK(draw(true, 2)[0] == 128);
                        input.early_lighting_quality = renderer::EarlyLightingQuality::Normal;
                        const auto normal = draw_cached();
                        CHECK(draw(true, 2)[0] == 255);
                        const auto normal_hits = draw(true, true);
                        for (std::size_t i = 0; i < normal.size(); ++i) {
                            if (i % 4 == 0) {
                                CHECK(normal_hits[i] == 255);
                            }
                            const int tolerance = i % 4 == 3 ? 1 : 6;
                            CHECK(std::abs(int(normal[i]) - int(reference[i])) <= tolerance);
                        }
                        input.early_lighting_quality = renderer::EarlyLightingQuality::High;
                        CHECK(draw_cached() == actual);
                    }
                }
                if (fixture == 0) {
                    // AO attenuates only RGB, before encoding. Alpha keeps the
                    // same cascade visibility, including fully shadowed samples.
                    for (float strength : {1.0f, 2.0f}) {
                        ao_strength = strength;
                        render_cache();
                        const auto occluded = draw(true, false);
                        for (std::size_t i = 0; i < actual.size(); ++i) {
                            const float expected =
                                i % 4 == 3 ? float(actual[i]) : actual[i] * (1 - .5f * strength);
                            CHECK(std::abs(float(occluded[i]) - expected) <= 4);
                        }
                    }
                    ao_strength = 0;
                    render_cache();
                }
                if (fixture == 0) {
                    // Hero visibility stays separate from cascade visibility. Receiver
                    // exclusion is applied after reconstruction, not baked into the cache.
                    for (int hero = 1; hero <= 2; ++hero) {
                        hero_mode = hero;
                        render_cache();
                        for (bool self : {false, true}) {
                            const auto expected = draw(false, false, 0, true, self);
                            const auto cached = draw(true, false, 0, true, self);
                            for (std::size_t i = 0; i < cached.size(); ++i) {
                                CHECK(std::abs(int(cached[i]) - int(expected[i])) <= 1);
                            }
                            const auto centre = (height / 2 * width + width / 2) * 4;
                            CHECK(cached[centre] == (hero == 2 && self ? 255 : 0));
                        }
                    }
                    hero_mode = 0;
                    render_cache();
                }
                auto hits = draw(true, true);
                unsigned accepted = 0;
                for (std::size_t i = 0; i < hits.size(); i += 4) {
                    accepted += hits[i] > 0;
                }
                if (fixture == 0) {
                    CHECK(accepted > unsigned(width * height) * 8 / 10);
                }
                if (fixture == 4) {
                    // Even a fully empty 2x2 blends ambient and unoccluded shadows from the cache.
                    CHECK(accepted == unsigned(width * height));
                    input.cloud_shadows.enabled = true;
                    input.cloud_shadows.minimum = input.cloud_shadows.maximum = .5f;
                    render_cache();
                    const auto seeded = draw(true, false);
                    const auto hero = draw(true, false, 0, true);
                    for (std::size_t i = 0; i < seeded.size(); i += 4) {
                        CHECK(seeded[i] > 0 && seeded[i + 1] > 0 && seeded[i + 2] > 0);
                        CHECK(seeded[i + 3] == 255);
                        CHECK(hero[i] == 255);
                    }
                    input.cloud_shadows.enabled = false;
                    render_cache();
                }
                hits = draw(true, true, 1);
                for (std::size_t i = 0; i < hits.size(); i += 4) {
                    CHECK(hits[i] == 0);
                }
                // Same-size secondary cameras and stale frames cannot reuse it.
                input.camera.view[12] += .25f;
                hits = draw(true, true);
                for (std::size_t i = 0; i < hits.size(); i += 4) {
                    CHECK(hits[i] == 0);
                }
                input.camera.view[12] -= .25f;
                state.target.framebuffer = depth.id();
                hits = draw(true, true);
                for (std::size_t i = 0; i < hits.size(); i += 4) {
                    CHECK(hits[i] == 0);
                }
                state.target.framebuffer = output.id();
                ++input.frame;
                hits = draw(true, true);
                for (std::size_t i = 0; i < hits.size(); i += 4) {
                    CHECK(hits[i] == 0);
                }
            }
        }
    }
    early.release();
    probes.release();
    glDeleteTextures(1, &ao_texture);
    glDeleteBuffers(1, &ao_buffer);
    glDeleteVertexArrays(1, &vao);
}
}  // namespace

int main() {
    test::GlContext context;
    std::string why;
    if (!context.open(why)) {
        std::puts(why.c_str());
        return test::kSkip;
    }
    if (!lighting::EarlyLighting::supported()) {
        return test::kSkip;
    }
    check();
#if defined(OPENRAC_VULKAN)
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
