// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic rectangles verify exact frame extents, emitting directions and Vulkan visible/hidden marker drawing.

#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

#include "enhanced/lighting/interior_lights.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/water_float.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/viewer/area_light_overlay.h"
#include "enhanced/viewer/environment_lighting.h"
#include "enhanced/viewer/environment_overlay.h"
#include "enhanced/viewer/medial_axis_overlay.h"
#include "enhanced/viewer/probe_volume_overlay.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
// A two-unit square at Z=2 with a downward emitting hemisphere and a known RGB ratio.
constexpr std::string_view kReport = R"({"version":1,"lights":[{
    "position":[0,0,2],"axis_u":[1,0,0],"axis_v":[0,-1,0],"radiance":[2,1,0.5]}]})";

// Frame extents and arrow direction come directly from source half-edges, independently of mesh winding.
void test_exact_source_geometry() {
    const auto vertices = openrac::lighting::area_light_debug_vertices(kReport);
    CHECK(vertices.has_value());
    CHECK(vertices->size() == 18);
    CHECK(vertices->at(0).position == (openrac::renderer::Vec3{-1, 1, 2}));
    CHECK(vertices->at(1).position == (openrac::renderer::Vec3{1, 1, 2}));
    CHECK(vertices->at(0).colour == (openrac::renderer::Vec3{1, .5f, .25f}));
    CHECK(vertices->at(8).position == (openrac::renderer::Vec3{0, 0, 2}));
    // The shaft travels three units toward the source's emitting side.
    CHECK(vertices->at(9).position == (openrac::renderer::Vec3{0, 0, 2 - 3}));
}

// Invalid source vectors and unknown formats cannot leave partial markers behind.
void test_invalid_report_rejection() {
    using openrac::lighting::area_light_debug_vertices;
    CHECK(!area_light_debug_vertices(R"({"version":2,"lights":[]})"));
    CHECK(!area_light_debug_vertices(R"({"version":1,"lights":[{}]})"));
    CHECK(!area_light_debug_vertices(R"({"version":1,"lights":[{
        "position":[0,0,0],"axis_u":[0,0,0],"axis_v":[0,0,1],"radiance":[1,1,1]}]})"));
    CHECK(area_light_debug_vertices(R"({"version":1,"lights":[]})")->empty());
}

// The diagnostic accepts the baker's entire source budget, rejecting only reports beyond that limit.
void test_full_bake_light_budget() {
    constexpr std::size_t kVerticesPerLight = 18;  // Four frame edges and five arrow segments.
    constexpr std::string_view kLight =
        R"({"position":[0,0,2],"axis_u":[1,0,0],"axis_v":[0,-1,0],"radiance":[1,1,1]})";
    std::string report = R"({"version":1,"lights":[)";

    for (unsigned n = 0; n < openrac::lighting::kMaximumInteriorLightsPerLevel; ++n) {
        // JSON separators appear only between source records.
        if (n != 0) {
            report += ',';
        }

        report += kLight;
    }

    const auto vertices = openrac::lighting::area_light_debug_vertices(report + "]}");
    CHECK(vertices.has_value());
    CHECK(
        vertices->size() == openrac::lighting::kMaximumInteriorLightsPerLevel * kVerticesPerLight
    );
    report += ',';
    report += kLight;
    CHECK(!openrac::lighting::area_light_debug_vertices(report + "]}"));
}

