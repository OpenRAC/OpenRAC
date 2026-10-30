// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Production blur passes against synthetic FSR vectors, without an SDK or game asset dependency.
#include <array>
#include <iostream>
#include <limits>
#include <vector>

#include "enhanced/fsr3/temporal_math.h"
#include "enhanced/tests/motion_blur_fixture.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/vulkan/vkgl.h"
#include "scene/level.h"
#include "scene/screenshot.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;
using namespace openrac::test::motion_blur;
bool write_preview = false;

void test_camera_cancellation() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    // A translated camera plus raster jitter must not blur even a detailed stationary image.
    f.previous = renderer::translation({.25f, -.125f, 0});
    f.jitter = {.25f, -.25f};
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const int i = y * kSize + x;
            f.vectors[i] = {8.25f, -4.25f, 0, 0};
            f.colour[i] =
                {static_cast<unsigned char>((x & 1) * 255),
                 static_cast<unsigned char>((y & 1) * 255),
                 128,
                 255};
        }
    }
    CHECK(f.render() == f.colour);
    // Rotation cancellation varies over the screen; use exact analytically generated FSR motion.
    f.previous = renderer::identity();
    f.previous[0] = f.previous[5] = std::cos(.2f);
    f.previous[1] = std::sin(.2f);
    f.previous[4] = -std::sin(.2f);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float px = (float(x) + .5f) / 32.f - 1.f;
            const float py = (float(y) + .5f) / 32.f - 1.f;
            f.vectors[y * kSize + x] =
                {((f.previous[0] * px + f.previous[4] * py) - px) * 32.f + f.jitter[0],
                 ((f.previous[1] * px + f.previous[5] * py) - py) * 32.f + f.jitter[1],
                 0,
                 0};
        }
    }
    CHECK(f.render() == f.colour);
}

void test_cutscene_camera_blur() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.previous = renderer::translation({.25f, 0, 0});
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const int i = y * kSize + x;
            f.vectors[i] = {8, 0, 0, 0};
            f.colour[i] = {static_cast<unsigned char>((x & 1) * 255), 128, 128, 255};
        }
    }
    CHECK(f.render() == f.colour);  // Gameplay removes the same camera pan.
    f.camera_relative = true;
    const auto cinematic = f.render();
    CHECK(cinematic[32 * kSize + 32] != f.colour[32 * kSize + 32]);
    f.reference_delta = {.25f, .5f, 1};
    CHECK(f.render() == cinematic);  // The tracking focus does not affect cutscene blur.
    for (auto& depth : f.depths) {
        depth[0] = 0;
    }
    CHECK(f.render()[32 * kSize + 32] != f.colour[32 * kSize + 32]);  // Rotating sky can blur.
    f.camera_relative = false;
    CHECK(f.render() == f.colour);  // Gameplay sky remains sharp.
}

void test_object_motion_and_controls() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.previous = renderer::translation({.25f, 0, 0});
    for (auto& v : f.vectors) {
        v[0] = 8;  // Shared camera component.
    }
    f.moving_square();
    CHECK(f.render(0) == f.colour);
    const auto full = f.render();
    CHECK(full[32 * kSize + 30][0] > 20);  // Moving silhouette extends into the background.
    CHECK(full[32 * kSize + 23][2] > 0);   // Object interior integrates background along the trail.
    CHECK(full[4 * kSize + 4] == f.colour[4 * kSize + 4]);
    if (write_preview) {
        std::vector<std::uint8_t> image(kSize * kSize * 2 * 4);
        for (int y = 0; y < kSize; ++y) {
            for (int x = 0; x < kSize; ++x) {
                for (int c = 0; c < 4; ++c) {
                    image[(y * kSize * 2 + x) * 4 + c] = f.colour[y * kSize + x][c];
                    image[(y * kSize * 2 + x + kSize) * 4 + c] = full[y * kSize + x][c];
                }
            }
        }
        CHECK(scene::write_png("motion_blur_fixture.png", kSize * 2, kSize, image));
    }
    const auto half = f.render(.5f);
    CHECK(half[32 * kSize + 33][0] == 0);
    CHECK(full[32 * kSize + 33][0] > 0);
    // Doubling the interval and displacement preserves the exposure's spatial extent.
    for (int y = 24; y < 40; ++y) {
        for (int x = 20; x < 28; ++x) {
            f.vectors[y * kSize + x][0] = 24;
        }
    }
    CHECK(f.render(1, 33.333334f) == full);
    // New and teleported objects must never leave a false trail.
    for (int y = 24; y < 40; ++y) {
        for (int x = 20; x < 28; ++x) {
            f.vectors[y * kSize + x][2] = 1;
        }
    }
    CHECK(f.render() == f.colour);
}

