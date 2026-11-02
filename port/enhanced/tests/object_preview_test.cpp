// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Owned pose extraction, stable selection and live point/spot lighting in the isolated studio.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <utility>

#include "enhanced/viewer/object_preview_lighting.h"
#include "renderer/gl.h"
#include "renderer/vulkan/vkgl.h"
#include "scene/object_preview.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;

// A textured, skinned square with a second material, independent of retail game assets.
scene::LevelData fixture() {
    scene::LevelData level;

    for (const auto p :
         std::array<renderer::Vec3, 4>{{{-1, 0, -1}, {1, 0, -1}, {1, 0, 1}, {-1, 0, 1}}}) {
        scene::Vertex v{};
        std::copy(p.begin(), p.end(), v.position);
        v.normal[1] = -1;
        v.uv[0] = (p[0] + 1) * .5f;
        v.uv[1] = (p[2] + 1) * .5f;
        std::fill_n(v.colour, 4, 1.f);
        v.weights[0] = 1;
        level.vertices.push_back(v);
    }

    level.indices = {0, 1, 2, 0, 2, 3};
    scene::Material material;
    material.image = 0;
    material.cutout = true;
    level.materials.push_back(material);
    level.materials.push_back(material);
    scene::Model model;
    model.primitives.push_back({0, 3, 0});
    model.primitives.push_back({3, 3, 1});
    model.joints = 1;
    level.models.push_back(model);
    renderer::Rgba8Image image(2, 2);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            image.set_texel(x, y, renderer::rgba(180, 100, 60, 255));
        }
    }

    level.images.push_back(image);
    return level;
}

// Failures preserve the last good pose; snapshots own only referenced data and survive source edits.
void check_snapshot(
    scene::LevelData& level,
    const scene::PickHit& hit,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
) {
    auto mesh = scene::preview_mesh(level, hit, instance, palette);
    CHECK(mesh);

    if (!mesh) {
        return;
    }

    CHECK(mesh.value().source.size() == 6);
    CHECK(mesh.value().parts.size() == 2);
    CHECK(mesh.value().images.size() == 1);
    CHECK(std::abs(mesh.value().radius - std::sqrt(2.f)) < 1e-5f);
    const auto vertices = mesh.value().vertices;
    const auto invalid = scene::preview_pose(mesh.value(), instance, {});
    CHECK(!invalid);
    CHECK(mesh.value().vertices[0].position[0] == vertices[0].position[0]);
    auto moved = instance;
    moved.tint[0] = .25f;
    moved.matrix[12] += 100;
    CHECK(scene::preview_pose(mesh.value(), moved, palette));
    CHECK(mesh.value().vertices[0].colour[0] == .25f);
    CHECK(mesh.value().vertices[0].position[0] == vertices[0].position[0]);
    CHECK(mesh.value().vertices[0].colour[0] == .25f);
    auto changed = palette[0];
    changed[14] = 1;
    CHECK(scene::preview_pose(mesh.value(), instance, std::span(&changed, 1)));
    CHECK(mesh.value().vertices[0].position[2] > vertices[0].position[2] + .7f);
    auto terrain = hit;
    terrain.layer = scene::Layer::Terrain;
    terrain.primitive = 1;
    const auto fragment = scene::preview_mesh(level, terrain, instance, palette);
    CHECK(fragment && fragment.value().source.size() == 3);
    auto bad = hit;
    bad.model = 100;
    CHECK(!scene::preview_mesh(level, bad, instance, palette));
    std::array<scene::Instance, 2> live{instance, instance};
    live[0].temporal_id = 99;
    live[1].temporal_id = hit.identity;
    CHECK(scene::preview_instance(level, hit, live) == &live[1]);
    CHECK(scene::preview_instance(level, hit, std::span(live.data(), 1)) == nullptr);
    const auto old_texel = mesh.value().images[0].texel(0, 0);
    level.images[0].set_texel(0, 0, 0);
    CHECK(mesh.value().images[0].texel(0, 0) == old_texel);
    level.images[0].set_texel(0, 0, old_texel);
}