// Checks inclusive dense overlap, sparse fallback and missing coverage against explicit world bounds.
void test_player_volume_membership() {
    using namespace openrac::lighting;
    ProbeGrid sparse;
    sparse.origin = {-10, -10, -10};
    sparse.spacing = {40, 40, 40};
    ProbeGrid first;
    first.origin = {0, 0, 0};
    first.spacing = {4, 6, 8};
    ProbeGrid second = first;
    second.origin = {4, 0, 0};
    const std::array dense{first, second};
    CHECK(containing_probe_volumes(sparse, dense, {1, 1, 1}) == std::vector<int>{0});
    CHECK(containing_probe_volumes(sparse, dense, {4, 3, 8}) == (std::vector<int>{0, 1}));
    CHECK(containing_probe_volumes(sparse, dense, {5, 1, 1}) == std::vector<int>{1});
    CHECK(containing_probe_volumes(sparse, dense, {-1, 1, 1}) == std::vector<int>{-1});
    CHECK(containing_probe_volumes(sparse, dense, {1, 1, 9}) == std::vector<int>{-1});
    CHECK(containing_probe_volumes(sparse, dense, {31, 0, 0}).empty());
    CHECK(containing_probe_volumes(sparse, dense, {std::numeric_limits<float>::quiet_NaN(), 0, 0})
              .empty());
}

// Maps a nominal endpoint to its corner bits, rejecting positions between the box corners.
unsigned checked_corner(
    const openrac::lighting::ProbeGrid& grid, openrac::lighting::Float3 position
) {
    unsigned corner = 0;

    // Each far endpoint sets one corner bit.
    for (std::size_t axis = 0; axis < grid.origin.size(); ++axis) {
        CHECK(
            position[axis] == grid.origin[axis]
            || position[axis] == grid.origin[axis] + grid.spacing[axis]
        );
        corner |= static_cast<unsigned>(position[axis] != grid.origin[axis]) << axis;
    }

    return corner;
}

// Checks twelve distinct axis-aligned box edges with nominal endpoints and diagnostic colours.
void test_probe_box_geometry() {
    using namespace openrac::lighting;
    ProbeGrid grid;
    grid.origin = {1, 2, 3};
    grid.spacing = {4, 5, 6};
    const std::array dense{grid};
    const std::array dense_index{0};
    const auto vertices = probe_volume_debug_vertices(grid, dense, dense_index);
    CHECK(vertices.size() == 24);
    std::array<unsigned, 8> degrees{};

    // A box has eight vertices of degree three and each edge changes exactly one coordinate.
    for (std::size_t n = 0; n < vertices.size(); n += 2) {
        unsigned changed = 0;

        for (std::size_t axis = 0; axis < grid.origin.size(); ++axis) {
            changed += vertices[n].position[axis] != vertices[n + 1].position[axis];
        }

        CHECK(changed == 1);

        ++degrees[checked_corner(grid, vertices[n].position)];
        ++degrees[checked_corner(grid, vertices[n + 1].position)];
        CHECK(vertices[n].colour == (Float3{1, 1, 0}));
        CHECK(vertices[n + 1].colour == (Float3{1, 1, 0}));
    }

    for (unsigned degree : degrees) {
        CHECK(degree == 3);
    }

    const std::array sparse_index{-1};
    const auto fallback = probe_volume_debug_vertices(grid, dense, sparse_index);
    CHECK(fallback.size() == 24);
    CHECK(fallback.front().colour == (Float3{0, 1, 1}));
}

// Every loaded reflection capture is represented, including distant boxes outside the player region.
void test_environment_capture_geometry() {
    using namespace openrac::lighting;
    const std::array<EnvironmentProbe, 2> probes{
        {{{1, 2, 3}, {-2, -3, -4}, {8, 9, 10}}, {{100, 200, 300}, {90, 180, 270}, {120, 240, 360}}}
    };
    const auto vertices = environment_probe_debug_vertices(probes);
    CHECK(vertices.size() == 2 * (24 + 3 * 24 * 2 + 6));
    const auto stride = vertices.size() / probes.size();

    for (std::size_t i = 0; i < probes.size(); ++i) {
        CHECK(vertices[i * stride].position == probes[i].minimum);

        // Box endpoints must use the baked bounds, not a fixed radius around the capture.
        for (std::size_t n = 0; n < 24; ++n) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float value = vertices[i * stride + n].position[axis];
                CHECK(value == probes[i].minimum[axis] || value == probes[i].maximum[axis]);
            }
        }

        const auto& cross_end = vertices[(i + 1) * stride - 1].position;
        CHECK(cross_end[0] == probes[i].position[0]);
        CHECK(cross_end[1] == probes[i].position[1]);
        CHECK(std::abs(cross_end[2] - probes[i].position[2] - .35f) < 1e-4f);
    }

    CHECK(environment_probe_debug_vertices({}).empty());
}

