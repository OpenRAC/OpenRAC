// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The enhanced lighting of a level, scene_lighting.h: each call of the scene's hook made on the
// probe lighting or the early lighting.

#include "enhanced/viewer/scene_lighting.h"

#include <algorithm>
#include <bit>
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

std::string hero_shader(std::string source, const char* fur_source, bool fragment = false) {
    if (gl::is_vulkan() && gl::enhanced_rendering_available()) {
        source.replace(0, source.find('\n'), "#version 430 core");
        source.insert(source.find('\n') + 1, "#define OPENRAC_HERO_WETNESS 1\n");
        // Static world variants cannot contain the live, skinned hero: keep their fur path absent.
        source.insert(
            source.find('\n') + 1,
            "#ifndef OPENRAC_STATIC_COLOUR\n#define OPENRAC_HERO_FUR 1\n#endif\n"
        );
        source.insert(source.find('\n') + 1, hero_only_source(fur_source));
        source.insert(source.find('\n') + 1, hero_only_source(shaders::fur_shell_glsl));
        if (fragment) {
            source.insert(source.find('\n') + 1, "#define OPENRAC_FUR_FRAGMENT 1\n");
        }
    }
    return source;
}

// Normal and early-lit variants share the same fur detail and diagnostic functions.
std::string hero_fragment(std::string source) {
    if (gl::is_vulkan() && gl::enhanced_rendering_available()) {
        const auto surface = source.find("vec3 probe_surface(");
        // Material response shares existing lighting helpers and never changes coverage or depth.
        if (surface != std::string::npos) {
            source.insert(surface, shaders::foliage_glsl);
        }
        const auto fur_surface = source.find("vec3 probe_surface(");
        if (fur_surface != std::string::npos) {
            source.insert(
                fur_surface,
                hero_only_source(shaders::fur_lighting_glsl)
                    + hero_only_source(shaders::fur_environment_glsl)
            );
        }
        source.insert(source.find('\n') + 1, hero_only_source(shaders::fur_detail_glsl));
    }
    return hero_shader(std::move(source), shaders::fur_glsl, true);
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
    // Snapshot session defaults once, avoiding locks in each material binding.
    m_default_fur = enhanced::fur_material_settings();
    m_default_wetness = enhanced::wetness_material_settings();
    m_wetness.prepare(input, palette, hero);
    m_fur.prepare(hero, input.camera.position);
}

void SceneLighting::bind_wetness(const renderer::Shader& shader) const {
    m_wetness.bind(shader, m_default_wetness);
    m_fur.bind(shader, m_default_fur);
}

int SceneLighting::surface_layers(
    std::uint32_t model, std::uint32_t first, std::uint32_t count
) const {
    return m_fur.shell_layers(model, first, count);
}

void SceneLighting::bind_surface_layer(const renderer::Shader& shader, int layer) const {
    // Temporal programs need the same verified palette, wetness and coverage maps as colour.
    if (layer == 0) {
        m_wetness.bind(shader);
        m_fur.bind(shader);
    }
    m_fur.bind_shell(shader, layer);
}

void SceneLighting::bind_moby_model(const renderer::Shader& shader, std::uint32_t model) const {
    m_fur.bind_model(shader, model, m_default_fur);
}

