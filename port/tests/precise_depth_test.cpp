// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Precise world depth (renderer/gl/precise_depth.h): two walls facing the camera, the nearer one
// 1 cm in front, drawn nearer first with the reversed test (GEQUAL, cleared to 0). Wherever the
// depth cannot part them the farther one wins the tie and shows. GL's form keeps 2^-24 of the
// range after (z + w) / 2, so from about 150 units on (near 0.05) it cannot; precise depth (a
// float buffer, glClipControl GL_ZERO_TO_ONE, zero_to_one()) parts them at every distance drawn.
// Skipped where the context has no glClipControl (macOS's OpenGL 4.1).

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/precise_depth.h"
#include "renderer/gl/shader.h"
#include "renderer/math.h"
#include "tests/renderer_gl_context.h"

using namespace openrac;
using namespace openrac::gl;

namespace {

constexpr int kSize = 64;
constexpr float kGap = 0.01f;  // game units between the walls

// World positions, drawn as the level renderer draws them: a view and projection made on the CPU.
const char* const kVertex = R"(#version 410 core
layout(location = 0) in vec3 position;
uniform mat4 view_projection;
void main() {
    gl_Position = view_projection * vec4(position, 1.0);
}
)";

// The camera: somewhere in a level, as Maktar's landing pad is, looking off the axes; the large
// coordinates are what GL's (z + w) / 2 cancels against.
const renderer::Vec3 kEye{512.3f, 317.9f, 121.4f};
const renderer::Vec3 kForward = renderer::normalize({0.31f, 0.94f, -0.12f});
const renderer::Vec3 kUp{0.0f, 0.0f, 1.0f};

// A wall facing the camera `distance` along its view, across the whole view (90 degrees).
void upload_wall(GLuint buffer, float distance) {
    const renderer::Vec3 right = renderer::normalize(renderer::cross(kForward, kUp));
    const renderer::Vec3 up = renderer::cross(right, kForward);
    float corners[12];
    const float s[4][2] = {{-2, -2}, {2, -2}, {-2, 2}, {2, 2}};
    for (int i = 0; i < 4; ++i) {
        for (int k = 0; k < 3; ++k) {
            corners[i * 3 + k] = kEye[k] + kForward[k] * distance
                                 + (right[k] * s[i][0] + up[k] * s[i][1]) * distance;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof corners, corners, GL_DYNAMIC_DRAW);
}

const char* const kFragment = R"(#version 410 core
uniform vec4 colour;
out vec4 frag;
void main() {
    frag = colour;
}
)";

// The share of the picture where the farther wall shows over the nearer one, at `distance`.
double lost(
    GLuint buffer,
    const renderer::Shader& shader,
    const renderer::FrameBuffer& frame,
    const renderer::Mat4& projection,
    bool precise,
    float distance
) {
    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glViewport(0, 0, kSize, kSize);
    glClearColor(0, 0, 0, 1);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    if (precise) {
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    }
    shader.use();
    const renderer::Mat4 drawn = renderer::multiply(
        precise ? renderer::zero_to_one(projection) : projection,
        renderer::look_along(kEye, kForward, kUp)
    );
    glUniformMatrix4fv(shader.uniform("view_projection"), 1, GL_FALSE, drawn.data());
    // The nearer wall first (green), then the farther (red): a tie goes to the farther.
    upload_wall(buffer, distance - kGap);
    glUniform4f(shader.uniform("colour"), 0, 1, 0, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    upload_wall(buffer, distance);
    glUniform4f(shader.uniform("colour"), 1, 0, 0, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (precise) {
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
    }
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frame.id());
    glReadPixels(0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    int red = 0;
    int drawn_pixels = 0;
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        red += pixels[i] > 128 ? 1 : 0;
        drawn_pixels += pixels[i] > 128 || pixels[i + 1] > 128 ? 1 : 0;
    }
    // Every pixel is one wall or the other.
    CHECK(drawn_pixels == kSize * kSize);
    return static_cast<double>(red) / (kSize * kSize);
}

}  // namespace

int main() {
    test::GlContext context;
    std::string why;
    if (!context.open(why, kSize, kSize)) {
        std::printf("SKIP: no OpenGL 4.1 core context: %s\n", why.c_str());
        return test::kSkip;
    }
    if (glClipControl == nullptr) {
        std::printf("SKIP: no glClipControl (OpenGL 4.5 or ARB_clip_control)\n");
        return test::kSkip;
    }

    renderer::Shader shader;
    std::string error;
    if (!shader.build("precise depth test", kVertex, kFragment, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    upload_wall(vbo, 1.0f);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // The window's frame buffers in GL's form, and in precise depth's float one.
    renderer::FrameBuffer plain, precise;
    renderer::request_precise_depth(false);
    CHECK(plain.create(kSize, kSize, error));
    renderer::request_precise_depth(true);
    CHECK(renderer::precise_depth());
    CHECK(precise.create(kSize, kSize, error));
    renderer::request_precise_depth(false);

    // The scene's projection (scene_frame.cpp): near 0.05, far 2000; 90 degrees, so a wall of
    // 2 x distance either way fills the view.
    const renderer::Mat4 projection =
        renderer::perspective_reversed(1.5707964f, 1.0f, 0.05f, 2000.0f);
    double gl_lost_far = 0.0;
    for (const float distance : {10.0f, 50.0f, 150.0f, 300.0f, 600.0f, 1500.0f}) {
        const double gl_form = lost(vbo, shader, plain, projection, false, distance);
        const double precise_form = lost(vbo, shader, precise, projection, true, distance);
        std::printf(
            "walls 1 cm apart at %5.0f units: the far wall shows over %5.1f%% (GL's form), %5.1f%% "
            "(precise)\n",
            distance,
            gl_form * 100.0,
            precise_form * 100.0
        );
        // Precise depth parts them at every distance; GL's form only near. At 50 units the
        // walls are 2e-7 apart in depth, and GL's z + w (the shader's, under Vulkan) rounds by
        // up to 8e-8: whether they part there is the driver's rounding (llvmpipe in Mesa 26.0
        // loses 87.7%, in 26.2 none).
        CHECK(precise_form == 0.0);
        if (distance <= 10.0f) {
            CHECK(gl_form == 0.0);
        } else {
            gl_lost_far += gl_form;
        }
    }
    // GL's form loses somewhere far: what the test is for (when it stops, it shows nothing).
    CHECK(gl_lost_far > 0.0);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    return test::result();
}
