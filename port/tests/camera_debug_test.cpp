// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Camera-tool lifecycle and finite clip geometry; no SDL window, GPU or retail data is needed.
#include <array>
#include <cmath>
#include <vector>

#include "game/common/debug_camera.h"
#include "renderer/debug_lines.h"
#include "scene/sun_shadow.h"
#include "tests/check.h"

namespace {
using namespace openrac;

// A known game pose and off-axis perspective make accidental observer/culling coupling visible.
renderer::Camera camera() {
    renderer::Camera result;
    result.position = {7, 9, 3};
    result.view = renderer::look_along(result.position, {1, 0, 0}, {0, 0, 1});
    result.projection = renderer::perspective_off_centre_reversed(-.5f, .7f, .4f, -.6f, .1f, 500);
    return result;
}

// Free flight preserves the lens, holds visibility independently and resets after travel or exit.
void test_camera_lifecycle() {
    frontend::DebugCamera tools;
    frontend::CameraDebugFrame frame;
    renderer::FrameInput input;
    const auto game = camera();
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.camera.view == game.view);
    CHECK(!input.culling_camera);
    frame.settings.enabled = true;
    frame.settings.mode = frontend::CameraDebugMode::CullingInspection;
    tools.apply(input, frame, 1);
    CHECK(input.culling_camera.has_value());
    const auto held = *input.culling_camera;
    frame.controls.forward = true;
    frame.seconds = .05f;
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.camera.position[0] > game.position[0]);
    CHECK(input.camera.view != held.view);
    CHECK(input.camera.projection == game.projection);
    CHECK(input.culling_camera->view == held.view);
    CHECK(input.culling_inspection == 0);
    // A paused game still supplies new displayed-frame controls.
    const auto before = input.camera.position;
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.camera.position[0] > before[0]);
    frame.seconds = 0;
    input.camera = game;
    tools.apply(input, frame, 2);
    CHECK(input.camera.position == game.position);
    CHECK(input.culling_camera->position == game.position);
    frame.settings = {};
    input.camera = game;
    tools.apply(input, frame, 2);
    CHECK(input.camera.view == game.view);
    CHECK(!input.culling_camera);
}

// Inspection tracks a changing game or cutscene pose independently of the freecam.
void test_live_game_culling() {
    frontend::DebugCamera tools;
    frontend::CameraDebugFrame frame;
    renderer::FrameInput input;
    frame.settings.enabled = true;
    input.camera = camera();
    tools.apply(input, frame, 1);
    frame.controls.forward = true;
    frame.seconds = .05f;
    input.camera = camera();
    tools.apply(input, frame, 1);
    frame.settings.mode = frontend::CameraDebugMode::CullingInspection;
    auto game = camera();
    game.position = {30, 40, 50};
    game.view = renderer::look_along(game.position, {0, 1, 0}, {0, 0, 1});
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.culling_camera->position == game.position);
    CHECK(input.culling_camera->view == game.view);
    CHECK(input.camera.position != game.position);
    game.position[0] += 5;
    game.view = renderer::look_along(game.position, {0, -1, 0}, {0, 0, 1});
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.culling_camera->position == game.position);
    CHECK(input.culling_camera->view == game.view);
}

// Each inspection target selects only its bounds while retaining the live game culling camera.
void test_culling_targets() {
    frontend::DebugCamera tools;
    frontend::CameraDebugFrame frame;
    renderer::FrameInput input;
    frame.settings.enabled = true;
    frame.settings.mode = frontend::CameraDebugMode::CullingInspection;
    for (int target = 0; target <= 4; ++target) {
        frame.settings.culling_target = target;
        input.camera = camera();
        tools.apply(input, frame, 1);
        CHECK(input.culling_camera->view == camera().view);
        CHECK(input.culling_inspection == static_cast<unsigned>(target));
    }
}

