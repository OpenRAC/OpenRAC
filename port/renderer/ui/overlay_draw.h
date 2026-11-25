// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// An OverlayList's triangles blended over a framebuffer, through the renderer's GL: on OpenGL and
// on the Vulkan renderer's alike. It keeps its program and buffer from the first draw on; the
// textures it samples are an OverlayTextures', which several draws can share. release() frees
// each while the context is current.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "renderer/gl/shader.h"
#include "renderer/ui/overlay_list.h"

namespace openrac::renderer::ui {

// A texture for each mod's picture (overlay_images.h) drawn, made when first drawn and let go once
// the picture is, and the plain white one rectangles and text sample. One for all the overlay's
// draws, so a picture drawn in two of them is uploaded once: the draws that share it use the same
// GL objects (one context, or the Vulkan renderer's one set of them) and run in turn, on the thread
// that holds the graphics, so it has no lock.
class OverlayTextures {
public:
    OverlayTextures() = default;
    OverlayTextures(const OverlayTextures&) = delete;
    OverlayTextures& operator=(const OverlayTextures&) = delete;

    // The texture of picture `image` (0: the plain white one), made the first time; 0 when there
    // is no such picture. `mips`: its levels.
    unsigned texture(int image, unsigned& mips);

    // Lets go of the textures of pictures that are gone, once for each change of the pictures.
    void sweep();

    // How many pictures have a texture.
    std::size_t size() const { return m_textures.size(); }

    // Frees the textures; the context is current.
    void release();

private:
    struct Texture {
        unsigned name = 0;
        unsigned mips = 1;
    };

    unsigned m_white = 0;
    std::unordered_map<int, Texture> m_textures;
    std::uint64_t m_swept = 0;
};

class OverlayDraw {
public:
    // Samples `textures`' textures, which outlives it.
    explicit OverlayDraw(OverlayTextures& textures) : m_textures(textures) {}

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

    // Frees its program and buffer if a draw made them; not the textures (OverlayTextures').
    void release();

private:
    bool ready();

    OverlayTextures& m_textures;
    Shader m_shader;
    unsigned m_vao = 0;
    unsigned m_buffer = 0;
    bool m_failed = false;
};

}  // namespace openrac::renderer::ui
