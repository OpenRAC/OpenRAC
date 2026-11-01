// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The enhanced lighting of a level, scene_lighting.h: each call of the scene's hook made on the
// probe lighting or the early lighting as the level renderer made it itself before restructure
// package P2-39b.

#include "enhanced/viewer/scene_lighting.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "common/log.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/water.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "scene/level_renderer.h"

namespace openrac::lighting {

namespace {
// Static world variants cannot contain the live, skinned hero; discard every fur declaration there.
std::string hero_only_source(const char* source) {
    return std::string("#ifndef OPENRAC_STATIC_COLOUR\n") + source + "\n#endif\n";
}

std::string hero_shader(std::string source, const char* fur_source) {
    if (gl::is_vulkan() && gl::enhanced_rendering_available()) {
        source.replace(0, source.find('\n'), "#version 430 core");
        source.insert(source.find('\n') + 1, "#define OPENRAC_HERO_WETNESS 1\n");
        // Static world variants cannot contain the live, skinned hero: keep their fur path absent.
        source.insert(
            source.find('\n') + 1,
            "#ifndef OPENRAC_STATIC_COLOUR\n#define OPENRAC_HERO_FUR 1\n#endif\n"
        );
        source.insert(source.find('\n') + 1, hero_only_source(fur_source));
    }
    return source;
}

// Normal and early-lit variants share the same fur detail and diagnostic functions.
std::string hero_fragment(std::string source) {
    if (gl::is_vulkan() && gl::enhanced_rendering_available()) {
        const auto surface = source.find("vec3 probe_surface(");
        if (surface != std::string::npos) {
            source.insert(
                surface,
                hero_only_source(shaders::fur_lighting_glsl)
                    + hero_only_source(shaders::fur_environment_glsl)
            );
        }
        source.insert(source.find('\n') + 1, hero_only_source(shaders::fur_detail_glsl));
    }
    return hero_shader(std::move(source), shaders::fur_glsl);
}
}  // namespace

std::string SceneLighting::mesh_vertex(std::string source) const {
    return enhanced::WaterFloat::vertex_source(
        hero_shader(std::move(source), shaders::fur_vertex_glsl)
    );
}

std::string SceneLighting::mesh_fragment(std::string source) const {
    return enhanced::Atmosphere::fragment_source(
        hero_fragment(probe_mesh_fragment(std::move(source)))
    );
}

void SceneLighting::prepare_wetness(
    const renderer::FrameInput& input, unsigned palette, const std::optional<scene::Instance>& hero
) {
    m_wetness.prepare(input, palette, hero);
    m_fur.prepare(hero);
}

void SceneLighting::bind_wetness(const renderer::Shader& shader) const {
    m_wetness.bind(shader);
    m_fur.bind(shader);
}

void SceneLighting::prepare_deformation(
    const renderer::FrameInput& input, const scene::LevelScene& scene
) {
#ifdef OPENRAC_LIGHT_AUTHORING
    if (gl::enhanced_rendering_available()) {
        const auto attached = m_object_lights.prepare(input.frame, scene.joint_palette());
        const auto applied = m_local_lights.set_attached_lights(attached);
        if (!applied) {
            if (m_attachment_error != applied.error().message) {
                log::error("object lights: {}", applied.error().message);
            }
            m_attachment_error = applied.error().message;
            (void)m_local_lights.set_attached_lights({});
        } else {
            m_attachment_error.clear();
        }
    }
#endif

    m_local_lights.prepare(input.frame, scene.point_lights(), active());
    m_water_float.prepare(input, &scene);
}

void SceneLighting::bind_deformation(const renderer::Shader& shader, bool enabled) const {
    m_water_float.bind(shader, enabled);
}

bool SceneLighting::deformation_active() const {
    return enhanced::water_available();
}

bool SceneLighting::atmosphere_wanted(const renderer::FrameInput& input) const {
    return active() && input.volumetric_quality != renderer::VolumetricQuality::Off
           && enhanced::atmosphere_settings().enabled;
}

void SceneLighting::prepare_atmosphere(
    const scene::LevelScene& scene,
    const renderer::FrameInput& input,
    const renderer::RenderTarget& target,
    const std::array<float, 4>& colour,
    const std::array<float, 4>& fog
) {
    m_atmosphere.prepare(scene, input, target, colour, fog, active());
}

void SceneLighting::draw_atmosphere_sky(const renderer::RenderTarget& target) {
    m_atmosphere.draw_sky(target);
}

std::string SceneLighting::atmosphere_source() const {
    if (!gl::is_vulkan() || !gl::enhanced_rendering_available()) {
        return {};
    }
    return "#define OPENRAC_EFFECT_ATMOSPHERE 1\n" + enhanced::Atmosphere::sampling_source();
}

void SceneLighting::bind_atmosphere(const renderer::Shader& shader) const {
    m_atmosphere.bind(shader);
}

void SceneLighting::bind_material(
    const renderer::Shader& shader, std::uint32_t model, const scene::Material& source
) const {
    const auto location = shader.uniform("object_reflection_enabled");
    if (location < 0) {
        return;
    }
    gl::glUniform1i(location, active() && source.metal ? 1 : 0);
    if (!source.metal) {
        return;
    }
    const auto found = m_reflection_classes.find(model);
    const int class_id = found != m_reflection_classes.end() ? found->second : -1;
    const auto material = enhanced::reflection_material_for(source.name, class_id);
    gl::glUniform3f(
        shader.uniform("object_reflection_material"),
        material.roughness,
        material.reflectivity,
        material.strength
    );
    gl::glUniform4f(
        shader.uniform("object_reflection_tint"),
        material.tint[0],
        material.tint[1],
        material.tint[2],
        material.opacity
    );
}

std::string SceneLighting::early_fragment(std::string source) const {
    return enhanced::Atmosphere::fragment_source(
        hero_fragment(probe_early_fragment(std::move(source)))
    );
}

// Keeps the level renderer's direct-light and shadow direction aligned.
renderer::Vec3 SceneLighting::sunlight_direction(renderer::Vec3 authored) const {
    return m_probes.sunlight_direction(authored);
}

bool SceneLighting::early_supported() const {
    return EarlyLighting::supported();
}

void SceneLighting::load(const scene::LevelData& level) {
#ifdef OPENRAC_LIGHT_AUTHORING
    if (gl::enhanced_rendering_available()) {
        m_object_lights.load(level);
    }
#endif

    m_reflection_classes.clear();
    std::vector<enhanced::ReflectionSubject> subjects;
    for (const auto& [class_id, entry] : level.moby_classes) {
        const auto [model, stand_in] = entry;
        if (stand_in) {
            continue;
        }
        m_reflection_classes[model] = class_id;
        if (model >= level.models.size()) {
            continue;
        }
        const auto& source = level.models[model];
        const bool chrome =
            std::any_of(source.primitives.begin(), source.primitives.end(), [&](const auto& p) {
                return level.materials[static_cast<std::size_t>(p.material)].metal;
            });
        if (chrome) {
            const auto name = class_id == 87 ? "Clank" : class_id == 71 ? "Wrench" : source.name;
            subjects.push_back({class_id, name, {}});
        }
    }
    enhanced::set_reflection_subjects(std::move(subjects));
    m_probes.load(level);
    m_wetness.load(level);
    m_fur.load(level, enhanced::rac1_fur_profile());
    m_water_float.load();
    m_atmosphere.load();
}

void SceneLighting::release() {
#ifdef OPENRAC_LIGHT_AUTHORING
    m_attachment_error.clear();
    m_object_lights.release();
#endif
    m_local_lights.release();
    m_wetness.release();
    m_fur.release();
    m_water_float.release();
    m_atmosphere.release();
    m_probes.release();
    m_early_lighting.release();
}

std::string_view SceneLighting::load_error() const {
    return m_probes.load_error();
}

bool SceneLighting::has_data() const {
    return m_probes.has_data();
}

bool SceneLighting::active() const {
    return m_probes.active();
}

int SceneLighting::mode() const {
    return m_probes.mode;
}

void SceneLighting::set_mode(int mode) {
    m_probes.mode = mode;
}

void SceneLighting::set_lighting(bool enabled, float multiplier) {
    m_probes.enabled = enabled;
    m_probes.gi_multiplier = multiplier;
}

void SceneLighting::set_sunlight_direction(std::optional<renderer::Vec3> direction) {
    m_probes.runtime_sunlight_direction = direction;
}

void SceneLighting::set_bias(float normal, float view) {
    m_probes.normal_bias = normal;
    m_probes.view_bias = view;
}

void SceneLighting::set_spheres(bool shown) {
    m_probes.show_spheres = shown || m_probes.environment_spheres;
}

void SceneLighting::set_area_lights(bool shown) {
    m_probes.show_area_lights = shown;
}

bool SceneLighting::compute_selection() const {
    return m_probes.compute_selection() || (active() && gl::is_vulkan());
}

void SceneLighting::set_medial_axis_debug(bool shown) {
    m_probes.show_medial_axis = shown;
}

void SceneLighting::set_environment_probe_debug(bool shown) {
    m_probes.show_environment_probes = shown;
}

void SceneLighting::select_visible_regions(
    unsigned depth, const renderer::Camera& camera, int width, int height
) {
    m_probes.select_visible_regions(depth, camera, width, height);
    m_local_lights.dispatch(depth, camera, width, height);
}

void SceneLighting::update_regions(const renderer::Camera& camera, float seconds) {
    const auto clip = renderer::multiply(camera.projection, camera.view);
    m_probes.update_regions(camera.position, seconds, &clip);
}

void SceneLighting::draw_spheres(
    const renderer::FrameInput& input,
    renderer::RenderState& state,
    std::optional<renderer::Vec3> player
) {
    m_probes.draw_spheres(input, state, player);
}

void SceneLighting::bind(
    const renderer::Shader& shader,
    scene::Layer layer,
    bool all_static_receivers,
    const renderer::Camera* camera,
    bool depth_matched
) const {
    m_probes.bind(shader, layer, all_static_receivers, camera, depth_matched);
    m_local_lights.bind(shader, camera, depth_matched);
    m_atmosphere.bind(shader);
}

bool SceneLighting::render_early(
    unsigned depth,
    const renderer::FrameInput& input,
    const renderer::RenderTarget& target,
    const std::function<void(const renderer::Shader&)>& bind_lighting,
    std::string& error
) {
    return m_early_lighting.render(depth, input, target, bind_lighting, error);
}

void SceneLighting::bind_early(
    const renderer::Shader& shader,
    const renderer::FrameInput& input,
    const renderer::RenderState& state
) const {
    m_early_lighting.bind(shader, input, state);
}

bool SceneLighting::early_matches(
    const renderer::FrameInput& input, const renderer::RenderState& state
) const {
    return m_early_lighting.matches(input, state);
}

bool SceneLighting::set_probe_volume(
    const ProbeVolume& volume, std::string& error, const ProbeVolume* dense
) {
    return m_probes.upload(volume, error, dense);
}

bool SceneLighting::set_probe_regions(
    const ProbeVolume& volume, std::vector<ProbeVolume> regions, std::string& error
) {
    return m_probes.upload_regions(volume, std::move(regions), error);
}

SceneLighting& use_scene_lighting(scene::LevelScene& scene) {
    auto lighting = std::make_unique<SceneLighting>();
    SceneLighting& registered = *lighting;
    scene.set_level_lighting(std::move(lighting));
    return registered;
}

SceneLighting* scene_lighting(scene::LevelScene& scene) {
    return dynamic_cast<SceneLighting*>(scene.level_lighting());
}

}  // namespace openrac::lighting