// Bind pose removes animation skinning while restoring the corresponding attachment bone frame.
void test_bind_pose() {
    auto level = fixture();
    level.models[0].bones = {{"joint_000", renderer::translation({2, 0, 0}), -1}};
    scene::Instance instance;
    instance.palette = 0;
    const auto animated = renderer::translation({0, 0, 1});
    scene::PickHit hit;
    hit.layer = scene::Layer::Mobys;
    hit.model = 0;
    auto mesh = scene::preview_mesh(level, hit, instance, std::span(&animated, 1));
    CHECK(mesh);

    if (!mesh) {
        return;
    }

    const auto pivot = mesh.value().pivot;
    const float radius = mesh.value().radius;
    scene::ObjectPreview preview;
    CHECK(preview.select(std::move(mesh.value())));
    CHECK(preview.bind_pose());
    const auto& rest = preview.mesh();

    for (unsigned c = 0; c < 3; ++c) {
        const float expected = (level.vertices[0].position[c] - pivot[c]) / radius;
        CHECK(std::abs(rest.vertices[0].position[c] - expected) < 1e-5f);
    }

    CHECK(rest.bone_pose[0] == level.models[0].bones[0].bind);
    const auto attachment = scene::preview_attachment(rest, 0);
    CHECK(attachment);

    if (attachment) {
        const auto point = renderer::transform_point(*attachment, {0, 0, 0});
        CHECK(std::abs(point[0] - (2 - pivot[0]) / radius) < 1e-5f);
    }

    CHECK(preview.pose(instance, std::span(&animated, 1)));
    CHECK(
        std::abs(
            preview.mesh().vertices[0].position[2]
            - (level.vertices[0].position[2] + 1 - pivot[2]) / radius
        )
        < 1e-5f
    );
}

// Readback forces completion; average encoded brightness is sufficient for an on/off illumination check.
double brightness(const scene::ObjectPreview& preview) {
    const auto pixels = preview.target().read_rgba();
    double sum = 0;

    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        sum += pixels[i] + pixels[i + 1] + pixels[i + 2];
    }

    return sum * 4.0 / double(pixels.size());
}

// The optional provider is the only part of this general viewer that knows about Forward+.
Result<void> select(scene::ObjectPreview& preview, scene::PreviewMesh mesh) {
    std::unique_ptr<enhanced::ObjectPreviewLighting> lighting;

    if (gl::enhanced_rendering_available()) {
        lighting = std::make_unique<enhanced::ObjectPreviewLighting>();
    }

    return preview.select(std::move(mesh), std::move(lighting));
}

Result<void> draw(
    scene::ObjectPreview& preview,
    int width,
    int height,
    const scene::PreviewOrbit& orbit,
    const enhanced::LocalLight& light
) {
    auto* provider = dynamic_cast<enhanced::ObjectPreviewLighting*>(preview.lighting());

    if (provider) {
        const auto result = provider->set_lights(std::span(&light, 1));

        if (!result) {
            return result;
        }
    }

    return preview.draw(width, height, orbit);
}

// Partial-alpha cutout edges use the same opaque/transparent split as the world renderer.
void check_cutout(
    const scene::LevelData& level,
    const scene::PickHit& hit,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
) {
    // Enhanced material checks run only when the optional lighting provider is supported.
    if (!gl::enhanced_rendering_available()) {
        return;
    }

    scene::ObjectPreview preview;
    scene::PreviewOrbit orbit;
    orbit.yaw = -1.5707963f;
    orbit.pitch = 0;
    enhanced::LocalLight light;
    light.position = {0, -2, 0};
    light.range = 8;
    light.intensity = 2;
    std::array<double, 3> values{};
    const std::array<std::uint8_t, 3> alphas{0, 128, 255};

    for (std::size_t i = 0; i < alphas.size(); ++i) {
        auto mesh = scene::preview_mesh(level, hit, instance, palette);
        CHECK(mesh);

        if (!mesh) {
            return;
        }

        for (auto& image : mesh.value().images) {
            for (std::size_t j = 3; j < image.pixels.size(); j += 4) {
                image.pixels[j] = alphas[i];
            }
        }

        CHECK(select(preview, std::move(mesh.value())));
        CHECK(draw(preview, 128, 128, orbit, light));
        values[i] = brightness(preview);
    }

    CHECK(values[0] < 27);
    CHECK(values[1] > values[0] + 10);
    CHECK(values[2] > values[1] + 10);
}

// Point and spot edits must use the same production attenuation and fully suppress an away-facing cone.
std::pair<scene::PreviewOrbit, enhanced::LocalLight> check_light_types(
    scene::ObjectPreview& preview
) {
    scene::PreviewOrbit orbit;
    orbit.yaw = -1.5707963f;
    orbit.pitch = 0;
    enhanced::LocalLight light;
    light.position = {0, -2, 0};
    light.range = 8;
    light.intensity = 0;
    CHECK(draw(preview, 128, 128, orbit, light));
    const double dark = brightness(preview);
    light.intensity = 2;
    CHECK(draw(preview, 128, 128, orbit, light));
    const double point = brightness(preview);
    CHECK(point > dark + 15);
    light.spot = true;
    light.direction = {0, -1, 0};
    CHECK(draw(preview, 128, 128, orbit, light));
    CHECK(std::abs(brightness(preview) - dark) < .1);
    light.direction = {0, 1, 0};
    light.inner_angle = .6f;
    light.outer_angle = .8f;
    CHECK(draw(preview, 128, 128, orbit, light));
    CHECK(std::abs(brightness(preview) - point) < .5);
    return {orbit, light};
}

