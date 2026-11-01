// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// GPU tile masks and production spot attenuation compared with complete ascending light iteration.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <exception>
#include <limits>
#include <span>
#include <vector>

#include "common/stats.h"
#include "enhanced/renderer/local_lights.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace wgl = renderer::world::gl;
constexpr const char* kVertex = R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})";

// Receiver depth is independent of the opaque depth, as on transparent surfaces.
std::string fragment() {
    return std::string(
               "#version 430 core\nuniform vec4 probe_point_position[8],probe_point_colour[8];\n"
           )
           + lighting::shaders::shading_brdf_glsl + lighting::shaders::local_lights_glsl + R"(
uniform mat4 receiver_clip;
uniform vec3 receiver_eye;
uniform sampler2D receiver_depth_map;
uniform int receiver_opaque;
uniform float receiver_depth;
uniform int channel;
uniform int benchmark_brdf;
layout(location=0) out vec4 result;
void main(){
    vec2 ndc=(gl_FragCoord.xy-forward_view.xy)/forward_view.zw*2.-1.;
    float depth=receiver_opaque!=0?texelFetch(receiver_depth_map,ivec2(gl_FragCoord.xy),0).r:receiver_depth;
    if(receiver_opaque!=0 && depth==0.) {result=vec4(0);return;}
    vec4 h=inverse(receiver_clip)*vec4(ndc,depth*2.-1.,1);vec3 p=h.xyz/h.w;
    vec3 sum=vec3(0);
    LOCAL_LIGHT_LOOP(i) {
        vec4 lp=LOCAL_POSITION(i),lc=LOCAL_COLOUR(i);vec3 delta=lp.xyz-p;
        float distance=length(delta);
        if(lp.w<=0. || distance>=lp.w || distance<=1e-6) continue;
        vec3 radiance=max(lc.rgb,vec3(0))*LOCAL_ATTENUATION(distance,lp.w)*LOCAL_CONE(i,delta/distance);
        float response=1.;
        if(benchmark_brdf!=0) response=max(0.,delta.z/distance)+ggx_specular(vec3(0,0,1),normalize(receiver_eye-p),delta/distance,.35,.04);
        sum+=radiance*response;
    }
    uint bits=floatBitsToUint(sum[channel]);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
}

// Distributed lights include camera crossings, empty tiles, narrow spots and complete overlap.
std::vector<enhanced::LocalLight> lights(unsigned count, bool overlap = false) {
    std::vector<enhanced::LocalLight> result(count);
    for (unsigned i = 0; i < count; ++i) {
        auto& l = result[i];
        l.position = {float(i % 13) * 2 - 12, float((i / 13) % 9) * 2 - 8, -3 - float(i % 7) * 2};
        l.colour = {.11f + float(i % 3) * .03f, .08f, .05f};
        l.range = overlap ? 50.f : 2.3f;
        l.spot = i % 3 == 0;
        l.direction = {.1f, -.2f, -1};
        l.inner_angle = i % 2 ? .01f : .35f;
        l.outer_angle = i % 2 ? .02f : .8f;
        if (overlap) {
            l.position = {0, 0, -2};
            l.spot = false;
        }
    }
    return result;
}

// Device-owned resources are released before the context closes, including failed comparisons.
struct Fixture {
    Fixture() = default;
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;

    // Receiver parameters travel together so camera, depth matching and reference mode stay explicit.
    struct Receiver {
        float distance;
        int channel;
        bool matched, tiled;
    };

    enhanced::LocalLights local;
    renderer::FrameBuffer target;
    renderer::Shader shader;
    unsigned depth = 0, vao = 0;
    std::uint64_t frame = 0;
    int width = 97, height = 81;
    bool independent_receiver = false;  // Models an MSAA surface absent from sample-zero depth.
    std::string error;

    bool open() {
        if (!shader.build("Forward+ correctness", kVertex, fragment(), error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return false;
        }
        glGenTextures(1, &depth);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        return resize(width, height);
    }

    bool resize(int w, int h, int samples = 1) {
        width = w;
        height = h;
        return target.create(w, h, error, samples);
    }

    void upload(const std::vector<enhanced::LocalLight>& source, float scene_depth) {
        CHECK(local.set_lights(source));
        local.prepare(++frame, {}, true);
        std::vector<std::array<float, 4>> values(
            std::size_t(width) * height, {scene_depth, 0, 0, 1}
        );
        // A discontinuous depth range and sky holes exercise conservative reduction and empty tiles.
        if (scene_depth > 0) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width / 3; ++x) {
                    values[std::size_t(y) * width + x][0] = 0;
                }
            }
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, values.data()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void draw(const renderer::Camera& camera, Receiver receiver, std::array<int, 4> viewport) {
        local.reference = !receiver.tiled;
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        shader.use();
        local.bind(shader, &camera, receiver.matched);
        glUniform3f(
            shader.uniform("receiver_eye"),
            camera.position[0],
            camera.position[1],
            camera.position[2]
        );
        const auto clip = renderer::multiply(camera.projection, camera.view);
        glUniformMatrix4fv(shader.uniform("receiver_clip"), 1, GL_FALSE, clip.data());
        glUniform1i(shader.uniform("receiver_depth_map"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth);
        glUniform1i(
            shader.uniform("receiver_opaque"), receiver.matched && !independent_receiver ? 1 : 0
        );
        const auto& p = camera.projection;
        const float depth_value =
            .5f * ((-p[10] * receiver.distance + p[14]) / (-p[11] * receiver.distance + p[15]) + 1);
        glUniform1f(shader.uniform("receiver_depth"), depth_value);
        glUniform1i(shader.uniform("channel"), receiver.channel);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void compare(
        const renderer::Camera& camera, float receiver, bool matched, std::array<int, 4> viewport
    ) {
        for (int channel = 0; channel < 3; ++channel) {
            draw(camera, {receiver, channel, matched, false}, viewport);
            const auto reference = target.read_rgba();
            draw(camera, {receiver, channel, matched, true}, viewport);
            const auto tiled = target.read_rgba();
            CHECK(reference == tiled);
        }
    }

    ~Fixture() {
        local.release();
        shader.release();
        target.release();
        glDeleteTextures(1, &depth);
        glDeleteVertexArrays(1, &vao);
    }
};

// Independent cone arithmetic uses doubles, with a receiver three units below the spotlight apex.
double expected_attenuation(const enhanced::LocalLight& light, double distance) {
    double cone = 1;
    if (light.spot) {
        const double lo = std::cos(double(light.outer_angle)),
                     hi = std::cos(double(light.inner_angle));
        const double cosine = 3 / distance;
        const double t =
            hi == lo ? double(cosine >= lo) : std::clamp((cosine - lo) / (hi - lo), 0., 1.);
        cone = hi == lo ? t : t * t * (3 - 2 * t);
    }
    const double ratio = distance / 10, squared = ratio * ratio;
    const double window = std::max(0., 1 - squared * squared);
    return window * window / (1 + 3.03125 * squared) * cone;
}

// Independent double-precision reference checks range, spot direction and soft/hard cone edges.
void analytical(Fixture& f, const renderer::Camera& camera) {
    for (unsigned mode = 0; mode < 3; ++mode) {
        enhanced::LocalLight light;
        light.position = {0, 0, -2};
        light.range = 10;
        light.spot = mode != 0;
        light.inner_angle = .25f;
        light.outer_angle = mode == 2 ? .25f : .7f;
        f.upload({light}, .01f);
        f.draw(camera, {5, 0, false, false}, {0, 0, f.width, f.height});
        const auto pixels = f.target.read_rgba();
        bool matched = true;
        for (std::size_t pixel = 0; pixel < std::size_t(f.width) * f.height; ++pixel) {
            const auto x = pixel % std::size_t(f.width), y = pixel / std::size_t(f.width);
            const double px = ((double(x) + .5) / f.width * 2 - 1) * 5 / camera.projection[0];
            const double py = ((double(y) + .5) / f.height * 2 - 1) * 5 / camera.projection[5];
            const double expected = expected_attenuation(light, std::sqrt(px * px + py * py + 9));
            const auto at = pixel * 4;
            const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                              | (std::uint32_t(pixels[at + 2]) << 16)
                              | (std::uint32_t(pixels[at + 3]) << 24);
            matched = matched && std::abs(double(std::bit_cast<float>(bits)) - expected) < 2e-5;
        }
        CHECK(matched);
    }
}

// Edits apply at frame boundaries; invalid snapshots preserve data and game-only updates keep authors.
void snapshots(Fixture& f, const renderer::Camera& camera) {
    auto source = lights(1, true);
    f.upload(source, .01f);
    const auto draw = [&] {
        f.draw(camera, {5, 0, false, false}, {0, 0, f.width, f.height});
        return f.target.read_rgba();
    };
    const auto original = draw();
    source[0].colour = {1, 0, 0};
    CHECK(f.local.set_lights(source));
    f.local.prepare(f.frame, {}, true);
    CHECK(draw() == original);
    f.local.prepare(++f.frame, {}, true);
    const auto edited = draw();
    CHECK(edited != original);
    source[0].range = -1;
    CHECK(!f.local.set_lights(source));
    f.local.prepare(++f.frame, {}, true);
    CHECK(draw() == edited);
    std::array<float, 64> game{};
    game[0] = 1;
    game[6] = -2;
    game[7] = 10;
    f.local.prepare(++f.frame, game, true);
    CHECK(draw() != edited);
    game = {};
    f.local.prepare(++f.frame, game, true);
    CHECK(draw() == edited);
    f.local.release();
    f.local.prepare(++f.frame, {}, true);
    const auto empty = draw();
    CHECK(std::all_of(empty.begin(), empty.end(), [](auto x) { return x == 0; }));
}

// The game's eight slots share masks with authored lights, including moves, releases and slot reuse.
void built_in_lights(Fixture& f, const renderer::Camera& camera) {
    std::array<float, 64> game{};
    for (std::size_t i = 0; i < 8; ++i) {
        game[i * 8] = .05f * float(i + 1);
        game[i * 8 + 1] = .04f;
        game[i * 8 + 2] = .02f;
        game[i * 8 + 4] = float(i % 4) - 1.5f;
        const auto row = i / 4;  // Two rows of four built-in lights.
        game[i * 8 + 5] = float(row) - .5f;
        game[i * 8 + 6] = -3 - .25f * float(i);
        game[i * 8 + 7] = 5 + .5f * float(i);
    }
    for (unsigned authors : {0u, 64u}) {
        f.upload(lights(authors), .01f);
        f.local.prepare(++f.frame, game, true);
        f.local.reference = false;
        CHECK(f.local.dispatch(f.depth, camera, f.width, f.height, true));
        f.compare(camera, 10, true, {0, 0, f.width, f.height});
        f.compare(camera, 3, false, {0, 0, f.width, f.height});
        // Move/release live game slots while retaining the same authored table.
        game[4] += 2;
        game[7] = 0;
        game[15] = 0;
        f.local.prepare(++f.frame, game, true);
        f.local.reference = false;
        CHECK(f.local.dispatch(f.depth, camera, f.width, f.height, true));
        f.compare(camera, 3, false, {0, 0, f.width, f.height});
        game[7] = 8;
        game[15] = 6;
        game[0] = .3f;
    }
    CHECK(f.local.set_lights({}));
    f.local.prepare(++f.frame, game, true);
    f.local.reference = false;
    CHECK(!f.local.dispatch(f.depth, camera, f.width, f.height));
    f.compare(camera, 3, false, {0, 0, f.width, f.height});
}

// Authored HDR radiance stays finite and unshadowed, with exactly zero light on a back-facing surface.
void hdr_diffuse(Fixture& f) {
    enhanced::LocalLight light;
    light.position = {0, 0, -2};
    light.range = 10;
    light.colour = {1e6f, 2e5f, 3e5f};
    f.upload({light}, .01f);
    renderer::Shader shader;
    std::string error;
    auto source = lighting::probe_shader_source(R"(#version 430 core
layout(location=0) out vec4 result;
void main(){
 vec3 n=gl_FragCoord.x<1.?vec3(0,0,1):vec3(0,0,-1);
 float value=probe_direct(vec3(0,0,-5),n,vec4(0),0.).r;
 uint bits=floatBitsToUint(value);result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})");
    const bool built = shader.build("Forward+ HDR diffuse", kVertex, source, error);
    CHECK(built);
    if (!built) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    renderer::FrameBuffer target;
    CHECK(target.create(2, 1, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 2, 1);
    shader.use();
    f.local.bind(shader, nullptr, false);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    const auto pixels = target.read_rgba();
    for (unsigned pixel = 0; pixel < 2; ++pixel) {
        const auto at = pixel * 4;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        const auto value = std::bit_cast<float>(bits);
        CHECK(std::isfinite(value));
        CHECK(pixel == 0 ? value > 600000 : value == 0);
    }
    shader.release();
    target.release();
}

// GPU section times are read only after completion, excluding swap/presentation from lighting cost.
std::array<double, 2> gpu_times(const stats::Frame& frame) {
    std::array<double, 2> result{-1, 0};
    for (const auto& section : frame.sections) {
        if (section.name == "local light shading") {
            result[0] = section.gpu_ms;
        }
        if (section.name == "Forward+ light culling") {
            result[1] = std::max(0., section.gpu_ms);
        }
    }
    return result;
}

// Warmed means are reported separately for each alternating rendering path.
double mean(std::span<const double> values) {
    double sum = 0;
    for (double x : values) {
        sum += x;
    }
    return values.empty() ? -1 : sum / double(values.size());
}

// Averages warmed GPU timestamps, alternating the two paths to reduce clock/temperature bias.
void benchmark(Fixture& f, renderer::Camera camera, platform::Window& window) {
    CHECK(f.resize(1280, 720));
    f.shader.use();
    glUniform1i(f.shader.uniform("benchmark_brdf"), 1);
    stats::set_enabled(stats::Client::Dump, true);
    window.swap();
    for (unsigned count : {8u, 16u, 32u, 64u, 256u, 1024u}) {
        auto source = lights(count > 8 ? count - 8 : 0);
        f.upload(source, .01f);
        std::array<std::vector<double>, 2> total, shade, cull;
        for (unsigned sample = 0; sample < 100; ++sample) {
            const unsigned path = sample % 2;
            f.local.reference = path == 0;
            stats::begin_frame(stats::FrameKind::Tick, sample);
            if (path) {
                f.local.dispatch(f.depth, camera, f.width, f.height, true);
            }
            {
                const stats::Scope scope("local light shading", stats::kGpu);
                f.draw(camera, {10, 0, true, path != 0}, {0, 0, f.width, f.height});
            }
            glFinish();
            window.swap();
            stats::begin_frame(stats::FrameKind::Tick, sample + 1);
            const auto* frame = stats::history(0);
            if (sample < 30 || !frame) {
                continue;
            }
            const auto [shading, culling] = gpu_times(*frame);
            if (shading >= 0) {
                total[path].push_back(shading + std::max(0., culling));
                shade[path].push_back(shading);
                cull[path].push_back(std::max(0., culling));
            }
        }
        std::printf(
            "GPU lights=%u: all %.6f ms, tiled %.6f ms (cull %.6f, shade %.6f), samples=%zu/%zu\n",
            count,
            mean(total[0]),
            mean(total[1]),
            mean(cull[1]),
            mean(shade[1]),
            total[0].size(),
            total[1].size()
        );
    }
}

// Validation rejects malformed authoring records transactionally rather than silently dropping lights.
void validation() {
    enhanced::LocalLights local;
    auto source = lights(1);
    CHECK(local.set_lights(source));
    source[0].range = 0;
    CHECK(!local.set_lights(source));
    source = lights(1);
    source[0].position[0] = std::numeric_limits<float>::infinity();
    CHECK(!local.set_lights(source));
    source = lights(1);
    source[0].direction = {0, 0, 0};
    CHECK(!local.set_lights(source));
    source = lights(1);
    source[0].inner_angle = 1;
    source[0].outer_angle = .1f;
    CHECK(!local.set_lights(source));
    CHECK(!local.set_lights(lights(4097)));
}

// Camera, projection and sample-count changes must never consume an incompatible opaque mask.
void check_views(Fixture& f, renderer::Camera& camera) {
    camera.position = {2, 1, 0};
    camera.view = renderer::look_along(camera.position, {-.1f, 0, -1}, {0, 1, 0});
    f.upload(lights(256), 0);
    f.local.reference = false;
    CHECK(f.local.dispatch(f.depth, camera, f.width, f.height));
    f.compare(camera, 2, false, {0, 0, f.width, f.height});
    auto secondary = camera;
    secondary.position = {-2, 0, 0};
    secondary.view = renderer::look_along(secondary.position, {0, 0, -1}, {0, 1, 0});
    f.compare(secondary, 6, true, {0, 0, f.width, f.height});
    // Orthographic projections use the same homogeneous plane tests without perspective assumptions.
    auto ortho = camera;
    ortho.position = {0, 0, 0};
    ortho.view = renderer::identity();
    ortho.projection = {.1f, 0, 0, 0, 0, .1f, 0, 0, 0, 0, .02f, 0, 0, 0, 1, 1};
    f.upload(lights(96), .7f);
    f.local.reference = false;
    CHECK(f.local.dispatch(f.depth, ortho, f.width, f.height));
    f.compare(ortho, 20, true, {0, 0, f.width, f.height});
    f.compare(ortho, 3, false, {0, 0, f.width, f.height});
    // Sample-zero depth is empty, but another MSAA sample can still contain an opaque receiver.
    CHECK(f.resize(64, 48, 4));
    f.independent_receiver = true;
    f.upload(lights(64), 0);
    f.local.reference = false;
    CHECK(f.local.dispatch(f.depth, camera, f.width, f.height));
    f.compare(camera, 3, true, {0, 0, f.width, f.height});
    f.independent_receiver = false;
}

// Runs fixtures while their GPU resources remain inside the live context.
int run(int argc) {
    validation();
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    Fixture f;
    CHECK(f.open());
    if (!f.shader.id()) {
        return test::result();
    }
    renderer::Camera camera;
    camera.view = renderer::identity();
    camera.projection = renderer::perspective_reversed(1, 1, .1f, 100);
    analytical(f, camera);
    snapshots(f, camera);
    hdr_diffuse(f);
    built_in_lights(f, camera);
    for (unsigned count : {0u, 1u, 24u, 25u, 56u, 257u, 4096u}) {
        f.shader.use();
        glUniform1i(f.shader.uniform("benchmark_brdf"), count >= 56 ? 1 : 0);
        for (bool overlap : {false, true}) {
            f.upload(lights(count, overlap), .01f);
            f.local.reference = false;
            f.local.dispatch(f.depth, camera, f.width, f.height);
            f.compare(camera, 10, true, {0, 0, f.width, f.height});
            f.compare(camera, 3, false, {0, 0, f.width, f.height});
            // Different viewport and camera must fall back to all lights without stale mask reads.
            f.compare(camera, 3, false, {3, 5, 73, 51});
        }
    }
    check_views(f, camera);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    CHECK(glGetError() == GL_NO_ERROR);
    if (argc > 1) {
        benchmark(f, camera, context.window());
    }
    std::printf("Forward+ complete-mask comparisons finished\n");
    return test::result();
}

}  // namespace

// Keep context, allocation and shader failures inside the test executable boundary.
int main(int argc, char**) {
    try {
        return run(argc);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Forward+ test: %s\n", error.what());
        return 1;
    }
}
