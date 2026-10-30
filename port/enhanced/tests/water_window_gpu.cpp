// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_window.h"
#include "enhanced/tests/water_gpu_fixture.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/shader.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace openrac::test::water {
using namespace openrac::gl;
namespace wgl = renderer::world::gl;

void check_window(Context& fixture) {
    const auto* vertex = fixture.vertex;
    auto& error = fixture.error;
    auto& pixels = fixture.pixels;
    const auto texture = fixture.texture;
    auto& compute = fixture.compute;
    auto& read = fixture.read;
    auto& buffers = fixture.buffers;
    const auto& bodies = fixture.bodies;
    const auto image_buffer = fixture.image_buffer;
    const std::string solver = lighting::shaders::water_comp;
    const std::string material = lighting::shaders::water_glsl;
    // Probe the production modulo/old-window helpers directly. Negative world
    // cells retain the same physical slot; only the newly exposed column resets.
    std::string window_probe_source = solver.substr(0, solver.find("vec3 reference_state"));
    window_probe_source += R"(
uniform int probe_history;
void main(){int at=int(gl_GlobalInvocationID.x);if(at>=cell_count)return;
wave_pixels[at]=probe_history!=0?wave_at(at):vec4(wave_at(at).x,float(window_physical(window_world(at))),coast_at(at).y,coast_at(at).x);}
)";
    renderer::Shader window_probe;
    CHECK(window_probe.build_compute("water toroidal addressing", window_probe_source, error));
    auto local_bodies = bodies;
    local_bodies[0][0] = -4;
    local_bodies[0][1] = -4;
    std::vector<Cell> local_coast(kCount, Cell{513, 8, 0, 0}),
        local_hydraulics(kCount, Cell{.75f, 0, 0, 1});
    std::vector<Cell> local_state(kCount);
    for (unsigned i = 0; i < kCount; ++i) {
        local_state[i] = {float(i) * .0001f, 0, 0, .4f};
    }
    const auto upload = [&](unsigned buffer, const auto& values) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(values.size() * sizeof(Cell)), values.data()
        );
    };
    for (unsigned i = 0; i < kCount; ++i) {
        local_coast[i][1] = float(i);
    }
    upload(buffers[0], local_state);
    upload(buffers[2], local_coast);
    upload(buffers[3], local_bodies);
    upload(buffers[4], local_hydraulics);
    for (unsigned binding : {0u, 2u, 3u}) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffers[binding]);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, image_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buffers[4]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buffers[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, buffers[4]);
    std::array<unsigned, 2> local_metadata{};
    glGenBuffers(2, local_metadata.data());
    for (unsigned i = 0; i < 2; ++i) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, local_metadata[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, kCount * sizeof(Cell), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7 + i, local_metadata[i]);
    }
    renderer::Shader prepare_window;
    CHECK(prepare_window
              .build_compute("water local metadata", lighting::shaders::water_window_comp, error));
    prepare_window.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1]);
    glUniform1i(prepare_window.uniform("initialize_history"), 1);
    glUniform2f(prepare_window.uniform("previous_window"), -4, -4);
    glUniform1f(prepare_window.uniform("water_current_scale"), 1);
    glUniform4f(prepare_window.uniform("window_config"), -4, -4, kWidth, 0);
    glDispatchCompute((kCount + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    upload(buffers[0], local_state);  // Restore controlled history after metadata initialization.
    window_probe.use();
    glUniform1i(window_probe.uniform("local_window"), 1);
    glUniform1i(window_probe.uniform("cell_count"), kCount);
    glUniform4f(window_probe.uniform("window_config"), -4, -4, kWidth, 0);
    glUniform2f(window_probe.uniform("previous_window"), -5, -4);
    glDispatchCompute((kCount + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    const auto read_local = [&](int channel) {
        read.use();
        glUniform1i(read.uniform("channel"), channel);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
#ifdef OPENRAC_VULKAN
        CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, kWidth, kWidth));
#else
        glBindBuffer(wgl::GL_PIXEL_UNPACK_BUFFER, image_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kWidth, kWidth, GL_RGBA, GL_FLOAT, nullptr);
        glBindBuffer(wgl::GL_PIXEL_UNPACK_BUFFER, 0);
#endif
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    };
    const auto pixel_float = [&](unsigned i) {
        unsigned at = i * 4;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        return std::bit_cast<float>(bits);
    };
    read_local(0);
    for (unsigned i = 0; i < kCount; ++i) {
        const int wx = int(i % kWidth) - 4, wy = int(i / kWidth) - 4;
        const unsigned slot = unsigned((wy % int(kWidth) + int(kWidth)) % int(kWidth)) * kWidth
                              + unsigned((wx % int(kWidth) + int(kWidth)) % int(kWidth));
        const float expected = i % kWidth == kWidth - 1 ? 0.f : float(slot) * .0001f;
        CHECK(std::abs(pixel_float(i) - expected) < 1e-7);
    }
    read_local(1);
    for (unsigned i = 0; i < kCount; ++i) {
        const int wx = int(i % kWidth) - 4, wy = int(i / kWidth) - 4;
        const int slot = ((wy % int(kWidth) + int(kWidth)) % int(kWidth)) * int(kWidth)
                         + (wx % int(kWidth) + int(kWidth)) % int(kWidth);
        CHECK(pixel_float(i) == float(slot));
    }
    read_local(2);
    for (unsigned i = 0; i < kCount; ++i) {
        const int wx = int(i % kWidth) - 4, wy = int(i / kWidth) - 4;
        const int cx = int(std::floor(((float(wx) + .5f) * .25f + 4.f) / .5f));
        const int cy = int(std::floor(((float(wy) + .5f) * .25f + 4.f) / .5f));
        CHECK(pixel_float(i) == float(cy * int(kWidth) + cx));
    }
    // The actual finer-grid solver must preserve a uniform still-water level
    // across the physical wrap seam at the smaller stable timestep.
    // A closed, flat basin isolates conservation from the separately tested sponge.
    upload(local_metadata[0], local_coast);
    std::fill(local_state.begin(), local_state.end(), Cell{.03f, 0, 0, .4f});
    upload(buffers[0], local_state);
    compute.use();
    glUniform1i(compute.uniform("local_window"), 1);
    glUniform4f(compute.uniform("window_config"), -4, -4, kWidth, 0);
    glUniform2f(compute.uniform("previous_window"), -4, -4);
    glUniform1f(compute.uniform("water_dt"), 1.f / 240.f);
    glUniform1i(compute.uniform("reset_waves"), 0);
    glUniform1i(compute.uniform("transport_reference"), 0);
    glUniform4f(compute.uniform("hero"), 0, 0, 0, 0);
    glUniform1i(compute.uniform("hand_count"), 0);
    for (unsigned step = 0; step < 240; ++step) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[step % 2]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - step % 2]);
        glDispatchCompute((kCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }
    read_local(0);
    for (unsigned i = 0; i < kCount; ++i) {
        CHECK(std::abs(pixel_float(i) - .03f) < 2e-5);
    }

    CHECK(enhanced::water_window_same_height(60.5f, 61.f));
    CHECK(enhanced::water_window_same_height(61.f, 60.5f));
    CHECK(!enhanced::water_window_same_height(60.5f, 61.01f));
    CHECK(!enhanced::water_window_same_height(60.5f, 63.5f));
    // Adjacent compatible fields fill one simulation window, while an upper
    // pool remains dry in that window. Use the real metadata-preparation shader.
    std::array<Cell, 10> shared_bodies{};
    std::copy(local_bodies.begin(), local_bodies.end(), shared_bodies.begin());
    std::copy(local_bodies.begin(), local_bodies.end(), shared_bodies.begin() + 5);
    shared_bodies[1][3] = 8;  // primary spans x=-4..0
    shared_bodies[5][0] = 0;
    shared_bodies[8][3] = float(kCount);  // second spans x=0..8
    std::vector<Cell> shared_coast(std::size_t{kCount} * 2, Cell{513, 8, 0, 0});
    std::vector<Cell> shared_hydraulics(std::size_t{kCount} * 2, Cell{.75f, 0, 0, 1});
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[3]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, sizeof(shared_bodies), shared_bodies.data(), GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[4]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        shared_hydraulics.size() * sizeof(Cell),
        shared_hydraulics.data(),
        GL_DYNAMIC_DRAW
    );
    prepare_window.use();
    glUniform1i(prepare_window.uniform("initialize_history"), 0);
    window_probe.use();
    glUniform2f(window_probe.uniform("previous_window"), -4, -4);
    std::fill(local_state.begin(), local_state.end(), Cell{.07f, 0, 0, .4f});
    upload(buffers[0], local_state);
    upload(buffers[1], local_state);
    for (float second_height : {1.3f, 4.f}) {
        for (unsigned i = kCount; i < kCount * 2; ++i) {
            shared_coast[i] = {512.f + second_height, 8, 0, 1};
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            shared_coast.size() * sizeof(Cell),
            shared_coast.data(),
            GL_DYNAMIC_DRAW
        );
        prepare_window.use();
        glUniform1i(prepare_window.uniform("water_body_count"), 2);
        glUniform1f(prepare_window.uniform("water_sim_height"), 1.f);
        glDispatchCompute((kCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        window_probe.use();
        glDispatchCompute((kCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        read_local(3);
        for (unsigned i = 0; i < kCount; ++i) {
            const int wx = int(i % kWidth) - 4;
            float expected = 0.f;
            if (wx < 0) {
                expected = 513.f;
            } else if (second_height < 2.f) {
                expected = 512.f + second_height;
            }
            CHECK(std::abs(pixel_float(i) - expected) < 1e-4);
        }
        read_local(0);
        for (unsigned i = 0; i < kCount; ++i) {
            const int wx = int(i % kWidth) - 4;
            const float expected = wx < 0 || second_height < 2.f ? .07f : 0.f;
            CHECK(std::abs(pixel_float(i) - expected) < 2e-6);
        }
    }
    // A newly wet layer must initialize both states and its reference current.
    for (unsigned i = kCount; i < kCount * 2; ++i) {
        shared_hydraulics[i] = {.75f, .2f, -.1f, 1};
    }
    upload(buffers[4], shared_hydraulics);
    std::fill(local_state.begin(), local_state.end(), Cell{.09f, 0, 0, .4f});
    upload(buffers[0], local_state);
    upload(buffers[1], local_state);
    prepare_window.use();
    glUniform4f(prepare_window.uniform("window_config"), -4, -4, kWidth, 1);
    glUniform1f(prepare_window.uniform("water_sim_height"), 4);
    glDispatchCompute((kCount + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    for (unsigned state : {0u, 1u}) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[state]);
        window_probe.use();
        glUniform1i(window_probe.uniform("probe_history"), 1);
        glDispatchCompute((kCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        read_local(0);
        for (unsigned i = 0; i < kCount; ++i) {
            CHECK(std::abs(pixel_float(i)) < 1e-7);
        }
        read_local(1);
        for (unsigned i = 0; i < kCount; ++i) {
            const float expected = int(i % kWidth) - 4 >= 0 ? .15f : 0.f;
            CHECK(std::abs(pixel_float(i) - expected) < 2e-6);
        }
    }

    // Shared surface sampling: no seam at world zero, no wake on another pool,
    // and a smooth radial fade to the unchanged procedural reference.
    std::fill(local_state.begin(), local_state.end(), Cell{.07f, .2f, -.1f, .6f});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, kWidth, kWidth, 0, GL_RGBA, GL_FLOAT, local_state.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    std::string local_sample = "#version 410 core\n";
    local_sample += lighting::shaders::water_window_glsl;
    local_sample += R"(
uniform sampler2D field;
layout(location=0) out vec4 result;
void main(){int test=int(gl_FragCoord.x)%12;float value=0.;
if(test==0)value=water_sim_sample(field,vec2(-.125,0),7,vec2(0),vec2(.3,0)).x;
if(test==1)value=water_sim_sample(field,vec2(.125,0),7,vec2(0),vec2(.3,0)).x;
if(test==2)value=water_sim_sample(field,vec2(0),8,vec2(0),vec2(.3,0)).x;
if(test==3)value=water_sim_sample(field,vec2(water_sim_window.z*water_sim_window.w*.484375,0),7,vec2(0),vec2(.3,0)).x;
if(test==4)value=water_sim_sample(field,vec2(water_sim_window.z*water_sim_window.w*.421875,0),7,vec2(0),vec2(.3,0)).x;
if(test==5)value=water_sim_sample(field,vec2(0),-1,vec2(0),vec2(.3,0)).x;
if(test==6)value=water_sim_sample(field,vec2(water_sim_window.z*water_sim_window.w*.484375,0),7,vec2(0),vec2(.3,0)).y;
if(test==7)value=water_sim_sample(field,vec2(0),8,vec2(0),vec2(.3,0)).w;
if(test==8)value=water_sim_sample(field,vec2(0),water_sim_shared_body(8,1.3),vec2(0),vec2(.3,0)).x;
if(test==9)value=water_sim_sample(field,vec2(0),water_sim_shared_body(8,1.5),vec2(0),vec2(.3,0)).x;
if(test==10)value=water_sim_sample(field,vec2(0),water_sim_shared_body(8,1.51),vec2(0),vec2(.3,0)).x;
if(test==11)value=water_sim_sample(field,vec2(0),water_sim_shared_body(8,4.),vec2(0),vec2(.3,0)).x;
uint bits=floatBitsToUint(value);result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;}
)";
    renderer::Shader local_sample_test;
    CHECK(local_sample_test.build("shared local water sampling", vertex, local_sample, error));
    local_sample_test.use();
    glUniform1i(local_sample_test.uniform("field"), 0);
    glUniform1i(local_sample_test.uniform("water_sim_local"), 1);
    glUniform1i(local_sample_test.uniform("water_sim_body"), 7);
    glUniform1f(local_sample_test.uniform("water_sim_height"), 1.f);
    const std::array<float, 12> local_expected{
        .07f, .07f, 0, 0, .035f, 0, .3f, .5f, .07f, .07f, 0, 0
    };
    for (float extent : {128.f, 256.f}) {
        glUniform4f(
            local_sample_test.uniform("water_sim_window"),
            -extent * .125f,
            -extent * .125f,
            .25f,
            extent
        );
        for (float weight : {1.f, .5f, 0.f}) {
            glUniform1f(local_sample_test.uniform("water_sim_weight"), weight);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            for (unsigned i = 0; i < kCount; ++i) {
                const unsigned test = i % kWidth % 12;
                const bool displacement =
                    test == 0 || test == 1 || test == 4 || test == 8 || test == 9;
                const float expected = local_expected[test] * (displacement ? weight : 1.f);
                CHECK(std::abs(pixel_float(i) - expected) < 2e-6);
            }
        }
    }

    // Compile the real combined effect shader as well as the isolated helpers.
    std::string water_vertex = renderer::shaders::effect_vert;
    water_vertex.insert(water_vertex.find('\n') + 1, "#define OPENRAC_EFFECT_POSITION 1\n");
    water_vertex.insert(
        water_vertex.find("void main()"),
        std::string(lighting::shaders::water_uv_glsl)
            + std::string(lighting::shaders::water_mesh_glsl)
            + std::string(lighting::shaders::water_window_glsl)
            + std::string(lighting::shaders::water_swell_glsl)
            + std::string(lighting::shaders::water_displacement_glsl) + "\n"
            + std::string(lighting::shaders::water_vertex_glsl)
    );
    std::string water_fragment = renderer::shaders::direct_frag;
    water_fragment.insert(water_fragment.find('\n') + 1, "#define OPENRAC_EFFECT_MATERIAL 1\n");
    water_fragment.insert(
        water_fragment.find("void main()"),
        std::string(lighting::shaders::water_window_glsl)
            + std::string(lighting::shaders::water_swell_glsl)
            + std::string(lighting::shaders::shading_brdf_glsl)
            + std::string(lighting::shaders::probes_glsl)
            + std::string(lighting::shaders::environment_glsl) + material
    );
    renderer::Shader water_material;
    CHECK(water_material.build("complete water material", water_vertex, water_fragment, error));
    // A clipped bank can retain an encoded rest height, but it is not a wet
    // lighting receiver. Exercise the production cache shader with simple lights.
    std::string cache_source = R"(#version 430 core
uniform int probe_mode,probe_receiver;
bool sample_probe_irradiance(vec3 p,vec3 n,out vec3 value){value=p;return true;}
float sun_shadow_visibility(vec3 p,vec3 n,vec3 plane){return 1.;}
)";
    cache_source += lighting::shaders::water_lighting_comp;
    renderer::Shader cache_test;
    CHECK(cache_test.build_compute("water wet lighting receivers", cache_source, error));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[3]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(bodies), bodies.data(), GL_DYNAMIC_DRAW);
    std::vector<Cell> receiver_coast(kCount, Cell{-513, 1, 0, 0});
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, kCount * sizeof(Cell), receiver_coast.data(), GL_DYNAMIC_DRAW
    );
    std::array<unsigned, 2> cache_textures{};
    glGenTextures(2, cache_textures.data());
    for (unsigned i = 0; i < 2; ++i) {
        glActiveTexture(GL_TEXTURE0 + 3 + i);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, cache_textures[i]);
        glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32F, buffers[2 + i]);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, image_buffer);
    cache_test.use();
    glUniform1i(cache_test.uniform("water_coast"), 3);
    glUniform1i(cache_test.uniform("water_bodies"), 4);
    glUniform1i(cache_test.uniform("water_body_count"), 1);
    glUniform1i(cache_test.uniform("water_light_count"), 25);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    read_local(3);
    for (unsigned i = 0; i < 25; ++i) {
        CHECK(pixel_float(i) == -1.f);
    }
    receiver_coast[8 * kWidth + 8] = {513, 1, 0, 0};
    upload(buffers[2], receiver_coast);
    cache_test.use();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    read_local(3);
    CHECK(pixel_float(12) == 1.f);
    CHECK(pixel_float(0) == -1.f);
    // Compare the optimized receiver search with the original full scan. Position
    // encoded as irradiance exposes changed receivers, including equal-distance ties.
    cache_test.use();
    glUniform1i(cache_test.uniform("probe_mode"), 1);
    glUniform1i(cache_test.uniform("probe_receiver"), 1);
    for (unsigned pattern = 0; pattern < 8; ++pattern) {
        for (unsigned i = 0; i < kCount; ++i) {
            const bool wet =
                pattern == 0 || ((i * 1664525u + pattern * 1013904223u) >> 27) < pattern * 4;
            receiver_coast[i] = {wet ? 513.f : -513.f, 1, 0, 0};
        }
        upload(buffers[2], receiver_coast);
        cache_test.use();
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        for (unsigned axis = 0; axis < 4; ++axis) {
            read_local(axis);
            for (unsigned node = 0; node < 25; ++node) {
                const int centre_x = std::min(int(node % 5) * 4, int(kWidth) - 1);
                const int centre_y = std::min(int(node / 5) * 4, int(kWidth) - 1);
                int found_x = 0, found_y = 0, closest = 1000;
                for (int y = -2; y <= 2; ++y) {
                    for (int x = -2; x <= 2; ++x) {
                        const int px = centre_x + x, py = centre_y + y;
                        if (px < 0 || py < 0 || px >= int(kWidth) || py >= int(kWidth)) {
                            continue;
                        }
                        if (receiver_coast[py * kWidth + px][0] > .5f && x * x + y * y < closest) {
                            closest = x * x + y * y;
                            found_x = px;
                            found_y = py;
                        }
                    }
                }
                float expected = axis == 3 ? -1.f : 0.f;
                if (closest < 1000) {
                    const std::array<float, 4> receiver{
                        bodies[0][0] + (float(found_x) + .5f) * bodies[4][2],
                        bodies[0][1] + (float(found_y) + .5f) * bodies[4][2],
                        bodies[0][2] + 1.f,
                        1.f
                    };
                    expected = receiver[axis];
                }
                CHECK(std::abs(pixel_float(node) - expected) < 1e-6f);
            }
        }
    }
    glDeleteTextures(2, cache_textures.data());

    glDeleteBuffers(2, local_metadata.data());
}
}  // namespace openrac::test::water
