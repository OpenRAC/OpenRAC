// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// An OverlayList's triangles blended over a framebuffer, through the renderer's GL: on OpenGL and
// on the Vulkan renderer's alike. It keeps its program and buffer from the first draw on, and a
// texture for each mod's picture (overlay_images.h) it has drawn, made when first drawn and let go
// once the picture is; release() frees them while the context is current. Draws into one context
// can share one OverlayTextures, so a picture is uploaded once.

#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>

#include "renderer/gl/shader.h"
#include "renderer/ui/overlay_list.h"

namespace openrac::renderer::ui {

// The textures of the mods' pictures in one GL context, made when first drawn and let go once the
// picture is gone. Used from the thread that holds the graphics only.
class OverlayTextures {
public:
    OverlayTextures() = default;
    OverlayTextures(const OverlayTextures&) = delete;
    OverlayTextures& operator=(const OverlayTextures&) = delete;

    // The texture of picture `image`, made the first time; 0 when there is no such picture.
    // `mips`: its levels.
    unsigned texture(int image, unsigned& mips);

    // Lets go of the textures of pictures that are gone.
    void sweep();

    // Frees every texture. The context is current.
    void release();

private:
    struct Texture {
        unsigned name = 0;
        unsigned mips = 1;
    };

    std::unordered_map<int, Texture> m_textures;
    std::uint64_t m_swept = 0;
};

class OverlayDraw {
public:
    // `shared`: the pictures' textures to use, kept by the caller (it releases them); null keeps
    // its own.
    explicit OverlayDraw(OverlayTextures* shared = nullptr) : m_shared(shared) {}
    OverlayDraw(const OverlayDraw&) = delete;
    OverlayDraw& operator=(const OverlayDraw&) = delete;

    // Blends `vertices` over `framebuffer`, `width` x `height` pixels, the picture stretched over
    // it, by alpha and keeping the frame's own alpha. Nothing for no vertices. The context is
    // current; leaves blending off and the draw framebuffer at 0.
    void draw(unsigned framebuffer, int width, int height, std::span<const OverlayVertex> vertices);

    // The same for a frame of rectangles and text with per-batch blend modes: each of `batches`
    // (an OverlayList's, over its `vertices`) in turn, so what was asked first is under what was
    // asked after.
    void draw(
        unsigned framebuffer,
        int width,
        int height,
        std::span<const OverlayVertex> vertices,
        std::span<const OverlayBatch> batches
    );

    // Frees the GL objects if a draw made them (not a shared OverlayTextures).
    void release();

private:
    bool ready();

    // The texture of picture `image` (0: the plain white one rectangles and text sample); 0 when
    // there is no such picture. `mips`: its levels.
    unsigned texture(int image, unsigned& mips);

    OverlayTextures& textures() { return m_shared != nullptr ? *m_shared : m_own; }

    Shader m_shader;
    unsigned m_vao = 0;
    unsigned m_buffer = 0;
    unsigned m_white = 0;
    OverlayTextures* m_shared = nullptr;
    OverlayTextures m_own;
    bool m_failed = false;
};

}  // namespace openrac::renderer::ui
