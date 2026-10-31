// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_float.h"
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
using namespace openrac;
using namespace openrac::gl;
namespace wgl = renderer::world::gl;
using Cell = std::array<float, 4>;

int main() {
    auto settings = enhanced::WaterDebugSettings{};
    settings.current_scale = 100;
    settings.interaction_radius = 0;
    settings.view = 99;
    settings.fine_normal_strength = 100;
    settings.projectile_strength = 100;
    settings.projectile_radius = 0;
    settings.explosion_strength = std::numeric_limits<float>::quiet_NaN();
    settings.explosion_radius = std::numeric_limits<float>::infinity();
    enhanced::set_water_debug_settings(settings);
    auto sanitized = enhanced::water_debug_settings();
    CHECK(
        sanitized.current_scale == 2.5f && sanitized.interaction_radius == .1f
        && sanitized.view == 17 && sanitized.fine_normal_strength == 2
        && sanitized.projectile_strength == 4 && sanitized.projectile_radius == .1f
        && sanitized.explosion_strength == 1 && sanitized.explosion_radius == 1
    );
    enhanced::set_water_debug_settings(enhanced::WaterDebugSettings{});

    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::printf("SKIP %s\n", error.c_str());
        return test::kSkip;
    }
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    renderer::Shader compute, read;
    bool built = compute.build_compute("water fixture", lighting::shaders::water_comp, error);
    if (!built) {
        std::printf("%s\n", error.c_str());
    }
    CHECK(built);
    constexpr const char* vertex = R"(#version 410 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})";
    constexpr const char* fragment = R"(#version 410 core
