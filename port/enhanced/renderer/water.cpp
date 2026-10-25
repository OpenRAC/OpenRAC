// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/renderer/water.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/renderer/water_mesh.h"
#include "enhanced/renderer/water_window.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif
#include "scene/level_renderer.h"

namespace openrac::enhanced {
namespace {
using namespace gl;
namespace wgl = renderer::world::gl;
using Texel = std::array<float, 4>;
constexpr unsigned kWindowSize = 128, kWindowCells = kWindowSize * kWindowSize;
constexpr float kWindowSpacing = .25f;

enum BufferIndex : unsigned {
    kStateA,
    kStateB,
    kCoast,
    kBodies,
    kLighting,
    kRenderPixels,
    kHydraulics,
    kWindowCoast,
    kWindowHydraulics,
    kBufferCount
};

enum TextureIndex : unsigned {
    kCoastTexture,
    kBodyTexture,
    kLightTexture,
    kTextureCount
};

std::atomic<bool> water_ready{false};
std::mutex water_debug_mutex;
WaterDebugSettings water_debug;
std::atomic<std::uint64_t> water_reset_epoch{0};

bool water_disabled() {
    static const bool& off = stats::probe("OPENRAC_WATER_OFF", "Original Novalis water reference");
    return off;
}

class Water final : public renderer::EffectMaterial {
public:
    Water(scene::LevelScene& scene, const std::filesystem::path& path) : m_scene(scene) {
        std::ifstream file(path, std::ios::binary);
        std::array<char, 8> magic{};
        unsigned version = 0, bodies = 0, cells = 0;
        std::array<char, 32> fingerprint{};
        file.read(magic.data(), 8);
        file.read(reinterpret_cast<char*>(&version), 4);
        file.read(reinterpret_cast<char*>(&bodies), 4);
        file.read(reinterpret_cast<char*>(&cells), 4);
        file.read(fingerprint.data(), 32);
        if (!file || std::string_view(magic.data(), 8) != "ORWATER1" || version != 3 || bodies == 0
            || bodies > 64 || cells == 0 || cells > 4 * 1024 * 1024) {
            throw std::runtime_error("invalid water bake header");
        }
        m_bodies.resize(bodies * 5);
        m_coast.resize(cells);
        m_hydraulics.resize(cells);
        file.read(
            reinterpret_cast<char*>(m_bodies.data()),
            std::streamsize(m_bodies.size() * sizeof(Texel))
        );
        file.read(
            reinterpret_cast<char*>(m_coast.data()), std::streamsize(m_coast.size() * sizeof(Texel))
        );
        file.read(
            reinterpret_cast<char*>(m_hydraulics.data()), std::streamsize(cells * sizeof(Texel))
        );
        for (const auto& h : m_hydraulics) {
            if (!std::isfinite(h[0]) || h[0] < .25f || h[0] > 1.5f || !std::isfinite(h[1])
                || !std::isfinite(h[2]) || !std::isfinite(h[3]) || h[3] < 0.f
                || std::abs(h[1]) > 1.f || std::abs(h[2]) > 1.f) {
                throw std::runtime_error("invalid water hydraulics");
            }
        }
        if (!file || file.peek() != std::char_traits<char>::eof()) {
            throw std::runtime_error("invalid water bake size");
        }
        for (const auto& values :
             {std::span<const Texel>(m_bodies), std::span<const Texel>(m_coast)}) {
            for (const auto& texel : values) {
                for (float x : texel) {
                    if (!std::isfinite(x)) {
                        throw std::runtime_error("nonfinite water bake");
                    }
                }
            }
        }
        std::size_t expected = 0;
        for (unsigned i = 0; i < bodies; ++i) {
            const auto* b = m_bodies.data() + i * 5;
            const float width = b[1][3], height = b[2][3];
            if (width < 1 || height < 1 || width > 4096 || height > 4096
                || std::floor(width) != width || std::floor(height) != height
                || b[3][3] != float(expected) || b[4][2] != .5f || b[0][3] < 1 || b[0][3] > 7
                || std::floor(b[0][3]) != b[0][3] || std::abs(b[0][0]) > 100000
                || std::abs(b[0][1]) > 100000 || b[0][2] != 0 || b[1][0] != 1 || b[1][1] != 0
                || b[1][2] != 0 || b[2][0] != 0 || b[2][1] != 1 || b[2][2] != 0 || b[3][0] != 0
                || b[3][1] != 0 || b[3][2] != 1) {
                throw std::runtime_error("invalid water grid");
            }
            const auto first = expected;
            expected += std::size_t(width) * std::size_t(height);
            if (expected > cells) {
                throw std::runtime_error("water grid outside atlas");
            }
            for (auto cell = first; cell < expected; ++cell) {
                if (m_coast[cell][3] != float(i)) {
                    throw std::runtime_error("water boundary belongs to another grid");
                }
            }
        }
        if (expected != cells) {
            throw std::runtime_error("incomplete water atlas");
        }
        for (const auto& cell : m_coast) {
            if (cell[0] < -2048 || cell[0] > 2048 || cell[1] < 0 || cell[2] < 0 || cell[2] > 1
                || cell[3] < 0 || cell[3] >= float(bodies) || std::floor(cell[3]) != cell[3]) {
                throw std::runtime_error("invalid water boundary");
            }
        }
        m_mesh = std::make_unique<WaterMesh>(path, fingerprint);
        log::info("enhanced Novalis water: {} bodies, {} simulation cells", bodies, cells);
    }

