// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "renderer/framebuffer.h"
#include "renderer/shader.h"
#include "renderer/vulkan/native_compute.h"
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
out vec4 colour;
void main(){ colour=vec4(vec3(texelFetch(depth,ivec2(gl_FragCoord.xy),0).r),1.); }
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
    unsigned textures[3]{};
    glGenTextures(3, textures);
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
    renderer::FrameBuffer target;
    CHECK(target.create(size, size, error));
    renderer::Shader display;
    CHECK(display.build("depth union test", kVertex, kFragment, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    for (const int offset : {-9, 0, 3, 15}) {
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
        context.window().swap();
    }
    CHECK(vkgl::submission_counters().validation_errors == 0);
    glDeleteVertexArrays(1, &vao);
    display.release();
    target.release();
    glDeleteTextures(3, textures);
    return test::result();
}
