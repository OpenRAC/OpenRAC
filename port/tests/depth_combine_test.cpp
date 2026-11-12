// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compares shadow combination against arithmetic and the original barriers, including exact R32F samples.
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "renderer/gl/framebuffer.h"
#include "renderer/gl/shader.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/passes/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
constexpr auto kVertex = R"(#version 410 core
void main(){ vec2 p=vec2((gl_VertexID<<1)&2, gl_VertexID&2); gl_Position=vec4(p*2.-1.,0.,1.); }
)";
constexpr auto kFragment = R"(#version 410 core
uniform sampler2D depth;
uniform sampler2D reference_depth;
uniform int compare_mode;
out vec4 colour;
void main(){
    ivec2 pixel=ivec2(gl_FragCoord.xy);
    float value=texelFetch(depth,pixel,0).r;
    if(compare_mode!=0) {
        float reference=texelFetch(reference_depth,pixel,0).r;
        colour=vec4(value==reference?vec3(0):vec3(1),1);
    } else colour=vec4(vec3(value),1);
}
)";
}  // namespace

int main() {
    using namespace openrac;
    using namespace gl;
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !vkgl::active()) {
        return test::kSkip;
    }
    constexpr int size = 8;
    unsigned textures[4]{};
    glGenTextures(4, textures);
    std::array<float, size * size * 4> cached{};
    constexpr int atlas_size = size * 2;
    std::array<float, atlas_size * atlas_size * 4> dynamic{};
    for (int i = 0; i < size * size; ++i) {
        cached[i * 4] = static_cast<float>(i % 7) / 8;
    }
    for (int i = 0; i < atlas_size * atlas_size; ++i) {
        dynamic[i * 4] = static_cast<float>((i * 3) % 7) / 8;
    }
    for (unsigned i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            renderer::world::gl::GL_RGBA32F,
            i == 0 ? size : atlas_size,
            i == 0 ? size : atlas_size,
            0,
            GL_RGBA,
            GL_FLOAT,
            i == 0 ? cached.data() : dynamic.data()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    CHECK(vkgl::allocate_storage_texture(textures[2], 0x822E, size, size));
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    CHECK(vkgl::allocate_storage_texture(textures[3], 0x822E, size, size));
    renderer::FrameBuffer target;
    CHECK(target.create(size, size, error));
    renderer::Shader display;
    CHECK(display.build("depth union test", kVertex, kFragment, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    for (const int offset : {-9, 0, 3, 15}) {
        // Compare repeated graph recordings using separate R32F outputs.

        CHECK(
            vkgl::combine_shadow_depth(
                textures[0], textures[1], textures[3], {offset, offset}, {3, 5, size, size}
            )
        );

        CHECK(
            vkgl::combine_shadow_depth(
                textures[0], textures[1], textures[2], {offset, offset}, {3, 5, size, size}
            )
        );
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, size, size);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glBindVertexArray(vao);
        display.use();
        glUniform1i(display.uniform("depth"), 0);
        glUniform1i(display.uniform("compare_mode"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[2]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::array<unsigned char, size * size * 4> pixels{};
        glReadPixels(0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        const int shift = (offset % size + size) % size;
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const int at = (y * size + x) * 4;
                const int wrapped = (((y + shift) % size) * size + (x + shift) % size) * 4;
                const int expected = static_cast<int>(std::lround(
                    255 * std::max(cached[wrapped], dynamic[((y + 5) * atlas_size + x + 3) * 4])
                ));
                CHECK(std::abs(static_cast<int>(pixels[at]) - expected) <= 1);
            }
        }
        // Equality is tested on the sampled float values, not their quantised display colours.
        glUniform1i(display.uniform("compare_mode"), 1);
        glUniform1i(display.uniform("reference_depth"), 1);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, textures[3]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        for (std::size_t at = 0; at < static_cast<std::size_t>(size) * size; ++at) {
            CHECK(pixels[at * 4] == 0 && pixels[at * 4 + 1] == 0 && pixels[at * 4 + 2] == 0);
        }
        context.window().swap();
    }
    // Repeated writes and duplicate read-only aliases retain a valid graph boundary.
    for (unsigned repeat = 0; repeat < 2; ++repeat) {
        CHECK(
            vkgl::combine_shadow_depth(
                textures[0], textures[0], textures[2], {1, -1}, {0, 0, size, size}
            )
        );
    }
    CHECK(!vkgl::combine_shadow_depth(
        textures[2], textures[0], textures[2], {0, 0}, {0, 0, size, size}
    ));
    CHECK(!vkgl::combine_shadow_depth(
        textures[0],
        textures[1],
        textures[2],
        {0, 0},
        {std::numeric_limits<int>::max(), 0, size, size}
    ));
    // Real shadow maps carry depth/stencil together on the legacy backend.
    renderer::FrameBuffer depth_target;
    CHECK(depth_target.create(size, size, error, 1, true));
    glBindFramebuffer(GL_FRAMEBUFFER, depth_target.id());
    glDepthMask(GL_TRUE);
    glClearDepth(.625);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // Stencil contents are irrelevant to the depth-only sampled view.
    const unsigned discard = GL_STENCIL_ATTACHMENT;
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &discard);
    CHECK(
        vkgl::combine_shadow_depth(
            textures[0], depth_target.depth_texture(), textures[2], {0, 0}, {0, 0, size, size}
        )
    );

    CHECK(
        vkgl::combine_shadow_depth(
            textures[0], depth_target.depth_texture(), textures[3], {0, 0}, {0, 0, size, size}
        )
    );

    CHECK(
        vkgl::combine_shadow_depth(
            textures[0], depth_target.depth_texture(), textures[2], {0, 0}, {0, 0, size, size}
        )
    );
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, size, size);
    glDisable(GL_DEPTH_TEST);
    display.use();
    glUniform1i(display.uniform("compare_mode"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::array<unsigned char, static_cast<std::size_t>(size) * size * 4> depth_pixels{};
    glReadPixels(0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, depth_pixels.data());
    for (std::size_t at = 0; at < static_cast<std::size_t>(size) * size; ++at) {
        const int expected = static_cast<int>(std::lround(255 * std::max(cached[at * 4], .625f)));
        CHECK(std::abs(static_cast<int>(depth_pixels[at * 4]) - expected) <= 1);
    }
    glUniform1i(display.uniform("compare_mode"), 1);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::array<unsigned char, static_cast<std::size_t>(size) * size * 4> differences{};
    glReadPixels(0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, differences.data());
    for (std::size_t at = 0; at < static_cast<std::size_t>(size) * size; ++at) {
        CHECK(
            differences[at * 4] == 0 && differences[at * 4 + 1] == 0 && differences[at * 4 + 2] == 0
        );
    }
    depth_target.release();
    CHECK(vkgl::submission_counters().validation_errors == 0);
    glDeleteVertexArrays(1, &vao);
    display.release();
    target.release();
    glDeleteTextures(4, textures);
    renderer::render_opt::reset();
    return test::result();
}