    std::string fragment_source() const override {
        return std::string(lighting::shaders::water_window_glsl) + "\n"
               + std::string(lighting::shaders::water_swell_glsl) + "\n"
               + std::string(lighting::shaders::probes_glsl) + "\n"
               + std::string(lighting::shaders::water_glsl);
    }

    std::string vertex_source() const override {
        return std::string(lighting::shaders::water_window_glsl) + "\n"
               + std::string(lighting::shaders::water_swell_glsl) + "\n"
               + std::string(lighting::shaders::water_vertex_glsl);
    }

    bool mesh(
        const renderer::EffectStrip& strip,
        std::span<const renderer::EffectVertex> vertices,
        std::vector<renderer::EffectVertex>& output
    ) const override {
        return m_enabled && m_mesh->build(strip, vertices, output);
    }

    bool init(std::string& error) override {
        if (!m_window_prepare.build_compute(
                "water window metadata", lighting::shaders::water_window_comp, error
            )) {
            return false;
        }
        if (!m_step.build_compute("water surface waves", lighting::shaders::water_comp, error)) {
            return false;
        }
        const std::string light_source =
            "#version 430 core\n#define OPENRAC_PROBE_PIXEL ivec2(-1)\n"
            + std::string(lighting::shaders::probes_glsl)
            + std::string(lighting::shaders::water_lighting_comp);
        if (!m_light.build_compute("water lighting cache", light_source, error)) {
            return false;
        }
        for (std::size_t b = 0; b < m_bodies.size(); b += 5) {
            m_bodies[b + 4][3] = float(m_light_count);
            const unsigned width = unsigned(m_bodies[b + 1][3]),
                           height = unsigned(m_bodies[b + 2][3]);
            m_light_count += ((width + 3) / 4 + 1) * ((height + 3) / 4 + 1);
        }
        for (std::size_t b = 0; b < m_bodies.size(); b += 5) {
            m_atlas_width = std::max(m_atlas_width, unsigned(m_bodies[b + 1][3]));
            m_atlas[b / 5] = {0, float(m_atlas_height), 0, 0};
            m_atlas_height += unsigned(m_bodies[b + 2][3]) + 2;
        }
        if (m_atlas_height > 16384
            || std::size_t(m_atlas_width) * m_atlas_height > 4 * 1024 * 1024) {
            error = "water image atlas exceeds texture size";
            return false;
        }
        glGenBuffers(kBufferCount, m_buffers.data());
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
        const std::array<std::span<const Texel>, 5> data{
            zeros, zeros, m_coast, m_bodies, empty_light
        };
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
            const unsigned width = unsigned(m_bodies[b + 1][3]),
                           height = unsigned(m_bodies[b + 2][3]),
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
        water_ready.store(!water_disabled());
        return true;
    }