// Pair overlap is distinct from receivers which have no alternative capture anywhere in the level.
// Tuning is isolated per surface, bounded, and resettable without changing baked data.
void test_reflection_material_controls() {
    using namespace openrac::enhanced;

    for (auto surface :
         {ReflectionSurface::Water,
          ReflectionSurface::Clank,
          ReflectionSurface::Wrench,
          ReflectionSurface::Chrome,
          ReflectionSurface::Glass,
          ReflectionSurface::Stylised}) {
        const auto original = reflection_material_settings(surface);
        const auto defaults = default_reflection_material(surface);
        set_reflection_material_settings(surface, {-1, 2, 9});
        const auto bounded = reflection_material_settings(surface);
        CHECK(bounded.roughness == .04f && bounded.reflectivity == 1 && bounded.strength == 4);
        set_reflection_material_settings(
            surface,
            {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -1}
        );
        const auto finite = reflection_material_settings(surface);
        CHECK(
            finite.roughness == defaults.roughness && finite.reflectivity == defaults.reflectivity
        );
        CHECK(finite.strength == 0);
        auto tinted = defaults;
        tinted.tint = {-1, 2, std::numeric_limits<float>::quiet_NaN()};
        tinted.opacity = 2;
        set_reflection_material_settings(surface, tinted);
        const auto limited = reflection_material_settings(surface);
        CHECK((limited.tint == std::array<float, 3>{0, 1, 1}));
        CHECK(limited.opacity == 1);
        set_reflection_material_settings(surface, original);
    }

    CHECK(reflection_material_for("metal_chrome", 500).reflectivity == .65f);
    CHECK(reflection_material_for("metal_glass", 87).reflectivity == .04f);
    CHECK(reflection_material_for("metal_stylised", 71).reflectivity == .12f);
    set_reflection_override(87, ReflectionSurface::Stylised);
    CHECK(reflection_material_for("metal_glass", 87).reflectivity == .12f);
    CHECK(reflection_material_for("metal_glass", 71).reflectivity == .04f);
    set_reflection_subjects({{87, "Clank", {}}, {71, "Wrench", {}}});
    const auto subjects = reflection_subjects();
    CHECK(subjects[0].class_id == 71 && !subjects[0].override);
    CHECK(subjects[1].override == ReflectionSurface::Stylised);
    clear_reflection_overrides();
    CHECK(reflection_material_for("metal_chrome", 87).roughness == .26f);
    set_reflection_subjects({});
}

// Sums RGB only so preserved framebuffer alpha cannot mask an empty debug draw.
std::uint64_t colour_sum(const std::vector<std::uint8_t>& pixels) {
    std::uint64_t sum = 0;

    // Readback pixels are interleaved RGBA bytes.
    for (std::size_t n = 0; n < pixels.size(); ++n) {
        if (n % 4 != 3) {
            sum += pixels[n];
        }
    }

    return sum;
}

// Checks that boxes disappear and reappear after dynamic geometry updates on the same GPU buffer.
void test_gpu_box_updates(
    const openrac::renderer::FrameInput& input,
    openrac::renderer::RenderState& state,
    const openrac::renderer::FrameBuffer& target
) {
    using namespace openrac::gl;
    std::string error;
    openrac::lighting::DebugLines bounds;
    const auto box = openrac::lighting::probe_volume_debug_vertices(
        openrac::lighting::ProbeGrid{}, {}, std::array{-1}
    );
    const std::array<openrac::lighting::EnvironmentProbe, 1> captures{
        {{{0, 0, 2}, {-1, -1, 1}, {1, 1, 3}}}
    };
    bounds.set_vertices(openrac::lighting::environment_probe_debug_vertices(captures));
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK(bounds.draw(input, state, error));
    CHECK(colour_sum(target.read_rgba()) > 0);
    bounds.set_vertices({});
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto calls = state.stats.draw_calls;
    CHECK(bounds.draw(input, state, error));
    CHECK(colour_sum(target.read_rgba()) == 0);
    CHECK(state.stats.draw_calls == calls);
    bounds.set_vertices(box);
    CHECK(bounds.draw(input, state, error));
    CHECK(colour_sum(target.read_rgba()) > 0);
    bounds.release();
}

