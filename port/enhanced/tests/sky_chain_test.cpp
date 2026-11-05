// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic authored sky, current shadows and nontransparent fog compare shared and immediate recording.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "enhanced/renderer/atmosphere.h"
#include "enhanced/tests/probe_fixture.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/colour_graph.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/frame_capture.h"
#include "renderer/vulkan/vkgl.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace opt = renderer::render_opt;

// Adds one untextured triangle with independent bounds and synthetic vertex colour.
void add_triangle(
    scene::LevelData& level,
    const std::array<renderer::Vec3, 3>& points,
    const std::array<float, 4>& colour,
    scene::Layer layer
) {
    const auto first = static_cast<std::uint32_t>(level.indices.size());
    scene::Model model;
    model.name = layer == scene::Layer::Sky ? "sky.glb" : "terrain.glb";
    model.min = points[0];
    model.max = points[0];
    // Separate indices retain the original triangle winding for sky and shadow casters.
    for (const auto& point : points) {
        scene::Vertex vertex{};
        std::copy(point.begin(), point.end(), vertex.position);
        std::copy(colour.begin(), colour.end(), vertex.colour);
        vertex.normal[2] = 1;
        vertex.light_slot = -1;
        level.indices.push_back(static_cast<std::uint32_t>(level.vertices.size()));
        level.vertices.push_back(vertex);
        // Caster fitting uses real bounds rather than the sky shell's depth.
        for (unsigned axis = 0; axis < 3; ++axis) {
            model.min[axis] = std::min(model.min[axis], point[axis]);
            model.max[axis] = std::max(model.max[axis], point[axis]);
        }
    }
    model.primitives.push_back({first, 3, 0});
    scene::Instance instance;
    instance.model = static_cast<std::uint32_t>(level.models.size());
    level.instances[static_cast<unsigned>(layer)].push_back(instance);
    level.models.push_back(model);
}

// The broad translucent sky exposes blending and alpha preservation; the small world triangle casts shadows.
scene::LevelData make_level() {
    scene::LevelData level;
    level.materials.push_back({});
    add_triangle(
        level,
        {{{-30, -30, -10}, {30, -30, -10}, {0, 30, -10}}},
        {.2f, .4f, .7f, .65f},
        scene::Layer::Sky
    );
    add_triangle(
        level, {{{-1, -1, -4}, {1, -1, -4}, {0, 1, -4}}}, {.7f, .5f, .2f, 1}, scene::Layer::Terrain
    );
    level.light_sets.resize(256);
    level.light_sets[0] = .5f;
    level.light_sets[1] = .4f;
    level.light_sets[2] = .3f;
    level.light_sets[4] = .2f;
    level.light_sets[5] = -.3f;
    level.light_sets[6] = -.93f;
    return level;
}

// Owns scene/provider resources through every synchronous preparation and deferred callback.
struct Fixture {
    renderer::Renderer renderer;   // Owns the sky and optional world buckets.
    scene::LevelScene scene;       // Owns real shadow targets and registered transport.
    renderer::FrameBuffer target;  // Offscreen colour/depth used by both recording paths.
    renderer::FrameInput input;    // Stable camera and quality settings during each chain.
    // Initializes enhanced lighting with optional world and sky buckets.
    explicit Fixture(bool world, bool sky = true, bool probes = true);
    // Releases the scene while its borrowed renderer texture pool and graphics context still exist.
    ~Fixture();
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
    // Recreates target storage before any pass can import it.
    void resize(int width, int height, int samples);
};

Fixture::Fixture(bool world, bool sky, bool probes) {
    std::string error;
    lighting::use_scene_lighting(scene);
    CHECK(scene.upload(make_level(), renderer.textures(), error));
    // The no-probe fixture uses the normal missing-bake path.
    if (probes) {
        CHECK(test::shadow_probe_fixture(scene, error));
        scene.set_probe_view(1);
        CHECK(scene.enable_sun_shadow(true, error));
        CHECK(scene.enable_hero_shadow(true, error));
    }
    scene::Instance hero;
    hero.model = 1;
    hero.hero_shadow_anchor = hero.hero_shadow_caster = true;
    hero.matrix[12] = .2f;
    hero.matrix[14] = .5f;
    scene.set_instances(scene::Layer::Mobys, {hero});
    scene.set_fog({.3f, .45f, .6f}, 0, 100 * 1024, 255, 0);
    // Some levels draw world geometry without a sky bucket.
    if (sky) {
        CHECK(
            renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Sky), error)
        );
    }
    // A sky-only frame isolates the single shadow/transport/sky executable plan.
    if (world) {
        CHECK(renderer.add(
            std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Terrain), error
        ));
    }
    CHECK(renderer.init(error));
    input.camera.view = renderer::identity();
    input.clear_colour = {.1f, .15f, .2f, .4f};
    input.sun_pixel_shadow = true;
    input.hero_pixel_shadow = true;
    input.shadow_quality = renderer::ShadowQuality::Low;
    input.volumetric_quality = renderer::VolumetricQuality::Medium;
}

Fixture::~Fixture() {
    renderer.release();
    scene.release();
}