    void update_lighting(const renderer::FrameInput& input) {
        m_light.use();
        glUniform1i(m_light.uniform("probe_region_masks"), 15);
        m_scene.bind_surface_lighting(m_light, input.camera);
        glUniform1i(m_light.uniform("water_coast"), 12);
        glUniform1i(m_light.uniform("water_bodies"), 11);
        glUniform1i(m_light.uniform("water_body_count"), int(m_bodies.size() / 5));
        glUniform1i(m_light.uniform("water_light_count"), int(m_light_count));
        glActiveTexture(GL_TEXTURE0 + 12);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[kCoastTexture]);
        glActiveTexture(GL_TEXTURE0 + 11);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[kBodyTexture]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_buffers[kLighting]);
        glDispatchCompute((m_light_count + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        glActiveTexture(GL_TEXTURE0);
    }

    void prepare(const renderer::FrameInput& input) override {
        m_bound = false;
        const auto options = water_debug_settings();
        const auto epoch = water_reset_epoch.load();
        const bool debug_reset =
            epoch != m_reset_epoch || options.current_scale != m_debug.current_scale;
        m_debug = options;
        m_enabled = input.enhanced_water && !water_disabled();
        water_ready.store(m_enabled);
        if (!m_enabled) {
            m_last_frame = m_prepared_frame = m_clock_frame =
                std::numeric_limits<std::uint64_t>::max();
            m_sim_body = -1;
            m_exit_time = -1;
            m_sim_weight = 1;
            m_reset_epoch = epoch;
            return;
        }
        if (input.frame == m_prepared_frame && !debug_reset) {
            return;
        }
        m_prepared_frame = input.frame;
        if (m_clock_frame != input.frame) {
            if (!m_debug.paused && m_clock_frame != std::numeric_limits<std::uint64_t>::max()
                && input.frame > m_clock_frame) {
                m_time += double(std::min<std::uint64_t>(input.frame - m_clock_frame, 4)) / 60.;
            }
            m_clock_frame = input.frame;
        }
        auto hero = input.water_interactor.value_or(renderer::Vec3{0, 0, 100000});
        unsigned visible_surfaces = 0;
        const auto view_projection = renderer::multiply(input.camera.projection, input.camera.view);
        for (const auto& strip : input.strips) {
            if (strip.surface == 0 || strip.surface > 7 || strip.count < 3
                || std::size_t(strip.first) + strip.count > input.strip_vertices.size()) {
                continue;
            }
            renderer::Vec3 low{1e20f, 1e20f, 1e20f}, high{-1e20f, -1e20f, -1e20f};
            for (const auto& v : input.strip_vertices.subspan(strip.first, strip.count)) {
                const renderer::Vec3 p{v.x, v.y, v.z};
                for (unsigned axis = 0; axis < 3; ++axis) {
                    low[axis] = std::min(low[axis], p[axis]);
                    high[axis] = std::max(high[axis], p[axis]);
                }
            }
            if (water_window_visible(view_projection, low, high)) {
                visible_surfaces |= 1u << strip.surface;
            }
        }
        int selected = -1;
        bool contact_water = false;
        float selected_height = m_sim_height;
        float best = std::numeric_limits<float>::max();
        // Only static bake metadata is queried here. The dynamic state stays on the GPU.
        for (unsigned b = 0; b < m_bodies.size() / 5; ++b) {
            const auto* body = m_bodies.data() + b * 5;
            const int width = int(body[1][3]), height = int(body[2][3]);
            const int x = std::clamp(int(std::floor((hero[0] - body[0][0]) / .5f)), 0, width - 1);
            const int y = std::clamp(int(std::floor((hero[1] - body[0][1]) / .5f)), 0, height - 1);
            const bool visible = (visible_surfaces & (1u << unsigned(body[0][3]))) != 0;
            // Clipped bounds visit each edge cell once, retaining scan/tie order.
            for (int cy = std::max(y - 3, 0); cy <= std::min(y + 3, height - 1); ++cy) {
                for (int cx = std::max(x - 3, 0); cx <= std::min(x + 3, width - 1); ++cx) {
                    const auto& cell = m_coast[unsigned(body[3][3]) + cy * width + cx];
                    if (cell[0] < .5f) {
                        continue;
                    }
                    const float px = body[0][0] + (float(cx) + .5f) * .5f;
                    const float py = body[0][1] + (float(cy) + .5f) * .5f;
                    const float dz = hero[2] - (cell[0] - 512.f);
                    const float xy =
                        (hero[0] - px) * (hero[0] - px) + (hero[1] - py) * (hero[1] - py);
                    const bool contact = xy < 4.f && dz > -1.8f && dz < .5f;
                    if ((!visible && !contact)
                        || xy > (kWindowSize * kWindowSpacing * .46875f)
                                    * (kWindowSize * kWindowSpacing * .46875f)
                        || std::abs(dz) > 12.f) {
                        continue;
                    }
                    contact_water = contact_water || contact;
                    float score = xy + dz * dz * 4.f;
                    if (int(b) == m_sim_body) {
                        score *= .9f;
                    }
                    if (score < best) {
                        best = score;
                        selected = int(b);
                        selected_height = cell[0] - 512.f;
                    }
                }
            }
        }
        // Let the last wake settle for five seconds after contact/relevance ends.
        // Keep its window fixed so Ratchet walking away cannot drag or reset it.
        const bool retiring = m_sim_body >= 0 && !contact_water;
        if (contact_water) {
            m_exit_time = -1;
            m_sim_weight = 1;
        } else if (retiring) {
            if (m_exit_time < 0) {
                m_exit_time = m_time;
            }
            const float elapsed = float((m_time - m_exit_time) / 5.);
            const float t = std::clamp(elapsed, 0.f, 1.f);
            m_sim_weight = 1.f - t * t * (3.f - 2.f * t);
            selected = t < 1.f ? m_sim_body : -1;
            selected_height = m_sim_height;
            hero = m_hero;
        } else if (m_exit_time >= 0) {
            selected = -1;
        }
        const bool compatible = selected >= 0 && m_sim_body >= 0
                                && water_window_same_height(selected_height, m_sim_height);
        const bool changed = selected != m_sim_body && !compatible;
        m_sim_body = selected;
        m_sim_height = selected_height;
        stats::gauge("water simulation cells", selected >= 0 ? double(kWindowCells) : 0.);
        stats::gauge("water simulation body", double(selected));
        if (visible_surfaces != 0 || selected >= 0) {
            update_lighting(input);
        }
        if (selected < 0) {
            remember_tick(input, hero, epoch);
            m_last_frame = std::numeric_limits<std::uint64_t>::max();
            return;
        }
        const std::array<int, 2> window =
            retiring ? m_window_origin
                     : std::array<int, 2>{
                           int(std::floor(hero[0] / kWindowSpacing)) - int(kWindowSize / 2),
                           int(std::floor(hero[1] / kWindowSpacing)) - int(kWindowSize / 2)
                       };
        float distance = 0;
        for (unsigned a = 0; a < 3; ++a) {
            distance += (hero[a] - m_hero[a]) * (hero[a] - m_hero[a]);
        }
        const bool reset =
            changed || debug_reset || m_last_frame == std::numeric_limits<std::uint64_t>::max()
            || input.frame < m_last_frame || input.frame - m_last_frame > 8 || distance > 144;
        const unsigned steps =
            reset            ? 1
            : m_debug.paused ? 0
                             : unsigned(std::min<std::uint64_t>(input.frame - m_last_frame, 4));
        if (steps == 0) {
            remember_tick(input, hero, epoch);
            return;
        }
        const float impulse =
            input.water_interactor && !reset && !retiring
                ? std::min(1.0f, std::sqrt(distance) * 2.5f) * m_debug.interaction_strength
                : 0.f;
        std::array<float, 2> hand_impulses{};
        for (unsigned hand = 0; hand < hand_impulses.size(); ++hand) {
            if (reset || retiring || !input.water_hands[hand] || !m_hands[hand]) {
                continue;
            }
            float moved = 0;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float delta = (*input.water_hands[hand])[axis] - (*m_hands[hand])[axis];
                moved += delta * delta;
            }
            // Ignore discontinuous poses; a hand cannot sweep metres in one tick.
            if (moved < 4.f) {
                hand_impulses[hand] =
                    std::min(1.f, std::sqrt(moved) * 5.f) * m_debug.interaction_strength * .5f;
            }
        }
        stats::Scope timer("water simulation", stats::kGpu);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, m_buffers[kWindowCoast]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, m_buffers[kWindowHydraulics]);
        if (m_cached_body != selected || m_cached_origin != window
            || m_cached_height != m_sim_height) {
            m_window_prepare.use();
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffers[kStateA]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffers[kStateB]);
            glUniform1i(m_window_prepare.uniform("initialize_history"), reset ? 1 : 0);
            glUniform2f(
                m_window_prepare.uniform("previous_window"),
                float(m_cached_origin[0]),
                float(m_cached_origin[1])
            );
            glUniform1f(m_window_prepare.uniform("water_current_scale"), m_debug.current_scale);
            glUniform1i(m_window_prepare.uniform("water_body_count"), int(m_bodies.size() / 5));
            glUniform1f(m_window_prepare.uniform("water_sim_height"), m_sim_height);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffers[kCoast]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffers[kBodies]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_buffers[kHydraulics]);
            glUniform4f(
                m_window_prepare.uniform("window_config"),
                float(window[0]),
                float(window[1]),
                float(kWindowSize),
                float(selected)
            );
            glDispatchCompute((kWindowCells + 63) / 64, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            m_cached_body = selected;
            m_cached_height = m_sim_height;
            m_cached_origin = window;
        }

