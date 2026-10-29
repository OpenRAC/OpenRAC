// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Fixture uploads and exact reference comparisons for the motion blur GPU tests.
#include "enhanced/tests/motion_blur_fixture.h"

#include "renderer/gl.h"
#include "tests/check.h"

namespace openrac::test::motion_blur {
using namespace openrac::gl;

std::vector<Pixels> reference_frames;
std::size_t compared_frames = 0;
bool compare_direct = false;

void compare_pixels(const Pixels& result) {
    if (compare_direct) {
        CHECK(compared_frames < reference_frames.size());
        if (compared_frames < reference_frames.size()) {
            CHECK(result == reference_frames[compared_frames]);
        }
        ++compared_frames;
    } else {
        reference_frames.push_back(result);
    }
}

bool Fixture::create(std::string& error) {
    return output.create(kSize, kSize, error) && motion.create(kSize, kSize, 0x881A, 0, error)
           && depth.create(kSize, kSize, 0x8814, 0, error);
}

void Fixture::moving_square(bool reject) {
    for (int y = 24; y < 40; ++y) {
        for (int x = 20; x < 28; ++x) {
            const int i = y * kSize + x;
            colour[i] = {255, 0, 0, 255};
            vectors[i][0] += 8;
            vectors[i][2] = reject ? 1.f : 0.f;
            depths[i][0] = .6f;
        }
    }
}

Pixels Fixture::render(float strength, float ms) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, output.colour_texture());
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, colour.data()
    );
    glBindTexture(GL_TEXTURE_2D, motion.texture());
    glTexImage2D(GL_TEXTURE_2D, 0, 0x881A, kSize, kSize, 0, GL_RGBA, GL_FLOAT, vectors.data());
    glBindTexture(GL_TEXTURE_2D, depth.texture());
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8814, kSize, kSize, 0, GL_RGBA, GL_FLOAT, depths.data());
    unsigned source = 0;
    if (compare_direct) {
        source = blur.reconstruction_output(kSize, kSize);
        CHECK(source != 0);
        glBindTexture(GL_TEXTURE_2D, source);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, colour.data()
        );
        // The direct resolve must write every stationary pixel, not rely on existing display data.
        const Pixels green(kSize * kSize, {0, 255, 0, 255});
        glBindTexture(GL_TEXTURE_2D, output.colour_texture());
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, green.data()
        );
    }
    const bool applied = blur.apply(
        {{output.id(), kSize, kSize},
         {kSize, kSize},
         motion.texture(),
         depth.texture(),
         inverse_raster,
         previous,
         jitter,
         {0, 0, 10},
         reference_delta,
         camera_relative,
         strength,
         ms,
         tuning,
         source}
    );
    if (!applied && source) {
        blur.present_reconstruction({output.id(), kSize, kSize});
    }
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    Pixels result(kSize * kSize);
    glReadPixels(0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
    CHECK(glGetError() == GL_NO_ERROR);
    compare_pixels(result);
    return result;
}

Fixture::~Fixture() {
    blur.release();
}
}  // namespace openrac::test::motion_blur
