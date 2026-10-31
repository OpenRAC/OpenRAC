// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Smooth normal transport through real depth, early-lighting and mesh passes.
// Opposite red/blue lobes distinguish shading normals from the flat receiver plane.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include "common/stats.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/tests/prepass_capture.h"
#include "enhanced/viewer/early_lighting.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;

// Builds a flat quad with smoothly varying normals, optional skinning and a cutout material.
scene::LevelData smooth_level(float sign, bool skinned, bool cutout) {
    scene::LevelData level;
    level.light_sets.resize(256);
    // Red arrives from +X, blue from -X, so a flat +Z normal receives neither.
    level.light_sets[0] = 1;
    level.light_sets[10] = 1;
    level.light_sets[4] = -1;
    level.light_sets[12] = 1;

    for (const renderer::Vec3 p : {renderer::Vec3{-4, -4, 0}, {4, -4, 0}, {4, 4, 0}, {-4, 4, 0}}) {
        scene::Vertex vertex{};
        std::copy(p.begin(), p.end(), vertex.position);
        vertex.normal[0] = p[0] * .2f * sign;
        vertex.normal[2] = .6f * sign;
        std::fill(std::begin(vertex.colour), std::end(vertex.colour), 1.0f);
        vertex.light_slot = -1;
        vertex.uv[0] = (p[0] + 4.f) * .125f;
        vertex.uv[1] = (p[1] + 4.f) * .125f;
        vertex.weights[0] = 1;
        level.vertices.push_back(vertex);
    }

    level.indices = {0, 1, 2, 0, 2, 3};
    level.materials.push_back({});
    level.materials[0].cutout = cutout;
    if (cutout) {
        renderer::Rgba8Image image(4, 4);
        for (unsigned y = 0; y < 4; ++y) {
            for (unsigned x = 0; x < 4; ++x) {
                image.set_texel(
                    x, y, ((x == 0 && y == 0) || (x == 3 && y == 3)) ? 0x00ffffffu : 0xffffffffu
                );
            }
        }
        level.images.push_back(std::move(image));
        level.image_paths.push_back("prepass-cutout.png");
        level.translucent_images.push_back(true);
        level.materials[0].image = 0;
    }
    scene::Model model;
    model.primitives.push_back({0, 6, 0});
    model.min = {-4, -4, 0};
    model.max = {4, 4, 0};
    model.joints = skinned ? 1 : 0;
    level.models.push_back(model);
    return level;
}

// Creates a spatially constant probe field with opposite red/blue directional lobes.
lighting::ProbeVolume directional_probes() {
    lighting::ProbeGrid grid;
    grid.origin = {-16, -16, -16};
    grid.spacing = {32, 32, 32};
    auto volume = lighting::ProbeVolume::allocate(grid);

    for (auto& probe : volume.probes) {
        probe.valid = true;
    }

    for (auto& moments : volume.distance_moments) {
        moments = {1024, 1024 * 1024};
    }

    for (std::size_t probe = 0; probe < volume.probes.size(); ++probe) {
        for (unsigned y = 0; y < grid.map_resolution; ++y) {
            for (unsigned x = 0; x < grid.map_resolution; ++x) {
                const auto n = lighting::oct_decode(
                    {(static_cast<float>(x) + .5f) / static_cast<float>(grid.map_resolution),
                     (static_cast<float>(y) + .5f) / static_cast<float>(grid.map_resolution)}
                );
                const std::size_t texel =
                    probe * grid.texels_per_probe()
                    + (static_cast<std::size_t>(y) + 1) * (grid.map_resolution + 2) + x + 1;
                volume.irradiance[texel] =
                    {std::numbers::pi_v<float> * std::max(n[0], 0.0f),
                     0,
                     std::numbers::pi_v<float> * std::max(-n[0], 0.0f)};
            }
        }
    }

    lighting::update_probe_gutters(volume);
    return test::gpu_probes(volume);
}

