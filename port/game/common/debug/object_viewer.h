// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// General object-viewer state; optional enhanced light authoring is kept in its own section.
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <imgui.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

#ifdef OPENRAC_LIGHT_AUTHORING
#include "enhanced/lighting/object_lights.h"
#include "enhanced/viewer/object_preview_lighting.h"
#endif
#include "scene/object_preview.h"

namespace openrac::frontend::debug_overlay {
struct Shown;

// Window-owned viewer state; only snapshot_needed is read by the render worker.
struct ObjectViewerState {
    scene::ObjectPreview renderer;
    scene::PreviewOrbit orbit;
    scene::PickResult selection;
    const scene::Model* level_models = nullptr;  // Identity only, never dereferenced after capture.
    bool open = false, selected = false, frozen = true, show_bones = false, frame_requested = false;
    std::atomic<bool> snapshot_needed{false};
    std::uint64_t pose_tick = ~std::uint64_t{0};
    std::string error, draw_error, light_error, status;
    double report_at = -1;
#ifdef OPENRAC_LIGHT_AUTHORING
    std::string game, key;
    std::filesystem::path profile_path;
    enhanced::ObjectPreviewLighting* lighting = nullptr;  // Owned by renderer, never by the UI.
    enhanced::ObjectLightCatalog catalog;
    std::vector<enhanced::AttachedLight> lights;
    std::map<std::filesystem::path, enhanced::ObjectLightCatalog> drafts;
    int selected_light = -1, gizmo_mode = 0, drag_axis = -1;
    bool preview_enhanced = false, light_gizmos = true, local_axes = true;
    float rotate_angle = 0;
#endif
    int pose_mode = 0;  // Frozen, follow gameplay, or original bind pose.
    ImVec2 image_low{}, image_size{};
    std::map<std::string, std::array<float, 4>, std::less<>> controls;
};

// Native diagnostics can target the same visible controls the user operates.
void remember_object_control(std::string_view key);
bool object_viewer_button(const char* label, const char* key);
bool object_viewer_combo(const char* label, int* value, const char* options, const char* key);

// Shared studio-to-screen projection for bone overlays and optional light handles.
std::optional<ImVec2> object_viewer_project(const renderer::Camera&, renderer::Vec3);
// Skeleton glyphs remain available with classic graphics and without light authoring.
void object_viewer_bones(const renderer::Camera&);
// General image interaction and overlays, including optional enhanced-light gizmos.
void object_viewer_picture(const Shown&);
#ifdef OPENRAC_LIGHT_AUTHORING
// Optional authoring controls, always nested below Enhanced graphics.
void object_light_controls(const Shown&);
// Mark the current object type's edits without losing them when inspecting another object.
void object_lights_changed();
// Recompute all enabled preview lights through the selected object's current/frozen bone frames.
void update_object_preview_lights();
// Gizmos consume left input before camera navigation.
bool object_light_gizmos(const renderer::Camera&, bool interact);
#endif
// Agent-readable viewer, light, pose and projected-handle state.
void write_object_viewer_report();
}  // namespace openrac::frontend::debug_overlay
