// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/backdrop_blur.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>

#include "common/log.h"
#include "renderer/gl.h"
#include "renderer/shader.h"

namespace openrac::renderer::ui::backdrop_blur {
namespace {

using namespace openrac::gl;

constexpr const char* kVertex = R"glsl(#version 410 core
out vec2 v_uv;
void main() {
    const vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    v_uv = corners[gl_VertexID] * 0.5 + 0.5;
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)glsl";

// One direction of the Gaussian: `step` is one texel along it, in texture units.
constexpr const char* kFragment = R"glsl(#version 410 core
in vec2 v_uv;
uniform sampler2D tex_T0;
uniform vec2 step;
uniform float sigma;
uniform float calm;
out vec4 out_colour;
void main() {
    int radius = int(ceil(sigma * 2.5));
    vec4 sum = texture(tex_T0, v_uv);
    float total = 1.0;
    for (int i = 1; i <= radius; ++i) {
        float w = exp(-float(i * i) / (2.0 * sigma * sigma));
        sum += w * (texture(tex_T0, v_uv + step * float(i)) + texture(tex_T0, v_uv - step * float(i)));
        total += 2.0 * w;
    }
    vec3 c = sum.rgb / total;
    // Calmer colours (the last pass only): a little less saturated, the brightest pulled down, so
    // something bright under the menu does not glare through it.
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    c = mix(c, mix(vec3(lum), c, 0.8) * (1.0 - 0.3 * smoothstep(0.5, 1.0, lum)), calm);
    out_colour = vec4(c, 1.0);
}
)glsl";

// Halvings at most: the blur runs where its sigma is about three texels.
constexpr int kLevels = 5;

struct Target {
    GLuint texture = 0;
    GLuint framebuffer = 0;
    int width = 0;
    int height = 0;
};

struct State {
    Shader shader;
    GLuint vao = 0;
    std::array<Target, kLevels + 1> levels{};  // [0] the first half, then halves again
    Target across;                             // the horizontal pass's result
    Target shown;                              // what is shown: eased toward each new blur
    std::chrono::steady_clock::time_point last{};
    bool failed = false;
};

std::unique_ptr<State> s;

bool fit(Target& t, int width, int height) {
    if (t.texture != 0 && t.width == width && t.height == height) {
        return true;
    }
    if (t.texture == 0) {
        glGenTextures(1, &t.texture);
        glGenFramebuffers(1, &t.framebuffer);
    }
    glBindTexture(GL_TEXTURE_2D, t.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, t.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.texture, 0);
    t.width = width;
    t.height = height;
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void free_target(Target& t) {
    glDeleteFramebuffers(1, &t.framebuffer);
    glDeleteTextures(1, &t.texture);
    t = Target{};
}

// One pass of the Gaussian from `from` into `to`, along (dx, dy) texels.
void pass(
    const Target& from, const Target& to, float dx, float dy, float sigma, float calm = 0.0f
) {
    glBindFramebuffer(GL_FRAMEBUFFER, to.framebuffer);
    glViewport(0, 0, to.width, to.height);
    glBindTexture(GL_TEXTURE_2D, from.texture);
    glUniform2f(
        s->shader.uniform("step"),
        dx / static_cast<float>(from.width),
        dy / static_cast<float>(from.height)
    );
    glUniform1f(s->shader.uniform("sigma"), sigma);
    glUniform1f(s->shader.uniform("calm"), calm);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

}  // namespace

Picture blur(int width, int height, int rows, float sigma) {
    rows = std::min(rows, height);
    if (width <= 0 || rows <= 0 || !(sigma > 0.0f)) {
        return {};
    }
    if (s == nullptr) {
        s = std::make_unique<State>();
        std::string error;
        if (!s->shader.build("Backdrop blur", kVertex, kFragment, error)) {
            log::error("The menu's blur is off: {}", error);
            s->failed = true;
        } else {
            glGenVertexArrays(1, &s->vao);
        }
    }
    if (s->failed) {
        return {};
    }
    // Halved until the blur is a few texels wide there: each halving averages four pixels.
    int level = 0;
    float left = sigma / 2.0f;
    while (level + 1 < kLevels && left > 3.0f) {
        ++level;
        left /= 2.0f;
    }
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    int w = width;
    int h = rows;
    for (int i = 0; i <= level; ++i) {
        w = std::max(1, (w + 1) / 2);
        h = std::max(1, (h + 1) / 2);
        if (!fit(s->levels[i], w, h)) {
            s->failed = true;
            log::error("The menu's blur is off: its framebuffer is incomplete");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return {};
        }
        // The window's top rows first, then each level from the one before.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, i == 0 ? 0 : s->levels[i - 1].framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s->levels[i].framebuffer);
        const int sx0 = 0;
        const int sy0 = i == 0 ? height - rows : 0;
        const int sx1 = i == 0 ? width : s->levels[i - 1].width;
        const int sy1 = i == 0 ? height : s->levels[i - 1].height;
        glBlitFramebuffer(sx0, sy0, sx1, sy1, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    Target& out = s->levels[level];
    if (!fit(s->across, w, h)) {
        s->failed = true;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return {};
    }
    glBindVertexArray(s->vao);
    s->shader.use();
    glActiveTexture(GL_TEXTURE0);
    pass(out, s->across, 1.0f, 0.0f, left);
    pass(s->across, out, 0.0f, 1.0f, left, 1.0f);
    // Eased toward the new blur over about a tenth of a second, so a flash under the menu fades
    // in and out instead of jumping. A new size or a menu just opened takes it at once.
    const auto now = std::chrono::steady_clock::now();
    const float since = std::chrono::duration<float>(now - s->last).count();
    s->last = now;
    const bool fresh = s->shown.width != w || s->shown.height != h || since > 0.25f;
    if (!fit(s->shown, w, h)) {
        s->failed = true;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return {};
    }
    if (!fresh) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
        glBlendEquation(GL_FUNC_ADD);
        glBlendColor(0.0f, 0.0f, 0.0f, 1.0f - std::exp(-since / 0.1f));
    }
    pass(out, s->shown, 0.0f, 0.0f, 0.01f);  // a copy: a Gaussian this narrow is one texel
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    return {s->shown.texture, rows};
}

void release() {
    if (s == nullptr) {
        return;
    }
    s->shader.release();
    glDeleteVertexArrays(1, &s->vao);
    for (Target& t : s->levels) {
        free_target(t);
    }
    free_target(s->across);
    free_target(s->shown);
    s.reset();
}

}  // namespace openrac::renderer::ui::backdrop_blur
