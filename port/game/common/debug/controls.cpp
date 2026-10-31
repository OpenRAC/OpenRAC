// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's settings controls (debug/overlay.h), which the drawer and the menu bar share:
// a setting moved on, and the enhanced graphics' switches and sliders with GTAO's.

#include <algorithm>
#include <array>
#include <imgui.h>
#include <string_view>

#include "debug/overlay.h"
#include "enhanced/fsr3/fsr3.h"
#include "enhanced/renderer/atmosphere.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/water.h"
#include "enhanced/renderer/wetness.h"
#include "frontend.h"
#include "options_menu.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {

namespace {

void control_tip(const char* text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", text);
    }
}

// Each row owns its table so feature trees and reset buttons can remain full width.
template <class Draw>
bool setting_row(const char* label, Draw draw) {
    ImGui::PushID(label);
    bool changed = false;
    if (ImGui::BeginTable("row", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch, 1.15f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        const std::string_view visible(label);
        const auto count = visible.find("##");
        ImGui::TextUnformatted(
            label, label + (count == std::string_view::npos ? visible.size() : count)
        );
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1);
        changed = draw();
        ImGui::EndTable();
    }
    ImGui::PopID();
    return changed;
}

bool checkbox(const char* label, bool* value) {
    return setting_row(label, [&] { return ImGui::Checkbox("##value", value); });
}

bool slider_float(
    const char* label,
    float* value,
    float lo,
    float hi,
    const char* format = "%.3f",
    ImGuiSliderFlags flags = 0
) {
    return setting_row(label, [&] {
        return ImGui::SliderFloat("##value", value, lo, hi, format, flags);
    });
}

bool slider_int(
    const char* label,
    int* value,
    int lo,
    int hi,
    const char* format = "%d",
    ImGuiSliderFlags flags = 0
) {
    return setting_row(label, [&] {
        return ImGui::SliderInt("##value", value, lo, hi, format, flags);
    });
}

bool combo(const char* label, int* value, const char* items) {
    return setting_row(label, [&] { return ImGui::Combo("##value", value, items); });
}

bool combo(const char* label, int* value, const char* const items[], int count) {
    return setting_row(label, [&] { return ImGui::Combo("##value", value, items, count); });
}

bool colour_edit(const char* label, float* value) {
    return setting_row(label, [&] { return ImGui::ColorEdit3("##value", value); });
}

bool input_float3(const char* label, float* value) {
    return setting_row(label, [&] { return ImGui::InputFloat3("##value", value); });
}

