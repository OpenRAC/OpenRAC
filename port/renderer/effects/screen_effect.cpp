// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements screen_effect.h's post passes, with separate input and output textures at every size.
#include "renderer/effects/screen_effect.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer_shaders.h"

namespace openrac::renderer {
using namespace openrac::gl;

namespace {

// The shader's passes (screen_effect.frag `mode`).
constexpr int kCopy = 0;      // the frame, tinted, its pixels under the threshold left black
constexpr int kBlur = 1;      // a 3 x 3 binomial blur, from four bilinear reads
constexpr int kResample = 2;  // one bilinear read: a size from the next
constexpr int kAdd = 3;       // the frame with the blurred copy added
constexpr int kMix = 4;       // the frame mixed toward the blurred copy

// The game's units: a colour or alpha of 0x80 is 1.0.
constexpr float kUnit = 128.0f;

// The blur passes at each size, as the game's glow builds them (rac3 0x004DDE98, 0x004DE5B0):
// the largest copy blurred again only when the game asks (`blur`), the quarter once more than
// that, the eighth five times (three blurs and two four-tap passes), and the half once more after
// the copy is made large again.
constexpr int kQuarterBlurs = 1;
constexpr int kEighthBlurs = 5;
constexpr int kHalfBlursAfter = 1;
// The second effect (rac3 0x004DF070): the half-size copy blurred twice.
constexpr int kSoftenBlurs = 2;

// The bytes of a 0xAABBGGRR colour as 0x80 = 1.0.
std::array<float, 4> channels(std::uint32_t rgba) {
    constexpr int kByte = 8;
    constexpr std::uint32_t kMask = 0xFFu;
    std::array<float, 4> out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<float>((rgba >> (kByte * i)) & kMask) / kUnit;
    }
    return out;
}

}  // namespace

ScreenEffects::~ScreenEffects() {
    release();
}

void ScreenEffects::release() {
    m_shader.release();
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_frame != 0) {
        glDeleteTextures(1, &m_frame);
        m_frame = 0;
    }
    m_frame_width = m_frame_height = 0;
    for (Level& level : m_levels) {
        level.buffers[0].release();
        level.buffers[1].release();
        level.current = 0;
    }
    m_failed = false;
}

bool ScreenEffects::prepare(int width, int height) {
    std::string error;
    // The program is built once, at the first effect.
    if (!m_shader.id()
        && !m_shader.build(
            "Screen effects", shaders::screen_effect_vert, shaders::screen_effect_frag, error
        )) {
        log::error("Screen effects disabled: {}", error);
        m_failed = true;
        return false;
    }
    if (m_vao == 0) {
        glGenVertexArrays(1, &m_vao);
    }
    // The frame's copy follows the target's size.
    if (m_frame == 0 || m_frame_width != width || m_frame_height != height) {
        if (m_frame == 0) {
            glGenTextures(1, &m_frame);
        }
        glBindTexture(GL_TEXTURE_2D, m_frame);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
        m_frame_width = width;
        m_frame_height = height;
    }
    int w = width, h = height;
    for (Level& level : m_levels) {
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
        // Each size is made again only when the target's changes.
        if (level.width() == w && level.height() == h) {
            continue;
        }
        for (FrameBuffer& buffer : level.buffers) {
            if (!buffer.create_colour(w, h, error)) {
                log::error("Screen effects disabled: {}", error);
                m_failed = true;
                return false;
            }
        }
        level.current = 0;
    }
    return true;
}

void ScreenEffects::pass(int mode, unsigned source, int width, int height, Level& into) {
    FrameBuffer& out = into.buffers[into.current ^ 1];
    glBindFramebuffer(GL_FRAMEBUFFER, out.id());
    glViewport(0, 0, out.width(), out.height());
    // Both samplers belong to one program; an unused sampler must not retain the next blur output.
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, source);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    glUniform1i(m_shader.uniform("mode"), mode);
    glUniform2f(
        m_shader.uniform("texel"),
        1.0f / static_cast<float>(std::max(width, 1)),
        1.0f / static_cast<float>(std::max(height, 1))
    );
    const RasterTexture read{source};
    const RenderTarget target{out.id(), out.width(), out.height()};
    raster_pass("screen effect", target, {{}, {&read, 1}, ColourAttachments::ColourOverwrite}, [] {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    });
    into.current ^= 1;
}

void ScreenEffects::blur(Level& level, int times) {
    for (int i = 0; i < times; ++i) {
        pass(kBlur, level.texture(), level.width(), level.height(), level);
    }
}

void ScreenEffects::resample(const Level& from, Level& into) {
    pass(kResample, from.texture(), from.width(), from.height(), into);
}

void ScreenEffects::put_back(int mode, const RenderTarget& target) {
    const Level& half = m_levels[0];
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, half.texture());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_frame);
    glUniform1i(m_shader.uniform("mode"), mode);
    glUniform1f(m_shader.uniform("strength"), static_cast<float>(m_effect.strength) / kUnit);
    const RasterTexture reads[]{{m_frame}, {half.texture()}};
    raster_pass("screen effect", target, {{}, reads, ColourAttachments::Colour}, [] {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    });
}

void ScreenEffects::copy_frame(const RenderTarget& target, unsigned readable) {
    // World renderers may leave another unit active; the post pass owns only units zero and one.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_frame);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readable);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, target.width, target.height);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void ScreenEffects::apply(
    const ScreenEffect& effect, const RenderTarget& target, unsigned readable
) {
    // Nothing to put back, or a program that would not build, draws nothing.
    if (m_failed || effect.kind == ScreenEffectKind::None || target.width <= 0
        || target.height <= 0) {
        return;
    }

    // Resource creation must not overwrite a texture unit owned by the preceding world renderer.
    glActiveTexture(GL_TEXTURE0);

    // prepare() logs creation failures and latches them until release().
    if (!prepare(target.width, target.height)) {
        return;
    }

    const stats::Scope timing("Screen effect", stats::kGpu);
    m_effect = effect;
    // The target as drawn so far: every pass reads this copy, and the last writes the target.
    copy_frame(target, readable);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // The 2D renderer's samplers would override the copies' own linear, clamped reads.
    glBindSampler(0, 0);
    glBindSampler(1, 0);
    glBindVertexArray(m_vao);
    m_shader.use();
    glUniform1i(m_shader.uniform("tex_T0"), 0);
    glUniform1i(m_shader.uniform("tex_T1"), 1);
    const std::array<float, 4> tint = channels(effect.tint);
    glUniform4f(m_shader.uniform("tint"), tint[0], tint[1], tint[2], tint[3]);
    glUniform1f(m_shader.uniform("threshold"), static_cast<float>(effect.threshold) / kUnit);

    auto& [half, quarter, eighth] = m_levels;
    // The glow: its pixels shrunk and blurred down to an eighth, made large again and added.
    if (effect.kind == ScreenEffectKind::Glow) {
        pass(kCopy, m_frame, target.width, target.height, half);
        blur(half, effect.blur);
        resample(half, quarter);
        blur(quarter, kQuarterBlurs + (effect.blur != 0 ? 1 : 0));
        resample(quarter, eighth);
        blur(eighth, kEighthBlurs);
        resample(eighth, quarter);
        resample(quarter, half);
        blur(half, kHalfBlursAfter);
        put_back(kAdd, target);
    } else {
        // The second effect: the whole frame at half size, blurred, mixed back.
        pass(kResample, m_frame, target.width, target.height, half);
        blur(half, kSoftenBlurs + effect.blur);
        put_back(kMix, target);
    }
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

}  // namespace openrac::renderer
