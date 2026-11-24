// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/overlay_draw.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <string>
#include <vector>

#include "common/log.h"
#include "renderer/frame/colour_transfer.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer/ui/overlay_images.h"

namespace openrac::renderer::ui {

using namespace openrac::gl;

namespace {

// The picture's positions to device coordinates, y down; colours as they come, times the texel of
// the run's picture (a plain white one for rectangles and text).
constexpr const char* kVertex = R"glsl(#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 colour;
out vec2 v_uv;
out vec4 v_colour;
void main() {
    v_uv = uv;
    v_colour = colour;
    gl_Position = vec4(position.x / 256.0 - 1.0, 1.0 - position.y / 224.0, 0.0, 1.0);
}
)glsl";

constexpr const char* kFragment = R"glsl(#version 410 core
in vec2 v_uv;
in vec4 v_colour;
uniform sampler2D tex_T0;
out vec4 out_colour;
void main() {
    out_colour = v_colour * texture(tex_T0, v_uv);
}
)glsl";

constexpr std::array<std::uint8_t, 4> kWhite{255, 255, 255, 255};

// How many levels a full chain of mipmaps of a w x h picture has.
unsigned mip_levels(int width, int height) {
    const auto side = static_cast<unsigned>(std::max(width, height));
    return static_cast<unsigned>(std::bit_width(side));
}

// Makes `name` the picture: smooth (linear, mipmapped) or by the nearest texel; clamped at its
// edges.
void upload(unsigned name, int width, int height, std::span<const std::uint8_t> rgba, bool smooth) {
    upload_rgba8(name, width, height, rgba);
    glBindTexture(GL_TEXTURE_2D, name);
    if (smooth) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        static_cast<GLint>(smooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST)
    );
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(smooth ? GL_LINEAR : GL_NEAREST)
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glBindTexture(GL_TEXTURE_2D, 0);
}

// What the blending does for `blend`, which the list has already checked is one it has. In each the
// frame's alpha is kept (the alpha factors are 0 and 1 whichever equation: dst * 1 +- src * 0).
// Subtract is the game's own "destination minus source * alpha" (the GS pass of
// renderer/world/gs_pass.cpp): GL's reverse subtract is dst * dfactor - src * sfactor, here
// dst - src * alpha, and a colour target holds nothing below 0.
void set_blend(OverlayBlend blend) {
    switch (blend) {
        case OverlayBlend::Alpha:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
            break;
        case OverlayBlend::Add:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
            break;
        case OverlayBlend::Subtract:
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
            break;
    }
}

}  // namespace

bool OverlayDraw::ready() {
    if (m_failed) {
        return false;
    }
    if (m_vao == 0) {
        std::string error;
        if (!m_shader.build("mod overlay", kVertex, kFragment, error)) {
            log::warn("what mods draw over the game cannot be drawn: {}", error);
            m_failed = true;
            return false;
        }
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_buffer);
        glGenTextures(1, &m_white);
        upload(m_white, 1, 1, kWhite, false);
    }
    return true;
}

unsigned OverlayDraw::texture(int image, unsigned& mips) {
    mips = 1;
    if (image <= 0) {
        return m_white;
    }
    if (const auto it = m_textures.find(image); it != m_textures.end()) {
        mips = it->second.mips;
        return it->second.name;
    }
    const auto picture = overlay_images::find(image);
    if (picture == nullptr) {
        return 0;
    }
    Texture t;
    glGenTextures(1, &t.name);
    upload(t.name, picture->width, picture->height, picture->rgba, picture->smooth);
    t.mips = picture->smooth ? mip_levels(picture->width, picture->height) : 1;
    m_textures[image] = t;
    mips = t.mips;
    return t.name;
}

void OverlayDraw::sweep() {
    const std::uint64_t changes = overlay_images::changes();
    if (changes == m_swept) {
        return;
    }
    m_swept = changes;
    std::erase_if(m_textures, [](const auto& item) {
        if (overlay_images::find(item.first) != nullptr) {
            return false;
        }
        glDeleteTextures(1, &item.second.name);
        return true;
    });
}

void OverlayDraw::draw(
    unsigned framebuffer, int width, int height, std::span<const OverlayVertex> vertices
) {
    const OverlayBatch all{0, static_cast<std::uint32_t>(vertices.size()), OverlayBlend::Alpha, 0};
    draw(framebuffer, width, height, vertices, {&all, 1});
}

void OverlayDraw::draw(
    unsigned framebuffer,
    int width,
    int height,
    std::span<const OverlayVertex> vertices,
    std::span<const OverlayBatch> batches
) {
    if (vertices.empty() || width <= 0 || height <= 0 || !ready()) {
        return;
    }
    sweep();
    // Every picture the runs sample, made before the pass (uploads belong outside it); a run whose
    // picture is gone is left out.
    std::vector<unsigned> run_textures(batches.size(), 0);
    std::vector<RasterTexture> sampled;
    for (std::size_t i = 0; i < batches.size(); ++i) {
        unsigned mips = 1;
        run_textures[i] = texture(batches[i].image, mips);
        if (run_textures[i] != 0
            && std::ranges::find(sampled, run_textures[i], &RasterTexture::name) == sampled.end()) {
            sampled.push_back({run_textures[i], mips});
        }
    }
    // All of the state the triangles depend on, whatever the frame left.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND);
    m_shader.use();
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size_bytes()),
        vertices.data(),
        GL_STREAM_DRAW
    );
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    const auto stride = static_cast<GLsizei>(sizeof(OverlayVertex));
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(OverlayVertex, u))
    );
    glVertexAttribPointer(
        2,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<const void*>(offsetof(OverlayVertex, rgba))
    );
    glUniform1i(m_shader.uniform("tex_T0"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindSampler(0, 0);
    const RasterBuffer geometry{m_buffer, vertices.size_bytes()};
    // Upload and vertex layout are complete before the pass borrows the buffer.
    raster_pass("mod overlay", {framebuffer, width, height}, {{&geometry, 1}, sampled}, [&] {
        bool blend_set = false;
        OverlayBlend blend = OverlayBlend::Alpha;
        unsigned bound = 0;
        for (std::size_t i = 0; i < batches.size(); ++i) {
            const OverlayBatch& batch = batches[i];
            if (batch.count == 0 || run_textures[i] == 0
                || static_cast<std::size_t>(batch.first) + batch.count > vertices.size()) {
                continue;
            }
            if (run_textures[i] != bound) {
                glBindTexture(GL_TEXTURE_2D, run_textures[i]);
                bound = run_textures[i];
            }
            if (!blend_set || batch.blend != blend) {
                set_blend(batch.blend);
                blend = batch.blend;
                blend_set = true;
            }
            glDrawArrays(
                GL_TRIANGLES, static_cast<GLint>(batch.first), static_cast<GLsizei>(batch.count)
            );
        }
    });
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Whoever draws next finds the default equation, as it did before there was a subtract.
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void OverlayDraw::release() {
    for (const auto& [image, t] : m_textures) {
        glDeleteTextures(1, &t.name);
    }
    m_textures.clear();
    m_swept = 0;
    if (m_white != 0) {
        glDeleteTextures(1, &m_white);
        m_white = 0;
    }
    if (m_buffer != 0) {
        glDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_shader.release();
    m_failed = false;
}

}  // namespace openrac::renderer::ui
