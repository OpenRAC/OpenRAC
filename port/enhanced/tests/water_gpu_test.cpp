// SPDX-License-Identifier: GPL-3.0-or-later
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
    enhanced::set_water_debug_settings(settings);
    auto sanitized = enhanced::water_debug_settings();
    CHECK(
        sanitized.current_scale == 2.5f && sanitized.interaction_radius == .1f
        && sanitized.view == 4 && sanitized.fine_normal_strength == 2
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
    for (int scenario = 0; scenario < 5; ++scenario) {
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
                    initial_energy +=
                        9.81 * flow_state[i][0] * flow_state[i][0]
                        + (flow_state[i][1] * flow_state[i][1] + flow_state[i][2] * flow_state[i][2]
                          ) / .75;
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
    // Shared Gerstner displacement/tangents against independent CPU positions
    // and finite differences, including a spatially varying shoreline taper.
    std::string swell_source = "#version 410 core\n";
    swell_source += lighting::shaders::water_swell_glsl;
    swell_source += R"(
layout(location=0) out vec4 result;
void main(){
    int test=int(gl_FragCoord.x)%16;
    vec3 dx,dy;
    vec3 d=water_swell(vec2(.37,.91),.43,.8,vec2(.1,-.08),dx,dy);
    float value=0.;
    if(test<3) value=d[test];
    else if(test<6) value=dx[test-3];
    else if(test<9) value=dy[test-6];
    else if(test==9) value=length(water_swell(vec2(.37,.91),.43,0.,vec2(0),dx,dy));
    else if(test==10) value=water_art_tint(vec3(.025,.19,.30),vec3(.2)).b;
    else if(test==11) value=water_art_tint(vec3(.025,.19,.30),vec3(0)).b;
    else if(test==12) value=water_boundary_swell(vec3(.3,1,0),.43,.8).z;
    else if(test==13) value=water_boundary_swell(vec3(.3,1,0),.43,.8).x;
    else if(test==14) value=length(water_boundary_swell(vec3(.3,1,0),.43,0.));
    else value=length(water_boundary_swell(vec3(.3,0,0),.43,.8));
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader swell_test;
    CHECK(swell_test.build("Gerstner water displacement and tangents", vertex, swell_source, error)
    );
    swell_test.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const auto swell_position = [](double x, double y) {
        const double norm = std::hypot(.87, .49), dx = .87 / norm, dy = .49 / norm;
        const double phase = (6.28318531 / 2.75) * (dx * x + dy * y) - .43 * 1.4;
        const double weight = .8 + .1 * (x - .37) - .08 * (y - .91);
        return std::array<double, 3>{
            weight * .045 * .65 * dx * std::cos(phase),
            weight * .045 * .65 * dy * std::cos(phase),
            weight * .045 * std::sin(phase)
        };
    };
    const auto centre = swell_position(.37, .91);
    const double epsilon = 1e-4;
    const auto left = swell_position(.37 - epsilon, .91),
               right = swell_position(.37 + epsilon, .91);
    const auto bottom = swell_position(.37, .91 - epsilon),
               top = swell_position(.37, .91 + epsilon);
    for (unsigned x = 0; x < width; ++x) {
        unsigned at = x * 4, test = x % 16;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        double expected = 0.;
        if (test < 3) {
            expected = centre[test];
        } else if (test < 6) {
            expected = (right[test - 3] - left[test - 3]) / (2 * epsilon);
        } else if (test < 9) {
            expected = (top[test - 6] - bottom[test - 6]) / (2 * epsilon);
        } else if (test == 10) {
            expected = .30;
        } else if (test == 11) {
            expected = .30 * (.68 + .32 * .4);
        } else if (test == 12) {
            expected =
                .042 * .8 * std::exp(-.3 * .12) * std::sin((6.28318531 / 2.2) * .3 - .43 * 2.);
        } else if (test == 13) {
            expected = .014 * .8 * std::exp(-.3 * .12) * .65
                       * std::cos((6.28318531 / 2.2) * .3 - .43 * 2.);
        }
        CHECK(std::isfinite(actual));
        CHECK(std::abs(actual - expected) < 2e-5);
    }
    // Exercise the production BRDF/absorption helpers on the GPU against analytic values.
    const std::string material = lighting::shaders::water_glsl;
    const auto helper_start = material.find("// Air/water interface");
    const auto helper_end = material.find("vec3 water_direct_specular");
    CHECK(helper_start != std::string::npos && helper_end > helper_start);
    std::string physical =
        "#version 410 core\n" + material.substr(helper_start, helper_end - helper_start);
    physical += R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%8;
    vec3 n=vec3(0,0,1);
    float value=0.;
    if(test==0) value=water_fresnel(1.);
    if(test==1) value=water_fresnel(0.);
    if(test==2) value=water_specular(n,n,n,.5);
    if(test==3) value=water_specular(n,n,-n,.5);
    if(test==4) value=water_transmittance(0.,1.).r;
    if(test==5) value=water_transmittance(2.,1.).r;
    if(test==6) value=water_transmittance(2.,1.).b;
    if(test==7) value=water_transmittance(2.,0.).g;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader physical_test;
    CHECK(physical_test.build("water physical lighting", vertex, physical.c_str(), error));
    physical_test.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const double refracted_cosine = std::sqrt(1. - 1. / (1.333 * 1.333));
    const std::array<double, 8> expected_light{
        .02037,
        1.,
        .02037 / (4. * 3.141592653589793 * .0625),
        0.,
        1.,
        std::exp(-.85 * 2),
        std::exp(-.12 * 2),
        std::exp(-.24 * 2 / refracted_cosine)
    };
    for (unsigned x = 0; x < width; ++x) {
        const auto at = x * 4;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        CHECK(std::isfinite(actual));
        CHECK(std::abs(actual - expected_light[x % 8]) < 2e-5);
    }
    // Smooth slopes must preserve a planar ramp at subcell positions and atlas borders.
    // Poison outside the selected 8x8 body catches cross-body filtering.
    std::vector<Cell> ramp(count, Cell{1000, 0, 0, 0});
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            ramp[(y + 4) * width + x + 4][0] = .1f * float(x) - .05f * float(y);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, width, width, 0, GL_RGBA, GL_FLOAT, ramp.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const auto smooth_start = material.find("// Smooth simulation slopes");
    CHECK(smooth_start != std::string::npos);
    std::string smooth =
        "#version 410 core\nuniform sampler2D water_wave_map;\nuniform vec2 water_atlas_extent;\n"
        + material.substr(smooth_start, helper_start - smooth_start);
    smooth += R"(
layout(location=0) out vec4 result;
uniform int component;
void main() {
    vec2 cell=vec2(.5)+vec2(gl_FragCoord.xy-vec2(.5))*7./15.;
    vec2 slope=water_height_gradient(cell,vec2(4),vec2(8),.5);
    vec3 n=water_smooth_normal(normalize(vec3(-.3,.1,1)),slope);
    uint bits=floatBitsToUint(n[component]);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader smooth_test;
    CHECK(smooth_test.build("water smooth normals", vertex, smooth.c_str(), error));
    smooth_test.use();
    glUniform1i(smooth_test.uniform("water_wave_map"), 0);
    glUniform2f(smooth_test.uniform("water_atlas_extent"), width, width);
    // Authored slope (.3,-.1) plus wave slope (.2,-.1), normalized.
    const std::array<double, 3> expected_normal{
        -.5 / std::sqrt(1.29), .2 / std::sqrt(1.29), 1. / std::sqrt(1.29)
    };
    for (int component = 0; component < 3; ++component) {
        glUniform1i(smooth_test.uniform("component"), component);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        for (unsigned at = 0; at < pixels.size(); at += 4) {
            auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                        | (std::uint32_t(pixels[at + 2]) << 16)
                        | (std::uint32_t(pixels[at + 3]) << 24);
            float value = std::bit_cast<float>(bits);
            CHECK(std::isfinite(value));
            // Bilinear filter weights can have only eight fractional bits; edge
            // differences have unequal spans and do not cancel that quantization.
            CHECK(std::abs(value - expected_normal[unsigned(component)]) < 5e-4);
        }
    }
    // The actual wake kernel must produce two oppositely rotating eddies.
    const std::string solver = lighting::shaders::water_comp;
    const auto wake_start = solver.find("vec2 wake_pair");
    const auto wake_end = solver.find("void main()", wake_start);
    CHECK(wake_start != std::string::npos && wake_end > wake_start);
    std::string wake_source =
        "#version 410 core\n" + solver.substr(wake_start, wake_end - wake_start);
    wake_source += R"(
uniform int component;
layout(location=0) out vec4 result;
void main() {
    vec2 p=vec2(1.2,2)+gl_FragCoord.xy*.25;
    vec2 velocity=wake_pair(p,vec2(4),vec2(1,0),1.);
    uint bits=floatBitsToUint(velocity[component]);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader wake_test;
    CHECK(wake_test.build("counter-rotating water wake", vertex, wake_source.c_str(), error));
    wake_test.use();
    std::vector<Cell> wake_velocity(count);
    for (int c = 0; c < 2; ++c) {
        glUniform1i(wake_test.uniform("component"), c);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        for (unsigned i = 0; i < count; ++i) {
            unsigned at = i * 4;
            auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                        | (std::uint32_t(pixels[at + 2]) << 16)
                        | (std::uint32_t(pixels[at + 3]) << 24);
            wake_velocity[i][unsigned(c)] = std::bit_cast<float>(bits);
        }
    }
    const auto curl_at = [&](unsigned x, unsigned y) {
        unsigned i = y * width + x;
        return (wake_velocity[i + 1][1] - wake_velocity[i - 1][1] - wake_velocity[i + width][0]
                + wake_velocity[i - width][0])
               / .5f;
    };
    CHECK(curl_at(8, 10) > .5f);
    CHECK(curl_at(8, 5) < -.5f);
    // Fine detail: analytic slope, disable, minification, seamless flow-phase
    // resets, mirrored-UV orientation, unit normals and time variation.
    const auto fine_start = material.find("// Fine ripples");
    const auto fine_end = material.find("vec3 effect_material");
    CHECK(fine_start != std::string::npos && fine_end > fine_start);
    const auto combine_start = material.find("vec3 water_smooth_normal");
    std::string fine_source = "#version 410 core\n"
                              + material.substr(combine_start, helper_start - combine_start)
                              + material.substr(fine_start, fine_end - fine_start);
    fine_source += R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%11;
    vec2 p=vec2(.13,.27),axis=vec2(1,0),velocity=vec2(.8,-.2);
    vec2 g=water_fine_gradient(p,vec2(0),axis,0.,vec2(0),vec2(0),1.);
    float value=0.;
    if(test==0) value=g.x;
    if(test==1) value=g.y;
    if(test==2) value=length(water_fine_gradient(p,velocity,axis,.4,vec2(0),vec2(0),0.));
    if(test==3) value=length(water_fine_gradient(p,velocity,axis,.4,vec2(1,0),vec2(0,1),1.));
    if(test==4) value=water_ripple_band(vec2(20,0),vec2(.2,0),vec2(0,.2));
    if(test==5) value=length(water_fine_gradient(p,velocity,axis,1.4999,vec2(0),vec2(0),1.)
                          -water_fine_gradient(p,velocity,axis,1.5001,vec2(0),vec2(0),1.));
    if(test==6) value=length(water_fine_gradient(p,velocity,axis,.7499,vec2(0),vec2(0),1.)
                          -water_fine_gradient(p,velocity,axis,.7501,vec2(0),vec2(0),1.));
    if(test==7) value=dot(water_art_direction(vec2(1,0),vec2(0,1),vec2(1,0),vec2(0,-1),axis),axis);
    if(test==8) value=length(water_smooth_normal(vec3(0,0,1),vec2(.2,-.1)+g));
    if(test==9) value=length(g-water_fine_gradient(p,velocity,axis,.3,vec2(0),vec2(0),1.))>.001?1.:0.;
    if(test==10) value=length(g-water_fine_gradient(p,vec2(0),axis,.3,vec2(0),vec2(0),1.))>.001?1.:0.;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader fine_test;
    CHECK(fine_test.build("fine water normals", vertex, fine_source.c_str(), error));
    fine_test.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const double tau = 6.28318531;
    const std::array<double, 2> k1{.94 * tau / 1.1, .34 * tau / 1.1},
        k2{-.35 * tau / .67, .94 * tau / .67}, bend_k{.61, .93}, envelope_k{.83, -.47};
    const double bend_phase = .13 * bend_k[0] + .27 * bend_k[1];
    const double envelope_phase = .13 * envelope_k[0] + .27 * envelope_k[1];
    const double envelope = .7 + .3 * std::sin(envelope_phase);
    const double phase1 = .13 * k1[0] + .27 * k1[1] + 1.6 * std::sin(bend_phase);
    const double phase2 = .13 * k2[0] + .27 * k2[1] - 1.1 * std::sin(bend_phase) + .8;
    const double height = .014 * std::sin(phase1) + .006 * std::sin(phase2);
    for (unsigned x = 0; x < width; ++x) {
        unsigned at = x * 4;
        auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                    | (std::uint32_t(pixels[at + 2]) << 16) | (std::uint32_t(pixels[at + 3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        const unsigned test = x % 11;
        CHECK(std::isfinite(actual));
        if (test < 2) {
            CHECK(
                std::abs(
                    actual
                    - (envelope
                           * (.014 * (k1[test] + 1.6 * bend_k[test] * std::cos(bend_phase))
                                  * std::cos(phase1)
                              + .006 * (k2[test] - 1.1 * bend_k[test] * std::cos(bend_phase))
                                    * std::cos(phase2))
                       + .3 * envelope_k[test] * std::cos(envelope_phase) * height)
                )
                < 2e-5
            );
        } else if (test <= 4) {
            CHECK(actual == 0);
        } else if (test <= 6) {
            CHECK(actual < 5e-4);
        } else {
            CHECK(std::abs(actual - 1) < 1e-5);
        }
    }
    CHECK(enhanced::water_window_visible(renderer::identity(), {-.5f, -.5f, 0}, {.5f, .5f, 0}));
    CHECK(!enhanced::water_window_visible(renderer::identity(), {3, 0, 0}, {4, 1, 0}));
    CHECK(!enhanced::water_window_visible(renderer::identity(), {-4, 0, 0}, {-3, 1, 0}));
    CHECK(!enhanced::water_window_visible(renderer::identity(), {0, 3, 0}, {1, 4, 0}));
    CHECK(enhanced::water_window_visible(renderer::identity(), {1.1f, 0, 0}, {1.2f, .1f, 0}));

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
    std::vector<Cell> local_coast(count, Cell{513, 8, 0, 0}),
        local_hydraulics(count, Cell{.75f, 0, 0, 1});
    std::vector<Cell> local_state(count);
    for (unsigned i = 0; i < count; ++i) {
        local_state[i] = {float(i) * .0001f, 0, 0, .4f};
    }
    const auto upload = [&](unsigned buffer, const auto& values) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(values.size() * sizeof(Cell)), values.data()
        );
    };
    for (unsigned i = 0; i < count; ++i) {
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
        glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(Cell), nullptr, GL_DYNAMIC_DRAW);
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
    glUniform4f(prepare_window.uniform("window_config"), -4, -4, width, 0);
    glDispatchCompute((count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    upload(buffers[0], local_state);  // Restore controlled history after metadata initialization.
    window_probe.use();
    glUniform1i(window_probe.uniform("local_window"), 1);
    glUniform1i(window_probe.uniform("cell_count"), count);
    glUniform4f(window_probe.uniform("window_config"), -4, -4, width, 0);
    glUniform2f(window_probe.uniform("previous_window"), -5, -4);
    glDispatchCompute((count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    const auto read_local = [&](int channel) {
        read.use();
        glUniform1i(read.uniform("channel"), channel);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
#ifdef OPENRAC_VULKAN
        CHECK(vkgl::copy_buffer_to_texture(image_buffer, texture, width, width));
#else
        glBindBuffer(wgl::GL_PIXEL_UNPACK_BUFFER, image_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, width, GL_RGBA, GL_FLOAT, nullptr);
        glBindBuffer(wgl::GL_PIXEL_UNPACK_BUFFER, 0);
#endif
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    };
    const auto pixel_float = [&](unsigned i) {
        unsigned at = i * 4;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        return std::bit_cast<float>(bits);
    };
    read_local(0);
    for (unsigned i = 0; i < count; ++i) {
        const int wx = int(i % width) - 4, wy = int(i / width) - 4;
        const unsigned slot = unsigned((wy % int(width) + int(width)) % int(width)) * width
                              + unsigned((wx % int(width) + int(width)) % int(width));
        const float expected = i % width == width - 1 ? 0.f : float(slot) * .0001f;
        CHECK(std::abs(pixel_float(i) - expected) < 1e-7);
    }
    read_local(1);
    for (unsigned i = 0; i < count; ++i) {
        const int wx = int(i % width) - 4, wy = int(i / width) - 4;
        const int slot = ((wy % int(width) + int(width)) % int(width)) * int(width)
                         + (wx % int(width) + int(width)) % int(width);
        CHECK(pixel_float(i) == float(slot));
    }
    read_local(2);
    for (unsigned i = 0; i < count; ++i) {
        const int wx = int(i % width) - 4, wy = int(i / width) - 4;
        const int cx = int(std::floor(((float(wx) + .5f) * .25f + 4.f) / .5f));
        const int cy = int(std::floor(((float(wy) + .5f) * .25f + 4.f) / .5f));
        CHECK(pixel_float(i) == float(cy * int(width) + cx));
    }
    // The actual finer-grid solver must preserve a uniform still-water level
    // across the physical wrap seam at the smaller stable timestep.
    // A closed, flat basin isolates conservation from the separately tested sponge.
    upload(local_metadata[0], local_coast);
    std::fill(local_state.begin(), local_state.end(), Cell{.03f, 0, 0, .4f});
    upload(buffers[0], local_state);
    compute.use();
    glUniform1i(compute.uniform("local_window"), 1);
    glUniform4f(compute.uniform("window_config"), -4, -4, width, 0);
    glUniform2f(compute.uniform("previous_window"), -4, -4);
    glUniform1f(compute.uniform("water_dt"), 1.f / 240.f);
    glUniform1i(compute.uniform("reset_waves"), 0);
    glUniform1i(compute.uniform("transport_reference"), 0);
    glUniform4f(compute.uniform("hero"), 0, 0, 0, 0);
    glUniform1i(compute.uniform("hand_count"), 0);
    for (unsigned step = 0; step < 240; ++step) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[step % 2]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1 - step % 2]);
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }
    read_local(0);
    for (unsigned i = 0; i < count; ++i) {
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
    shared_bodies[8][3] = float(count);  // second spans x=0..8
    std::vector<Cell> shared_coast(count * 2, Cell{513, 8, 0, 0});
    std::vector<Cell> shared_hydraulics(count * 2, Cell{.75f, 0, 0, 1});
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
        for (unsigned i = count; i < count * 2; ++i) {
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
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        window_probe.use();
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        read_local(3);
        for (unsigned i = 0; i < count; ++i) {
            const int wx = int(i % width) - 4;
            const float expected =
                wx < 0 ? 513.f : (second_height < 2.f ? 512.f + second_height : 0.f);
            CHECK(std::abs(pixel_float(i) - expected) < 1e-4);
        }
        read_local(0);
        for (unsigned i = 0; i < count; ++i) {
            const int wx = int(i % width) - 4;
            const float expected = wx < 0 || second_height < 2.f ? .07f : 0.f;
            CHECK(std::abs(pixel_float(i) - expected) < 2e-6);
        }
    }
    // A newly wet layer must initialize both states and its reference current.
    for (unsigned i = count; i < count * 2; ++i) {
        shared_hydraulics[i] = {.75f, .2f, -.1f, 1};
    }
    upload(buffers[4], shared_hydraulics);
    std::fill(local_state.begin(), local_state.end(), Cell{.09f, 0, 0, .4f});
    upload(buffers[0], local_state);
    upload(buffers[1], local_state);
    prepare_window.use();
    glUniform4f(prepare_window.uniform("window_config"), -4, -4, width, 1);
    glUniform1f(prepare_window.uniform("water_sim_height"), 4);
    glDispatchCompute((count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    for (unsigned state : {0u, 1u}) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[state]);
        window_probe.use();
        glUniform1i(window_probe.uniform("probe_history"), 1);
        glDispatchCompute((count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        read_local(0);
        for (unsigned i = 0; i < count; ++i) {
            CHECK(std::abs(pixel_float(i)) < 1e-7);
        }
        read_local(1);
        for (unsigned i = 0; i < count; ++i) {
            const float expected = int(i % width) - 4 >= 0 ? .15f : 0.f;
            CHECK(std::abs(pixel_float(i) - expected) < 2e-6);
        }
    }

    // Shared surface sampling: no seam at world zero, no wake on another pool,
    // and a smooth radial fade to the unchanged procedural reference.
    std::fill(local_state.begin(), local_state.end(), Cell{.07f, .2f, -.1f, .6f});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, width, width, 0, GL_RGBA, GL_FLOAT, local_state.data()
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
            glReadPixels(0, 0, width, width, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            for (unsigned i = 0; i < count; ++i) {
                const unsigned test = i % width % 12;
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
            + std::string(lighting::shaders::water_vertex_glsl)
    );
    std::string water_fragment = renderer::shaders::direct_frag;
    water_fragment.insert(water_fragment.find('\n') + 1, "#define OPENRAC_EFFECT_MATERIAL 1\n");
    water_fragment.insert(
        water_fragment.find("void main()"),
        std::string(lighting::shaders::water_window_glsl)
            + std::string(lighting::shaders::water_swell_glsl)
            + std::string(lighting::shaders::probes_glsl) + material
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
    std::vector<Cell> receiver_coast(count, Cell{-513, 1, 0, 0});
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, count * sizeof(Cell), receiver_coast.data(), GL_DYNAMIC_DRAW
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
    receiver_coast[8 * width + 8] = {513, 1, 0, 0};
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
        for (unsigned i = 0; i < count; ++i) {
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
                const int centre_x = std::min(int(node % 5) * 4, int(width) - 1);
                const int centre_y = std::min(int(node / 5) * 4, int(width) - 1);
                int found_x = 0, found_y = 0, closest = 1000;
                for (int y = -2; y <= 2; ++y) {
                    for (int x = -2; x <= 2; ++x) {
                        const int px = centre_x + x, py = centre_y + y;
                        if (px < 0 || py < 0 || px >= int(width) || py >= int(width)) {
                            continue;
                        }
                        if (receiver_coast[py * width + px][0] > .5f && x * x + y * y < closest) {
                            closest = x * x + y * y;
                            found_x = px;
                            found_y = py;
                        }
                    }
                }
                float expected = axis == 3 ? -1.f : 0.f;
                if (closest < 1000) {
                    expected = axis == 0   ? bodies[0][0] + (float(found_x) + .5f) * bodies[4][2]
                               : axis == 1 ? bodies[0][1] + (float(found_y) + .5f) * bodies[4][2]
                               : axis == 2 ? bodies[0][2] + 1.f
                                           : 1.f;
                }
                CHECK(std::abs(pixel_float(node) - expected) < 1e-6f);
            }
        }
    }
    glDeleteTextures(2, cache_textures.data());

    glDeleteBuffers(2, local_metadata.data());
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(5, buffers.data());
    glDeleteBuffers(1, &image_buffer);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
