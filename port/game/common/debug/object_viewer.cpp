// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Isolated object inspection, pose controls and optional enhanced graphics authoring.
#include <algorithm>
#include <cmath>

#include "debug/overlay.h"
#include "renderer/gl.h"

namespace openrac::frontend::debug_overlay {
namespace {
// Follow the exact displayed pose; no writes to the selected moby's animation fields.
void follow_pose(const Shown& shown) {
    auto& p = s.object_viewer;

    if (p.pose_mode != 1 || p.pose_tick == shown.picker_tick || !p.selection.hit
        || !shown.picker_level) {
        return;
    }

    const auto* instance =
        scene::preview_instance(*shown.picker_level, *p.selection.hit, shown.picker_live);

    if (!instance) {
        p.pose_mode = 0;
        p.status = "Object disappeared; its last pose is retained.";
        return;
    }

    const auto result = p.renderer.pose(*instance, shown.picker_palette);

    if (!result) {
        p.error = result.error().message;
        p.pose_mode = 0;
        return;
    }

    p.pose_tick = shown.picker_tick;
}

// Standard pose controls never change the selected actor in the running game.
void pose_controls(const Shown& shown) {
    auto& p = s.object_viewer;

    if (object_viewer_combo(
            "Pose", &p.pose_mode, "Frozen pose\0Follow game\0Default / bind pose\0", "pose"
        )) {
        // Re-entering Follow game must refresh even if the displayed game tick did not advance.
        p.pose_tick = ~std::uint64_t{0};

        if (p.pose_mode == 2) {
            const auto result = p.renderer.bind_pose();

            if (!result) {
                p.error = result.error().message;
                p.pose_mode = 0;
            }
        }
    }

    ImGui::Checkbox("Show bones", &p.show_bones);
    remember_object_control("show_bones");
    ImGui::SameLine();

    if (ImGui::Button("Frame object (F)")) {
        p.orbit = {};
    }

    follow_pose(shown);
    p.frozen = p.pose_mode != 1;
}

// A scrolling property pane never pushes the independently navigable image out of view.
void viewer_panels(const Shown& shown) {
    auto& p = s.object_viewer;
    const float height = std::max(dp(230), ImGui::GetContentRegionAvail().y - dp(45));

    if (ImGui::BeginTable(
            "Object inspector", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
        )) {
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, .65f);
        ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch, .35f);
        ImGui::TableNextColumn();
        ImGui::BeginChild(
            "Object picture",
            {0, height},
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );
        object_viewer_picture(shown);
        ImGui::EndChild();
        ImGui::TableNextColumn();
        ImGui::BeginChild("Object properties", {0, height});
        const auto& mesh = p.renderer.mesh();
        ImGui::Text("%zu vertices | %zu materials", mesh.vertices.size(), mesh.parts.size());
        ImGui::Text("%zu bones | radius %.3f", mesh.bones.size(), mesh.radius);
#ifdef OPENRAC_LIGHT_AUTHORING
        object_light_controls(shown);
#endif
        ImGui::EndChild();
        ImGui::EndTable();
    }
}

// General controls and optional properties share only the viewer's owned selection.
void viewer_contents(const Shown& shown) {
    auto& p = s.object_viewer;
    p.controls.clear();
    ImGui::BeginDisabled(!s.picker_result.hit);

    if (object_viewer_button("Inspect selected object", "inspect")) {
        select_object_viewer(shown);
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button(p.selected ? "Pick another object" : "Enable click picker")) {
        set_picker_enabled(true);

        if (p.selected) {
            ImGui::SetWindowCollapsed("Object Viewer", true);
        }
    }

    if (p.selected && p.selection.hit) {
        ImGui::TextUnformatted(p.selection.hit->name.c_str());
        pose_controls(shown);
        viewer_panels(shown);
    } else {
        ImGui::TextWrapped("Pick an object in the game, then inspect it here.");
    }

    if (!p.error.empty()) {
        ImGui::TextWrapped("%s", p.error.c_str());
    }

    if (!p.light_error.empty()) {
        ImGui::TextWrapped("%s", p.light_error.c_str());
    }

    if (!p.draw_error.empty()) {
        ImGui::TextWrapped("%s", p.draw_error.c_str());
    }

    if (!p.status.empty()) {
        ImGui::TextWrapped("%s", p.status.c_str());
    }
}
}  // namespace

bool picker_snapshot_needed() {
    return picker_enabled() || s.object_viewer.snapshot_needed.load();
}