void Fixture::resize(int width, int height, int samples) {
    std::string error;
    CHECK(target.create(width, height, error, samples));
    input.camera.projection = renderer::perspective_reversed(
        1, static_cast<float>(width) / static_cast<float>(height), .1f, 100
    );
}

// ---- Frame comparisons ----

// Full readback and native boundary counts from one completed synthetic frame.
struct Frame {
    std::vector<std::uint8_t> pixels;  // RGBA bytes, including preserved alpha.
    std::uint64_t chains = 0;          // Counts only successfully executed shared plans.
    std::uint64_t barriers = 0;        // Native barriers before the readback's transitions.
};

// Resets incoming attachments so keep-colour frames compare the same existing background.
Frame render(Fixture& fixture) {
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.target.id());
    glViewport(0, 0, fixture.target.width(), fixture.target.height());
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClearColor(.1f, .15f, .2f, .4f);
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    const auto before = vkgl::submission_counters();
    ++fixture.input.frame;
    fixture.renderer.render(
        fixture.input, {fixture.target.id(), fixture.target.width(), fixture.target.height()}
    );
    const auto after = vkgl::submission_counters();
    return {
        fixture.target.read_rgba(),
        after.surface_chains - before.surface_chains,
        after.barriers - before.barriers
    };
}

// Exact images and actual scheduling are compared after normalising allocation and pipeline warmup.
void compare(Fixture& fixture, bool expected_chain = true) {
    render(fixture);
    const auto reference = render(fixture);
    const auto shared = render(fixture);
    CHECK(reference.pixels == shared.pixels);
    CHECK(reference.chains == shared.chains);
    const bool world_colour = renderer::colour_graph_enabled()
                              && std::any_of(
                                  fixture.renderer.renderers().begin(),
                                  fixture.renderer.renderers().end(),
                                  [](const auto& bucket) { return bucket->prepares_colour_chain(); }
                              );
    CHECK(shared.chains == (expected_chain ? 1u : 0u) + (world_colour ? 1u : 0u));
    CHECK(std::any_of(shared.pixels.begin(), shared.pixels.end(), [](auto byte) {
        return byte > 0 && byte < 255;
    }));
    std::printf(
        "sky chain barriers: %llu -> %llu\n",
        static_cast<unsigned long long>(reference.barriers),
        static_cast<unsigned long long>(shared.barriers)
    );
}

// A live sky tint reaches the authored shell without changing its coverage or other channels.
void test_sky_material_tint() {
    Fixture fixture(false);
    fixture.resize(64, 48, 1);
    fixture.input.volumetric_quality = renderer::VolumetricQuality::Off;
    const auto original = render(fixture).pixels;
    auto& materials = lighting::scene_lighting(fixture.scene)->materials();
    CHECK(materials.patch(0, "surface.tint", std::array<double, 3>{0, 1, 1}));
    const auto tinted = render(fixture).pixels;
    CHECK(original.size() == tinted.size());
    bool red_changed = false;
    // Red must decrease somewhere in the shell while green, blue and alpha stay identical.
    for (std::size_t n = 0; n + 3 < std::min(original.size(), tinted.size()); n += 4) {
        red_changed |= tinted[n] < original[n];
        CHECK(tinted[n] <= original[n]);
        CHECK(
            std::equal(original.begin() + n + 1, original.begin() + n + 4, tinted.begin() + n + 1)
        );
    }
    CHECK(red_changed);
    fixture.scene.level_lighting()->set_material_overrides(false);
    CHECK(render(fixture).pixels == original);
}

// Authored surface changes render using retail lighting when probes are disabled.
void test_material_without_probes() {
    Fixture fixture(true, false, false);
    fixture.resize(64, 48, 1);
    CHECK(!fixture.scene.level_lighting()->has_data());
    fixture.input.sun_pixel_shadow = false;
    fixture.input.hero_pixel_shadow = false;
    const auto original = render(fixture).pixels;
    auto& materials = lighting::scene_lighting(fixture.scene)->materials();
    CHECK(materials.patch(1, "surface.diffuse_scale", .25));
    const auto dimmed = render(fixture).pixels;
    CHECK(dimmed != original);
    CHECK(materials.patch(1, "emission.runtime_intensity", 1.0));
    CHECK(render(fixture).pixels != dimmed);
    fixture.scene.level_lighting()->set_material_overrides(false);
    CHECK(render(fixture).pixels == original);
}

// Current shadows, resized transport and real sky blending retain identical pixels at both sample counts.
void test_shared_sky() {
    Fixture fixture(false);
    enhanced::AtmosphereSettings settings;
    settings.height_density = .025f;
    settings.height_falloff = 0;
    settings.distance = 64;
    enhanced::set_atmosphere_settings(settings);
    // Odd sizes exercise rounded froxel extents and recreation of imported attachments.
    for (const auto size : {std::array{37, 23}, std::array{53, 31}}) {
        // Multisampled sky still writes the same preserved colour target before later resolve.
        for (int samples : {1, std::min(4, renderer::FrameBuffer::max_samples())}) {
            fixture.resize(size[0], size[1], samples);
            compare(fixture);
        }
    }
    const auto fogged = render(fixture);
    settings.height_density = 0;
    enhanced::set_atmosphere_settings(settings);
    const auto clear = render(fixture);
    CHECK(fogged.pixels != clear.pixels);
    fixture.input.volumetric_quality = renderer::VolumetricQuality::Off;
    compare(fixture, false);
    fixture.input.volumetric_quality = renderer::VolumetricQuality::Medium;
    fixture.input.keep_colour = true;
    fixture.input.sun_pixel_shadow = false;
    fixture.input.hero_pixel_shadow = false;
    compare(fixture, false);
    fixture.input.keep_colour = false;
    settings.height_density = .025f;
    enhanced::set_atmosphere_settings(settings);

    compare(fixture);
}

