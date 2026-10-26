// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <cmath>
#include <vector>

#include "scene/shadow_atlas.h"
#include "scene/shadow_scroll.h"
#include "tests/check.h"

namespace {
using namespace openrac::scene;

// Every new texel is updated exactly once, in the physical cell of its world coordinate.
void check_regions(std::array<std::int64_t, 2> old, std::array<std::int64_t, 2> next, bool full) {
    constexpr int size = 8;
    const auto regions = shadow_scroll_regions(old, next, size, full);
    std::array<int, size * size> visits{};
    for (const auto& r : regions) {
        CHECK(r.physical[0] >= 0 && r.physical[0] + r.physical[2] <= size);
        CHECK(r.physical[1] >= 0 && r.physical[1] + r.physical[3] <= size);
        for (int y = 0; y < r.logical[3]; ++y) {
            for (int x = 0; x < r.logical[2]; ++x) {
                const int lx = r.logical[0] + x, ly = r.logical[1] + y;
                ++visits[ly * size + lx];
                CHECK(r.physical[0] + x == shadow_wrap(next[0] + lx, size));
                CHECK(r.physical[1] + y == shadow_wrap(next[1] + ly, size));
            }
        }
    }
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const auto wx = next[0] + x, wy = next[1] + y;
            const bool exposed =
                full || wx < old[0] || wx >= old[0] + size || wy < old[1] || wy >= old[1] + size;
            CHECK(visits[y * size + x] == (exposed ? 1 : 0));
        }
    }
}

// Sub-texel changes retain the cache; scene and sun changes refresh it even at the same camera.
void test_invalidation() {
    ShadowScrollGrid grid;
    const auto requested = openrac::renderer::identity();
    const openrac::renderer::Vec3 low{-5, -5, -5}, high{5, 5, 5};
    auto update = grid.update(requested, {0, 0, -1}, low, high, 16, 1, false);
    CHECK(update.full);
    update = grid.update(requested, {0, 0, -1}, low, high, 16, 1, false);
    CHECK(!update.full && update.regions.empty());
    update = grid.update(requested, {0, 0, -1}, low, high, 16, 2, false);
    CHECK(update.full);
    update = grid.update(requested, {1, 0, -1}, low, high, 16, 2, false);
    CHECK(update.full);
    update = grid.update(requested, {1, 0, -1}, low, high, 16, 2, true);
    CHECK(update.full);
    for (const auto& r : update.regions) {
        const auto matrix = shadow_scroll_matrix(update, r, 16);
        CHECK(std::isfinite(matrix[12]) && std::isfinite(matrix[13]));
    }
    auto moved = requested;
    moved[12] = -20;
    update = grid.update(moved, {1, 0, -1}, low, high, 16, 2, false);
    CHECK(update.full);
    moved[0] = .1f;
    update = grid.update(moved, {1, 0, -1}, low, high, 16, 2, false);
    CHECK(update.full);
    update = grid.update(moved, {1, 0, -1}, low, {500, 500, 500}, 16, 2, false);
    CHECK(update.full);
}

// Oscillation across the old half-texel boundary should not repeatedly dirty the cache.
void test_subtexel_jitter() {
    using namespace openrac::renderer;
    ShadowScrollGrid grid;
    auto requested = identity();
    auto update = grid.update(requested, {0, 0, -1}, {-5, -5, -5}, {5, 5, 5}, 512, 1, false);
    const auto origin = update.origin;
    const double scale = std::sqrt(
        update.matrix[0] * update.matrix[0] + update.matrix[4] * update.matrix[4]
        + update.matrix[8] * update.matrix[8]
    );
    const double texel = 2 / (512 * scale);
    for (unsigned n = 0; n < 12; ++n) {
        requested[12] = static_cast<float>(-(n % 2 ? .51 : .49) * texel);
        update = grid.update(requested, {0, 0, -1}, {-5, -5, -5}, {5, 5, 5}, 512, 1, false);
        CHECK(update.origin == origin);
        CHECK(update.regions.empty());
    }
    requested[12] = static_cast<float>(-2 * texel);
    update = grid.update(requested, {0, 0, -1}, {-5, -5, -5}, {5, 5, 5}, 512, 1, false);
    CHECK(update.origin != origin);
    CHECK(!update.regions.empty() && !update.full);
}

// Rasterisation and cropped culling agree at each wrapped region's centre.
void test_projection() {
    using namespace openrac::renderer;
    ShadowScrollGrid grid;
    auto requested = identity();
    requested[12] = -1.25f;
    requested[13] = .75f;
    const auto update = grid.update(requested, {0, 0, -1}, {-5, -5, -5}, {5, 5, 5}, 16, 1, false);
    for (const auto& region : update.regions) {
        Vec3 world{};
        for (unsigned k = 0; k < 2; ++k) {
            const float scale = std::sqrt(
                update.matrix[k] * update.matrix[k] + update.matrix[4 + k] * update.matrix[4 + k]
                + update.matrix[8 + k] * update.matrix[8 + k]
            );
            const float coord = static_cast<float>(update.origin[k])
                                + static_cast<float>(region.logical[k])
                                + static_cast<float>(region.logical[2 + k]) * .5f;
            for (unsigned axis = 0; axis < 3; ++axis) {
                world[axis] += update.matrix[axis * 4 + k] * coord * 2 / (16 * scale * scale);
            }
        }
        const auto raster = shadow_scroll_matrix(update, region, 16);
        const auto point = transform_point(raster, world);
        const auto clipped =
            transform_point(shadow_region_clip(raster, region.physical, 16), world);
        for (unsigned k = 0; k < 2; ++k) {
            const float expected = (2.f * static_cast<float>(region.physical[k])
                                    + static_cast<float>(region.physical[2 + k]))
                                       / 16
                                   - 1;
            CHECK(std::abs(point[k] - expected) < 1e-5f);
            CHECK(std::abs(clipped[k]) < 1e-5f);
        }
    }
}
}  // namespace

int main() {
    for (int y = -10; y <= 10; ++y) {
        for (int x = -10; x <= 10; ++x) {
            check_regions({-3, 6}, {x, y}, false);
            check_regions({-3, 6}, {x, y}, true);
        }
    }
    const auto high = shadow_atlas_layout(openrac::renderer::ShadowQuality::High);
    const auto medium = shadow_atlas_layout(openrac::renderer::ShadowQuality::Medium);
    CHECK(high.size == 3072 && medium.size == 1536);
    for (unsigned i = 0; i < high.rects.size(); ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            CHECK(high.rects[i][j] == medium.rects[i][j] * 2);
        }
        CHECK(medium.rects[i][0] + medium.rects[i][2] <= medium.size);
        CHECK(medium.rects[i][1] + medium.rects[i][3] <= medium.size);
    }
    test_invalidation();
    test_projection();
    test_subtexel_jitter();
    return openrac::test::result();
}