// Persistent blur tuning lives in its own section, separate from lighting diagnostics.
bool motion_blur_controls(settings::Settings& graphics) {
    ImGui::SeparatorText("Motion blur");
    ImGui::BeginDisabled(!enhanced::Fsr3::available());
    bool changed = checkbox("Enabled##motion_blur", &graphics.motion_blur);
    control_tip("Camera-target-relative during play; normal camera blur in cutscenes.");
    ImGui::BeginDisabled(!graphics.motion_blur);
    ImGui::PushItemWidth(dp(190.f));
    auto& blur = graphics.motion_blur_settings;
    changed |= slider_float(
        "Strength", &graphics.motion_blur_strength, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    control_tip("Exposure strength. 0.5 is a 1/120-second exposure; 1 is 1/60 second.");
    if (ImGui::TreeNode("Advanced##motion_blur")) {
        changed |= slider_float(
            "Cutscene multiplier",
            &blur.cutscene_multiplier,
            0,
            2,
            "%.2fx",
            ImGuiSliderFlags_AlwaysClamp
        );
        control_tip("Scales strength only in real-time cutscenes. Zero disables cutscene blur.");
        changed |= slider_int(
            "Max trail length", &blur.max_radius, 1, 128, "%d px", ImGuiSliderFlags_AlwaysClamp
        );
        control_tip("Caps trails in display pixels. Longer trails can cost more GPU time.");
        changed |= slider_int("Samples", &blur.samples, 8, 64, "%d", ImGuiSliderFlags_AlwaysClamp);
        control_tip("Higher counts make trails smoother, at a higher GPU cost.");
        changed |= slider_float(
            "Minimum motion", &blur.min_motion, .1f, 4, "%.2f px", ImGuiSliderFlags_AlwaysClamp
        );
        control_tip(
            "Ignores tiny movements after exposure scaling. Raise it to suppress subtle smearing."
        );
        changed |=
            slider_float("Trail spread", &blur.spread, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        control_tip(
            "Higher values include more nearby motion but "
            "may "
            "introduce halos."
        );
        changed |= slider_float(
            "Depth tolerance",
            &blur.depth_tolerance,
            .001f,
            .05f,
            "%.3f",
            ImGuiSliderFlags_AlwaysClamp
        );
        control_tip(
            "Allowed relative depth difference when gathering colour. Lower values protect "
            "occlusion "
            "edges; higher values blend more freely."
        );
        ImGui::TreePop();
    }
    ImGui::PopItemWidth();
    ImGui::EndDisabled();
    if (ImGui::Button("Reset blur tuning")) {
        graphics.motion_blur_strength = .5f;
        blur = renderer::MotionBlurSettings{};
        changed = true;
    }
    control_tip("Restores tuning defaults and keeps the Enabled switch unchanged.");
    ImGui::EndDisabled();
    return changed;
}

// Independent surface controls; the renderer reads bounded copies under a lock.
void reflection_material_controls(const char* label, enhanced::ReflectionSurface surface) {
    if (!ImGui::TreeNode(label)) {
        return;
    }
    auto material = enhanced::reflection_material_settings(surface);
    bool changed = slider_float(
        "Roughness", &material.roughness, .04f, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= slider_float(
        "Reflectivity (F0)", &material.reflectivity, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= slider_float(
        "Reflection strength", &material.strength, 0, 4, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    if (surface != enhanced::ReflectionSurface::Water) {
        changed |= colour_edit("Reflection tint", material.tint.data());
        changed |= slider_float(
            "Chrome opacity", &material.opacity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
    }
    if (ImGui::Button("Reset material")) {
        material = enhanced::default_reflection_material(surface);
        changed = true;
    }
    if (changed) {
        enhanced::set_reflection_material_settings(surface, material);
    }
    ImGui::TreePop();
}

// Class identities are retained by the renderer so exceptions do not depend on texture guesses.
void reflection_override_controls() {
    if (!ImGui::TreeNode("Object overrides")) {
        return;
    }
    const auto subjects = enhanced::reflection_subjects();
    if (subjects.empty()) {
        ImGui::TextDisabled("No authored chrome objects loaded.");
        ImGui::TreePop();
        return;
    }
    static int selected = -1;
    auto current = std::find_if(subjects.begin(), subjects.end(), [&](const auto& s) {
        return s.class_id == selected;
    });
    if (current == subjects.end()) {
        current = subjects.begin();
        selected = current->class_id;
    }
    const auto label = [](const auto& s) {
        return s.name + " (" + std::to_string(s.class_id) + ")";
    };
    if (ImGui::BeginCombo("Object", label(*current).c_str())) {
        for (auto entry = subjects.begin(); entry != subjects.end(); ++entry) {
            if (ImGui::Selectable(label(*entry).c_str(), entry == current)) {
                selected = entry->class_id;
                current = entry;
            }
        }
        ImGui::EndCombo();
    }
    constexpr std::array profiles{
        enhanced::ReflectionSurface::Chrome,
        enhanced::ReflectionSurface::Glass,
        enhanced::ReflectionSurface::Stylised,
        enhanced::ReflectionSurface::Clank,
        enhanced::ReflectionSurface::Wrench
    };
    int profile = 0;
    for (unsigned i = 0; i < profiles.size(); ++i) {
        if (current->override == profiles[i]) {
            profile = static_cast<int>(i) + 1;
        }
    }
    if (combo(
            "Material profile", &profile, "Automatic\0Chrome\0Glass\0Stylised\0Clank\0Wrench\0"
        )) {
        enhanced::set_reflection_override(
            selected,
            profile == 0 ? std::nullopt
                         : std::optional{profiles[static_cast<unsigned>(profile - 1)]}
        );
    }
    if (ImGui::Button("Clear object overrides")) {
        enhanced::clear_reflection_overrides();
    }
    control_tip(
        "Overrides affect this object's chrome and glass packets. Painted faces keep their "
        "material."
    );
    ImGui::TreePop();
}

// Live, session-only water controls; the render worker consumes a locked copy.
void water_controls() {
    auto options = enhanced::water_debug_settings();
    ImGui::TextDisabled("Session only; changes are not saved.");
    ImGui::BeginDisabled(!enhanced::water_available());
    bool changed = checkbox("Pause simulation", &options.paused);
    ImGui::PushItemWidth(dp(170.f));
    changed |= slider_float(
        "Current speed", &options.current_scale, 0, 2.5f, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Changing current speed resets the water. The pond has no base current.");
    }
    changed |=
        slider_float("Friction", &options.friction, 0, 4, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    changed |= slider_float(
        "Outlet damping", &options.outlet_damping, 0, 30, "%.1f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= slider_float(
        "Ratchet strength",
        &options.interaction_strength,
        0,
        4,
        "%.2fx",
        ImGuiSliderFlags_AlwaysClamp
    );
    changed |= slider_float(
        "Ratchet radius",
        &options.interaction_radius,
        .1f,
        2,
        "%.2f units",
        ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::SeparatorText("Projectiles");
    changed |= slider_float(
        "Projectile strength",
        &options.projectile_strength,
        0,
        4,
        "%.2fx",
        ImGuiSliderFlags_AlwaysClamp
    );
    control_tip(
        "Scales ripples from new shots and thrown bombs. Zero disables their response; "
        "existing waves continue to settle."
    );
    changed |= slider_float(
        "Projectile radius",
        &options.projectile_radius,
        .1f,
        4,
        "%.2fx",
        ImGuiSliderFlags_AlwaysClamp
    );
    control_tip(
        "Scales each projectile's water-contact radius. 1x keeps its normal size. "
        "The resulting radius is limited to 4 world units."
    );
    ImGui::SeparatorText("Explosions");
    changed |= slider_float(
        "Explosion strength",
        &options.explosion_strength,
        0,
        4,
        "%.2fx",
        ImGuiSliderFlags_AlwaysClamp
    );
    control_tip(
        "Scales waves from new explosions. Zero disables their response; "
        "existing waves continue to settle."
    );
    changed |= slider_float(
        "Explosion radius", &options.explosion_radius, .1f, 4, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    control_tip(
        "Scales each explosion's reach across and above the water. 1x keeps its normal size. "
        "The resulting radius is limited to 4 world units."
    );
    ImGui::SeparatorText("Surface");
    changed |= slider_float(
        "Displacement limit",
        &options.displacement_limit,
        0,
        .5f,
        "%.2f units",
        ImGuiSliderFlags_AlwaysClamp
    );
    changed |= slider_float(
        "Fine ripples", &options.fine_normal_strength, 0, 2, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= combo(
        "Water view",
        &options.view,
        "Lit\0Height\0Velocity\0Banks\0Flow detail\0Direct highlights\0Environment "
        "reflections\0Illumination\0Surface mask\0Captured reflections\0GI fallback\0Cached "
        "GI\0Reference reflection selection\0Foam off\0Foam mask\0Reference foam\0Shoreline blend "
        "mask\0Reference shoreline blend\0"
    );
    ImGui::PopItemWidth();
    if (options.view == 1) {
        ImGui::TextDisabled("Red: raised  Blue: lowered  Gray: rest");
    }
    if (options.view == 2) {
        ImGui::TextDisabled("Red/green encode X/Y velocity.");
    }
    if (options.view == 3) {
        ImGui::TextDisabled("Red: active water  Green: shore influence");
    }
    if (options.view == 4) {
        ImGui::TextDisabled("Surface pattern carried by the simulated flow.");
    }
    if (ImGui::Button("Reset simulation")) {
        enhanced::reset_water_simulation();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore water defaults")) {
        options = enhanced::WaterDebugSettings{};
        changed = true;
        enhanced::reset_water_simulation();
    }
    if (changed) {
        enhanced::set_water_debug_settings(options);
    }
    ImGui::EndDisabled();
    if (!enhanced::water_available()) {
        ImGui::TextDisabled("A valid Novalis water bake is required.");
    }
}

// Draws GTAO's controls: its quality, its resolution, its directions and steps, its radius and
// strength, and a button that resets them. A change is saved and marked for the window to apply.
void gtao_controls() {
    auto& options = settings::get().gtao_settings;
    bool changed = false;
    ImGui::PushItemWidth(dp(170.0f));
    changed |= checkbox("Half resolution", &options.half_resolution);
    changed |= slider_int("Directions", &options.slices, 1, 12, "%d", ImGuiSliderFlags_AlwaysClamp);
    changed |=
        slider_int("Steps per side", &options.steps, 4, 24, "%d", ImGuiSliderFlags_AlwaysClamp);
    changed |=
        slider_float("Radius", &options.radius, 0.05f, 8.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    changed |= slider_float(
        "Strength multiplier", &options.strength, 0.0f, 3.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    ImGui::PopItemWidth();
    if (ImGui::Button("Reset GTAO settings")) {
        options = renderer::GtaoSettings{};
        changed = true;
    }
    if (changed) {
        settings::save();
        s.settings_changed = true;
    }
}

}  // namespace

// A setting moved in the drawer or the menus: saved, and applied by the window
// (take_settings_changed). A picture shape of the player's own is the settings file's.
void change_setting(int row, int by) {
    options_menu::step(row, by);
    if (row == options_menu::kAspect && settings::get().aspect == settings::Aspect::Custom) {
        options_menu::step(row, by);
    }
    settings::save();
    s.settings_changed = true;
}

void enhanced_settings_window() {
    if (!s.enhanced_settings) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(dp(490), dp(660)), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Enhanced Settings", &s.enhanced_settings)) {
        ImGui::End();
        return;
    }
    auto& graphics = settings::get();
    const bool available = enhanced_graphics_available();
    if (!available) {
        const auto error = enhanced_graphics_error();
        if (!error.empty()) {
            ImGui::TextColored(ImVec4(1.f, .4f, .3f, 1.f), "Probe data could not be loaded");
            ImGui::TextWrapped("%s", error.c_str());
            ImGui::TextWrapped(
                "If the probe data is out of date, rebake this level with the current version of "
                "the game, then reload the level."
            );
        } else {
            ImGui::TextWrapped(
                "Compatible probe data is required for this level. Prepare its lighting in the "
                "launcher's Enhanced graphics page."
            );
        }
    }
    ImGui::BeginDisabled(!available);
    bool changed = checkbox("Enhanced graphics (F4)", &graphics.enhanced_graphics);
    if (ImGui::Button("Reset to Defaults##enhanced")) {
        graphics.reset_enhanced();
        enhanced::set_wetness_material_settings({});
        enhanced::set_wetness_debug_view(false);
        enhanced::set_water_debug_settings({});
        enhanced::set_atmosphere_settings({});
        enhanced::reset_water_simulation();
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Restore enhanced graphics, post processing and session tuning defaults."
        );
    }
    ImGui::BeginDisabled(!graphics.enhanced_graphics);
    ImGui::PushItemWidth(dp(190));

    if (ImGui::CollapsingHeader("Image quality", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* texture_filters[] =
            {settings::name(renderer::TextureFilter::Normal),
             settings::name(renderer::TextureFilter::EdgeAware)};
        int texture_filter = static_cast<int>(graphics.texture_filter);
        if (combo(
                "Texture upscaling",
                &texture_filter,
                texture_filters,
                static_cast<int>(renderer::TextureFilter::Count)
            )) {
            graphics.texture_filter = static_cast<renderer::TextureFilter>(texture_filter);
            changed = true;
        }
        ImGui::BeginDisabled(!enhanced::Fsr3::available());
        int temporal_quality = static_cast<int>(graphics.temporal_quality);
        if (combo(
                "FSR temporal upscaling",
                &temporal_quality,
                renderer::kTemporalNames,
                static_cast<int>(renderer::TemporalQuality::Count)
            )) {
            graphics.temporal_quality = static_cast<renderer::TemporalQuality>(temporal_quality);
            changed = true;
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enhanced::Fsr3::available());
        changed |= slider_float(
            "FSR sharpening",
            &graphics.temporal_sharpness,
            0,
            1,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::EndDisabled();
        if (ImGui::Button("Reset image quality")) {
            const settings::Settings defaults;
            graphics.texture_filter = defaults.texture_filter;
            graphics.temporal_quality = defaults.temporal_quality;
            graphics.temporal_sharpness = defaults.temporal_sharpness;
            changed = true;
        }
    }
    if (ImGui::CollapsingHeader("Lighting & shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= checkbox("Modern shadows", &graphics.enhanced_shadows);
        ImGui::BeginDisabled(!graphics.enhanced_shadows);
        int shadow_quality = static_cast<int>(graphics.shadow_quality);
        if (combo(
                "Shadow quality",
                &shadow_quality,
                renderer::kShadowQualityNames,
                static_cast<int>(renderer::ShadowQuality::Count)
            )) {
            graphics.shadow_quality = static_cast<renderer::ShadowQuality>(shadow_quality);
            changed = true;
        }
        ImGui::EndDisabled();
        changed |= checkbox("Ambient occlusion", &graphics.gtao);
        ImGui::BeginDisabled(!graphics.gtao);
        int quality = static_cast<int>(graphics.gtao_settings.quality);
        if (combo("Occlusion quality", &quality, "Performance\0Quality\0")) {
            graphics.gtao_settings.quality = static_cast<renderer::GtaoQuality>(quality);
            changed = true;
        }
        ImGui::EndDisabled();
        if (ImGui::TreeNode("Advanced##occlusion")) {
            ImGui::BeginDisabled(!graphics.gtao);
            gtao_controls();
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
        changed |= slider_float(
            "Bounce light",
            &graphics.probe_gi_multiplier,
            0,
            5,
            "%.2fx",
            ImGuiSliderFlags_AlwaysClamp
        );
        changed |= checkbox("Gameplay rim light", &graphics.rim_light);
        if (ImGui::TreeNode("Advanced##rim")) {
            ImGui::BeginDisabled(!graphics.rim_light);
            changed |= slider_float(
                "Rim strength",
                &graphics.rim_light_strength,
                0,
                1,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            changed |= slider_float(
                "Rim falloff",
                &graphics.rim_light_falloff,
                1,
                8,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            changed |= checkbox("Rim shadow lift", &graphics.rim_shadow_lift);
            ImGui::BeginDisabled(!graphics.rim_shadow_lift);
            changed |= slider_float(
                "Lift exponent",
                &graphics.rim_lift_exponent,
                0.01f,
                1,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            changed |= slider_float(
                "Maximum lift gain",
                &graphics.rim_lift_max_gain,
                1,
                16,
                "%.2fx",
                ImGuiSliderFlags_AlwaysClamp
            );
            ImGui::EndDisabled();
            control_tip(
                "Higher falloff narrows the edge. Lower lift exponents brighten dim surroundings, "
                "preserving the local bounced-light colour. Exponent 1 leaves brightness unchanged."
            );
            ImGui::EndDisabled();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Main light direction")) {
            changed |= checkbox("Override light direction", &graphics.sun_direction_override);
            ImGui::BeginDisabled(!graphics.sun_direction_override);
            changed |= slider_float(
                "Azimuth",
                &graphics.sun_azimuth,
                -180,
                180,
                "%.1f degrees",
                ImGuiSliderFlags_AlwaysClamp
            );
            changed |= slider_float(
                "Elevation",
                &graphics.sun_elevation,
                -90,
                90,
                "%.1f degrees",
                ImGuiSliderFlags_AlwaysClamp
            );
            ImGui::EndDisabled();
            if (ImGui::Button("Restore level light direction")) {
                graphics.sun_direction_override = false;
                graphics.sun_azimuth = 0;
                graphics.sun_elevation = 45;
                changed = true;
            }
            control_tip(
                "Updates direct lighting and shadows. Baked bounce lighting keeps its baked "
                "direction."
            );
            ImGui::TreePop();
        }
        if (ImGui::Button("Reset lighting and shadows")) {
            const settings::Settings defaults;
            graphics.enhanced_shadows = defaults.enhanced_shadows;
            graphics.shadow_quality = defaults.shadow_quality;
            graphics.gtao = defaults.gtao;
            graphics.gtao_settings = defaults.gtao_settings;
            graphics.probe_gi_multiplier = defaults.probe_gi_multiplier;
            graphics.rim_light = defaults.rim_light;
            graphics.rim_light_strength = defaults.rim_light_strength;
            graphics.rim_light_falloff = defaults.rim_light_falloff;
            graphics.rim_shadow_lift = defaults.rim_shadow_lift;
            graphics.rim_lift_exponent = defaults.rim_lift_exponent;
            graphics.rim_lift_max_gain = defaults.rim_lift_max_gain;
            graphics.sun_direction_override = defaults.sun_direction_override;
            graphics.sun_azimuth = defaults.sun_azimuth;
            graphics.sun_elevation = defaults.sun_elevation;
            changed = true;
        }
    }
    if (ImGui::CollapsingHeader("Post processing")) {
        ImGui::SeparatorText("AgX");
        changed |= checkbox("AgX", &graphics.agx_preview);
        control_tip(
            "Adjusts scene colour and contrast. "
            "HUD colours are unchanged."
        );
        ImGui::BeginDisabled(!graphics.agx_preview);
        changed |= slider_float(
            "Exposure (EV)", &graphics.agx_exposure, -4, 4, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        changed |= slider_float(
            "Brightness", &graphics.agx_brightness, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::EndDisabled();
        if (ImGui::Button("Reset colour treatment")) {
            const settings::Settings defaults;
            graphics.agx_preview = defaults.agx_preview;
            graphics.agx_exposure = defaults.agx_exposure;
            graphics.agx_brightness = defaults.agx_brightness;
            changed = true;
        }
        changed |= motion_blur_controls(graphics);
    }
    if (ImGui::CollapsingHeader("Materials & water")) {
        if (ImGui::TreeNode("Reflection materials")) {
            ImGui::TextDisabled("Session only; changes are not saved.");
            reflection_material_controls("Water", enhanced::ReflectionSurface::Water);
            reflection_material_controls("Chrome", enhanced::ReflectionSurface::Chrome);
            reflection_material_controls("Glass", enhanced::ReflectionSurface::Glass);
            reflection_material_controls("Stylised", enhanced::ReflectionSurface::Stylised);
            reflection_material_controls("Clank", enhanced::ReflectionSurface::Clank);
            reflection_material_controls("Wrench", enhanced::ReflectionSurface::Wrench);
            reflection_override_controls();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Ratchet wetness")) {
            ImGui::TextDisabled("Session only; changes are not saved.");
            auto material = enhanced::wetness_material_settings();
            bool material_changed = false;
            material_changed |= slider_float(
                "Wet diffuse brightness",
                &material.diffuse_scale,
                0,
                1,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            material_changed |= slider_float(
                "Wet specular strength",
                &material.specular_strength,
                0,
                4,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            material_changed |= slider_float(
                "Dry roughness",
                &material.dry_roughness,
                .08f,
                1,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            material_changed |= slider_float(
                "Wet roughness",
                &material.wet_roughness,
                .08f,
                1,
                "%.2f",
                ImGuiSliderFlags_AlwaysClamp
            );
            material_changed |= slider_float(
                "Wet reflectivity (F0)",
                &material.reflectivity,
                0,
                .16f,
                "%.4f",
                ImGuiSliderFlags_AlwaysClamp
            );
            if (ImGui::Button("Reset wetness material")) {
                material = enhanced::WetnessMaterialSettings{};
                material_changed = true;
            }
            if (material_changed) {
                enhanced::set_wetness_material_settings(material);
            }
            control_tip(
                "Session only. Lower roughness gives sharper reflections. Dry roughness "
                "sets the coat as it fades; fully dry Ratchet keeps his original material."
            );
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Water simulation")) {
            water_controls();
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Diagnostics")) {
        bool wetness_view = enhanced::wetness_debug_view();
        if (checkbox("Ratchet wetness view (GPU)", &wetness_view)) {
            enhanced::set_wetness_debug_view(wetness_view);
        }
        if (wetness_view) {
            ImGui::TextDisabled("Blue: wet. Grey: dry. Session only; dries in 20 seconds.");
        }
        changed |= checkbox("Probe spheres", &graphics.probe_spheres);
        changed |= checkbox("Ceiling area lights", &graphics.ceiling_area_lights);
        changed |= checkbox("Medial axis", &graphics.medial_axis_debug);
        if (graphics.medial_axis_debug) {
            control_tip("Green: centre paths. Pink: junctions. White: component centres.");
        }
        changed |= checkbox("Environment probes", &graphics.environment_probe_debug);
        if (graphics.environment_probe_debug) {
            ImGui::TextDisabled("Mirror spheres: captured reflections. Cyan boxes: bounds.");
            ImGui::TextDisabled("All loaded probes; hidden edges remain faintly visible.");
        }
        if (graphics.ceiling_area_lights) {
            ImGui::TextDisabled("Source-coloured frames; cyan arrows point outward.");
            ImGui::TextDisabled("Hidden edges are faintly visible through geometry.");
        }
        if (graphics.probe_spheres) {
            ImGui::TextDisabled("Cyan: sparse   Green: dense");
            ImGui::TextDisabled("Gold: relocated   Magenta: invalid");
            ImGui::TextDisabled("Boxes at player: yellow dense, cyan sparse");
        }
        if (ImGui::TreeNode("Advanced##probe_bias")) {
            changed |= slider_float(
                "Probe normal bias",
                &graphics.probe_normal_bias,
                0,
                2,
                "%.2f units",
                ImGuiSliderFlags_AlwaysClamp
            );
            changed |= slider_float(
                "Probe view bias",
                &graphics.probe_view_bias,
                0,
                2,
                "%.2f units",
                ImGuiSliderFlags_AlwaysClamp
            );
            ImGui::TreePop();
        }
        if (ImGui::Button("Reset diagnostics")) {
            const settings::Settings defaults;
            graphics.probe_spheres = defaults.probe_spheres;
            graphics.ceiling_area_lights = defaults.ceiling_area_lights;
            graphics.medial_axis_debug = defaults.medial_axis_debug;
            graphics.environment_probe_debug = defaults.environment_probe_debug;
            graphics.probe_normal_bias = defaults.probe_normal_bias;
            graphics.probe_view_bias = defaults.probe_view_bias;
            enhanced::set_wetness_debug_view(false);
            changed = true;
        }
    }
    if (ImGui::CollapsingHeader("Experimental")) {
        ImGui::TextDisabled("Work in progress. Disabled by default.");
        if (ImGui::TreeNode("Atmosphere and fog")) {
            ImGui::TextDisabled("Quality is saved; other controls are session only.");
            auto atmosphere = enhanced::atmosphere_settings();
            int quality = static_cast<int>(graphics.volumetric_quality);
            if (combo("Volumetrics", &quality, "Off\0Low\0Medium\0High\0")) {
                graphics.volumetric_quality = static_cast<renderer::VolumetricQuality>(quality);
                changed = true;
            }
            control_tip(
                "Higher quality uses a denser volume grid. Fog thickness stays the same. "
                "Shared haze and fog affect surfaces, water and particles; authored skies stay "
                "intact."
            );
            ImGui::BeginDisabled(graphics.volumetric_quality == renderer::VolumetricQuality::Off);
            bool edited = false;
            edited |= slider_float("Authored haze", &atmosphere.haze, 0, 4);
            edited |= slider_float("Fog range", &atmosphere.distance, 10, 2000, "%.0f units");
            edited |=
                slider_float("Height fog density", &atmosphere.height_density, 0, .1f, "%.4f");
            edited |=
                slider_float("Fog base height", &atmosphere.base_height, -100, 200, "%.1f units");
            edited |= slider_float("Height falloff", &atmosphere.height_falloff, 0, 1, "%.3f");
            edited |= slider_float("Scattered sunlight", &atmosphere.sun_strength, 0, 1);
            edited |= slider_float("Scattering directionality", &atmosphere.anisotropy, -.8f, .8f);
            if (ImGui::TreeNode("Local fog volumes")) {
                for (std::size_t i = 0; i < atmosphere.volumes.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::TreeNode("volume", "Volume %d", static_cast<int>(i + 1))) {
                        auto& volume = atmosphere.volumes[i];
                        edited |= input_float3("Centre", volume.centre_radius.data());
                        edited |= slider_float("Radius", &volume.centre_radius[3], .1f, 100);
                        edited |= colour_edit("Fog colour", volume.colour_density.data());
                        edited |=
                            slider_float("Density", &volume.colour_density[3], 0, .1f, "%.4f");
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::EndDisabled();
            if (ImGui::Button("Reset atmosphere")) {
                atmosphere = enhanced::AtmosphereSettings{};
                graphics.volumetric_quality = settings::Settings{}.volumetric_quality;
                changed = true;
                edited = true;
            }
            if (edited) {
                enhanced::set_atmosphere_settings(atmosphere);
            }
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Misc")) {
        changed |= slider_float(
            "Lily momentum", &graphics.lily_momentum, 0, 1, "%.2f s", ImGuiSliderFlags_AlwaysClamp
        );
        control_tip(
            "Higher values make lilies ease more slowly toward the waves. Zero follows waves "
            "immediately. Requires enhanced water."
        );
        if (ImGui::Button("Reset lily momentum")) {
            graphics.lily_momentum = settings::Settings{}.lily_momentum;
            changed = true;
        }
    }
    ImGui::PopItemWidth();
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (changed) {
        settings::save();
        s.settings_changed = true;
    }
    ImGui::End();
}

}  // namespace openrac::frontend::debug_overlay
