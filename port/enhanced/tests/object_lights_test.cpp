// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Profile persistence, parent-frame correctness and shared per-instance runtime light additions.
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>

#include "enhanced/lighting/object_lights.h"
#include "scene/object_preview.h"
#include "tests/check.h"

namespace {
using namespace openrac;
namespace fs = std::filesystem;

// Only this test's exclusively created directory is removed, never a caller-supplied path.
struct Fixture {
    fs::path root = fs::temp_directory_path();
    fs::path dir;
    scene::LevelData level;
    scene::Instance instance;
    std::array<renderer::Mat4, 1> palette{};

    Fixture() {
        dir = root
              / ("object-lights-test-"
                 + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        CHECK(fs::create_directory(dir));
        level.game = "rac1";
        level.level = 1;
        level.dir = dir / "levels/level_01";
        fs::create_directories(level.dir);
        scene::Model model;
        model.name = "moby_601.glb";
        model.joints = 1;
        scene::ModelJoint bone;
        bone.name = "joint_000";
        bone.bind = renderer::translation({2, 0, 0});
        model.bones.push_back(bone);
        model.primitives.push_back({0, 3, 0});
        level.models.push_back(model);
        level.materials.emplace_back();
        level.moby_classes[601] = {0, false};

        for (const auto point : std::array<renderer::Vec3, 3>{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}}) {
            scene::Vertex v{};
            std::copy(point.begin(), point.end(), v.position);
            v.normal[2] = 1;
            v.weights[0] = 1;
            std::fill_n(v.colour, 4, 1.f);
            level.vertices.push_back(v);
        }

        level.indices = {0, 1, 2};
        auto pose = renderer::identity();
        pose[0] = 0;
        pose[1] = 1;
        pose[4] = -1;
        pose[5] = 0;
        pose[12] = 3;
        pose[13] = 4;
        palette[0] = renderer::multiply(pose, *renderer::inverse_affine(bone.bind));
        instance.matrix = renderer::translation({10, 20, 30});
        instance.palette = 0;
        instance.temporal_id = 42;
    }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;