// Checks opposite colour lobes on one plane, or their equal support for a missing normal.
void check_smooth_colour(const std::vector<std::uint8_t>& pixels, float sign) {
    // Both samples share one geometric plane; only the smooth normal changes their colour.
    const std::size_t left = (std::size_t{32} * 64 + 20) * 4;
    const std::size_t right = (std::size_t{32} * 64 + 44) * 4;

    // Missing normals use the neutral fallback, with equal red/blue support on this plane.
    if (sign == 0) {
        CHECK(std::abs(static_cast<int>(pixels[left]) - static_cast<int>(pixels[left + 2])) <= 2);
        CHECK(std::abs(static_cast<int>(pixels[right]) - static_cast<int>(pixels[right + 2])) <= 2);
    } else {
        CHECK(pixels[left + 2] > pixels[left] + 25);
        CHECK(pixels[right] > pixels[right + 2] + 25);
    }

    CHECK(pixels[left + 1] < 5 && pixels[right + 1] < 5);
}

// Checks colour direction for GI and direct light through the real normal/depth and colour passes.
void test_smooth_normals(float sign, bool skinned, bool cutout, int samples) {
    renderer::Renderer renderer;
    scene::LevelScene scene;
    lighting::use_scene_lighting(scene);
    std::string error;
    const bool uploaded =
        scene.upload(smooth_level(sign, skinned, cutout), renderer.textures(), error);
    CHECK(uploaded);
    if (!uploaded) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    CHECK(lighting::scene_lighting(scene)->set_probe_volume(directional_probes(), error));
    const auto layer = skinned ? scene::Layer::Mobys : scene::Layer::Terrain;
    scene::Instance instance;
    // Nonuniform scale must use the same normal transform in both raster passes.
    instance.matrix[0] = .9f;
    instance.matrix[10] = 1.2f;

    // A nonidentity joint exercises skinned position and normal transforms.
    if (skinned) {
        instance.palette = 0;
        auto pose = renderer::identity();
        if (sign != 0) {
            pose[0] = pose[10] = .98f;
            pose[8] = .15f;
            pose[2] = -.15f;
        }
        pose[14] = .2f;
        scene.set_palette({pose});
    }

    scene.set_instances(layer, {instance});
    if (skinned) {
        // Terrain owns the shared early-lighting/prepass request, even in a moby-only fixture.
        CHECK(renderer.add(
            std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Terrain), error
        ));
    }
    CHECK(renderer.add(std::make_unique<scene::LayerRenderer>(scene, layer), error));
    auto observer = std::make_unique<test::PrepassCapture>();
    auto* capture = observer.get();
    CHECK(renderer.add(std::move(observer), error));
    CHECK(renderer.init(error));
    renderer::FrameBuffer frame;
    CHECK(frame.create(64, 64, error, samples, samples == 1));
    renderer::FrameInput input;
    input.camera.view = renderer::translation({0, 0, -8});
    input.camera.projection = renderer::perspective_reversed(1, 1, .1f, 100);

    for (const int mode : {2, 3}) {
        scene.set_probe_view(mode);
        renderer::render_opt::set(renderer::render_opt::Opt::PrepassRigid, false);
        renderer.render(input, {frame.id(), 64, 64});
        const auto reference = frame.read_rgba(), normals = capture->normals,
                   depth = capture->depths;
        CHECK(!normals.empty());
        CHECK(samples != 1 || !depth.empty());
        renderer::render_opt::set(renderer::render_opt::Opt::PrepassRigid, true);
        renderer.render(input, {frame.id(), 64, 64});
        CHECK(frame.read_rgba() == reference);
        CHECK(capture->normals == normals && capture->depths == depth);
        check_smooth_colour(frame.read_rgba(), sign);
    }

    if (sign != 0 && !cutout) {
        // Close surfaces have tiny position derivatives, especially at gameplay resolutions.
        // Their smooth normals must remain valid rather than becoming the neutral fallback.
        for (float distance : {.03f, .1f, 1.f}) {
            input.camera.view = renderer::translation({0, 0, -(skinned ? .24f : 0.f) - distance});
            input.camera.projection = renderer::perspective_reversed(1, 1, .001f, 100);
            renderer.render(input, {frame.id(), 64, 64});
            CHECK(!capture->normals.empty());
            if (!capture->normals.empty()) {
                constexpr std::size_t centre = (32 * 64 + 32) * 4;
                CHECK(capture->normals[centre + 3] > 250);
                CHECK(capture->normals[centre + 2] > 220);
            }
        }
    }

    CHECK(gl::glGetError() == gl::GL_NO_ERROR);
    renderer.release();
    scene.release();
    frame.release();
}