namespace {
// Geometry, materials and the displayed pose are copied into a private renderer.
bool upload_selection(
    const Shown& shown, const scene::LevelData& level, const scene::PickHit& hit
) {
    auto& p = s.object_viewer;
    const auto* instance = scene::preview_instance(level, hit, shown.picker_live);

    if (!instance) {
        p.error = "The selected object disappeared. Pick another object.";
        return false;
    }

    auto mesh = scene::preview_mesh(level, hit, *instance, shown.picker_palette);

    if (!mesh) {
        p.error = mesh.error().message;
        return false;
    }

#ifdef OPENRAC_LIGHT_AUTHORING
    std::unique_ptr<enhanced::ObjectPreviewLighting> lighting;

    if (gl::enhanced_rendering_available()) {
        lighting = std::make_unique<enhanced::ObjectPreviewLighting>();
    }

    p.lighting = lighting.get();
    auto result = p.renderer.select(std::move(mesh.value()), std::move(lighting));
#else
    auto result = p.renderer.select(std::move(mesh.value()));
#endif

    if (!result) {
        p.error = result.error().message;
#ifdef OPENRAC_LIGHT_AUTHORING
        p.lighting = nullptr;
#endif
        p.selected = false;
        p.snapshot_needed = false;
        return false;
    }

    return true;
}

#ifdef OPENRAC_LIGHT_AUTHORING
// Read enhancements only for an enhanced viewer; classic inspection has no profile dependency.
void load_selection_lights(const scene::LevelData& level, const scene::PickHit& hit) {
    auto& p = s.object_viewer;
    if (!p.lighting) {
        p.lights.clear();
        return;
    }
    p.game = level.game;
    p.key = enhanced::object_light_key(level, hit.layer, hit.model);
    p.profile_path = enhanced::object_light_author_path(level);
    auto catalog = enhanced::load_object_lights(level);

    if (!catalog) {
        p.error = catalog.error().message;
        p.catalog = {};
    } else {
        p.catalog = std::move(catalog.value());
    }

    p.lights = p.catalog.objects[p.key];

    if (const auto drafts = p.drafts.find(p.profile_path); drafts != p.drafts.end()) {
        if (const auto draft = drafts->second.objects.find(p.key);
            draft != drafts->second.objects.end()) {
            p.lights = draft->second;
        }
    }

    p.selected_light = p.lights.empty() ? -1 : 0;
    p.preview_enhanced = !p.lights.empty();
    p.drag_axis = -1;
}
#endif
}  // namespace

// Select the picked object without changing gameplay or retaining borrowed render data.
void select_object_viewer(const Shown& shown) {
    auto& p = s.object_viewer;

    if (!s.developer || !shown.picker_level || !s.picker_result.hit) {
        return;
    }

    const auto& level = *shown.picker_level;
    const auto& hit = *s.picker_result.hit;

    if (!upload_selection(shown, level, hit)) {
        return;
    }

    p.open = true;
    p.selected = true;
    p.frame_requested = true;
    p.pose_mode = 0;
    p.frozen = true;
    p.error.clear();
    p.draw_error.clear();
    p.status.clear();
    p.orbit = {};
    p.snapshot_needed = true;
    p.selection = s.picker_result;
    p.pose_tick = shown.picker_tick;
    p.level_models = level.models.data();
#ifdef OPENRAC_LIGHT_AUTHORING
    load_selection_lights(level, hit);
#endif
    s.inspector = false;
    ImGui::SetWindowFocus("Object Viewer");
#ifdef OPENRAC_LIGHT_AUTHORING
    update_object_preview_lights();
#endif
    const auto result = p.renderer.draw(512, 360, p.orbit);

    if (!result) {
        p.draw_error = result.error().message;
    }

    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
    gl::glViewport(0, 0, shown.window_width, shown.window_height);
    write_object_viewer_report();
}

void release_object_viewer() {
    auto& p = s.object_viewer;
    p.snapshot_needed = false;
#ifdef OPENRAC_LIGHT_AUTHORING
    p.lighting = nullptr;
#endif
    p.renderer.release();
    p.selected = false;
    p.open = false;
    p.level_models = nullptr;

    if (p.report_at >= 0 && ImGui::GetCurrentContext()) {
        write_object_viewer_report();
    }

    p.report_at = -1;
}

void object_viewer_window(const Shown& shown) {
    auto& p = s.object_viewer;

    if (!p.open || !s.developer) {
        if (p.selected) {
            release_object_viewer();
        }

        return;
    }

    if (p.selected
        && (!shown.picker_level || shown.picker_level_number != p.selection.level
            || shown.picker_level->models.data() != p.level_models)) {
        p.snapshot_needed = false;
#ifdef OPENRAC_LIGHT_AUTHORING
        p.lighting = nullptr;
#endif
        p.renderer.release();
        p.selected = false;
        p.error = "Level changed. Pick an object from the new level.";
    }

    const auto available = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowSizeConstraints(
        {dp(400), dp(100)},
        {std::max(dp(400), available.x - dp(20)), std::max(dp(100), available.y - dp(30))}
    );
    ImGui::SetNextWindowSize(
        {dp(1000), dp(830)}, p.frame_requested ? ImGuiCond_Always : ImGuiCond_FirstUseEver
    );

    if (p.frame_requested) {
        ImGui::SetNextWindowPos(
            {std::max(dp(10), (available.x - dp(1000)) * .5f), dp(35)}, ImGuiCond_Always
        );
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowPos({dp(540), dp(35)}, ImGuiCond_FirstUseEver);
    }

    p.frame_requested = false;

    if (ImGui::Begin(
            "Object Viewer",
            &p.open,
            p.selected ? ImGuiWindowFlags_None : ImGuiWindowFlags_AlwaysAutoResize
        )
        && p.open) {
        viewer_contents(shown);
    }

    ImGui::End();

    if (!p.open) {
        release_object_viewer();
    } else if (ImGui::GetTime() - p.report_at > .5) {
        write_object_viewer_report();
    }
}
}  // namespace openrac::frontend::debug_overlay