void test_target_reference() {
    renderer::Camera current, previous;
    current.target = renderer::Vec3{.1f, .2f, .3f};
    previous.target = renderer::Vec3{};
    current.target_id = previous.target_id = 42;
    const auto delta = enhanced::fsr3_detail::reference_displacement(current, previous);
    CHECK(delta && *delta == renderer::Vec3({.1f, .2f, .3f}));
    // Eye orbit, rotation and zoom must not affect the focus translation.
    current.position = {20, 30, 40};
    current.view[0] = 0;
    current.view[1] = 1;
    CHECK(enhanced::fsr3_detail::reference_displacement(current, previous) == delta);
    current.target_id = 43;
    CHECK(!enhanced::fsr3_detail::reference_displacement(current, previous));
    current.target_id = 42;
    current.target = renderer::Vec3{20, 0, 0};
    CHECK(!enhanced::fsr3_detail::reference_displacement(current, previous));
    (*current.target)[0] = std::numeric_limits<float>::quiet_NaN();
    CHECK(!enhanced::fsr3_detail::reference_displacement(current, previous));
    current.target.reset();
    CHECK(!enhanced::fsr3_detail::reference_displacement(current, previous));
}

void test_jump_with_stationary_target() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.moving_square();
    // Ratchet rises, but the camera's eased target has not moved: only his image gets velocity.
    for (int y = 24; y < 40; ++y) {
        for (int x = 20; x < 28; ++x) {
            f.vectors[y * kSize + x] = {0, 8, 0, 0};
        }
    }
    const auto result = f.render();
    CHECK(result[4 * kSize + 4] == f.colour[4 * kSize + 4]);
    unsigned trail_red = 0;
    for (int y = 16; y < 48; ++y) {
        if (y >= 24 && y < 40) {
            continue;
        }
        for (int x = 20; x < 28; ++x) {
            trail_red += result[y * kSize + x][0];
        }
    }
    CHECK(trail_red > 0);  // The jumping object still leaves a trail outside its silhouette.
}

void test_translation_relative_blur() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.reference_delta =
        {-.25f, 0, 0};  // Tracking focus moved left: previous-point vectors point right.
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            f.colour[y * kSize + x] = {0, static_cast<unsigned char>((x & 1) * 255), 255, 255};
        }
    }
    f.moving_square();  // The body travels with the tracking focus and must remain sharp.
    const auto result = f.render();
    CHECK(result[32 * kSize + 23] == f.colour[32 * kSize + 23]);
    CHECK(
        result[4 * kSize + 50] != f.colour[4 * kSize + 50]
    );  // Scenery now moves relative to the focus.
    f.previous = renderer::translation({.25f, 0, 0});
    for (auto& v : f.vectors) {
        v[0] += 8;
    }
    CHECK(f.render() == result);  // Adding camera motion does not change the reference result.
    for (int y = 24; y < 40; ++y) {
        for (int x = 20; x < 28; ++x) {
            f.vectors[y * kSize + x][0] += 8;
        }
    }
    const auto animated = f.render();  // Extra motion (e.g. limbs) survives root cancellation.
    CHECK(animated[32 * kSize + 23][0] < result[32 * kSize + 23][0]);
    CHECK(animated[32 * kSize + 23][1] > result[32 * kSize + 23][1]);
}

void test_reference_at_different_depths() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.reference_delta = {.01f, -.02f, .01f};
    const auto projection = renderer::perspective_reversed(1, 1, .05f, 2000);
    CHECK(enhanced::fsr3_detail::invert(projection, f.inverse_raster));
    f.previous = renderer::multiply(projection, renderer::translation({.01f, 0, 0}));
    const auto transform = [](const renderer::Mat4& m, const std::array<float, 4>& v) {
        std::array<float, 4> result{};
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result[row] += m[col * 4 + row] * v[col];
            }
        }
        return result;
    };
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const int i = y * kSize + x;
            const float d = x < kSize / 2 ? .3f : .8f;
            f.depths[i][0] = d;
            const float px = (float(x) + .5f) / 32.f - 1.f;
            const float py = (float(y) + .5f) / 32.f - 1.f;
            auto world = transform(f.inverse_raster, {px, py, d * 2 - 1, 1});
            for (int n = 0; n < 3; ++n) {
                world[n] = world[n] / world[3] - f.reference_delta[n];
            }
            world[3] = 1;
            const auto old = transform(f.previous, world);
            f.vectors[i] = {(old[0] / old[3] - px) * 32.f, (old[1] / old[3] - py) * 32.f, 0, 0};
            f.colour[i] =
                {static_cast<unsigned char>((x & 1) * 255),
                 static_cast<unsigned char>((y & 1) * 255),
                 128,
                 255};
        }
    }
    CHECK(f.render() == f.colour);  // Objects at both depths travelling with the focus stay sharp.
}

