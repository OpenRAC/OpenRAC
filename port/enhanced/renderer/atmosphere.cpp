// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/atmosphere.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <utility>
#include <vector>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"
#include "scene/level_renderer.h"

namespace openrac::enhanced {
using namespace gl;

namespace {
std::mutex settings_mutex;
AtmosphereSettings settings;
constexpr unsigned kAtmosphereUnit = 24;

float finite(float value, float low, float high, float fallback) {
    return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
}
}  // namespace

AtmosphereSettings atmosphere_settings() {
    std::lock_guard lock(settings_mutex);
    return settings;
}

void set_atmosphere_settings(AtmosphereSettings value) {
    const AtmosphereSettings defaults;
    value.haze = finite(value.haze, 0, 4, defaults.haze);
    value.height_density = finite(value.height_density, 0, 1, 0);
    value.base_height = finite(value.base_height, -100000, 100000, 0);
    value.height_falloff = finite(value.height_falloff, 0, 4, .1f);
    value.distance = finite(value.distance, 10, 2000, 750);
    value.sun_strength = finite(value.sun_strength, 0, 1, .15f);
    value.anisotropy = finite(value.anisotropy, -.8f, .8f, .3f);
    for (auto& volume : value.volumes) {
        for (int k = 0; k < 3; ++k) {
            volume.centre_radius[k] = finite(volume.centre_radius[k], -100000, 100000, 0);
            volume.colour_density[k] = finite(volume.colour_density[k], 0, 1, .7f);
        }
        volume.centre_radius[3] = finite(volume.centre_radius[3], .01f, 2000, 8);
        volume.colour_density[3] = finite(volume.colour_density[3], 0, 1, 0);
    }
    std::lock_guard lock(settings_mutex);
    settings = value;
}

Atmosphere::~Atmosphere() {
    release();
}

std::string Atmosphere::sampling_source() {
    return std::string(lighting::shaders::atmosphere_transport_glsl) + "\n"
           + std::string(lighting::shaders::atmosphere_sample_glsl) + "\n";
}

std::string Atmosphere::fragment_source(std::string source) {
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return source;
    }
    source.insert(source.find('\n') + 1, "#define OPENRAC_ATMOSPHERE 1\n");
    source.insert(source.find("void main()"), sampling_source());
    return source;
}

void Atmosphere::load() {
    release();
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return;
    }
    const std::array<float, 16> empty{};
    glGenBuffers(1, &m_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(empty), empty.data(), GL_DYNAMIC_DRAW);
    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + kAtmosphereUnit);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_texture);
    glTexBuffer(renderer::world::gl::GL_TEXTURE_BUFFER, renderer::world::gl::GL_RGBA32F, m_buffer);
    glGenTextures(1, &m_dummy_depth);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, m_dummy_depth);
    const std::array<float, 4> clear_depth{};
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        renderer::world::gl::GL_RGBA32F,
        1,
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        clear_depth.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE0);
}

void Atmosphere::release() {
    for (auto& program : m_compute) {
        program.release();
    }
    m_sky.release();
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }
    if (m_dummy_depth) {
        glDeleteTextures(1, &m_dummy_depth);
    }
    if (m_buffer) {
        glDeleteBuffers(1, &m_buffer);
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_texture = m_buffer = m_vao = m_dummy_depth = 0;
    m_width = m_height = m_slices = 0;
    m_active = m_failed = m_sky_active = false;
}