// Water attachments and reflection fragments must keep independent texture bindings in either order.
void test_water_and_reflection_bindings(
    openrac::lighting::EnvironmentLighting& captures, const openrac::renderer::FrameBuffer& target
) {
    using namespace openrac;
    using namespace gl;
    renderer::Shader shader;
    std::string error;
    const auto vertex = enhanced::WaterFloat::vertex_source(R"(#version 410 core
void main() { vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
 gl_Position=water_float_pose(vec3(0))*vec4(p*2.-1.,0,1); })");
    const std::string fragment = R"(#version 430 core
uniform sampler2D environment_map;
uniform int environment_count;
layout(location=0) out vec4 colour;
void main() { colour=vec4(environment_count>0 ? texelFetch(environment_map,ivec2(2),0).rgb : vec3(0),1); })";
    CHECK(shader.build("water and reflection binding independence", vertex, fragment, error));
    enhanced::WaterFloat attachment;
    attachment.load();
    shader.use();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    attachment.bind(shader, false);
    captures.bind(shader, true);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    const auto reference = target.read_rgba();
    CHECK(reference[0] > 50);
    // Reload clears the attachment's per-shader binding cache before reversing the call order.
    attachment.load();
    captures.bind(shader, true);
    attachment.bind(shader, false);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    CHECK(target.read_rgba() == reference);
    attachment.release();
    shader.release();
    glEnable(GL_DEPTH_TEST);
}

// Production preview rendering must sample each capture independently, without tint or GI blending.
void test_environment_previews(
    const openrac::renderer::FrameInput& input,
    openrac::renderer::RenderState& state,
    const openrac::renderer::FrameBuffer& target
) {
    using namespace openrac;
    using namespace gl;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder = std::filesystem::temp_directory_path()
                        / ("openrac-environment-preview-" + std::to_string(stamp));
    CHECK(std::filesystem::create_directory(folder));
    const auto write = [&](const char* name, const std::vector<std::uint8_t>& bytes) {
        std::ofstream file(folder / name, std::ios::binary);
        file.write(
            reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        );
        CHECK(bool(file));
    };
    const lighting::ProbeGrid grid;
    write("sparse.orpg", lighting::encode_probes(lighting::ProbeVolume::allocate(grid)));
    lighting::EnvironmentAsset asset;
    asset.resolution = 8;
    asset
        .probes = {{{-1.5f, 0, 2}, {-3, -2, 0}, {3, 2, 5}}, {{1.5f, 0, 2}, {-3, -2, 0}, {3, 2, 5}}};
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());

    for (unsigned i = 0; i < 2; ++i) {
        const auto colour = lighting::pack_environment_colour(
            i == 0 ? lighting::Float3{.25f, 0, 0} : lighting::Float3{0, 0, .25f}
        );
        const auto begin = std::size_t(i) * asset.stack_height() * asset.width();
        std::fill_n(
            asset.pixels.begin() + static_cast<std::ptrdiff_t>(begin),
            std::size_t(asset.stack_height()) * asset.width(),
            colour
        );
    }

    asset.gi_hash = lighting::environment_gi_hash(folder);
    write("environment.orpe", lighting::encode_environment(asset));
    lighting::EnvironmentLighting captures;
    captures.load(folder, grid);
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK(std::filesystem::remove(folder / "sparse.orpg"));
    CHECK(std::filesystem::remove(folder / "environment.orpe"));
    CHECK(std::filesystem::remove(folder));
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto calls = state.stats.draw_calls;
    captures.draw_debug(input, state);
    CHECK(state.stats.draw_calls == calls + 3);  // Two box passes and one procedural sphere draw.
    const auto pixels = target.read_rgba();
    unsigned red = 0, blue = 0;

    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        red += pixels[i] > 125 && pixels[i] < 150 && pixels[i + 1] < 5 && pixels[i + 2] < 5;
        blue += pixels[i + 2] > 125 && pixels[i + 2] < 150 && pixels[i + 1] < 5 && pixels[i] < 5;
    }

    CHECK(red > 10);
    CHECK(blue > 10);
    // Hidden spheres obey scene depth; the x-ray wire markers remain a separate diagnostic.
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    captures.draw_debug(input, state);
    const auto hidden = target.read_rgba();
    unsigned opaque_red = 0;

    for (std::size_t i = 0; i < hidden.size(); i += 4) {
        opaque_red += hidden[i] > 125 && hidden[i] < 150 && hidden[i + 1] < 5 && hidden[i + 2] < 5;
    }

    CHECK(opaque_red == 0);
    test_water_and_reflection_bindings(captures, target);
    captures.release();
}

