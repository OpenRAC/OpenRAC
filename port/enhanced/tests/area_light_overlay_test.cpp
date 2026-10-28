// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic rectangles verify exact frame extents, emitting directions and Vulkan visible/hidden marker drawing.

#include <array>
#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

#include "enhanced/lighting/interior_lights.h"
#include "enhanced/viewer/area_light_overlay.h"
#include "enhanced/viewer/probe_volume_overlay.h"
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
    bounds.set_vertices(box);
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
    return openrac::test::result();
}
}  // namespace

int main(int argc, char** argv) {
    // Test failures must report through CTest even when optional source files are malformed.
    try {
        test_exact_source_geometry();
        test_invalid_report_rejection();
        test_full_bake_light_budget();
        test_player_volume_membership();
        test_probe_box_geometry();

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