// Exercises both normal signs and missing data for every static/skinned material variant.
void test_mesh_variants(int samples) {
    for (const bool skinned : {false, true}) {
        for (const bool cutout : {false, true}) {
            test_smooth_normals(1, skinned, cutout, samples);
            test_smooth_normals(-1, skinned, cutout, samples);
            test_smooth_normals(0, skinned, cutout, samples);
        }
    }
}

// A depth callback requests the plain prepass without GI or a normal attachment.
void test_depth_only() {
    stats::set_enabled(stats::Client::Dump, true);
    std::uint64_t stamp = 0;
    for (bool cutout : {false, true}) {
        renderer::Renderer renderer;
        scene::LevelScene scene;
        lighting::use_scene_lighting(scene);
        std::string error;
        const bool uploaded =
            scene.upload(smooth_level(1, false, cutout), renderer.textures(), error);
        CHECK(uploaded);
        if (!uploaded) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return;
        }
        scene::Instance instance;
        instance.matrix[0] = .9f;
        instance.matrix[10] = 1.2f;
        scene.set_instances(scene::Layer::Terrain, {instance});
        CHECK(renderer.add(
            std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Terrain), error
        ));
        auto observer = std::make_unique<test::PrepassCapture>();
        auto* capture = observer.get();
        CHECK(renderer.add(std::move(observer), error));
        CHECK(renderer.init(error));
        renderer::FrameBuffer frame;
        CHECK(frame.create(67, 53, error, 1, true));
        renderer::RenderTarget target{frame.id(), 67, 53};
        target.depth_texture = frame.depth_texture();
        renderer::FrameInput input;
        input.camera.view = renderer::translation({0, 0, -8});
        input.camera.projection = renderer::perspective_reversed(1, 67.f / 53.f, .1f, 100);
        input.camera.projection[8] = .13f;
        input.after_world_depth = [](unsigned depth, int, int) {
            CHECK(depth != 0);
        };
        std::vector<std::uint8_t> pixels, depths;
        for (bool rigid : {false, true}) {
            renderer::render_opt::set(renderer::render_opt::Opt::PrepassRigid, rigid);
            stats::begin_frame(stats::FrameKind::Tick, ++stamp);
            renderer.render(input, target);
            stats::begin_frame(stats::FrameKind::Tick, ++stamp);
            const auto actual = frame.read_rgba();
            CHECK(capture->normals.empty() && !capture->depths.empty());
            if (!rigid) {
                pixels = actual;
                depths = capture->depths;
            } else {
                CHECK(actual == pixels && capture->depths == depths);
            }
            const auto* measured = stats::history(0);
            CHECK(measured != nullptr);
            if (measured) {
                const auto at = std::find_if(
                    measured->gauges.begin(), measured->gauges.end(), [](const auto& g) {
                        return g.first == "prepass rigid draws terrain";
                    }
                );
                CHECK(at != measured->gauges.end() && (rigid ? at->second > 0 : at->second == 0));
            }
        }
        renderer.release();
        scene.release();
        frame.release();
    }
    stats::set_enabled(stats::Client::Dump, false);
}
}  // namespace

// Runs static/skinned, cutout, reversed-normal and MSAA cases on the enhanced backend.
int main() {
    try {
        test::GlContext context;
        std::string why;

        // Compatibility backends cannot exercise the enhanced compute path.
        if (!context.open(why) || !lighting::EarlyLighting::supported()) {
            return test::kSkip;
        }

        for (const int samples : {1, 4}) {
            test_mesh_variants(samples);
        }

        test_depth_only();
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "%s\n", failure.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