        m_step.use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffers[kCoast]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffers[kBodies]);
        glUniform4f(
            m_step.uniform("water_physics"),
            m_debug.current_scale,
            m_debug.friction,
            m_debug.outlet_damping,
            m_debug.interaction_radius * m_debug.interaction_radius
        );
        glUniform1f(m_step.uniform("displacement_limit"), m_debug.displacement_limit);
        glUniform1i(m_step.uniform("transport_reference"), 0);
        glUniform1f(m_step.uniform("wake_strength"), .9f);
        glUniform1i(m_step.uniform("cell_count"), int(kWindowCells));
        glUniform1i(m_step.uniform("local_window"), 1);
        glUniform1f(m_step.uniform("water_dt"), 1.f / 240.f);
        glUniform4f(
            m_step.uniform("window_config"),
            float(window[0]),
            float(window[1]),
            float(kWindowSize),
            float(selected)
        );
        glUniform2f(
            m_step.uniform("previous_window"), float(m_window_origin[0]), float(m_window_origin[1])
        );
        glUniform1i(m_step.uniform("atlas_width"), int(m_atlas_width));
        glUniform4fv(m_step.uniform("water_atlas[0]"), 64, m_atlas[0].data());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_buffers[kRenderPixels]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_buffers[kHydraulics]);
        glUniform3f(m_step.uniform("previous_hero"), m_hero[0], m_hero[1], m_hero[2]);
        for (unsigned hand = 0; hand < hand_impulses.size(); ++hand) {
            const auto point = m_hands[hand].value_or(renderer::Vec3{});
            glUniform3f(
                m_step.uniform(hand == 0 ? "previous_hands[0]" : "previous_hands[1]"),
                point[0],
                point[1],
                point[2]
            );
        }
        for (unsigned step = 0; step < steps * 4; ++step) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffers[m_current]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffers[1 - m_current]);
            glUniform1i(m_step.uniform("reset_waves"), reset && step == 0 ? 1 : 0);
            glUniform4f(
                m_step.uniform("hero"), hero[0], hero[1], hero[2], step == 0 ? impulse : 0.f
            );
            glUniform1i(
                m_step.uniform("hand_count"),
                step == 0 && (hand_impulses[0] > 0 || hand_impulses[1] > 0) ? 2 : 0
            );
            for (unsigned hand = 0; hand < hand_impulses.size(); ++hand) {
                const auto point = input.water_hands[hand].value_or(renderer::Vec3{});
                glUniform4f(
                    m_step.uniform(hand == 0 ? "hands[0]" : "hands[1]"),
                    point[0],
                    point[1],
                    point[2],
                    hand_impulses[hand]
                );
            }
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            glDispatchCompute((kWindowCells + 63) / 64, 1, 1);
            glUniform2f(m_step.uniform("previous_window"), float(window[0]), float(window[1]));
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            m_current = 1 - m_current;
        }
