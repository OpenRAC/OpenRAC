// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe receiver-normal checks: red and blue hemispheres supply the expected front/back colours.
#include "enhanced/tests/probe_receiver_normals.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

#include "enhanced/renderer/gtao.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "scene/level_renderer.h"
#include "tests/check.h"

namespace openrac::test {
namespace {
using lighting::Float3;
using namespace openrac::gl;

// Positive, reversed and absent authored normals remain distinct meshes.
constexpr std::array<float, 3> kCameraNormalSigns{1, -1, 0};

// Packs both windings and all authored normal signs without changing their geometry.
scene::LevelData camera_normal_level() {
    scene::LevelData level;
    level.light_sets.resize(256);
    level.light_sets[0] = 1;
    level.light_sets[6] = -1;
    level.light_sets[10] = 1;
    level.light_sets[14] = 1;
    level.materials.push_back({});
    for (const bool reversed : {false, true}) {
        for (const float sign : kCameraNormalSigns) {
            const auto first_vertex = static_cast<std::uint32_t>(level.vertices.size());
            for (const Float3 p :
                 {Float3{-1, -1, -0.5f}, Float3{1, -1, -0.5f}, Float3{0, 1, -0.5f}}) {
                scene::Vertex vtx{};
                std::copy(p.begin(), p.end(), vtx.position);
                vtx.normal[2] = sign;
                vtx.colour[3] = 1;
                vtx.light_slot = -1;
                level.vertices.push_back(vtx);
            }
            const auto first_index = static_cast<std::uint32_t>(level.indices.size());
            for (const auto index : reversed ? std::array{0u, 2u, 1u} : std::array{0u, 1u, 2u}) {
                level.indices.push_back(first_vertex + index);
            }
            scene::Model model;
            model.primitives.push_back({first_index, 3, 0});
            level.models.push_back(model);
        }
    }
    level.instances[static_cast<std::size_t>(scene::Layer::Terrain)].push_back({});
    return level;
}

// Builds the same red/blue directional illumination for every normal and winding case.
lighting::ProbeVolume camera_normal_volume() {
    lighting::ProbeGrid grid;
    grid.origin = {-2, -2, -2};
    grid.spacing = {4, 4, 4};
    auto volume = lighting::ProbeVolume::allocate(grid);

    for (auto& probe : volume.probes) {
        probe.valid = true;
    }

    for (std::size_t i = 0; i < volume.probes.size(); ++i) {
        for (unsigned y = 0; y < grid.map_resolution; ++y) {
            for (unsigned x = 0; x < grid.map_resolution; ++x) {
                const auto n = lighting::oct_decode(
                    {(x + 0.5f) / grid.map_resolution, (y + 0.5f) / grid.map_resolution}
                );
                volume.irradiance
                    [i * grid.texels_per_probe() + (y + 1) * (grid.map_resolution + 2) + x + 1] =
                    {3.14159265f * std::max(n[2], 0.0f), 0, 3.14159265f * std::max(-n[2], 0.0f)};
            }
        }
    }

    lighting::update_probe_gutters(volume);
    return volume;
}

// Checks both sides of the selected mesh against the directional GI and direct-light lobes.
void check_camera_normal(
    renderer::Renderer& renderer, scene::LevelScene& scene, renderer::FrameBuffer& frame, float sign
) {
    for (bool below : {false, true}) {
        renderer::FrameInput input;
        input.camera.view = renderer::identity();
        input.camera.projection = renderer::identity();

        if (below) {
            input.camera.view[0] = input.camera.view[10] = -1;
            input.camera.view[14] = -1;
        }

        for (int mode : {2, 3}) {
            if (sign == 0 && mode == 3) {
                continue;  // authored direct-light shading is unchanged
            }

            scene.set_probe_view(mode);
            renderer.render(input, {frame.id(), 32, 32});
            const auto pixels = frame.read_rgba();
            const unsigned at = (16 * 32 + 16) * 4;
            CHECK(pixels[at + (below ? 2 : 0)] > 220);
            CHECK(pixels[at + (below ? 0 : 2)] < 10);
            CHECK(pixels[at + 1] < 10);
        }
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

}  // namespace

void test_probe_receiver_normals() {
    const auto level = camera_normal_level();
    renderer::Renderer renderer;
    renderer.set_ambient_occlusion(std::make_unique<renderer::Gtao>());
    scene::LevelScene scene;
    lighting::use_scene_lighting(scene);
    std::string error;
    CHECK(scene.upload(level, renderer.textures(), error));
    CHECK(
        renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Terrain), error)
    );
    CHECK(renderer.init(error));
    renderer::FrameBuffer frame;
    CHECK(frame.create(32, 32, error));
    CHECK(
        lighting::scene_lighting(scene)
            ->set_probe_volume(openrac::test::gpu_probes(camera_normal_volume()), error)
    );
    for (std::size_t model = 0; model < level.models.size(); ++model) {
        scene::Instance instance;
        instance.model = static_cast<std::uint32_t>(model);
        scene.set_instances(scene::Layer::Terrain, {instance});
        check_camera_normal(
            renderer, scene, frame, kCameraNormalSigns[model % kCameraNormalSigns.size()]
        );
    }
    renderer.clear_renderers();
    scene.release();
    frame.release();
}

}  // namespace openrac::test
