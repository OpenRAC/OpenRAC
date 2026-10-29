// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <cmath>
#include <cstdio>

#include "enhanced/renderer/agx.h"
#include "renderer/gl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

int main(int argc, char**) {
    using namespace openrac;
    using namespace openrac::gl;
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return test::kSkip;
    }
    if (argc > 1) { CHECK(enhanced_rendering_available()); }
    enhanced::AgxPreview agx;
    renderer::FrameBuffer target;
    // Expected bytes evaluated independently from the HLSL CPU reference equations.
    const std::array<std::array<int, 3>, 6> input{
        {{0, 0, 0}, {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {64, 128, 192}}
    };
    const std::array<std::array<int, 3>, 6> expected{
        {{0, 0, 0}, {127, 127, 127}, {219, 57, 34}, {110, 197, 81}, {30, 84, 211}, {76, 132, 177}}
    };
    const std::array<std::array<int, 3>, 6> exposed{
        {{0, 0, 0},
         {163, 163, 163},
         {244, 106, 88},
         {151, 219, 131},
         {82, 120, 233},
         {123, 165, 203}}
    };
    for (int size : {8, 13}) {
        CHECK(target.create(size, size, error));
        for (int ev : {0, 1}) {
            for (std::size_t n = 0; n < input.size(); ++n) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                glDisable(GL_SCISSOR_TEST);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glClearColor(
                    input[n][0] / 255.f, input[n][1] / 255.f, input[n][2] / 255.f, 77 / 255.f
                );
                glClear(GL_COLOR_BUFFER_BIT);
                agx.apply(target, float(ev));
                const auto pixels = target.read_rgba();
                const auto& want =
                    enhanced_rendering_available() ? (ev ? exposed[n] : expected[n]) : input[n];
                for (std::size_t p = 0; p < pixels.size(); p += 4) {
                    for (std::size_t c = 0; c < 3; ++c) {
                        CHECK(std::abs(int(pixels[p + c]) - want[c]) <= 2);
                    }
                    CHECK(pixels[p + 3] == 77);
                }
            }
        }
    }
    CHECK(glGetError() == GL_NO_ERROR);
    std::puts("AgX reference colour, exposure, alpha and resize checks passed");
}