#ifdef OPENRAC_VULKAN
        if (steps > 0
            && !vkgl::copy_buffer_to_texture(
                m_buffers[kRenderPixels], m_maps[0], int(kWindowSize), int(kWindowSize)
            )) {
            m_enabled = false;
            water_ready.store(false);
            return;
        }
        water_ready.store(true);
#endif
        m_window_origin = window;
        remember_tick(input, hero, epoch);
        stats::gauge("water simulation state bytes", double(kWindowCells * sizeof(Texel) * 2));
        stats::gauge("water impulse", double(impulse));
        stats::gauge("water hand impulse", double(hand_impulses[0] + hand_impulses[1]));
    }

    void bind(
        const renderer::Shader& shader, std::uint32_t surface, const renderer::FrameInput& input
    ) override {
        glUniform1i(shader.uniform("water_debug_view"), m_debug.view);
        glUniform1f(shader.uniform("water_debug_height_limit"), m_debug.displacement_limit);
        glUniform1f(shader.uniform("water_fine_strength"), m_debug.fine_normal_strength);
        glUniform1i(shader.uniform("water_surface"), m_enabled ? int(surface) : 0);
        glUniform1i(shader.uniform("water_sim_local"), 1);
        glUniform1i(shader.uniform("water_sim_body"), m_sim_body);
        glUniform1f(shader.uniform("water_sim_height"), m_sim_height);
        glUniform1f(shader.uniform("water_sim_weight"), m_sim_weight);
        glUniform4f(
            shader.uniform("water_sim_window"),
            float(m_window_origin[0]) * .25f,
            float(m_window_origin[1]) * .25f,
            .25f,
            float(kWindowSize)
        );
        // All statically referenced Vulkan descriptors must be valid even on the original-
        // material branch. Bind once per effects pass, then only change the semantic surface.
        if (m_bound) {
            return;
        }
        m_bound = true;
        m_scene.bind_surface_lighting(shader, input.camera);
        glUniform1i(shader.uniform("water_bodies"), 11);
        glUniform1i(shader.uniform("water_coast_map"), 12);
        glUniform1i(shader.uniform("water_wave_map"), 14);
        glUniform4fv(shader.uniform("water_atlas[0]"), 64, m_atlas[0].data());
        glUniform2f(
            shader.uniform("water_atlas_extent"), float(m_atlas_width), float(m_atlas_height)
        );
        glUniform1i(shader.uniform("water_light"), 2);
        glUniform1i(shader.uniform("water_body_count"), int(m_bodies.size() / 5));
        glUniform1f(shader.uniform("water_time"), float(m_time));
        glUniform1f(shader.uniform("water_current_scale"), m_debug.current_scale);
        glUniform3f(
            shader.uniform("water_eye"),
            input.camera.position[0],
            input.camera.position[1],
            input.camera.position[2]
        );
        for (const auto& binding :
             {std::array<unsigned, 2>{11, kBodyTexture}, {2, kLightTexture}}) {
            glActiveTexture(GL_TEXTURE0 + binding[0]);
            glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[binding[1]]);
        }
        glActiveTexture(GL_TEXTURE0 + 12);
        glBindTexture(GL_TEXTURE_2D, m_maps[1]);
        glActiveTexture(GL_TEXTURE0 + 14);
        glBindTexture(GL_TEXTURE_2D, m_maps[0]);
        glActiveTexture(GL_TEXTURE0);
    }

    void release() override {
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
        m_mesh->clear_cache();
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

private:
    void remember_tick(
        const renderer::FrameInput& input, const renderer::Vec3& hero, std::uint64_t epoch
    ) {
        m_reset_epoch = epoch;
        m_last_frame = input.frame;
        m_hero = hero;
        m_hands = input.water_hands;
    }

    std::unique_ptr<WaterMesh> m_mesh;
    scene::LevelScene& m_scene;
    std::vector<Texel> m_bodies, m_coast, m_hydraulics;
    renderer::Shader m_step, m_light, m_window_prepare;
    std::array<unsigned, kBufferCount> m_buffers{};
    std::array<unsigned, kTextureCount> m_textures{};
    std::array<unsigned, 2> m_maps{};
    std::array<Texel, 64> m_atlas{};
    unsigned m_atlas_width = 0, m_atlas_height = 0;
    unsigned m_light_count = 0;
    unsigned m_current = 0;
    int m_sim_body = -1;
    float m_sim_height = 0, m_cached_height = 0;
    int m_cached_body = -1;
    std::array<int, 2> m_cached_origin{};
    std::array<int, 2> m_window_origin{};
    std::uint64_t m_prepared_frame = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t m_clock_frame = std::numeric_limits<std::uint64_t>::max();
    WaterDebugSettings m_debug;
    std::uint64_t m_reset_epoch = 0;
    std::uint64_t m_last_frame = std::numeric_limits<std::uint64_t>::max();
    renderer::Vec3 m_hero{};
    std::array<std::optional<renderer::Vec3>, 2> m_hands{};
    double m_time = 0, m_exit_time = -1;
    float m_sim_weight = 1;
    bool m_enabled = false;
    bool m_bound = false;
};
}  // namespace