    ~Fixture() {
        if (dir.parent_path() == root
            && dir.filename().string().starts_with("object-lights-test-")) {
            std::error_code error;
            fs::remove_all(dir, error);
        }
    }
};

// A rotated translated joint exposes the difference between the pose P and skinning palette P*S.
void check_bones(Fixture& fixture) {
    enhanced::AttachedLight light;
    light.bone = "joint_000";
    light.light.position = {1, 0, 0};
    light.light.direction = {1, 0, 0};
    light.light.range = 5;
    light.light.spot = true;
    auto resolved = enhanced::resolve_attached_light(
        light, fixture.instance, fixture.palette, fixture.level.models[0].bones
    );
    CHECK(resolved);

    if (!resolved) {
        return;
    }

    CHECK((resolved.value().position == renderer::Vec3{13, 25, 30}));
    CHECK((resolved.value().direction == renderer::Vec3{0, 1, 0}));
    CHECK(resolved.value().range == 5);
    auto scaled = fixture.instance;
    scaled.matrix[0] = 2;
    scaled.matrix[5] = .5f;
    resolved = enhanced::resolve_attached_light(
        light, scaled, fixture.palette, fixture.level.models[0].bones
    );
    CHECK(resolved && (resolved.value().position == renderer::Vec3{16, 22.5f, 30}));
    CHECK(resolved && resolved.value().range == 5);
    light.bone = "missing";
    CHECK(!enhanced::resolve_attached_light(
        light, fixture.instance, fixture.palette, fixture.level.models[0].bones
    ));
    light.bone = "joint_000";
    CHECK(!enhanced::resolve_attached_light(
        light, fixture.instance, {}, fixture.level.models[0].bones
    ));
    scene::PickHit hit;
    hit.layer = scene::Layer::Mobys;
    auto mesh = scene::preview_mesh(fixture.level, hit, fixture.instance, fixture.palette);
    CHECK(mesh);

    if (!mesh) {
        return;
    }

    const auto frame = scene::preview_attachment(mesh.value(), 0);
    CHECK(frame);
    resolved = enhanced::resolve_attached_light(
        light, fixture.instance, fixture.palette, fixture.level.models[0].bones
    );
    const auto preview = renderer::transform_point(*frame, light.light.position);

    for (std::size_t c = 0; c < 3; ++c) {
        const float expected =
            (resolved.value().position[c] - fixture.instance.matrix[12 + c] - mesh.value().pivot[c])
            / mesh.value().radius;
        CHECK(std::abs(preview[c] - expected) < 1e-5f);
    }
}

// Save replaces an existing file atomically and preserves unrelated object records and escaped names.
void check_io(Fixture& fixture) {
    const auto path = fixture.level.dir / "object-lights.json";
    enhanced::ObjectLightCatalog catalog;
    enhanced::AttachedLight light;
    light.name = "lamp\n\"quoted\"";
    light.bone = "joint_000";
    light.light.spot = true;
    light.light.intensity = 3.25f;
    light.light.colour = {.2f, .5f, 1};
    const auto key = enhanced::object_light_key(fixture.level, scene::Layer::Mobys, 0);
    catalog.objects[key] = {light};
    catalog.objects["ties/other.glb"] = {};
    const auto saved = enhanced::write_object_lights(path, catalog);
    CHECK(saved);

    if (!saved) {
        std::fprintf(stderr, "save: %s\n", saved.error().message.c_str());
        return;
    }

    auto loaded = enhanced::read_object_lights(path);
    CHECK(loaded);
    CHECK(loaded && loaded.value().objects == catalog.objects);
    catalog.objects[key][0].light.intensity = 7;
    CHECK(enhanced::write_object_lights(path, catalog));
    loaded = enhanced::read_object_lights(path);
    CHECK(loaded && loaded.value().objects.at(key)[0].light.intensity == 7);
    CHECK(loaded && loaded.value().objects.contains("ties/other.glb"));
    const auto previous = loaded.value().objects;
    catalog.objects[key][0].light.range = -1;
    CHECK(!enhanced::write_object_lights(path, catalog));
    loaded = enhanced::read_object_lights(path);
    CHECK(loaded && loaded.value().objects == previous);
    CHECK(enhanced::read_object_lights(fixture.dir / "missing.json"));
    CHECK(!enhanced::read_object_lights(fixture.dir));
    CHECK(!enhanced::write_object_lights(fixture.level.dir, enhanced::ObjectLightCatalog{}));
}

// Invalid settings cannot quietly turn into a different default light or an invalid rewritten file.
void check_invalid(Fixture& fixture) {
    const auto path = fixture.dir / "invalid.json";

    for (const auto text :
         {"{",
          R"({"version":2,"objects":{}})",
          R"({"version":1,"objects":{"a":{}}})",
          R"({"version":1,"objects":{"a":[{"range":0}]}})",
          R"({"version":1,"objects":{"a":[{"type":"area"}]}})",
          R"({"version":1,"objects":{"a":[{"type":"spot","direction":[0,0,0]}]}})",
          R"({"version":1,"objects":{"a":[{"colour":[-1,1,1]}]}})",
          R"({"version":1,"objects":{"a":[{"color":[1,0,0]}]}})",
          R"({"version":1,"objects":{"a":[{"position":[1,2]}]}})",
          R"({"version":1,"objects":{"a":[{"intensity":"3"}]}})"}) {
        std::ofstream(path) << text;
        CHECK(!enhanced::read_object_lights(path));
    }

    enhanced::ObjectLightCatalog catalog;
    enhanced::AttachedLight light;
    light.light.intensity = std::numeric_limits<float>::infinity();
    catalog.objects["a"] = {light};
    CHECK(!enhanced::write_object_lights(path, catalog));
    catalog.objects["a"].assign(4097, enhanced::AttachedLight{});
    CHECK(!enhanced::write_object_lights(path, catalog));
}

// Shared type lights are instanced even for offscreen emitters and follow the displayed bone palette.
void check_instances(Fixture& fixture) {
    enhanced::ObjectLightCatalog catalog;
    enhanced::AttachedLight root, bone;
    root.light.position = {1, 0, 0};
    bone = root;
    bone.bone = "joint_000";
    const auto key = enhanced::object_light_key(fixture.level, scene::Layer::Mobys, 0);
    catalog.objects[key] = {root, bone};
    CHECK(enhanced::write_object_lights(fixture.level.dir / "object-lights.json", catalog));
    enhanced::ObjectLights runtime;
    runtime.load(fixture.level);
    std::array<scene::Instance, 3> instances{fixture.instance, fixture.instance, fixture.instance};
    instances[0].lighting_only = true;
    instances[1].matrix[12] += 10;
    instances[2].moby_light[3] = 2;
    runtime.instances(scene::Layer::Mobys, instances);
    auto lights = runtime.prepare(1, fixture.palette);
    CHECK(lights.size() == 4);
    CHECK(lights.size() == 4 && (lights[0].position == renderer::Vec3{11, 20, 30}));
    CHECK(lights.size() == 4 && (lights[1].position == renderer::Vec3{13, 25, 30}));
    CHECK(lights.size() == 4 && lights[2].position[0] == 21);
    auto palette = fixture.palette;
    palette[0][13] += 2;
    lights = runtime.prepare(2, palette);
    CHECK(lights.size() == 4 && lights[1].position[1] == 27);
    // Deleting a profile entry invalidates old bindings without dangling-instance map accesses.
    enhanced::publish_object_lights(fixture.level.game, {});
    CHECK(runtime.prepare(3, palette).empty());
    runtime.release();
}

// Fly look preserves the eye, orbit preserves the focus, and tiny uniform scales remain invertible.
void check_camera() {
    scene::PreviewOrbit camera;
    const auto eye = camera.camera(1).position;
    camera.navigate(20, 10, true);
    const auto turned = camera.camera(1).position;

    for (unsigned c = 0; c < 3; ++c) {
        CHECK(std::abs(eye[c] - turned[c]) < 1e-5f);
    }

    const auto focus = camera.focus;
    camera.navigate(20, -10, false);
    CHECK(camera.focus == focus);
    camera.travel({1, 0, 0});
    CHECK(camera.focus != focus);
    const auto small = renderer::scale(.00001f);
    const auto inverse = renderer::inverse_affine(small);
    CHECK(inverse);
    const auto point =
        renderer::transform_point(*inverse, renderer::transform_point(small, {1, 2, 3}));

    for (unsigned c = 0; c < 3; ++c) {
        CHECK(std::abs(point[c] - float(c + 1)) < 1e-5f);
    }

    CHECK(!renderer::inverse_affine(renderer::scale(0)));
}
}  // namespace

int main() {
    try {
        Fixture fixture;
        check_bones(fixture);
        check_io(fixture);
        check_invalid(fixture);
        check_instances(fixture);
        check_camera();
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "object lights: %s\n", error.what());
        return 1;
    }
}