void test_thin_opposing_motion() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    // Fast background moves right; a one-pixel foreground feature moves left more slowly.
    // A fastest-vector tile maximum loses the foreground direction in this exact case.
    for (auto& v : f.vectors) {
        v = {20, 0, 0, 0};
    }
    for (int y = 20; y < 44; ++y) {
        const int at = y * kSize + 34;
        f.colour[at] = {255, 0, 0, 255};
        f.depths[at][0] = .7f;
        f.vectors[at] = {-12, 0, 0, 0};
    }
    const auto result = f.render();
    CHECK(result[32 * kSize + 26][0] > 0);   // JFA retained the thin foreground's leftward trail.
    CHECK(result[32 * kSize + 48][0] == 0);  // No trail in the opposite direction.
    CHECK(result[4 * kSize + 26][0] == 0);   // No perpendicular wash beyond the silhouette.
    // A later stationary frame must not reuse stale seed coordinates.
    for (auto& v : f.vectors) {
        v = {0, 0, 0, 0};
    }
    CHECK(f.render() == f.colour);
}

void test_jump_flood_radius_and_edges() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    for (int y = 24; y < 40; ++y) {
        const int at = y * kSize + 1;
        f.colour[at] = {255, 0, 0, 255};
        f.depths[at][0] = .6f;
        f.vectors[at] = {32, 0, 0, 0};
    }
    const auto result = f.render();
    CHECK(
        result[32 * kSize + 29][0] > 0
    );  // Several jumps reach a thin source almost 32 pixels away.
    CHECK(result[32 * kSize + 40][0] == 0);  // Capped radius, no wrapping at the texture boundary.
    CHECK(result[4 * kSize + 29][0] == 0);
}

void test_live_blur_tuning() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.moving_square();
    const auto normal = f.render();
    f.tuning.max_radius = 4;
    const auto short_trail = f.render();
    CHECK(normal[32 * kSize + 33][0] > 0);
    CHECK(short_trail[32 * kSize + 33][0] == 0);
    f.tuning = {};
    // Cutscene strength can be disabled independently of gameplay.
    f.camera_relative = true;
    f.tuning.cutscene_multiplier = 0;
    CHECK(f.render() == f.colour);
    f.camera_relative = false;
    CHECK(f.render() == normal);
    // A deliberate motion cutoff suppresses tiny movement without disabling blur globally.
    for (auto& v : f.vectors) {
        v[0] *= .25f;
    }
    f.tuning.min_motion = 4;
    CHECK(f.render() == f.colour);
    f.tuning.min_motion = .1f;
    CHECK(f.render() != f.colour);
}

void test_sample_and_depth_tuning() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    for (int y = 24; y < 40; ++y) {
        const int at = y * kSize + 21;
        f.colour[at] = {255, 0, 0, 255};
        f.depths[at][0] = .6f;
        f.vectors[at] = {8, 0, 0, 0};
    }
    f.tuning.samples = 8;
    const auto low = f.render();
    f.tuning.samples = 64;
    const auto high = f.render();
    CHECK(low != high);  // Sample count affects actual reconstruction, not just the settings file.
    for (int y = 24; y < 40; ++y) {
        for (int x = 23; x < 27; ++x) {
            f.colour[y * kSize + x] = {0, 255, 0, 255};
            f.depths[y * kSize + x][0] = .61f;
        }
    }
    f.tuning.depth_tolerance = .001f;
    const auto strict = f.render();
    f.tuning.depth_tolerance = .05f;
    const auto loose = f.render();
    CHECK(strict[32 * kSize + 25][0] == 0);
    CHECK(loose[32 * kSize + 25][0] > 0);
}