WaterDebugSettings water_debug_settings() {
    std::lock_guard lock(water_debug_mutex);
    return water_debug;
}

void set_water_debug_settings(WaterDebugSettings settings) {
    const auto bounded = [](float value, float low, float high, float fallback) {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    settings.current_scale = bounded(settings.current_scale, 0, 2.5f, 1);
    settings.friction = bounded(settings.friction, 0, 4, .12f);
    settings.outlet_damping = bounded(settings.outlet_damping, 0, 30, 12);
    settings.interaction_strength = bounded(settings.interaction_strength, 0, 4, 3);
    settings.interaction_radius = bounded(settings.interaction_radius, .1f, 2, .54772256f);
    settings.displacement_limit = bounded(settings.displacement_limit, 0, .5f, .25f);
    settings.fine_normal_strength = bounded(settings.fine_normal_strength, 0, 2, 1);
    settings.view = std::clamp(settings.view, 0, 4);
    std::lock_guard lock(water_debug_mutex);
    water_debug = settings;
}

void reset_water_simulation() {
    water_reset_epoch.fetch_add(1);
}

bool water_available() {
    return water_ready.load();
}

std::unique_ptr<renderer::EffectMaterial> make_water(
    scene::LevelScene& scene, const std::filesystem::path& directory
) {
    if (!gl::enhanced_rendering_available() || !gl::is_vulkan()) {
        return {};
    }
    const auto path = directory / "water.bin";
    if (!std::filesystem::exists(path)) {
        log::info("Novalis enhanced water needs a local water.bin bake beside manifest.json");
        return {};
    }
    try {
        return std::make_unique<Water>(scene, path);
    } catch (const std::exception& error) {
        log::warn("Original water retained: {}", error.what());
        return {};
    }
}
}  // namespace openrac::enhanced
