// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Dear ImGui drawn with the renderer's own GL (renderer/ui/imgui_gl.h), as the Vulkan renderer's
// window draws the debug overlay and the Android drawer: a window's filled background lands where
// ImGui put it, in its colour, and nothing outside it. Run on OpenGL, ES (imgui_gl_es) and, under
// OPENRAC_GL_API=vulkan, the Vulkan renderer. Skipped (exit 77) where no context can be made.

#include <cstdio>
#include <imgui.h>
#include <string>

#include "check.h"
#include "renderer/gl/gl.h"
#include "renderer/ui/backdrop_blur.h"
#include "renderer/ui/imgui_gl.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac;
using namespace openrac::gl;

constexpr int kWidth = 320;
constexpr int kHeight = 180;

// The pixel at (x, y) counted from the top, as ImGui counts.
void pixel(int x, int y, unsigned char rgba[4]) {
    glReadPixels(x, kHeight - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

// A constant-colour background is desaturated equally in a fresh blur and an eased second blur.
void test_backdrop_blur() {
    constexpr int kRows = kHeight / 2;
    constexpr float kSigma = 8.0f;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // A primary colour has the same stored values in linear and sRGB window buffers.
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    renderer::ui::backdrop_blur::blur(kWidth, kHeight, kRows, kSigma);
    const auto blurred = renderer::ui::backdrop_blur::blur(kWidth, kHeight, kRows, kSigma);
    CHECK(blurred.texture != 0);
    CHECK(blurred.rows == kRows);

    GLuint read = 0;
    glGenFramebuffers(1, &read);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    glFramebufferTexture2D(
        GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurred.texture, 0
    );
    unsigned char rgba[4] = {};
    glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    // The last Gaussian mixes in 20 percent luminance (0.072), giving about (4, 4, 208).
    CHECK(
        rgba[0] >= 2 && rgba[0] <= 6 && rgba[1] >= 2 && rgba[1] <= 6 && rgba[2] >= 206
        && rgba[2] <= 210
    );
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &read);
    renderer::ui::backdrop_blur::release();
}

}  // namespace

int main() {
    test::GlContext context;
    std::string why;
    if (!context.open(why, kWidth, kHeight)) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return test::kSkip;
    }
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(kWidth, kHeight);
    std::string error;
    CHECK(renderer::ui::imgui_gl::init(error));
    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(40.0f, 30.0f));
    ImGui::SetNextWindowSize(ImVec2(100.0f, 60.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::TextUnformatted("OpenRAC");
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::Render();
    renderer::ui::imgui_gl::render(ImGui::GetDrawData());

    unsigned char inside[4] = {};
    unsigned char outside[4] = {};
    pixel(130, 80, inside);  // in the window, clear of its text
    pixel(200, 120, outside);
    CHECK(inside[0] > 200 && inside[1] < 40 && inside[2] < 40);
    CHECK(outside[0] == 0 && outside[1] == 0 && outside[2] == 0);
    test_backdrop_blur();
    CHECK(glGetError() == GL_NO_ERROR);

    renderer::ui::imgui_gl::shutdown();
    ImGui::DestroyContext();
    return test::result();
}