uniform sampler2D field;
uniform int channel;
layout(location=0) out vec4 result;
void main(){int at=int(gl_FragCoord.y)*16+int(gl_FragCoord.x);uint bits=floatBitsToUint(texelFetch(field,ivec2(gl_FragCoord.xy),0)[channel]);
result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;})";
    CHECK(read.build("water readback", vertex, fragment, error));
    constexpr unsigned width = 16, count = width * width;
    std::vector<Cell> coast(count), cpu(count), next(count), zero(count), hydraulics(count);
    const std::array<Cell, 5> bodies{
        {{0, 0, 0, 1}, {1, 0, 0, width}, {0, 1, 0, width}, {0, 0, 1, 0}, {0, 0, .5f, 0}}
    };
    for (unsigned y = 1; y < width - 1; ++y) {
        for (unsigned x = 1; x < width - 1; ++x) {
            if (x == 9 && y < 11) {
                continue;  // Interior bank, including a corner and an opening.
            }
            coast[y * width + x] = {512, 1, 0.f, 0};
        }
    }
    for (unsigned i = 0; i < count; ++i) {
        hydraulics[i] = {.75f + .15f * float(i % width) / float(width), .35f, 0, 0};
        if (coast[i][0] > .5f) {
            cpu[i][1] = hydraulics[i][0] * hydraulics[i][1];
        }
    }
    cpu[5 * width + 5][0] = .05f;
    cpu[5 * width + 5][1] += .05f * .35f;
    std::array<unsigned, 5> buffers{};
    glGenBuffers(5, buffers.data());
    const std::array<std::span<const Cell>, 5> initial{cpu, zero, coast, bodies, hydraulics};
    for (unsigned i = 0; i < 5; ++i) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[i]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            GLsizeiptr(initial[i].size_bytes()),
            initial[i].data(),
            GL_DYNAMIC_DRAW
        );
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buffers[4]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buffers[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, buffers[4]);
    unsigned image_buffer = 0;
    glGenBuffers(1, &image_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, image_buffer);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(zero.size() * sizeof(Cell)),
        zero.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, image_buffer);
    compute.use();
    glUniform4f(compute.uniform("water_physics"), 1, .35f, 12, .3f);
    glUniform1f(compute.uniform("displacement_limit"), .25f);
    glUniform1i(compute.uniform("transport_reference"), 1);
    glUniform1f(compute.uniform("wake_strength"), 0);
    glUniform1i(compute.uniform("atlas_width"), width);
    const std::array<Cell, 64> atlas{};
    glUniform4fv(compute.uniform("water_atlas[0]"), 64, atlas[0].data());
    unsigned texture = 0;
    glGenTextures(1, &texture);
    renderer::FrameBuffer target;
    CHECK(target.create(width, width, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    unsigned current = 0;
    // Independent host finite-volume reference: depth plus x/y discharge.
    using State = std::array<double, 3>;
    const auto reference = [](const Cell& bed) {
        return State{bed[0], bed[0] * bed[1], bed[0] * bed[2]};
    };
    bool improved = false;
    const auto flux = [&](State a, State b, double da, double db, int axis, int sign) {
        constexpr double g = 9.81;
        double top = std::max(-da, -db), ha = std::max(0., a[0] - da - top),
               hb = std::max(0., b[0] - db - top);
        State qa{ha, ha * a[1] / a[0], ha * a[2] / a[0]},
            qb{hb, hb * b[1] / b[0], hb * b[2] / b[0]};
        double va = sign * a[axis + 1] / a[0], vb = sign * b[axis + 1] / b[0];
        double speed = std::max(std::abs(va) + std::sqrt(g * ha), std::abs(vb) + std::sqrt(g * hb));
        State result{};
        for (unsigned j = 0; j < 3; ++j) {
            double fa = j == 0 ? ha * va : qa[j] * va, fb = j == 0 ? hb * vb : qb[j] * vb;
            if (j == unsigned(axis + 1)) {
                fa += sign * .5 * g * ha * ha;
                fb += sign * .5 * g * hb * hb;
            }
            if (!improved) {
                result[j] = .5 * (fa + fb - speed * (qb[j] - qa[j]));
            } else {
                double lo = std::min(va - std::sqrt(g * ha), vb - std::sqrt(g * hb));
                double hi = std::max(va + std::sqrt(g * ha), vb + std::sqrt(g * hb));
                result[j] = lo >= 0   ? fa
                            : hi <= 0 ? fb
                                      : (hi * fa - lo * fb + lo * hi * (qb[j] - qa[j])) / (hi - lo);
            }
        }
        if (improved) {
            const unsigned transverse = unsigned(2 - axis);
            result[transverse] =
                result[0] * (result[0] >= 0 ? a[transverse] / a[0] : b[transverse] / b[0]);
        }
        result[axis + 1] += sign * .5 * g * (a[0] * a[0] - ha * ha);
        return result;
    };
    const auto initial_cpu = cpu;
    float max_error = 0;
    double mass = 0;
    std::vector<unsigned char> pixels(count * 4);
    for (int mode = 0; mode < 2; ++mode) {
        improved = mode != 0;
        cpu = initial_cpu;
        current = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[0]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            GLsizeiptr(cpu.size() * sizeof(Cell)),
            cpu.data(),
            GL_DYNAMIC_DRAW
        );
        compute.use();
        glUniform1i(compute.uniform("transport_reference"), mode == 0 ? 1 : 0);
        // Independent coordinate-based MUSCL reference, evaluated in double precision.
        const auto reconstructed = [&](unsigned index, int axis, int sign) {
            const auto primitive = [&](unsigned cell) {
                const auto& h = hydraulics[cell];
                double depth = h[0] + cpu[cell][0];
                return State{
                    cpu[cell][0], cpu[cell][1] / depth - h[1], cpu[cell][2] / depth - h[2]
                };
            };
            State value = primitive(index), slope{};
            int x = int(index % width), y = int(index / width);
            int lx = x - (axis == 0), ly = y - (axis == 1), rx = x + (axis == 0),
                ry = y + (axis == 1);
            if (lx >= 0 && ly >= 0 && rx < int(width) && ry < int(width)
                && coast[unsigned(ly) * width + unsigned(lx)][0] > .5f
                && coast[unsigned(ry) * width + unsigned(rx)][0] > .5f) {
                const auto left = primitive(unsigned(ly) * width + unsigned(lx));
                const auto right = primitive(unsigned(ry) * width + unsigned(rx));
                for (unsigned c = 0; c < 3; ++c) {
                    double a = value[c] - left[c], b = right[c] - value[c];
                    if (a * b > 0) {
                        slope[c] = std::copysign(std::min(std::abs(a), std::abs(b)), a);
                    }
                }
            }
            for (unsigned c = 0; c < 3; ++c) {
                value[c] += .5 * sign * slope[c];
            }
            const auto& h = hydraulics[index];
            double depth = std::max(.02, h[0] + value[0]);
            return State{
                depth,
                depth * std::clamp(h[1] + value[1], -2.5, 2.5),
                depth * std::clamp(h[2] + value[2], -2.5, 2.5)
            };
        };
        constexpr unsigned substeps = 240;
        for (unsigned step = 0; step < substeps; ++step) {
            for (unsigned y = 0; y < width; ++y) {
                for (unsigned x = 0; x < width; ++x) {
                    unsigned at = y * width + x;
                    next[at] = {};
                    if (coast[at][0] < .5f) {
                        continue;
                    }
                    const auto& bed = hydraulics[at];
                    State centre{bed[0] + cpu[at][0], cpu[at][1], cpu[at][2]}, q = centre,
                                                                               r = reference(bed);
                    for (int axis = 0; axis < 2; ++axis) {
                        for (int sign : {-1, 1}) {
                            int nx = int(x) + (axis == 0 ? sign : 0),
                                ny = int(y) + (axis == 1 ? sign : 0);
                            State face = improved ? reconstructed(at, axis, sign) : centre;
                            State neighbour = face, s = r;
                            double depth = bed[0];
                            if (nx >= 0 && ny >= 0 && nx < int(width) && ny < int(width)
                                && coast[unsigned(ny) * width + unsigned(nx)][0] > .5f) {
                                unsigned index = unsigned(ny) * width + unsigned(nx);
                                depth = hydraulics[index][0];
                                neighbour =
                                    improved
                                        ? reconstructed(index, axis, -sign)
                                        : State{
                                              depth + cpu[index][0], cpu[index][1], cpu[index][2]
                                          };
                                s = reference(hydraulics[index]);
                            } else {
                                neighbour[axis + 1] *= -1;
                                s[axis + 1] *= -1;
                            }
                            auto f = flux(face, neighbour, bed[0], depth, axis, sign),
                                 base = flux(r, s, bed[0], depth, axis, sign);
                            for (unsigned j = 0; j < 3; ++j) {
                                q[j] -= (f[j] - base[j]) / 60.;  // dt/dx=(1/120)/.5
                            }
                        }
                    }
                    q[0] = std::clamp(q[0], .02, 4.);
                    if (step == 0) {
                        double rx = (double(x) + .5) * .5 - 2.25, ry = (double(y) + .5) * .5 - 2.75;
                        double t = std::clamp(rx / .5, 0., 1.);
                        rx -= t * .5;
                        double impulse = .3 * std::exp(-(rx * rx + ry * ry) / .3);
                        q[1] += q[0] * impulse * (rx * 2. + .125);
                        q[2] += q[0] * impulse * ry * 2.;
                    }
                    double damping = std::exp(-.35 / 120.);
                    for (unsigned axis = 0; axis < 2; ++axis) {
                        q[axis + 1] =
                            q[0] * bed[axis + 1] + (q[axis + 1] - q[0] * bed[axis + 1]) * damping;
                    }
                    next[at] =
                        {float(q[0] - bed[0]),
                         float(std::clamp(q[1], -2.5 * q[0], 2.5 * q[0])),
                         float(std::clamp(q[2], -2.5 * q[0], 2.5 * q[0])),
                         0};
                }
            }
            cpu.swap(next);
            compute.use();
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers[2]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[3]);
            glUniform1i(compute.uniform("cell_count"), count);
            glUniform1i(compute.uniform("reset_waves"), 0);
            glUniform4f(compute.uniform("hero"), 2.75f, 2.75f, 0, step == 0 ? .3f : 0.f);
            glUniform3f(compute.uniform("previous_hero"), 2.25f, 2.75f, 0);
            glDispatchCompute((count + 63) / 64, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            current = 1 - current;
        }
        read.use();
        glUniform1i(read.uniform("field"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, width, width, 0, GL_RGBA, GL_FLOAT, zero.data()
        );
#ifdef OPENRAC_VULKAN
        CHECK(!vkgl::copy_buffer_to_texture(image_buffer, texture, width + 1, width));
        CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, width, width);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glBindVertexArray(vao);
        max_error = 0;
        mass = 0;
        for (int channel = 0; channel < 3; ++channel) {
            glUniform1i(read.uniform("channel"), channel);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            for (unsigned i = 0; i < count; ++i) {
                unsigned at = i * 4;
                std::uint32_t bits =
                    std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                    | (std::uint32_t(pixels[at + 2]) << 16) | (std::uint32_t(pixels[at + 3]) << 24);
                float gpu = std::bit_cast<float>(bits);
                float expected = channel == 0 ? cpu[i][0]
                                 : coast[i][0] > .5f
                                     ? cpu[i][unsigned(channel)] / (hydraulics[i][0] + cpu[i][0])
                                     : 0.f;
                CHECK(std::isfinite(gpu));
                max_error = std::max(max_error, std::abs(gpu - expected));
                if (channel == 0) {
                    mass += gpu;
                    CHECK(hydraulics[i][0] + gpu > .02f);
                }
            }
        }
        std::printf(
            "Shallow-water CPU/GPU (%s) max state error after 240 substeps: %.9g; mass %.9g\n",
            improved ? "MUSCL/contact" : "reference",
            double(max_error),
            mass
        );
        CHECK(max_error < 3e-5f);
        CHECK(std::abs(mass - .05) < 5e-5);
    }
    // Exercise the actual enhanced vertex hook against the compute height image.
    // Disabling the material must leave the original position exactly unchanged.
    renderer::Shader displaced;
    std::string displacement_vertex = "#version 410 core\n";
    displacement_vertex += lighting::shaders::water_uv_glsl;
    displacement_vertex += lighting::shaders::water_mesh_glsl;
    displacement_vertex += lighting::shaders::water_window_glsl;
    displacement_vertex += lighting::shaders::water_swell_glsl;
    displacement_vertex += lighting::shaders::water_displacement_glsl;
    displacement_vertex += lighting::shaders::water_vertex_glsl;
    displacement_vertex += R"(
flat out float height;
void main(){
    vec2 q=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    height=effect_position(vec3(2.75,2.75,0)).z;
    gl_Position=vec4(q*2.-1.,0,1);
})";
    constexpr const char* displacement_fragment = R"(#version 410 core