void Atmosphere::prepare(
    const scene::LevelScene& scene,
    const renderer::FrameInput& input,
    const renderer::RenderTarget& target,
    const std::array<float, 4>& colour,
    const std::array<float, 4>& fog,
    bool enabled
) {
    m_active = m_sky_active = false;
    const auto controls = atmosphere_settings();
    const auto grid =
        renderer::volumetric_grid(input.volumetric_quality, target.width, target.height);
    if (grid.slices == 0) {
        if (m_buffer && m_slices != 0) {
            const std::array<float, 16> empty{};
            glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(empty), empty.data(), GL_DYNAMIC_DRAW);
            m_width = m_height = m_slices = 0;
        }
        return;
    }
    if (!m_buffer || m_failed || !enabled || !controls.enabled || input.keep_colour
        || target.width <= 0 || target.height <= 0) {
        return;
    }
    std::array<float, 32> centres{}, media{};
    int volume_count = 0;
    for (const auto& volume : controls.volumes) {
        if (volume.colour_density[3] == 0) {
            continue;
        }
        const int i = volume_count++;
        std::copy(
            volume.centre_radius.begin(), volume.centre_radius.end(), centres.begin() + i * 4
        );
        std::copy(
            volume.colour_density.begin(), volume.colour_density.end(), media.begin() + i * 4
        );
    }
    auto& compute = m_compute
        [renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderVariants)
                 && volume_count == 0
             ? 0
             : 1];
    std::string error;
    if (!compute.id()) {
        std::string source = "#version 430 core\n#define OPENRAC_PROBE_PIXEL ivec2(-1)\n"
                             + std::string(lighting::shaders::probes_glsl) + "\n"
                             + std::string(lighting::shaders::atmosphere_transport_glsl) + "\n"
                             + std::string(lighting::shaders::atmosphere_lighting_glsl) + "\n"
                             + std::string(lighting::shaders::atmosphere_comp);
        if (&compute == &m_compute[0]) {
            source.insert(source.find('\n') + 1, "#define OPENRAC_ATMOSPHERE_NO_VOLUMES 1\n");
            source =
                renderer::shader_constants(std::move(source), {{"atmosphere_volume_count", 0}});
        }
        if (!compute.build_compute("atmosphere transport", source, error)) {
            log::warn("Original fog retained: {}", error);
            m_failed = true;
            return;
        }
    }
    const stats::Scope timing("atmosphere transport", stats::kGpu);
    const int width = grid.width, height = grid.height;
    if (width != m_width || height != m_height || grid.slices != m_slices) {
        // A clipped texel-buffer view would silently lose the far slices. Keep original fog
        // until the requested tier fits; a later lower tier can still recover without reload.
        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
        const std::size_t texels = std::size_t(width) * height * grid.slices * 4;
        if (limit <= 0 || texels > static_cast<std::size_t>(limit)) {
            static log::Capped told(1);
            if (told()) {
                log::warn(
                    "Atmosphere grid exceeds the device's texel-buffer limit; original fog retained"
                );
            }
            return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
        glBufferData(
            GL_ARRAY_BUFFER, GLsizeiptr(texels * 4 * sizeof(float)), nullptr, GL_DYNAMIC_DRAW
        );
        m_width = width;
        m_height = height;
        m_slices = grid.slices;
    }
    m_screen = {0, 0, float(target.width), float(target.height)};
    m_view = input.camera.view;
    m_far = controls.distance;
    // Keep every statically referenced shadow descriptor valid when shadows are unavailable.
    for (unsigned unit = 7; unit <= 10; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_dummy_depth);
    }
    compute.use();
    glUniform1i(compute.uniform("atmosphere_slices"), m_slices);
    scene.bind_surface_lighting(compute, input.camera);
    glUniform2f(compute.uniform("atmosphere_grid_size"), float(width), float(height));
    glUniform3f(
        compute.uniform("atmosphere_eye"),
        input.camera.position[0],
        input.camera.position[1],
        input.camera.position[2]
    );
    const auto& v = input.camera.view;
    glUniform3f(compute.uniform("atmosphere_right"), v[0], v[4], v[8]);
    glUniform3f(compute.uniform("atmosphere_up"), v[1], v[5], v[9]);
    glUniform3f(compute.uniform("atmosphere_forward"), -v[2], -v[6], -v[10]);
    glUniform4f(
        compute.uniform("atmosphere_tangents"),
        1 / input.camera.projection[0],
        1 / input.camera.projection[5],
        input.camera.projection[8],
        input.camera.projection[9]
    );
    glUniform4fv(compute.uniform("atmosphere_fog"), 1, fog.data());
    glUniform4fv(compute.uniform("atmosphere_colour"), 1, colour.data());
    glUniform4f(
        compute.uniform("atmosphere_height"),
        controls.height_density,
        controls.base_height,
        controls.height_falloff,
        m_far
    );
    glUniform4f(
        compute.uniform("atmosphere_scatter"),
        controls.haze,
        controls.sun_strength,
        controls.anisotropy,
        0
    );
    glUniform1i(compute.uniform("atmosphere_volume_count"), volume_count);
    m_sky_active = controls.height_density > 0 || volume_count > 0;
    glUniform4fv(compute.uniform("atmosphere_centres[0]"), 8, centres.data());
    glUniform4fv(compute.uniform("atmosphere_media[0]"), 8, media.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_buffer);
    glDispatchCompute((unsigned(width) + 7) / 8, (unsigned(height) + 7) / 8, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    stats::gauge("atmosphere cells", width * height * grid.slices);
    m_active = true;
    glActiveTexture(GL_TEXTURE0);
}

void Atmosphere::bind(const renderer::Shader& shader) const {
    if (shader.uniform("atmosphere_volume") < 0) {
        return;
    }
    glUniform1i(shader.uniform("atmosphere_enabled"), m_active ? 1 : 0);
    glUniform1i(shader.uniform("atmosphere_volume"), kAtmosphereUnit);
    glUniform2f(
        shader.uniform("atmosphere_grid_size"),
        float(std::max(1, m_width)),
        float(std::max(1, m_height))
    );
    glUniform4fv(shader.uniform("atmosphere_screen"), 1, m_screen.data());
    glUniformMatrix4fv(shader.uniform("atmosphere_view"), 1, GL_FALSE, m_view.data());
    glUniform1f(shader.uniform("atmosphere_far"), m_far);
    glUniform1i(shader.uniform("atmosphere_slices"), std::max(1, m_slices));
    glActiveTexture(GL_TEXTURE0 + kAtmosphereUnit);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_texture);
    glActiveTexture(GL_TEXTURE0);
}