// Packed seeds retain exact source coordinates beyond the half-float integer range.
void test_wide_source_coordinates() {
    constexpr int width = 4099, height = 11;
    std::string error;
    enhanced::fsr3_detail::FrameTarget colour, motion, depth;
    CHECK(colour.create(width, height, GL_RGBA8, 0, error));
    CHECK(motion.create(width, height, 0x881A, 0, error));
    CHECK(depth.create(width, height, 0x8814, 0, error));
    std::vector<std::array<unsigned char, 4>> pixels(width * height, {0, 0, 255, 255});
    std::vector<std::array<float, 4>> vectors(width * height, {0, 0, 0, 0});
    std::vector<std::array<float, 4>> depths(width * height, {.5f, 0, 0, 0});
    for (int y = 0; y < height; ++y) {
        const int source = y * width + 4089;
        pixels[source] = {255, 0, 0, 255};
        vectors[source] = {-8, 0, 0, 0};
        depths[source][0] = .6f;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colour.texture());
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data()
    );
    glBindTexture(GL_TEXTURE_2D, motion.texture());
    glTexImage2D(GL_TEXTURE_2D, 0, 0x881A, width, height, 0, GL_RGBA, GL_FLOAT, vectors.data());
    glBindTexture(GL_TEXTURE_2D, depth.texture());
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8814, width, height, 0, GL_RGBA, GL_FLOAT, depths.data());
    enhanced::fsr3_detail::MotionBlur blur;
    blur.apply(
        {{colour.framebuffer(), width, height},
         {width, height},
         motion.texture(),
         depth.texture(),
         renderer::identity(),
         renderer::identity(),
         {},
         {0, 0, 10},
         {},
         true,
         1.f,
         16.666667f}
    );
    glBindFramebuffer(GL_FRAMEBUFFER, colour.framebuffer());
    std::array<unsigned char, 4> trail{}, untouched{};
    glReadPixels(4083, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, trail.data());
    glReadPixels(4093, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, untouched.data());
    CHECK(trail[0] > 0);
    CHECK(untouched[0] == 0);
    CHECK(glGetError() == GL_NO_ERROR);
    Pixels image(width * height);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    compare_pixels(image);
}

// Dense, opposing motion and depth discontinuities exercise candidate ordering and ties.
void test_dense_motion_equivalence() {
    for (int variant = 0; variant < 4; ++variant) {
        Fixture f;
        std::string error;
        CHECK(f.create(error));
        f.camera_relative = true;
        f.tuning.samples = variant == 0 ? 8 : variant == 1 ? 64 : 32;
        f.jitter = {variant & 1 ? .25f : -.25f, variant & 2 ? .25f : -.25f};
        unsigned state = 12345;
        const auto random = [&state]() {
            state = state * 1664525u + 1013904223u;
            return state;
        };
        for (int i = 0; i < kSize * kSize; ++i) {
            f.colour[i] =
                {static_cast<unsigned char>(random() >> 24),
                 static_cast<unsigned char>(random() >> 24),
                 static_cast<unsigned char>(random() >> 24),
                 255};
            f.depths[i][0] = .2f + .2f * float(random() % 4);
            f.vectors[i] = {float(int(random() % 41) - 20), float(int(random() % 41) - 20), 0, 0};
            if (random() % 4 == 0) {
                f.vectors[i] = {};
            }
        }
        f.render(.5f, variant & 1 ? 33.333334f : 16.666667f);
    }
}

void test_foreground_occlusion() {
    Fixture f;
    std::string error;
    CHECK(f.create(error));
    f.moving_square();
    for (int y = 24; y < 40; ++y) {
        for (int x = 29; x < 33; ++x) {
            f.colour[y * kSize + x] = {0, 255, 0, 255};
            f.depths[y * kSize + x][0] = .9f;
        }
    }
    const auto result = f.render();
    CHECK(result[32 * kSize + 30] == f.colour[32 * kSize + 30]);
}
}  // namespace

int main(int argc, char**) {
    write_preview = argc > 1;
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::cerr << error << '\n';
        return test::kSkip;
    }
    for (bool direct : {false, true}) {
        renderer::render_opt::set(renderer::render_opt::Opt::MotionBlurFastPath, direct);
        compare_direct = direct;
        test_cutscene_camera_blur();
        test_target_reference();
        test_jump_with_stationary_target();
        test_translation_relative_blur();
        test_reference_at_different_depths();
        test_camera_cancellation();
        test_object_motion_and_controls();
        test_live_blur_tuning();
        test_sample_and_depth_tuning();
        test_foreground_occlusion();
        test_thin_opposing_motion();
        test_jump_flood_radius_and_edges();
        test_wide_source_coordinates();
        test_dense_motion_equivalence();
    }
    CHECK(compared_frames == reference_frames.size());
    std::cout << "Compared " << compared_frames << " GPU frames byte-for-byte\n";
    renderer::render_opt::reset();
    CHECK(vkgl::submission_counters().validation_errors == 0);
    return test::result();
}
