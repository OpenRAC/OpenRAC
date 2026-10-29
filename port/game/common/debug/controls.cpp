// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's settings controls (debug/overlay.h), which the drawer and the menu bar share:
// a setting moved on, and the enhanced graphics' switches and sliders with GTAO's.

#include <imgui.h>

#include "debug/overlay.h"
#include "enhanced/fsr3/fsr3.h"
#include "enhanced/renderer/water.h"
#include "enhanced/renderer/wetness.h"
#include "frontend.h"
#include "options_menu.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {

namespace {

// Persistent blur tuning lives in its own section, separate from lighting diagnostics.
bool motion_blur_controls(settings::Settings& graphics) {
    if (!ImGui::CollapsingHeader("Motion Blur", ImGuiTreeNodeFlags_DefaultOpen)) {
        return false;
    }
    ImGui::BeginDisabled(!enhanced::Fsr3::available());
    bool changed = ImGui::Checkbox("Enabled##motion_blur", &graphics.motion_blur);
    ImGui::TextWrapped("Camera-target-relative during play; normal camera blur in cutscenes.");
    ImGui::BeginDisabled(!graphics.motion_blur);
    ImGui::PushItemWidth(dp(190.f));
    auto& blur = graphics.motion_blur_settings;
    const auto tip = [](const char* text) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", text);
        }
    };
    changed |= ImGui::SliderFloat(
        "Strength", &graphics.motion_blur_strength, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    tip("Exposure strength. 0.5 is a 1/120-second exposure; 1 is 1/60 second.");
    changed |= ImGui::SliderFloat(
        "Cutscene multiplier",
        &blur.cutscene_multiplier,
        0,
        2,
        "%.2fx",
        ImGuiSliderFlags_AlwaysClamp
    );
    tip("Scales strength only in real-time cutscenes. Zero disables cutscene blur.");
    changed |= ImGui::SliderInt(
        "Max trail length", &blur.max_radius, 1, 128, "%d px", ImGuiSliderFlags_AlwaysClamp
    );
    tip("Caps trails in display pixels. Longer limits can require more jump-flood passes.");
    changed |=
        ImGui::SliderInt("Samples", &blur.samples, 8, 64, "%d", ImGuiSliderFlags_AlwaysClamp);
    tip("Higher counts make trails smoother, at a higher GPU cost.");
    changed |= ImGui::SliderFloat(
        "Minimum motion", &blur.min_motion, .1f, 4, "%.2f px", ImGuiSliderFlags_AlwaysClamp
    );
    tip("Ignores tiny movements after exposure scaling. Raise it to suppress subtle smearing.");
    changed |= ImGui::SliderFloat(
        "Trail spread", &blur.spread, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    tip("Sideways reach of jump-flood candidates. Higher values find more nearby motion but may "
        "introduce halos.");
    changed |= ImGui::SliderFloat(
        "Depth tolerance", &blur.depth_tolerance, .001f, .05f, "%.3f", ImGuiSliderFlags_AlwaysClamp
    );
    tip("Allowed relative depth difference when gathering colour. Lower values protect occlusion "
        "edges; higher values blend more freely.");
    ImGui::PopItemWidth();
    ImGui::EndDisabled();
    if (ImGui::Button("Reset blur tuning")) {
        graphics.motion_blur_strength = .5f;
        blur = renderer::MotionBlurSettings{};
        changed = true;
    }
    tip("Restores tuning defaults and keeps the Enabled switch unchanged.");
    ImGui::EndDisabled();
    return changed;
}

// Live, session-only water controls; the render worker consumes a locked copy.
void water_controls() {
    auto options = enhanced::water_debug_settings();
    ImGui::TextDisabled("Session only; changes are not saved.");
    ImGui::BeginDisabled(!enhanced::water_available());
    bool changed = ImGui::Checkbox("Pause simulation", &options.paused);
    ImGui::PushItemWidth(dp(170.f));
    changed |= ImGui::SliderFloat(
        "Current speed", &options.current_scale, 0, 2.5f, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Changing current speed resets the water. The pond has no base current.");
    }
    changed |= ImGui::SliderFloat(
        "Friction", &options.friction, 0, 4, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
        "Outlet damping", &options.outlet_damping, 0, 30, "%.1f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
        "Ratchet strength",
        &options.interaction_strength,
        0,
        4,
        "%.2fx",
        ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
        "Ratchet radius",
        &options.interaction_radius,
        .1f,
        2,
        "%.2f units",
        ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
        "Displacement limit",
        &options.displacement_limit,
        0,
        .5f,
        "%.2f units",
        ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
        "Fine ripples", &options.fine_normal_strength, 0, 2, "%.2fx", ImGuiSliderFlags_AlwaysClamp
    );
    changed |=
        ImGui::Combo("Water view", &options.view, "Lit\0Height\0Velocity\0Banks\0Flow detail\0");
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
    changed |= ImGui::Checkbox("Half resolution", &options.half_resolution);
    changed |=
        ImGui::SliderInt("Directions", &options.slices, 1, 12, "%d", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderInt(
        "Steps per side", &options.steps, 4, 24, "%d", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
        "Radius", &options.radius, 0.05f, 8.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::SliderFloat(
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
    bool changed = ImGui::Checkbox("Enhanced graphics (F4)", &graphics.enhanced_graphics);
    if (ImGui::Button("Reset to Defaults##enhanced")) {
        graphics.reset_enhanced();
        enhanced::set_wetness_material_settings({});
        enhanced::set_wetness_debug_view(false);
        enhanced::set_water_debug_settings({});
        enhanced::reset_water_simulation();
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Restore enhanced graphics, post processing and session tuning defaults.");
    }
    ImGui::BeginDisabled(!graphics.enhanced_graphics);
    ImGui::PushItemWidth(dp(190));

    // Same main choices and order as the launcher's Enhanced graphics page.
    changed |= ImGui::Checkbox("Gameplay rim light", &graphics.rim_light);
    changed |= ImGui::Checkbox("Modern shadows", &graphics.enhanced_shadows);
    ImGui::BeginDisabled(!graphics.enhanced_shadows);
    int shadow_quality = static_cast<int>(graphics.shadow_quality);
    if (ImGui::Combo(
            "Shadow quality",
            &shadow_quality,
            renderer::kShadowQualityNames,
            static_cast<int>(renderer::ShadowQuality::Count)
        )) {
        graphics.shadow_quality = static_cast<renderer::ShadowQuality>(shadow_quality);
        changed = true;
    }
    ImGui::EndDisabled();
    changed |= ImGui::Checkbox("Ambient occlusion", &graphics.gtao);
    ImGui::BeginDisabled(!graphics.gtao);
    int quality = static_cast<int>(graphics.gtao_settings.quality);
    if (ImGui::Combo("Occlusion quality", &quality, "Performance\0Quality\0")) {
        graphics.gtao_settings.quality = static_cast<renderer::GtaoQuality>(quality);
        changed = true;
    }
    ImGui::EndDisabled();
    const char* texture_filters[] =
        {settings::name(renderer::TextureFilter::Normal),
         settings::name(renderer::TextureFilter::EdgeAware)};
    int texture_filter = static_cast<int>(graphics.texture_filter);
    if (ImGui::Combo(
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
    if (ImGui::Combo(
            "FSR temporal upscaling",
            &temporal_quality,
            renderer::kTemporalNames,
            static_cast<int>(renderer::TemporalQuality::Count)
        )) {
        graphics.temporal_quality = static_cast<renderer::TemporalQuality>(temporal_quality);
        changed = true;
    }
    ImGui::EndDisabled();

    if (ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::SeparatorText("AgX");
        changed |= ImGui::Checkbox("AgX", &graphics.agx_preview);
        ImGui::TextWrapped(
            "Colour treatment only: existing SDR buffers have already clipped highlights. "
            "HUD colours are unchanged."
        );
        ImGui::BeginDisabled(!graphics.agx_preview);
        changed |= ImGui::SliderFloat(
            "Exposure (EV)", &graphics.agx_exposure, -4, 4, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        changed |= ImGui::SliderFloat(
            "Brightness", &graphics.agx_brightness, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::EndDisabled();
        changed |= motion_blur_controls(graphics);
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::BeginDisabled(!enhanced::Fsr3::available());
        changed |= ImGui::SliderFloat(
            "FSR sharpening",
            &graphics.temporal_sharpness,
            0,
            1,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::EndDisabled();
        changed |= ImGui::SliderFloat(
            "Bounce light",
            &graphics.probe_gi_multiplier,
            0,
            5,
            "%.2fx",
            ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::SeparatorText("Gameplay rim light");
        ImGui::BeginDisabled(!graphics.rim_light);
        changed |= ImGui::SliderFloat(
            "Rim strength", &graphics.rim_light_strength, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        changed |= ImGui::SliderFloat(
            "Rim falloff", &graphics.rim_light_falloff, 1, 8, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        changed |= ImGui::Checkbox("Rim shadow lift", &graphics.rim_shadow_lift);
        ImGui::BeginDisabled(!graphics.rim_shadow_lift);
        changed |= ImGui::SliderFloat(
            "Lift exponent",
            &graphics.rim_lift_exponent,
            0.01f,
            1,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        changed |= ImGui::SliderFloat(
            "Maximum lift gain",
            &graphics.rim_lift_max_gain,
            1,
            16,
            "%.2fx",
            ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::EndDisabled();
        ImGui::TextWrapped(
            "Higher falloff narrows the edge. Lower lift exponents brighten dim surroundings, "
            "preserving the local bounced-light colour. Exponent 1 leaves brightness unchanged."
        );
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!graphics.gtao);
        gtao_controls();
        ImGui::EndDisabled();
        ImGui::SeparatorText("Main directional light");
        changed |= ImGui::Checkbox("Override light direction", &graphics.sun_direction_override);
        ImGui::BeginDisabled(!graphics.sun_direction_override);
        changed |= ImGui::SliderFloat(
            "Azimuth",
            &graphics.sun_azimuth,
            -180,
            180,
            "%.1f degrees",
            ImGuiSliderFlags_AlwaysClamp
        );
        changed |= ImGui::SliderFloat(
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
        ImGui::TextWrapped(
            "Updates direct lighting and shadows. Baked bounce lighting keeps its baked direction."
        );
    }
    if (ImGui::CollapsingHeader("Ratchet wetness material")) {
        auto material = enhanced::wetness_material_settings();
        bool material_changed = false;
        material_changed |= ImGui::SliderFloat(
            "Wet diffuse brightness",
            &material.diffuse_scale,
            0,
            1,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        material_changed |= ImGui::SliderFloat(
            "Wet specular strength",
            &material.specular_strength,
            0,
            4,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        material_changed |= ImGui::SliderFloat(
            "Dry roughness", &material.dry_roughness, .08f, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        material_changed |= ImGui::SliderFloat(
            "Wet roughness", &material.wet_roughness, .08f, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp
        );
        material_changed |= ImGui::SliderFloat(
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
        ImGui::TextWrapped(
            "Session only. Lower roughness gives sharper reflections. Dry roughness "
            "sets the coat as it fades; fully dry Ratchet keeps his original material."
        );
    }
    if (ImGui::CollapsingHeader("Diagnostics")) {
        bool wetness_view = enhanced::wetness_debug_view();
        if (ImGui::Checkbox("Ratchet wetness view (GPU)", &wetness_view)) {
            enhanced::set_wetness_debug_view(wetness_view);
        }
        if (wetness_view) {
            ImGui::TextDisabled("Blue: wet. Grey: dry. Session only; dries in 20 seconds.");
        }
        changed |= ImGui::Checkbox("Probe spheres", &graphics.probe_spheres);
        changed |= ImGui::Checkbox("Ceiling area lights", &graphics.ceiling_area_lights);
        if (graphics.ceiling_area_lights) {
            ImGui::TextDisabled("Source-coloured frames; cyan arrows point outward.");
            ImGui::TextDisabled("Hidden edges are faintly visible through geometry.");
        }
        if (graphics.probe_spheres) {
            ImGui::TextDisabled("Cyan: sparse   Green: dense");
            ImGui::TextDisabled("Gold: relocated   Magenta: invalid");
            ImGui::TextDisabled("Boxes at player: yellow dense, cyan sparse");
        }
        changed |= ImGui::SliderFloat(
            "Probe normal bias",
            &graphics.probe_normal_bias,
            0,
            2,
            "%.2f units",
            ImGuiSliderFlags_AlwaysClamp
        );
        changed |= ImGui::SliderFloat(
            "Probe view bias",
            &graphics.probe_view_bias,
            0,
            2,
            "%.2f units",
            ImGuiSliderFlags_AlwaysClamp
        );
    }
    if (ImGui::CollapsingHeader("Water simulation")) {
        water_controls();
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
