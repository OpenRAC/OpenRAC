// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

#include "enhanced/tests/water_gpu_fixture.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"

namespace openrac::test::water {
namespace {
// Interpolating cubic in polynomial form, independent of the shader's merged texture taps.
double cubic(double a, double b, double c, double d, double t) {
    return b + .5 * t * (c - a + t * (2 * a - 5 * b + 4 * c - d + t * (3 * (b - c) + d - a)));
}

double reference(const std::array<Cell, kCount>& image, double x, double y) {
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const auto at = [&](int a, int b) {
        const int width = int(kWidth);
        return image[((b % width + width) % width) * width + (a % width + width) % width][0];
    };
    std::array<double, 4> rows{};
    for (int row = 0; row < 4; ++row) {
        const int b = iy + row - 1;
        rows[row] = cubic(at(ix - 1, b), at(ix, b), at(ix + 1, b), at(ix + 2, b), x - ix);
    }
    return cubic(rows[0], rows[1], rows[2], rows[3], y - iy);
}
}  // namespace

void check_sampling(Context& fixture) {
    using namespace gl;
    renderer::Shader shader;
    const std::string fragment =
        std::string("#version 410 core\n") + lighting::shaders::water_window_glsl + R"(
uniform sampler2D field;
layout(location=0) out vec4 result;
void main() {
    ivec2 at=ivec2(gl_FragCoord.xy);
    vec2 p=at.y==0?vec2(float(at.x)-8.,7.):vec2(at)*vec2(.37,.41)-vec2(2.5,3.2);
    float height=water_sim_bicubic_height(field,(p+.5)/vec2(textureSize(field,0)));
    uint bits=floatBitsToUint(height);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    const bool built =
        shader.build("bicubic water height", fixture.vertex, fragment, fixture.error);
    CHECK(built);
    if (!built) {
        return;
    }
    std::array<Cell, kCount> image{};
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fixture.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    for (bool constant : {true, false}) {
        for (unsigned y = 0; y < kWidth; ++y) {
            for (unsigned x = 0; x < kWidth; ++x) {
                image[y * kWidth + x][0] =
                    constant ? -.037f : float(.04 * std::sin(x * 1.3) + .02 * std::cos(y * 1.7));
            }
        }
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            renderer::world::gl::GL_RGBA32F,
            kWidth,
            kWidth,
            0,
            GL_RGBA,
            GL_FLOAT,
            image.data()
        );
        shader.use();
        glUniform1i(shader.uniform("field"), 0);
        glViewport(0, 0, kWidth, kWidth);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
        for (unsigned y = 0; y < kWidth; ++y) {
            for (unsigned x = 0; x < kWidth; ++x) {
                const unsigned offset = (y * kWidth + x) * 4;
                const auto& pixels = fixture.pixels;
                const auto bits = std::uint32_t(pixels[offset])
                                  | (std::uint32_t(pixels[offset + 1]) << 8)
                                  | (std::uint32_t(pixels[offset + 2]) << 16)
                                  | (std::uint32_t(pixels[offset + 3]) << 24);
                const float actual = std::bit_cast<float>(bits);
                const double px = y == 0 ? double(x) - 8 : double(x) * .37 - 2.5;
                const double py = y == 0 ? 7 : double(y) * .41 - 3.2;
                CHECK(std::isfinite(actual));
                // Allow hardware bilinear subtexel rounding, while checking signed heights,
                // exact grid samples, fractional positions and both periodic boundaries.
                CHECK(
                    std::abs(actual - reference(image, px, py)) < (constant || y == 0 ? 2e-6 : 2e-4)
                );
            }
        }
    }
}
}  // namespace openrac::test::water