void Atmosphere::draw_sky(const renderer::RenderTarget& target) {
    if (!m_active || !m_sky_active) {
        return;
    }
    std::string error;
    if (!m_sky.id()
        && !m_sky.build(
            "local fog over authored sky",
            renderer::shaders::gtao_vert,
            fragment_source(std::string(lighting::shaders::atmosphere_sky_frag)),
            error
        )) {
        log::warn("Sky fog unavailable: {}", error);
        m_active = false;
        m_failed = true;
        return;
    }
    const stats::Scope timing("atmosphere sky", stats::kGpu);
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
    }
    glViewport(0, 0, target.width, target.height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    m_sky.use();
    bind(m_sky);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

namespace {
class AtmosphericMaterial final : public renderer::EffectMaterial {
public:
    AtmosphericMaterial(
        scene::LevelScene& scene, std::unique_ptr<renderer::EffectMaterial> material
    )
        : m_scene(scene),
          m_material(std::move(material)) {}

    std::string fragment_source() const override {
        return (m_material ? m_material->fragment_source()
                           : "vec3 effect_material(vec3 rgb){return rgb;}\n")
               + m_scene.atmosphere_source();
    }

    std::string vertex_source() const override {
        return m_material ? m_material->vertex_source() : std::string{};
    }

    std::optional<std::string> quad_fragment_source() const override {
        auto source =
            m_material
                ? m_material->quad_fragment_source()
                : std::optional<std::string>{"vec3 effect_material(vec3 rgb){return rgb;}\n"};
        if (source) {
            *source += m_scene.atmosphere_source();
        }
        return source;
    }

    void bind_quads(const renderer::Shader& shader, const renderer::FrameInput& input) override {
        if (m_material) {
            m_material->bind_quads(shader, input);
        }
        bind_atmosphere(shader);
    }

    bool plain_strips_supported() const override {
        return !m_material || m_material->plain_strips_supported();
    }

    bool init(std::string& error) override {
        if (m_material && !m_material->init(error)) {
            m_material->release();
            m_material.reset();
            error.clear();
        }
        return true;
    }

    void prepare(const renderer::FrameInput& input) override {
        m_bound_programs.clear();
        m_setups = 0;
        if (m_material) {
            m_material->prepare(input);
        }
    }

    std::optional<renderer::EffectMesh> mesh(
        const renderer::EffectStrip& s, std::span<const renderer::EffectVertex> v
    ) override {
        return m_material ? m_material->mesh(s, v) : std::nullopt;
    }

    void upload_meshes() override {
        if (m_material) {
            m_material->upload_meshes();
        }
    }

    // Preserve the child material's pre-effects scene snapshot.
    void prepare_scene(
        const renderer::FrameInput& input, const renderer::RenderTarget& target
    ) override {
        if (m_material) {
            m_material->prepare_scene(input, target);
        }
    }

    void finish_pass() override {
        if (m_material) {
            m_material->finish_pass();
        }
        stats::gauge("effect atmosphere setups", m_setups);
    }

    void draw_mesh(const renderer::Shader& shader, const renderer::EffectMesh& mesh) override {
        if (m_material) {
            m_material->draw_mesh(shader, mesh);
        }
    }

    void bind(
        const renderer::Shader& shader,
        std::uint32_t surface,
        const renderer::FrameInput& input,
        const renderer::EffectUvMotion& motion
    ) override {
        if (m_material) {
            m_material->bind(shader, surface, input, motion);
        }
        bind_atmosphere(shader);
    }

    void release() override {
        if (m_material) {
            m_material->release();
        }
    }

private:
    void bind_atmosphere(const renderer::Shader& shader) {
        const bool known = std::find(m_bound_programs.begin(), m_bound_programs.end(), shader.id())
                           != m_bound_programs.end();
        if (!known || !renderer::render_opt::enabled(renderer::render_opt::Opt::EffectSetupReuse)) {
            m_scene.bind_atmosphere(shader);
            ++m_setups;
            if (!known) {
                m_bound_programs.push_back(shader.id());
            }
        }
    }

    std::vector<unsigned> m_bound_programs;
    unsigned m_setups = 0;
    scene::LevelScene& m_scene;
    std::unique_ptr<renderer::EffectMaterial> m_material;
};
}  // namespace

std::unique_ptr<renderer::EffectMaterial> atmosphere_material(
    scene::LevelScene& scene, std::unique_ptr<renderer::EffectMaterial> material
) {
    if (!is_vulkan() || !enhanced_rendering_available() || scene.atmosphere_source().empty()) {
        return material;
    }
    return std::make_unique<AtmosphericMaterial>(scene, std::move(material));
}
}  // namespace openrac::enhanced