// World colour consumes the same transport after sky; camera and medium changes cannot retain stale contents.
void test_world_consumers() {
    Fixture fixture(true);
    fixture.resize(41, 29, 1);
    enhanced::AtmosphereSettings settings;
    settings.height_density = .02f;
    settings.height_falloff = 0;
    settings.distance = 64;
    enhanced::set_atmosphere_settings(settings);
    compare(fixture);
    fixture.input.camera.position = {.25f, 0, 0};
    fixture.input.camera.view =
        renderer::look_along(fixture.input.camera.position, {0, 0, -1}, {0, 1, 0});
    compare(fixture);
}

// Terrain must prepare its own transport when there is no sky phase to produce it.
void test_missing_sky() {
    Fixture fixture(true, false);
    fixture.resize(29, 21, 1);
    fixture.input.sun_pixel_shadow = false;
    fixture.input.hero_pixel_shadow = false;
    enhanced::AtmosphereSettings settings;
    settings.height_density = .02f;
    settings.height_falloff = 0;
    settings.distance = 64;
    enhanced::set_atmosphere_settings(settings);
    compare(fixture);
}

// ---- Declared dependencies ----

// Locates a work event without mistaking an allocation label for an executed pass.
std::size_t work_position(const std::string& description, const std::string& name) {
    const auto position = description.find(" work " + name + "\n");
    // Missing labels need their names to distinguish a diagnostic contract change from bad ordering.
    if (position == std::string::npos) {
        std::fprintf(stderr, "missing captured work: %s\n", name.c_str());
    }
    CHECK(position != std::string::npos);
    return position;
}

// The capture must show every producer before transport, and intermediate shadow exports after sky.
void check_order(const std::string& description) {
    const bool atlas = description.find(" work dynamic shadow atlas\n") != std::string::npos;
    const std::string shadow = atlas ? "dynamic shadow atlas" : "sun cascade map";
    const auto produced = work_position(description, shadow);
    const auto transport = work_position(description, "atmosphere transport");
    const auto authored = work_position(description, "authored sky");
    const auto read = work_position(description, "atmosphere sky inputs");
    const auto overlay = work_position(description, "atmosphere sky");
    // Frame capture restores export labels after compiling their dependency order.
    const auto exported = description.find(" export " + shadow + " consumers\n");
    CHECK(exported != std::string::npos);
    CHECK(produced < transport && transport < authored && authored < read && read < overlay);
    CHECK(overlay < exported);
    const auto combined = description.find(" work shadow depth combine\n");
    // Scrolling configurations must compose their depths before transport samples them.
    if (combined != std::string::npos) {
        CHECK(produced < combined && combined < transport);
    }
}

// Captures the real scene path after warmup without changing image or queue ownership.
void test_declared_order(test::GlContext& context) {
    Fixture fixture(false);
    fixture.resize(31, 19, 1);
    enhanced::AtmosphereSettings settings;
    settings.height_density = .025f;
    settings.height_falloff = 0;
    settings.distance = 64;
    enhanced::set_atmosphere_settings(settings);
    render(fixture);
    CHECK(vkgl::capture_frame_graph());
    render(fixture);
    fixture.target.blit_to(0, 64, 64);
    context.window().swap();
    const auto description = vkgl::frame_graph_description();
    // A diagnostic failure must remain visible rather than silently dropping dependency assertions.
    if (!description) {
        std::fprintf(stderr, "%s\n", description.error().message.c_str());
    }
    CHECK(description);
    // The completed capture describes actual execution, including the final shared exports.
    if (description) {
        check_order(*description);
    }
}
}  // namespace

// Uses hidden synthetic GPU work; unavailable Vulkan is the sole skip condition.
int main() {
    // Report setup or recording errors without allowing exceptions past the test boundary.
    try {
        test::GlContext context;
        std::string error;
        // Native shared plans cannot run on an unavailable or compatibility-only context.
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }
        // These cases exercise pass dependencies and pixels; prewarm has its own fixture.
        opt::set(opt::Opt::PipelinePrewarm, false);
        test_sky_material_tint();
        test_material_without_probes();
        test_shared_sky();
        test_world_consumers();
        test_missing_sky();
        test_declared_order(context);
        CHECK(vkgl::submission_counters().validation_errors == 0);
        enhanced::set_atmosphere_settings({});
        opt::reset();
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "sky chain: %s\n", error.what());
        return 1;
    }
}