flat in float height;
layout(location=0) out vec4 result;
void main(){uint bits=floatBitsToUint(height);
result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;})";
    CHECK(displaced
              .build("water mesh displacement", displacement_vertex, displacement_fragment, error));
    unsigned body_texture = 0;
    glGenTextures(1, &body_texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, body_texture);
    glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32F, buffers[3]);
    unsigned coast_texture = 0;
    glGenTextures(1, &coast_texture);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, coast_texture);
    std::vector<Cell> dry_coast(count, Cell{0, 0, 0, 0});
    glTexImage2D(
        GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, width, width, 0, GL_RGBA, GL_FLOAT, dry_coast.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    displaced.use();
    glUniform1i(displaced.uniform("water_coast_map"), 2);
    glUniform1i(displaced.uniform("water_bodies"), 1);
    glUniform1i(displaced.uniform("water_wave_map"), 0);
    glUniform1i(displaced.uniform("water_body_count"), 1);
    glUniform2f(displaced.uniform("water_atlas_extent"), width, width);

    glUniform4fv(displaced.uniform("water_atlas[0]"), 64, atlas[0].data());
    for (int mode : {0, 1, 2, 3, 4, 5, 6, 7}) {
        auto authored_body = bodies;
        authored_body[0][3] = float(mode ? mode : 1);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[3]);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(authored_body), authored_body.data());
        glUniform1i(displaced.uniform("water_surface"), mode);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        const std::uint32_t bits = std::uint32_t(pixels[0]) | (std::uint32_t(pixels[1]) << 8)
                                   | (std::uint32_t(pixels[2]) << 16)
                                   | (std::uint32_t(pixels[3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        CHECK(std::abs(actual - (mode ? cpu[5 * width + 5][0] : 0.f)) < 2e-6f);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[3]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bodies), bodies.data());
    // Wet interior vertices must receive the swell, while the dry-bank check
    // above and the original-material branch remain exactly unchanged.
    glActiveTexture(GL_TEXTURE0 + 2);
    std::vector<Cell> wet_coast(count, Cell{1, 3, 1, 0});
    glTexImage2D(
        GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, width, width, 0, GL_RGBA, GL_FLOAT, wet_coast.data()
    );
    glUniform1i(displaced.uniform("water_surface"), 1);
    glUniform1f(displaced.uniform("water_time"), .43f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    {
        const auto bits = std::uint32_t(pixels[0]) | (std::uint32_t(pixels[1]) << 8)
                          | (std::uint32_t(pixels[2]) << 16) | (std::uint32_t(pixels[3]) << 24);
        const double phase =
            (6.28318531 / 2.75) * 2.75 * (.87 + .49) / std::hypot(.87, .49) - .43 * 1.4;
        CHECK(
            std::abs(std::bit_cast<float>(bits) - cpu[5 * width + 5][0] - .045 * std::sin(phase))
            < 2e-6
        );
    }
    // The exact liquid evaluator also moves a rigid attachment, preserving its rest offset.
    renderer::Shader floating;
    std::string floating_vertex = enhanced::WaterFloat::vertex_source(R"(#version 410 core
uniform vec3 anchor;
uniform int attachment_check;
flat out float height;
void main(){
    vec2 q=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    mat4 pose=water_float_pose(anchor);
    mat3 rotation=mat3(pose);
    vec3 moved=(pose*vec4(anchor,1)).xyz;
    vec3 a=water_displaced_position(anchor-vec3(.25,0,0),0);
    vec3 b=water_displaced_position(anchor+vec3(.25,0,0),0);
    vec3 c=water_displaced_position(anchor-vec3(0,.25,0),0);
    vec3 d=water_displaced_position(anchor+vec3(0,.25,0),0);
    vec3 expected_up=normalize(cross(b-a,d-c));
    height=moved.z-anchor.z;
    if(attachment_check==1) height=length(rotation[0])-1.;
    if(attachment_check==2) height=dot(rotation[0],rotation[1]);
    if(attachment_check==3) height=length(rotation[2]-expected_up);
    if(attachment_check==4) height=length(rotation*vec3(.8,-.6,.3))-length(vec3(.8,-.6,.3));
    if(attachment_check==5) height=length(moved.xy-anchor.xy);
    if(attachment_check==6) height=determinant(rotation)-1.;
    if(attachment_check==7) height=length(rotation[2].xy);
    gl_Position=vec4(q*2.-1.,0,1);
})");
    CHECK(floating.build(
        "water attachment displacement", floating_vertex, displacement_fragment, error
    ));
    floating.use();
    glUniform1i(floating.uniform("water_float_poses"), 1);
    glUniform1i(floating.uniform("water_float_count"), 0);
    glUniform1i(floating.uniform("water_bodies"), 1);
    glUniform1i(floating.uniform("water_coast_map"), 2);
    glUniform1i(floating.uniform("water_wave_map"), 0);
    glUniform2f(floating.uniform("water_atlas_extent"), width, width);
    glUniform4fv(floating.uniform("water_atlas[0]"), 64, atlas[0].data());
    glUniform1f(floating.uniform("water_time"), .43f);
    for (int scenario = 0; scenario < 7; ++scenario) {
        // Real water grids have origin Z=0 even when the surface is high above it.
        if (scenario == 5) {
            for (auto& cell : wet_coast) {
                cell[3] = 39.f;
            }
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, coast_texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                wgl::GL_RGBA32F,
                width,
                width,
                0,
                GL_RGBA,
                GL_FLOAT,
                wet_coast.data()
            );
        }
        if (scenario == 6) {
            for (auto& cell : wet_coast) {
                cell[0] = 0;
                cell[3] = 0;
            }
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, coast_texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                wgl::GL_RGBA32F,
                width,
                width,
                0,
                GL_RGBA,
                GL_FLOAT,
                wet_coast.data()
            );
        }
        glUniform1i(floating.uniform("water_float_enabled"), scenario == 3 ? 0 : 1);
        glUniform1i(floating.uniform("water_body_count"), scenario == 4 ? 0 : 1);
        glUniform3f(
            floating.uniform("anchor"),
            scenario == 1 ? -100.f : 2.75f,
            2.75f,
            scenario == 2   ? 3.f
            : scenario == 5 ? 39.2f
                            : .2f
        );
        for (int channel = 0; channel < 8; ++channel) {
            glUniform1i(floating.uniform("attachment_check"), channel);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            const auto bits = std::uint32_t(pixels[0]) | (std::uint32_t(pixels[1]) << 8)
                              | (std::uint32_t(pixels[2]) << 16) | (std::uint32_t(pixels[3]) << 24);
            const float actual = std::bit_cast<float>(bits);
            CHECK(std::isfinite(actual));
            const bool active = scenario == 0 || scenario == 5;
            if (channel == 7 && active) {
                CHECK(actual > 1e-5f);
                continue;
            }
            if (channel == 3 && !active) {
                continue;  // Disabled/dry attachments keep identity rather than the sampled slope.
            }
            float expected = 0;
            if (channel == 0 && active) {
                const double phase =
                    (6.28318531 / 2.75) * 2.75 * (.87 + .49) / std::hypot(.87, .49) - .43 * 1.4;
                expected = cpu[5 * width + 5][0] + float(.045 * std::sin(phase));
            }
            CHECK(std::abs(actual - expected) < 8e-6f);
        }
    }
    // Exercise the shipped momentum kernel with a controlled step in height and tilt.
    // Read the resulting FP32 values through the same bit-exact fragment readback.
    {
        renderer::Shader momentum, pose_read;
        std::string kernel(lighting::shaders::water_float_comp);
        kernel.insert(kernel.find("void main()"), R"(
uniform vec4 target_pose;
vec4 water_float_target(vec3 anchor,out bool matched) { matched=true; return target_pose; }
)");
        CHECK(momentum.build_compute("attachment momentum regression", kernel, error));
        if (!momentum.id()) {
            std::printf("%s\n", error.c_str());
            return test::result();
        }
        CHECK(pose_read.build(
            "attachment pose readback",
            R"(#version 430 core
uniform samplerBuffer pose_values;
uniform int component;
flat out float height;
void main() {
    vec2 q=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    height=texelFetch(pose_values,1)[component];
    gl_Position=vec4(q*2.-1.,0,1);
})",
            displacement_fragment,
            error
        ));
        unsigned state[3]{}, views[2]{};
        glGenBuffers(3, state);
        glGenTextures(2, views);
        const std::array<Cell, 2> initial_pose{Cell{2, 3, .2f, 1}, Cell{0, 0, 1, .2f}};
        for (unsigned i = 0; i < 2; ++i) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, state[i]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER, sizeof(initial_pose), initial_pose.data(), GL_DYNAMIC_DRAW
            );
            glActiveTexture(GL_TEXTURE0 + 22 + i);
            glBindTexture(wgl::GL_TEXTURE_BUFFER, views[i]);
            glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32F, state[i]);
        }
        const auto step = [&](float previous, float alpha) {
            const Cell anchor{2, 3, .2f, previous};
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, state[2]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(anchor), anchor.data(), GL_STREAM_DRAW);
            momentum.use();
            glUniform1i(momentum.uniform("previous_poses"), 22);
            glUniform1i(momentum.uniform("attachment_count"), 1);
            glUniform4f(momentum.uniform("target_pose"), .6f, 0, .8f, 1.2f);
            glUniform2f(momentum.uniform("response"), alpha, alpha);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, state[2]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, state[1]);
            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            pose_read.use();
            glUniform1i(pose_read.uniform("pose_values"), 23);
            Cell result{};
            for (int c = 0; c < 4; ++c) {
                glUniform1i(pose_read.uniform("component"), c);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                const auto bits = std::uint32_t(pixels[0]) | (std::uint32_t(pixels[1]) << 8)
                                  | (std::uint32_t(pixels[2]) << 16)
                                  | (std::uint32_t(pixels[3]) << 24);
                result[c] = std::bit_cast<float>(bits);
            }
            return result;
        };
        const auto halfway = step(0, .5f);
        CHECK(std::abs(halfway[3] - .7f) < 1e-6f);
        CHECK(halfway[0] > 0 && halfway[0] < .6f);
        CHECK(std::abs(halfway[0] * halfway[0] + halfway[2] * halfway[2] - 1) < 1e-6f);
        const auto paused = step(0, 0);
        CHECK(paused == initial_pose[1]);
        const auto fresh = step(-1, .1f);
        CHECK(std::abs(fresh[3] - 1.2f) < 1e-6f && std::abs(fresh[0] - .6f) < 1e-6f);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, image_buffer);
        glDeleteTextures(2, views);
        glDeleteBuffers(3, state);
        glActiveTexture(GL_TEXTURE0);
    }
    glDeleteTextures(1, &body_texture);
    glDeleteTextures(1, &coast_texture);
    // Teleport/large-gap reset clears both height and velocity in the current output.
    compute.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
    glUniform1i(compute.uniform("reset_waves"), 1);
    glDispatchCompute((count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    read.use();
    glUniform1i(read.uniform("channel"), 0);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    for (auto byte : pixels) {
        CHECK(byte == 0);
    }
    current = 1 - current;
    for (unsigned step = 0; step < 60; ++step) {
        compute.use();
        glUniform1i(compute.uniform("reset_waves"), 0);
        glUniform4f(compute.uniform("hero"), 0, 0, 1000, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        current = 1 - current;
    }
    read.use();
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    for (auto byte : pixels) {
        CHECK(byte == 0);
    }
    // The same solver must preserve a still pond (zero maintained current).
    for (auto& h : hydraulics) {
        h[1] = h[2] = 0;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[4]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(hydraulics.size() * sizeof(Cell)),
        hydraulics.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buffers[4]);
    for (unsigned step = 0; step < 61; ++step) {
        compute.use();
        glUniform1i(compute.uniform("reset_waves"), step == 0 ? 1 : 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        current = 1 - current;
    }
    read.use();
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    for (auto byte : pixels) {
        CHECK(byte == 0);
    }
    // An open reservoir/sponge removes disturbances rather than reflecting them indefinitely.
    unsigned wet_count = 0;
    for (unsigned i = 0; i < count; ++i) {
        cpu[i] = {};
        if (coast[i][0] > .5f) {
            coast[i][2] = 1;
            cpu[i][0] = .1f;
            ++wet_count;
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[current]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, GLsizeiptr(cpu.size() * sizeof(Cell)), cpu.data(), GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(coast.size() * sizeof(Cell)),
        coast.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers[2]);
    for (unsigned step = 0; step < 60; ++step) {
        compute.use();
        glUniform1i(compute.uniform("reset_waves"), 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        current = 1 - current;
    }
    read.use();
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    double remaining_mass = 0;
    for (unsigned i = 0; i < count; ++i) {
        unsigned at = i * 4;
        std::uint32_t bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                             | (std::uint32_t(pixels[at + 2]) << 16)
                             | (std::uint32_t(pixels[at + 3]) << 24);
        float height = std::bit_cast<float>(bits);
        CHECK(std::isfinite(height));
        CHECK(height >= -1e-5f && height <= .10001f);
        remaining_mass += height;
    }
    CHECK(remaining_mass < .01 * wet_count);
    // Animated hands push locally even while the body is stationary. Both palms
    // work independently; airborne and stationary strokes must leave water still.
    for (auto& cell : coast) {
        cell[2] = 0;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(coast.size() * sizeof(Cell)),
        coast.data(),
        GL_DYNAMIC_DRAW
    );
    for (int scenario = 0; scenario < 7; ++scenario) {
        compute.use();
        glUniform1i(compute.uniform("reset_waves"), 1);
        glUniform1i(compute.uniform("hand_count"), 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        current = 1 - current;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
        glUniform1i(compute.uniform("reset_waves"), 0);
        glUniform1i(compute.uniform("hand_count"), 2);
        glUniform4f(compute.uniform("hands[0]"), 0, 0, 0, 0);
        glUniform4f(compute.uniform("hands[1]"), 0, 0, 0, 0);
        const int palm = scenario == 1 ? 1 : 0;
        const float z = scenario == 2 ? 2.f : 0.f;
        const float previous_x = scenario == 1 ? 3.25f : 2.25f;
        const float next_x = scenario == 1 ? 2.25f : scenario == 3 ? previous_x : 3.25f;
        glUniform3f(
            compute.uniform(palm == 0 ? "previous_hands[0]" : "previous_hands[1]"),
            previous_x,
            2.75f,
            scenario == 4 ? .2f : z
        );
        glUniform4f(
            compute.uniform(palm == 0 ? "hands[0]" : "hands[1]"),
            scenario == 4 ? previous_x : next_x,
            2.75f,
            scenario == 4 ? -.2f : z,
            1.f
        );
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        current = 1 - current;
#ifdef OPENRAC_VULKAN
        CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
        read.use();
        double sum_x = 0, max_speed = 0;
        for (int channel = 0; channel < 3; ++channel) {
            glUniform1i(read.uniform("channel"), channel);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            for (unsigned cell = 0; cell < count; ++cell) {
                const unsigned at = cell * 4;
                const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                                  | (std::uint32_t(pixels[at + 2]) << 16)
                                  | (std::uint32_t(pixels[at + 3]) << 24);
                const float value = std::bit_cast<float>(bits);
                CHECK(std::isfinite(value));
                if (channel == 0 || coast[cell][0] < .5f) {
                    CHECK(std::abs(value) < 1e-5);
                } else {
                    max_speed = std::max(max_speed, double(std::abs(value)));
                }
                if (channel == 1) {
                    sum_x += value;
                }
            }
        }
        if (scenario == 0) {
            CHECK(sum_x > .1);
        }
        if (scenario == 1) {
            CHECK(sum_x < -.1);
        }
        if (scenario == 2 || scenario == 3) {
            CHECK(max_speed < 1e-6);
        }
        if (scenario == 4) {
            CHECK(max_speed > .01);
        }
    }
    // Flow markers must advect with the velocity, including its sign, rather
    // than scroll according to wall-clock time or the authored flow direction.
    std::vector<Cell> flow_state(count);
    for (unsigned i = 0; i < count; ++i) {
        hydraulics[i] = {.75f, .4f, -.2f, 0};
        flow_state[i] =
            {0, .75f * .4f, -.75f * .2f, .1f + .02f * float(i % width) + .03f * float(i / width)};
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[4]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(hydraulics.size() * sizeof(Cell)),
        hydraulics.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[current]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(flow_state.size() * sizeof(Cell)),
        flow_state.data(),
        GL_DYNAMIC_DRAW
    );
    compute.use();
    glUniform1i(compute.uniform("transport_reference"), 0);
    glUniform1i(compute.uniform("hand_count"), 0);
    glUniform4f(compute.uniform("hero"), 0, 0, 1000, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
    glDispatchCompute((count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    current = 1 - current;
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
    read.use();
    glUniform1i(read.uniform("channel"), 3);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    for (unsigned y = 3; y < 8; ++y) {
        for (unsigned x = 3; x < 8; ++x) {
            const unsigned at = (y * width + x) * 4;
            auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                        | (std::uint32_t(pixels[at + 2]) << 16)
                        | (std::uint32_t(pixels[at + 3]) << 24);
            const double px = (double(x) + .5) * .5, py = (double(y) + .5) * .5;
            const double seed =
                .5 + .25 * std::sin(px * 1.7 + py * .9) + .25 * std::sin(-px * .8 + py * 2.1);
            const double advected =
                .1 + .02 * (double(x) - .4 / 60.) + .03 * (double(y) + .2 / 60.);
            const double expected =
                advected * std::exp(-.04 / 120.) + seed * (1. - std::exp(-.04 / 120.));
            CHECK(std::abs(std::bit_cast<float>(bits) - expected) < 2e-6);
        }
    }
    // Controlled decay comparisons demonstrate the reduction in numerical
    // diffusion: a gravity pulse and a smooth rotating velocity field.
    for (unsigned y = 0; y < width; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            unsigned i = y * width + x;
            coast[i] = {x > 0 && y > 0 && x + 1 < width && y + 1 < width ? 512.f : 0.f, 1, 0, 0};
            hydraulics[i] = {.75f, 0, 0, 0};
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(coast.size() * sizeof(Cell)),
        coast.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[4]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(hydraulics.size() * sizeof(Cell)),
        hydraulics.data(),
        GL_DYNAMIC_DRAW
    );
    for (int experiment = 0; experiment < 2; ++experiment) {
        std::array<double, 2> energy{}, enstrophy{};
        for (int mode = 0; mode < 2; ++mode) {
            double initial_mass = 0, initial_energy = 0;
            for (unsigned y = 0; y < width; ++y) {
                for (unsigned x = 0; x < width; ++x) {
                    unsigned i = y * width + x;
                    flow_state[i] = {};
                    if (coast[i][0] < .5f) {
                        continue;
                    }
                    const double rx = (double(x) + .5) * .5 - 4, ry = (double(y) + .5) * .5 - 4;
                    double weight = std::exp(-(rx * rx + ry * ry));
                    if (experiment == 0) {
                        flow_state[i][0] = float(.05 * weight);
                    } else {
                        flow_state[i][1] = float(-.75 * .3 * ry * weight);
                        flow_state[i][2] = float(.75 * .3 * rx * weight);
                    }
                    initial_mass += flow_state[i][0];
                    initial_energy += 9.81 * flow_state[i][0] * flow_state[i][0]
                                      + (flow_state[i][1] * flow_state[i][1]
                                         + flow_state[i][2] * flow_state[i][2])
                                            / .75;
                }
            }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[current]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER,
                GLsizeiptr(flow_state.size() * sizeof(Cell)),
                flow_state.data(),
                GL_DYNAMIC_DRAW
            );
            compute.use();
            glUniform1i(compute.uniform("transport_reference"), mode == 0 ? 1 : 0);
            for (unsigned step = 0; step < 120; ++step) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[current]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - current]);
                glDispatchCompute((count + 63) / 64, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
                current = 1 - current;
            }
#ifdef OPENRAC_VULKAN
            CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#endif
            std::vector<Cell> measured(count);
            read.use();
            for (int c = 0; c < 3; ++c) {
                glUniform1i(read.uniform("channel"), c);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                for (unsigned i = 0; i < count; ++i) {
                    unsigned at = i * 4;
                    auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                                | (std::uint32_t(pixels[at + 2]) << 16)
                                | (std::uint32_t(pixels[at + 3]) << 24);
                    measured[i][unsigned(c)] = std::bit_cast<float>(bits);
                    CHECK(std::isfinite(measured[i][unsigned(c)]));
                }
            }
            double final_mass = 0;
            for (unsigned i = 0; i < count; ++i) {
                const auto& q = measured[i];
                final_mass += q[0];
                CHECK(.75 + q[0] > .02);
                CHECK(std::abs(q[1]) <= 2.50001f && std::abs(q[2]) <= 2.50001f);
                energy[unsigned(mode)] += 9.81 * q[0] * q[0] + .75 * (q[1] * q[1] + q[2] * q[2]);
            }
            for (unsigned y = 2; y < width - 2; ++y) {
                for (unsigned x = 2; x < width - 2; ++x) {
                    const unsigned i = y * width + x;
                    const double curl = measured[i + 1][2] - measured[i - 1][2]
                                        - measured[i + width][1] + measured[i - width][1];
                    enstrophy[unsigned(mode)] += curl * curl;
                }
            }
            CHECK(std::abs(final_mass - initial_mass) < 5e-5);
            CHECK(energy[unsigned(mode)] <= initial_energy * 1.1);
        }
        std::printf(
            "%s retained energy reference %.9g improved %.9g; enstrophy %.9g %.9g\n",
            experiment == 0 ? "Gravity pulse" : "Vortex",
            energy[0],
            energy[1],
            enstrophy[0],
            enstrophy[1]
        );
        CHECK(energy[1] > energy[0] * 1.08);
        if (experiment == 1) {
            CHECK(enstrophy[1] > enstrophy[0] * 1.2);
        }
    }
    test::water::Context fixture{
        compute, read, error, pixels, buffers, bodies, texture, image_buffer, vertex
    };
    test::water::check_optics(fixture);
    test::water::check_foam(fixture);
    test::water::check_transmission(fixture);
    test::water::check_environment(fixture);
    test::water::check_window(fixture);
    test::water::check_sampling(fixture);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(5, buffers.data());
    glDeleteBuffers(1, &image_buffer);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
