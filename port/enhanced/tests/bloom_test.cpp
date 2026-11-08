// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bloom stays off in compatibility rendering, preserves alpha and spreads bright colour on Vulkan.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "enhanced/renderer/bloom.h"
#include "renderer/gl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
int rgb_sum(const std::vector<std::uint8_t>& pixels, int width, int x, int y) {
    const std::size_t p = (static_cast<std::size_t>(y) * width + x) * 4;
    return int(pixels[p]) + int(pixels[p + 1]) + int(pixels[p + 2]);
}

std::uint64_t rgb_total(const std::vector<std::uint8_t>& pixels) {
    std::uint64_t total = 0;
    for (std::size_t p = 0; p < pixels.size(); p += 4) {
        total += pixels[p] + pixels[p + 1] + pixels[p + 2];
    }
    return total;
}

int draw_patch(openrac::renderer::FrameBuffer& target) {
    using namespace openrac::gl;
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, target.width(), target.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 77.f / 255.f);
    glClear(GL_COLOR_BUFFER_BIT);

    const int side = std::min({8, target.width() - 2, target.height() - 2});
    const int left = (target.width() - side) / 2;
    const int bottom = (target.height() - side) / 2;
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, bottom, side, side);
    glClearColor(1.f, 0.85f, 0.35f, 77.f / 255.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    return std::min(target.width() - 1, left + side);
}

void check_frame(openrac::enhanced::Bloom& bloom, int width, int height) {
    using namespace openrac;
    using namespace openrac::gl;

    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(width, height, error, 1));

    const int near_x = draw_patch(target);
    const int cy = target.height() / 2;
    const auto before = target.read_rgba();
    bloom.apply(target, 1.0f);
    const auto strong = target.read_rgba();

    if (enhanced_rendering_available()) {
        CHECK(rgb_total(strong) > rgb_total(before));

        // On normal frame sizes the Gaussian halo is stronger next to the patch than at a corner.
        if (width >= 32 && height >= 24) {
            const int near_before = rgb_sum(before, target.width(), near_x, cy);
            const int near_strong = rgb_sum(strong, target.width(), near_x, cy);
            const int far_strong = rgb_sum(strong, target.width(), 0, 0);
            CHECK(near_before == 0);
            CHECK(near_strong > near_before);
            CHECK(near_strong > far_strong);
        }

        for (std::size_t p = 3; p < strong.size(); p += 4) {
            CHECK(strong[p] == before[p]);
        }

        draw_patch(target);
        bloom.apply(target);
        const auto subtle = target.read_rgba();
        CHECK(rgb_total(subtle) > rgb_total(before));
        CHECK(rgb_total(subtle) < rgb_total(strong));
    } else {
        CHECK(strong == before);
    }

    target.release();
}
}  // namespace

int main(int argc, char**) {
    using namespace openrac;
    using namespace openrac::gl;

    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return test::kSkip;
    }

    if (argc > 1) {
        CHECK(is_vulkan());
        CHECK(enhanced_rendering_available());
    }

    enhanced::Bloom bloom;
    check_frame(bloom, 129, 97);
    check_frame(bloom, 47, 31);
    check_frame(bloom, 9, 7);
    bloom.release();

    CHECK(glGetError() == GL_NO_ERROR);
    std::puts("Bloom compatibility, resize, alpha and Gaussian spread checks passed");
}
