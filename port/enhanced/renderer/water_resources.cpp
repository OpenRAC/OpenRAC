// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "enhanced/renderer/water_internal.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"

namespace openrac::enhanced::water_detail {
using namespace gl;
namespace wgl = renderer::world::gl;

bool Water::init(std::string& error) {
    if (!m_window_prepare
             .build_compute("water window metadata", lighting::shaders::water_window_comp, error)) {
        return false;
    }
    if (!m_step.build_compute("water surface waves", lighting::shaders::water_comp, error)) {
        return false;
    }
    const std::string light_source = "#version 430 core\n#define OPENRAC_PROBE_PIXEL ivec2(-1)\n"
                                     + std::string(lighting::shaders::probes_glsl)
                                     + std::string(lighting::shaders::water_lighting_comp);
    if (!m_light.build_compute("water lighting cache", light_source, error)) {
        return false;
    }
    for (std::size_t b = 0; b < m_bodies.size(); b += 5) {
        m_bodies[b + 4][3] = float(m_light_count);
        const unsigned width = unsigned(m_bodies[b + 1][3]), height = unsigned(m_bodies[b + 2][3]);
        m_light_count += static_cast<std::size_t>((width + 3) / 4 + 1) * ((height + 3) / 4 + 1);
    }
    for (std::size_t b = 0; b < m_bodies.size(); b += 5) {
        m_atlas_width = std::max(m_atlas_width, unsigned(m_bodies[b + 1][3]));
        m_atlas[b / 5] = {0, float(m_atlas_height), 0, 0};
        m_atlas_height += unsigned(m_bodies[b + 2][3]) + 2;
    }
    if (m_atlas_height > 16384
        || std::size_t(m_atlas_width) * m_atlas_height > std::size_t{4} * 1024 * 1024) {
        error = "water image atlas exceeds texture size";
        return false;
    }
    glGenBuffers(kBufferCount, m_buffers.data());
    if (!m_mesh->init(error)) {
        return false;
    }
    for (unsigned i : {unsigned(kWindowCoast), unsigned(kWindowHydraulics)}) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[i]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            GLsizeiptr(kWindowCells * sizeof(Texel)),
            nullptr,
            GL_DYNAMIC_DRAW
        );
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[kHydraulics]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(m_hydraulics.size() * sizeof(Texel)),
        m_hydraulics.data(),
        GL_STATIC_DRAW
    );
    glGenTextures(kTextureCount, m_textures.data());
    const std::vector<Texel> zeros(kWindowCells);
    const std::vector<Texel> empty_light(m_light_count, Texel{0, 0, 0, -1});
    const std::array<std::span<const Texel>, 5> data{zeros, zeros, m_coast, m_bodies, empty_light};
    for (unsigned i = 0; i < data.size(); ++i) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[i]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            GLsizeiptr(data[i].size_bytes()),
            data[i].data(),
            i < 2 || i == 4 ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW
        );
        if (i >= kCoast) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[i - kCoast]);
            glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32F, m_buffers[i]);
        }
    }
    std::vector<Texel> image_data(std::size_t(m_atlas_width) * m_atlas_height);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[kRenderPixels]);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(zeros.size() * sizeof(Texel)),
        zeros.data(),
        GL_DYNAMIC_DRAW
    );
    glGenTextures(2, m_maps.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_maps[0]);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        wgl::GL_RGBA32F,
        int(kWindowSize),
        int(kWindowSize),
        0,
        GL_RGBA,
        GL_FLOAT,
        zeros.data()
    );
    for (std::size_t b = 0; b < m_bodies.size(); b += 5) {
        const unsigned width = unsigned(m_bodies[b + 1][3]), height = unsigned(m_bodies[b + 2][3]),
                       begin = unsigned(m_bodies[b + 3][3]);
        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                const auto& cell = m_coast[begin + y * width + x];
                // The fourth hydraulic component stores optical depth, independent of
                // the solver's CFL-limited effective depth. Older v3 bakes use zero.
                const auto& hydraulic = m_hydraulics[begin + y * width + x];
                const float depth = hydraulic[3] > 0.f ? hydraulic[3] : hydraulic[0];
                image_data[(std::size_t(m_atlas[b / 5][1]) + y) * m_atlas_width + x] =
                    {cell[0] > .5f ? 1 - cell[2] : 0.f, cell[1], depth, 0.f};
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, m_maps[1]);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        wgl::GL_RGBA32F,
        int(m_atlas_width),
        int(m_atlas_height),
        0,
        GL_RGBA,
        GL_FLOAT,
        image_data.data()
    );
    for (unsigned map : m_maps) {
        glBindTexture(GL_TEXTURE_2D, map);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, map == m_maps[0] ? GL_REPEAT : GL_CLAMP_TO_EDGE
        );
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, map == m_maps[0] ? GL_REPEAT : GL_CLAMP_TO_EDGE
        );
    }
    glActiveTexture(GL_TEXTURE0);
    contact_source = this;
    water_ready.store(!water_disabled());
    return true;
}

void Water::release() {
    if (contact_source == this) {
        contact_source = nullptr;
    }
    water_ready.store(false);
    m_step.release();
    m_window_prepare.release();
    m_light.release();
    if (m_buffers[kStateA]) {
        glDeleteBuffers(kBufferCount, m_buffers.data());
    }
    if (m_textures[kCoastTexture]) {
        glDeleteTextures(kTextureCount, m_textures.data());
    }
    if (m_maps[0]) {
        glDeleteTextures(2, m_maps.data());
    }
    m_mesh->release();
    m_maps = {};
    m_buffers = {};
    m_textures = {};
    m_atlas = {};
    m_atlas_width = m_atlas_height = m_light_count = m_current = 0;
    m_sim_body = m_cached_body = -1;
    m_sim_height = m_cached_height = 0;
    m_cached_origin = m_window_origin = {};
    m_prepared_frame = m_clock_frame = m_last_frame = std::numeric_limits<std::uint64_t>::max();
    m_reset_epoch = 0;
    m_hero = {};
    m_hands = {};
    m_time = 0;
    m_exit_time = -1;
    m_sim_weight = 1;
    m_enabled = m_bound = false;
}
}  // namespace openrac::enhanced::water_detail