// The live model hook must select Clank/wrench independently and clear the override for other draws.
void test_reflection_model_selection(const openrac::renderer::FrameBuffer& target) {
    using namespace openrac;
    using namespace gl;
    scene::LevelData level;
    level.moby_classes[87] = {4, false};
    level.moby_classes[71] = {5, false};
    lighting::SceneLighting materials;
    materials.load(level);
    std::string error;
    CHECK(materials.set_probe_volume(test::gpu_probes(lighting::ProbeVolume::allocate({})), error));
    materials.set_lighting(true, 1);
    renderer::Shader shader;
    CHECK(shader.build(
        "reflection material model selection",
        R"(#version 410 core
void main() { vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
 gl_Position=vec4(p*2.-1.,0,1); })",
        R"(#version 410 core
uniform int object_reflection_enabled; uniform vec3 object_reflection_material;
layout(location=0) out vec4 colour;
void main() { colour=vec4(float(object_reflection_enabled),object_reflection_material.xy,
                         object_reflection_material.z/4.); })",
        error
    ));
    shader.use();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    scene::Material source;
    source.metal = true;
    source.name = "metal_chrome";
    const auto draw = [&](unsigned model) {
        materials.bind_material(shader, model, source);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return target.read_rgba();
    };
    const auto clank = draw(4);
    CHECK(
        clank[0] == 255 && std::abs(int(clank[1]) - 66) <= 1 && std::abs(int(clank[2]) - 166) <= 1
    );
    const auto wrench = draw(5);
    CHECK(
        wrench[0] == 255 && std::abs(int(wrench[1]) - 46) <= 1
        && std::abs(int(wrench[2]) - 191) <= 1
    );
    const auto other = draw(6);
    CHECK(other[0] == 255 && std::abs(int(other[1]) - 64) <= 1);
    CHECK(draw(~0u)[0] == 255);  // Authored chrome needs no gameplay class whitelist.
    source.name = "metal_glass";
    const auto glass = draw(4);
    CHECK(std::abs(int(glass[1]) - 20) <= 1 && std::abs(int(glass[2]) - 10) <= 1);
    source.name = "metal_stylised";
    const auto stylised = draw(4);
    CHECK(std::abs(int(stylised[1]) - 89) <= 1 && std::abs(int(stylised[2]) - 31) <= 1);
    enhanced::set_reflection_override(87, enhanced::ReflectionSurface::Wrench);
    CHECK(draw(4) == wrench);
    enhanced::clear_reflection_overrides();
    source.metal = false;
    CHECK(draw(4)[0] == 0);
    source.metal = true;
    materials.set_lighting(false, 1);
    CHECK(draw(4)[0] == 0);
    materials.release();
    shader.release();
    glEnable(GL_DEPTH_TEST);
}

