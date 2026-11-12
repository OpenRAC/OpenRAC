// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "common/stats.h"
#include "enhanced/renderer/water_internal.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/gl.h"
#include "renderer/world/gl_world.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif

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
                    {cell[0] > .5f ? 1 - cell[2] : 0.f,
                     cell[1],
                     depth,
                     std::abs(cell[0]) > .5f ? std::abs(cell[0]) - 512.f : 0.f};
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
    // Ocean variants share one texture between the FFT row and the coastal coefficients.
    if (!m_ocean_pixels.empty()) {
        const int side = int(m_ocean_grid[3]);
        glGenTextures(1, &m_ocean_texture);
        glBindTexture(GL_TEXTURE_2D, m_ocean_texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            wgl::GL_RGBA32F,
            side,
            side + 1,
            0,
            GL_RGBA,
            GL_FLOAT,
            m_ocean_pixels.data()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (m_ocean_spectra[0]) {
        if (!m_ocean_fft
                 .build_compute("ocean inverse FFT", lighting::shaders::ocean_fft_comp, error)) {
            return false;
        }
        glGenBuffers(int(m_ocean_fft_buffers.size()), m_ocean_fft_buffers.data());
        std::vector<Texel> seed(kOceanCells);
        for (std::size_t i = 0; i < m_ocean_spectra.size(); ++i) {
            const auto& spectrum = *m_ocean_spectra[i];
            for (int j = 0; j < kOceanCells; ++j) {
                seed[j] =
                    {float(spectrum.seed[j].real()),
                     float(spectrum.seed[j].imag()),
                     float(spectrum.frequency[j]),
                     0};
            }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ocean_fft_buffers[i]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER,
                GLsizeiptr(seed.size() * sizeof(Texel)),
                seed.data(),
                GL_STATIC_DRAW
            );
        }
        // Scratch holds both transform stages. Each output is one RGBA grid.
        for (std::size_t i = 2; i < m_ocean_fft_buffers.size(); ++i) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ocean_fft_buffers[i]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER,
                GLsizeiptr(kOceanCells * sizeof(Texel) * (i == 2 ? 2 : 1)),
                nullptr,
                GL_DYNAMIC_DRAW
            );
        }
        glGenTextures(int(m_ocean_detail_textures.size()), m_ocean_detail_textures.data());
        for (const unsigned texture : m_ocean_detail_textures) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                wgl::GL_RGBA32F,
                kOceanSide,
                kOceanSide,
                0,
                GL_RGBA,
                GL_FLOAT,
                nullptr
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    contact_source = this;
    if (m_ocean_spectra[0]) {
        begin_ocean_material(m_ocean_material);
    }
    ocean_ready.store(bool(m_ocean_spectra[0]));
    water_ready.store(!water_disabled());
    return true;
}

void Water::update_ocean() {
#ifdef OPENRAC_VULKAN
    if (!m_ocean_detail_textures[0] || m_ocean_fft_time == m_time) {
        return;
    }
    m_ocean_fft.use();
    glUniform1f(m_ocean_fft.uniform("ocean_clock"), float(m_time));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ocean_fft_buffers[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_ocean_fft_buffers[3]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_ocean_fft_buffers[4]);
    for (std::size_t i = 0; i < m_ocean_spectra.size(); ++i) {
        // Include the texture uploads that make this cascade available to shading.
        const stats::Scope timing(i == 0 ? "Ocean FFT swell" : "Ocean FFT detail", stats::kGpu);
        const auto& spectrum = *m_ocean_spectra[i];
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ocean_fft_buffers[i]);
        glUniform1i(m_ocean_fft.uniform("ocean_choppy"), i == 0 ? 1 : 0);
        glUniform2f(
            m_ocean_fft.uniform("ocean_scale"),
            float(spectrum.gain),
            float(kOceanSide / (2 * spectrum.tile))
        );
        for (int pass = 0; pass < 3; ++pass) {
            glUniform1i(m_ocean_fft.uniform("ocean_pass"), pass);
            glDispatchCompute(kOceanSide, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        if (!vkgl::copy_buffer_to_texture(
                m_ocean_fft_buffers[3], m_ocean_detail_textures[i], kOceanSide, kOceanSide
            )) {
            return;
        }
        if (i == 0
            && !vkgl::copy_buffer_to_texture(
                m_ocean_fft_buffers[4], m_ocean_detail_textures[2], kOceanSide, kOceanSide
            )) {
            return;
        }
    }
    m_ocean_fft_time = m_time;
#endif
}

// Release all GPU-owned water resources, including the transmission snapshot.
void Water::release() {
    if (contact_source == this) {
        contact_source = nullptr;
    }
    water_ready.store(false);
    ocean_ready.store(false);
    m_background.release();
    m_background_textures = {};
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
    if (m_ocean_texture) {
        glDeleteTextures(1, &m_ocean_texture);
        m_ocean_texture = 0;
    }
    if (m_ocean_detail_textures[0]) {
        glDeleteTextures(int(m_ocean_detail_textures.size()), m_ocean_detail_textures.data());
        m_ocean_detail_textures = {};
    }
    m_ocean_fft.release();
    if (m_ocean_fft_buffers[0]) {
        glDeleteBuffers(int(m_ocean_fft_buffers.size()), m_ocean_fft_buffers.data());
        m_ocean_fft_buffers = {};
    }
    m_ocean_fft_time = -1;
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
    m_impulse_frame = std::numeric_limits<std::uint64_t>::max();
    m_reset_epoch = 0;
    m_hero = {};
    m_hands = {};
    m_time = 0;
    m_exit_time = -1;
    m_sim_weight = 1;
    m_enabled = m_bound = false;
    m_lighting_pending = false;
}
}  // namespace openrac::enhanced::water_detail