// Exercise both light types, texture coverage, target replacement and safe resource reuse under validation.
void check_gpu(
    const scene::LevelData& level,
    const scene::PickHit& hit,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
) {
    scene::ObjectPreview preview;
    auto mesh = scene::preview_mesh(level, hit, instance, palette);
    CHECK(mesh);

    if (!mesh) {
        return;
    }

    // Off/classic paths reject the preview without allocating a target or compiling shaders.
    if (!gl::enhanced_rendering_available()) {
        CHECK(select(preview, std::move(mesh.value())));
        CHECK(preview.lighting() == nullptr);
        CHECK(preview.draw(128, 128, {}));
        CHECK(brightness(preview) > 20);
        return;
    }

    CHECK(select(preview, std::move(mesh.value())));
    auto [orbit, light] = check_light_types(preview);
    const auto frozen = preview.target().read_rgba();
    // Source palette changes alone cannot mutate a frozen, GPU-owned pose.
    auto moved = palette[0];
    moved[14] = 1;
    CHECK(draw(preview, 128, 128, orbit, light));
    CHECK(preview.target().read_rgba() == frozen);
    CHECK(preview.pose(instance, std::span(&moved, 1)));
    CHECK(draw(preview, 128, 128, orbit, light));
    CHECK(preview.target().read_rgba() != frozen);
    CHECK(draw(preview, 220, 140, orbit, light));
    CHECK(preview.target().width() == 220 && preview.target().height() == 140);
    orbit.move(10000, 10000, 10000);
    CHECK(orbit.pitch < 1.5f && orbit.distance >= 1.05f);
    CHECK(draw(preview, 128, 128, orbit, light));
    preview.release();
    preview.release();
    CHECK(preview.texture() == 0);
    CHECK(!draw(preview, 128, 128, orbit, light));
    mesh = scene::preview_mesh(level, hit, instance, palette);
    CHECK(select(preview, std::move(mesh.value())));
    CHECK(draw(preview, 128, 128, {}, light));
    preview.release();
}

// The viewer's depth prepass is exercised once eight or more authored lights enable tile masks.
void check_many_lights(
    const scene::LevelData& level,
    const scene::PickHit& hit,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
) {
    if (!gl::enhanced_rendering_available()) {
        return;
    }

    scene::ObjectPreview preview;
    auto mesh = scene::preview_mesh(level, hit, instance, palette);
    CHECK(mesh);

    if (!mesh) {
        return;
    }

    CHECK(select(preview, std::move(mesh.value())));
    auto* provider = dynamic_cast<enhanced::ObjectPreviewLighting*>(preview.lighting());
    CHECK(provider);

    if (!provider) {
        return;
    }

    scene::PreviewOrbit orbit;
    orbit.yaw = -1.5707963f;
    orbit.pitch = 0;
    std::vector<enhanced::LocalLight> lights(2);

    for (auto& light : lights) {
        light.position = {0, -2, 0};
        light.range = 8;
        light.intensity = .1f;
    }

    CHECK(provider->set_lights(std::span(lights.data(), 1)));
    CHECK(preview.draw(128, 128, orbit));
    const auto single = brightness(preview);
    CHECK(provider->set_lights(lights));
    CHECK(preview.draw(128, 128, orbit));
    CHECK(brightness(preview) > single + 2);
    lights.resize(20, lights.front());
    CHECK(provider->set_lights(lights));
    CHECK(preview.draw(128, 128, orbit));
    const auto tiled = preview.target().read_rgba();
    // Force all-light iteration by disabling culling in the provider, without changing radiance.
    provider->reference = true;
    CHECK(preview.draw(128, 128, orbit));
    CHECK(preview.target().read_rgba() == tiled);
    CHECK(preview.draw(2048, 1024, orbit));
    CHECK(preview.target().width() == 1024 && preview.target().height() == 512);
}

// The context outlives every preview object, matching the frontend's explicit release ordering.
int run() {
    auto level = fixture();
    scene::PickHit hit;
    hit.layer = scene::Layer::Mobys;
    hit.identity = 42;
    scene::Instance instance;
    instance.palette = 0;
    instance.temporal_id = 42;
    instance.matrix = renderer::translation({100, 200, 300});
    const std::array<renderer::Mat4, 1> palette{renderer::identity()};
    check_snapshot(level, hit, instance, palette);
    test::GlContext context;
    std::string error;

    if (!context.open(error)) {
        std::printf("%s\n", error.c_str());
        return test::kSkip;
    }

    test_bind_pose();
    check_gpu(level, hit, instance, palette);
    check_cutout(level, hit, instance, palette);
    check_many_lights(level, hit, instance, palette);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    CHECK(gl::glGetError() == gl::GL_NO_ERROR);
    return test::result();
}
}  // namespace

// Keep shader, context and extraction exceptions inside the test executable boundary.
int main() {
    try {
        return run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "object preview: %s\n", error.what());
        return 1;
    }
}