// Renders both depth categories into a hidden framebuffer and checks the hidden pass is faint.
int test_vulkan_marker_drawing() {
    using namespace openrac::gl;
    openrac::test::GlContext context;
    std::string error;

    // Machines without the requested Vulkan context cannot exercise this GPU fixture.
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        return openrac::test::kSkip;
    }

    openrac::renderer::FrameBuffer target;
    CHECK(target.create(64, 64, error));
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder = std::filesystem::temp_directory_path()
                        / ("openrac-area-light-overlay-" + std::to_string(stamp));

    // A new owned folder prevents parallel fixtures from replacing another process's report.
    if (!std::filesystem::create_directory(folder)) {
        return 1;
    }

    const auto path = folder / "report.json";
    std::ofstream(path) << kReport;
    openrac::lighting::AreaLightOverlay overlay;
    CHECK(overlay.load(path).value() == 1);
    CHECK(std::filesystem::remove(path));
    CHECK(std::filesystem::remove(folder));
    openrac::renderer::FrameInput input;
    input.camera.position = {0, -6, 3};
    input.camera.view = openrac::renderer::look_along(input.camera.position, {0, 6, -1}, {0, 0, 1});
    input.camera.projection = openrac::renderer::perspective_reversed(1, 1, .1f, 100);
    openrac::renderer::TexturePool textures;
    openrac::renderer::RenderState state{textures, {target.id(), 64, 64}, {}};
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK(overlay.draw(input, state, error));

    // Shader diagnostics explain a failed draw before pixel comparisons report an empty target.
    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }

    const auto visible = colour_sum(target.read_rgba());
    // Reversed depth one hides all markers; only the faint pass should remain.
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK(overlay.draw(input, state, error));
    const auto hidden = colour_sum(target.read_rgba());
    CHECK(visible > hidden);
    CHECK(hidden > 0);
    CHECK(state.stats.draw_calls == 4);
    overlay.release();
    test_gpu_box_updates(input, state, target);
    test_environment_previews(input, state, target);
    test_reflection_model_selection(target);
    return openrac::test::result();
}

// Solid floor edges and disconnected jump dashes share orange, with gaps retained across segments.
void test_medial_axis_jump_dashes() {
    const auto vertices = openrac::lighting::medial_axis_debug_vertices(
        R"({"version":1,"paths":[[0,0,0,3,0,0]],"jump_paths":[[0,0,1,1,0,1],[1,0,1,3,0,1]],"junctions":[],"centres":[]})"
    );
    // Three units contain four 0.45-unit dashes on a 0.75-unit period, one split at the row boundary.
    CHECK(vertices && vertices->size() == 12);

    if (!vertices) {
        return;
    }

    for (const auto& vertex : *vertices) {
        CHECK(vertex.colour == (openrac::renderer::Vec3{1, .45f, .05f}));
    }

    CHECK(std::abs((*vertices)[3].position[0] - .45f) < .0001f);
    CHECK(std::abs((*vertices)[4].position[0] - .75f) < .0001f);
}
}  // namespace

int main(int argc, char** argv) {
    // Test failures must report through CTest even when optional source files are malformed.
    try {
        const auto medial = openrac::lighting::medial_axis_debug_vertices(
            R"({"version":1,"paths":[[0,0,1,4,0,1]],"junctions":[[4,0,1]],"centres":[[0,0,1]]})"
        );
        CHECK(medial && medial->size() == 14);
        CHECK(!openrac::lighting::medial_axis_debug_vertices(
            R"({"version":1,"paths":[[0,1]],"junctions":[],"centres":[]})"
        ));
        CHECK(!openrac::lighting::medial_axis_debug_vertices(
            R"({"version":1,"paths":[],"junctions":[],"centres":[[0,"bad",1]]})"
        ));
        test_exact_source_geometry();
        test_invalid_report_rejection();
        test_full_bake_light_budget();
        test_player_volume_membership();
        test_probe_box_geometry();
        test_environment_capture_geometry();
        test_reflection_material_controls();
        test_medial_axis_jump_dashes();

        if (argc > 1 && std::string_view(argv[1]) == "--gpu") {
            return test_vulkan_marker_drawing();
        }

        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected area light overlay test failure\n");
        return 1;
    }
}