// The default freecam follows its own culling pose and travels along its pitched view.
void test_default_free_look() {
    frontend::DebugCamera tools;
    frontend::CameraDebugFrame frame;
    renderer::FrameInput input;
    const auto game = camera();
    CHECK(frame.settings.mode == frontend::CameraDebugMode::FreeLook);
    frame.settings.enabled = true;
    frame.controls.look_y = -200;
    input.camera = game;
    tools.apply(input, frame, 1);
    frame.controls = {};
    frame.controls.forward = true;
    frame.seconds = .05f;
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.camera.position[0] > game.position[0]);
    CHECK(input.camera.position[2] > game.position[2]);
    CHECK(!input.culling_camera);
    CHECK(!input.culling_inspection);
    frame.controls = {};
    frame.controls.up = true;
    const auto before_up = input.camera.position;
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.camera.position[2] == before_up[2] + 1);
    CHECK(input.camera.position[0] == before_up[0]);
    CHECK(input.camera.position[1] == before_up[1]);
    frame.settings.mode = frontend::CameraDebugMode::CullingInspection;
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(input.culling_camera.has_value());
    CHECK(input.culling_inspection == 0);
    frame.settings.mode = frontend::CameraDebugMode::FreeLook;
    input.camera = game;
    tools.apply(input, frame, 1);
    CHECK(!input.culling_camera);
    CHECK(!input.culling_inspection);
}

// Diagonal FPS travel retains the same speed, while the viewer's original flight stays available.
void test_normalise_movement() {
    scene::FlyCamera camera;
    scene::FlyControls controls;
    controls.normalise_movement = controls.forward = controls.right = true;
    camera.pitch = .5f;
    camera.update(controls, .05f);
    CHECK(camera.position[2] > 0);
    CHECK(std::abs(renderer::dot(camera.position, camera.position) - 1) < 1e-5f);
    controls.right = false;
    camera.position = {};
    camera.update(controls, .05f);
    const auto ahead = camera.position;
    controls.forward = false;
    controls.back = true;
    camera.update(controls, .05f);
    CHECK(renderer::dot(ahead, ahead) > .99f);
    CHECK(renderer::dot(camera.position, camera.position) < 1e-5f);
    camera.position = {};
    controls.back = false;
    controls.forward = controls.right = true;
    controls.normalise_movement = false;
    camera.update(controls, .05f);
    CHECK(camera.position[2] > 0);
    CHECK(std::abs(renderer::dot(camera.position, camera.position) - 2) < 1e-5f);
}

// Ctrl gives quarter-speed precision movement even when Shift is also held.
void test_slow_movement() {
    scene::FlyCamera camera;
    scene::FlyControls controls;
    controls.forward = controls.slow = true;
    camera.update(controls, .05f);
    CHECK(std::abs(camera.position[0] - .25f) < 1e-5f);
    camera.position = {};
    controls.fast = true;
    camera.update(controls, .05f);
    CHECK(std::abs(camera.position[0] - .25f) < 1e-5f);
}

// Every finite endpoint projects onto a true clip-space corner, including asymmetric lenses.
void check_clip(const renderer::Mat4& clip) {
    std::vector<renderer::DebugLineVertex> lines;
    CHECK(renderer::append_debug_frustum(lines, clip, {1, 1, 0}));
    CHECK(lines.size() == 24);
    for (const auto& vertex : lines) {
        const auto point = renderer::transform_point(clip, vertex.position);
        const float w = clip[3] * vertex.position[0] + clip[7] * vertex.position[1]
                        + clip[11] * vertex.position[2] + clip[15];
        for (float coordinate : point) {
            CHECK(std::abs(std::abs(coordinate / w) - 1) < .002f);
        }
    }
}

// Both camera and snapped orthographic shadow boxes use the same edge convention.
void test_clip_geometry() {
    check_clip(renderer::identity());
    const auto view = camera();
    check_clip(renderer::multiply(view.projection, view.view));
    const auto shadows = scene::sun_cascades(view, {1, 2, -3}, {-100, -100, -20}, {100, 100, 100});
    for (const auto& clip : shadows.matrices) {
        check_clip(clip);
    }
    std::vector<renderer::DebugLineVertex> lines;
    CHECK(!renderer::append_debug_frustum(lines, {}, {1, 1, 0}));
    CHECK(lines.empty());
}
}  // namespace

int main() {
    test_camera_lifecycle();
    test_live_game_culling();
    test_culling_targets();
    test_default_free_look();
    test_normalise_movement();
    test_slow_movement();
    test_clip_geometry();
    return openrac::test::result();
}
