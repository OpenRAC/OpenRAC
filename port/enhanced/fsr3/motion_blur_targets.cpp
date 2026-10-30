// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Motion blur shader preparation and render-target ownership.
#include "common/stats.h"
#include "enhanced/fsr3/motion_blur.h"
#include "fsr3_shaders.h"
#include "renderer/gl.h"
#include "renderer/vulkan/native_compute.h"

namespace openrac::enhanced::fsr3_detail {
using namespace openrac::gl;

bool MotionBlur::prepare(int width, int height, int rw, int rh, std::string& error) {
    if (!prepare_colour(width, height, error)) {
        return false;
    }
    if (!m_extract.id()) {
        std::string copy_resolve(shaders::blur_resolve_frag);
        copy_resolve.insert(copy_resolve.find('\n') + 1, "#define COPY_UNCHANGED 1\n");
        std::string fast_jump(shaders::blur_jump_frag);
        fast_jump.insert(fast_jump.find('\n') + 1, "#define FAST_FLOOD 1\n");
        std::string fast_first(fast_jump);
        fast_first.insert(fast_first.find('\n') + 1, "#define INITIALIZE_SEEDS 1\n");
        std::string first_jump(shaders::blur_jump_frag);
        first_jump.insert(first_jump.find('\n') + 1, "#define INITIALIZE_SEEDS 1\n");
        if (!m_fast_first_jump
                 .build("blur fast initial flood", shaders::fullscreen_vert, fast_first, error)
            || !m_fast_jump.build("blur fast flood", shaders::fullscreen_vert, fast_jump, error)
            || !m_extract.build(
                "relative blur velocity",
                shaders::fullscreen_vert,
                shaders::blur_velocity_frag,
                error
            )
            || !m_first_jump
                    .build("blur initial jump flood", shaders::fullscreen_vert, first_jump, error)
            || !m_jump.build(
                "blur jump flood", shaders::fullscreen_vert, shaders::blur_jump_frag, error
            )
            || !m_resolve.build(
                "relative blur resolve", shaders::fullscreen_vert, shaders::blur_resolve_frag, error
            )
            || !m_copy_resolve.build(
                "blur and copy resolve", shaders::fullscreen_vert, copy_resolve, error
            )) {
            return false;
        }
    }
    if (m_size != std::array{width, height, rw, rh}) {
        m_size = {};
        if (!m_velocity.create(rw, rh, 0x881A, 0, error)
            || !m_seeds[0].create(rw, rh, 0x8236, 0, error)
            || !m_seeds[1].create(rw, rh, 0x8236, 0, error)) {
            return false;
        }
        m_size = {width, height, rw, rh};
    }
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
    }
    return true;
}

bool MotionBlur::prepare_colour(int width, int height, std::string& error) {
    if (m_colour_size == std::array{width, height}) {
        return true;
    }
    m_colour_size = {};
    if (!m_colour.create(width, height, GL_RGBA8, 0, error)) {
        return false;
    }
    m_colour_size = {width, height};
    return true;
}

unsigned MotionBlur::reconstruction_output(int width, int height) {
    if (m_failed) {
        return 0;
    }
    std::string error;
    if (!prepare_colour(width, height, error)) {
        return 0;
    }
    if (!vkgl::storage_texture(m_colour.texture())
        && !vkgl::allocate_storage_texture(m_colour.texture(), GL_RGBA8, width, height)) {
        return 0;
    }
    return m_colour.texture();
}

void MotionBlur::present_reconstruction(const renderer::RenderTarget& display) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_colour.framebuffer());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, display.framebuffer);
    glBlitFramebuffer(
        0,
        0,
        display.width,
        display.height,
        0,
        0,
        display.width,
        display.height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, display.framebuffer);
    glViewport(0, 0, display.width, display.height);
}

void MotionBlur::copy_colour(const renderer::RenderTarget& display) {
    const stats::Scope copy("Motion blur colour copy", stats::kGpu);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, display.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_colour.framebuffer());
    glBlitFramebuffer(
        0,
        0,
        display.width,
        display.height,
        0,
        0,
        display.width,
        display.height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
}

void MotionBlur::release() {
    m_velocity.release();
    for (auto& seeds : m_seeds) {
        seeds.release();
    }
    m_colour.release();
    m_extract.release();
    m_fast_first_jump.release();
    m_fast_jump.release();
    m_first_jump.release();
    m_jump.release();
    m_resolve.release();
    m_copy_resolve.release();
    m_colour_size = {};
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
    m_size = {};
    m_failed = false;
}
}  // namespace openrac::enhanced::fsr3_detail