std::string SceneLighting::motion_fragment(std::string source) const {
    return hero_shader(std::move(source), shaders::fur_glsl, true);
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

    // Native lights also shade material fallbacks when probe GI is unavailable.
    m_local_lights.prepare(
        input.frame,
        scene.point_lights(),
        gl::enhanced_rendering_available(),
        scene.point_emitters()
    );
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

void SceneLighting::bind_surface_material(
    const renderer::Shader& shader, std::uint32_t model, int slot, const scene::Material& source
) const {
    // Every ordinary material resets its packed controls before the draw's immutable snapshot.
    if (gl::enhanced_rendering_available()) {
        m_foliage.bind(shader, source, m_material_overrides);
        const auto* response = m_material_overrides ? m_materials.response(model, slot) : nullptr;
        gl::glUniform1i(
            shader.uniform("material_fallback"), response ? response->fallback_channels : 0
        );
        // Catalog families override the legacy foliage adapter only when explicitly authored.
        if (response && response->family_override) {
            gl::glUniform1f(
                shader.uniform("foliage_reflectivity"), response->reflection.reflectivity
            );
            gl::glUniform1i(
                shader.uniform("foliage_parameters0"),
                std::bit_cast<std::int32_t>(response->foliage[0])
            );
            gl::glUniform1i(
                shader.uniform("foliage_parameters1"),
                std::bit_cast<std::int32_t>(response->foliage[1])
            );
        }
        if (response) {
            // Inherited values reset the preceding material's overrides within the same shader run.
            if (m_fur.hero_model(model)) {
                m_fur.bind_material(
                    shader, response->fur_override ? response->fur : m_default_fur
                );
            }
            m_wetness.bind_material(
                shader, response->wetness_override ? response->wetness : m_default_wetness
            );
        }
    }
}

void SceneLighting::bind_material(
    const renderer::Shader& shader, std::uint32_t model, int slot, const scene::Material& source
) const {
    bind_surface_material(shader, model, slot, source);
    const auto location = shader.uniform("object_reflection_enabled");
    if (location < 0) {
        return;
    }
    const auto* response = m_material_overrides ? m_materials.response(model, slot) : nullptr;
    // Explicit optics can use the original reflective colour when no probes are available.
    constexpr int kAuthoredReflection = 2;
    const int reflection = response && response->reflection_override ? kAuthoredReflection : 1;
    const bool enabled =
        m_material_overrides && source.metal && (active() || reflection == kAuthoredReflection);
    gl::glUniform1i(location, enabled ? reflection : 0);
    // Only reflective packets consume these uniforms.
    if (!source.metal) {
        return;
    }
    const auto found = m_reflection_classes.find(model);
    const int class_id = found != m_reflection_classes.end() ? found->second : -1;
    const auto material =
        response ? response->reflection : enhanced::reflection_material_for(source.name, class_id);
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

const scene::Material& SceneLighting::resolve_material(
    std::uint32_t model, int slot, const scene::Material& original
) const {
    const auto* binding = m_material_overrides && gl::enhanced_rendering_available()
                              ? m_materials.find(model, slot)
                              : nullptr;
    return binding ? binding->surface : original;
}

std::uint64_t SceneLighting::material_variant(std::uint32_t model, int slot) const {
    const auto* binding = m_material_overrides && gl::enhanced_rendering_available()
                              ? m_materials.find(model, slot)
                              : nullptr;
    return binding ? binding->variant : 0;
}

std::array<float, 3> SceneLighting::sky_tint(std::uint32_t model, int slot) const {
    // Classic rendering keeps the exported sky colours.
    const auto* response = m_material_overrides && gl::enhanced_rendering_available()
                               ? m_materials.response(model, slot)
                               : nullptr;
    return response ? response->sky_tint : std::array<float, 3>{1, 1, 1};
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
    // Foliage is optional: bad material authoring must not disable unrelated enhanced lighting.
    if (gl::enhanced_rendering_available()) {
        const auto foliage = m_foliage.load(level);
        if (!foliage) {
            log::warn("foliage materials: {}", foliage.error().message);
        }
        log::info("foliage materials: {} reviewed textures matched", m_foliage.size());
    }
    // Material resolution is CPU-only and completes before any colour draw can use the cache.
    const auto materials = m_materials.load(level);
    if (!materials) {
        log::warn("material overrides: {}", log::tidy(materials.error().message));
    }
    const auto editor = m_materials.editor();
    if (editor && editor->bindings) {
        log::info(
            "material overrides: {} bindings, {} foliage",
            editor->bindings->bindings.size(),
            editor->foliage_bindings
        );
    }
    m_wetness.load(level);
    m_fur.load(level, enhanced::fur_profile(level.game));
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
    m_foliage.clear();
    m_materials.clear();
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

// A missing bake changes lighting, while this switch follows the submitted graphics settings.
void SceneLighting::set_material_overrides(bool enabled) {
    m_material_overrides = enabled;
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
    // The coverage view draws with its own shaders (coverage_view).
    return !coverage_view() && m_early_lighting.matches(input, state);
}

std::span<const unsigned> SceneLighting::early_buffers(
    const renderer::FrameInput& input, const renderer::RenderState& state
) const {
    return m_early_lighting.buffers(input, state);
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
