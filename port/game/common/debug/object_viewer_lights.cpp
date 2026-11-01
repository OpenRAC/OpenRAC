// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Additive point/spot editing and canonical per-object JSON profiles, isolated below Enhanced graphics.
#include <algorithm>
#include <cstdio>

#include "debug/overlay.h"
#include "renderer/gl.h"

#ifdef OPENRAC_LIGHT_AUTHORING
namespace openrac::frontend::debug_overlay {
namespace {
// Fresh disk data is merged at Save so editing one object never replaces another object's saved lights.
void save_lights(const Shown& shown) {
    auto& p = s.object_viewer;
    auto catalog = enhanced::read_object_lights(p.profile_path);

    if (!catalog) {
        p.error = catalog.error().message;
        return;
    }

    if (const auto drafts = p.drafts.find(p.profile_path); drafts != p.drafts.end()) {
        for (const auto& [key, lights] : drafts->second.objects) {
            catalog.value().objects[key] = lights;
        }
    }

    const auto saved = enhanced::write_object_lights(p.profile_path, catalog.value());

    if (!saved) {
        p.error = saved.error().message;
        return;
    }

    auto merged = enhanced::load_object_lights(*shown.picker_level);

    if (!merged) {
        p.error = merged.error().message;
        return;
    }

    p.catalog = std::move(merged.value());
    enhanced::publish_object_lights(p.game, p.catalog);
    p.drafts.erase(p.profile_path);
    p.error.clear();
    p.status = "Saved object lights. All instances use these additions.";
}

// A useful default is positioned beside the object, then converted back to object-local coordinates.
void add_light(bool spot) {
    auto& p = s.object_viewer;
    enhanced::AttachedLight light;
    light.name = spot ? "Spot light" : "Point light";
    light.light.spot = spot;
    light.light.intensity = 2;
    light.light.range = p.renderer.mesh().radius * 6;
    const auto frame = scene::preview_attachment(p.renderer.mesh(), -1);
    const auto inverse = frame ? renderer::inverse_affine(*frame) : std::nullopt;

    if (!inverse) {
        p.error = "The selected object's transform is singular.";
        return;
    }

    light.light.position = renderer::transform_point(*inverse, {.8f, -.7f, .8f});
    light.light.direction =
        renderer::normalize(renderer::transform_vector(*inverse, {-.8f, .7f, -.8f}));
    p.lights.push_back(light);
    p.selected_light = int(p.lights.size() - 1);
    p.preview_enhanced = true;
    object_lights_changed();
}

// Changing parent preserves the visible light placement instead of snapping it to an unrelated bone origin.
void attach_bone(enhanced::AttachedLight& light, int joint) {
    auto& p = s.object_viewer;
    const auto& mesh = p.renderer.mesh();
    const int old = enhanced::attached_light_joint(light, mesh.bones);
    const auto before = scene::preview_attachment(mesh, old),
               after = scene::preview_attachment(mesh, joint);
    const auto inverse = after ? renderer::inverse_affine(*after) : std::nullopt;

    if (!before || !inverse) {
        p.error = "The attachment frame is missing or singular.";
        return;
    }

    const auto change = renderer::multiply(*inverse, *before);
    light.light.position = renderer::transform_point(change, light.light.position);
    light.light.direction =
        renderer::normalize(renderer::transform_vector(change, light.light.direction));
    light.bone = joint < 0 ? std::string{} : mesh.bones[std::size_t(joint)].name;
    object_lights_changed();
}

// Bone names are exported identities, rather than a guessed index in an inverse-bind palette.
void bone_control(enhanced::AttachedLight& light) {
    auto& p = s.object_viewer;
    const auto& bones = p.renderer.mesh().bones;
    const auto label = light.bone.empty() ? "Object root" : light.bone.c_str();

    const auto low = ImGui::GetCursorScreenPos();
    p.controls["bone"] = {low.x, low.y, ImGui::CalcItemWidth(), ImGui::GetFrameHeight()};

    if (ImGui::BeginCombo("Attach to", label)) {
        if (ImGui::Selectable("Object root", light.bone.empty())) {
            attach_bone(light, -1);
        }

        for (std::size_t j = 0; j < bones.size(); ++j) {
            if (bones[j].name.empty()) {
                continue;
            }

            if (ImGui::Selectable(bones[j].name.c_str(), light.bone == bones[j].name)) {
                attach_bone(light, int(j));
            }

            remember_object_control("bone:" + std::to_string(j));
        }

        ImGui::EndCombo();
    }
}

// Per-light fields stay in local coordinates; gizmos perform the corresponding inverse-frame conversion.
// Cone edits use degrees in the UI while retaining the shared renderer's radians in JSON.
bool spot_fields(enhanced::LocalLight& light) {
    bool changed = false;

    changed |= ImGui::DragFloat3(
        "Local direction", light.direction.data(), .01f, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp
    );
    constexpr float kDegrees = 57.2957795f;
    float inner = light.inner_angle * kDegrees, outer = light.outer_angle * kDegrees;
    changed |= ImGui::SliderFloat(
        "Inner cone", &inner, 0, outer, "%.1f deg", ImGuiSliderFlags_AlwaysClamp
    );
    changed |=
        ImGui::SliderFloat("Outer cone", &outer, .1f, 89, "%.1f deg", ImGuiSliderFlags_AlwaysClamp);
    light.inner_angle = std::min(inner, outer) / kDegrees;
    light.outer_angle = outer / kDegrees;
    return changed;
}

// Shared point fields do not need to know how cone parameters are presented.
void selected_light_controls() {
    auto& p = s.object_viewer;

    if (p.selected_light < 0 || std::size_t(p.selected_light) >= p.lights.size()) {
        return;
    }

    auto& selected = p.lights[std::size_t(p.selected_light)];
    auto& light = selected.light;
    bool changed = false;
    std::array<char, 129> name{};
    std::snprintf(name.data(), name.size(), "%s", selected.name.c_str());

    if (ImGui::InputText("Name", name.data(), name.size())) {
        selected.name = name.data();
        changed = true;
    }

    changed |= ImGui::Checkbox("Enabled", &selected.enabled);
    bone_control(selected);

    if (!selected.bone.empty() && object_viewer_button("Move to bone origin", "bone_origin")) {
        light.position = {};
        changed = true;
    }

    int type = light.spot ? 1 : 0;

    if (object_viewer_combo("Type", &type, "Point\0Spot\0", "light_type")) {
        light.spot = type == 1;
        changed = true;
    }

    changed |= ImGui::ColorEdit3("Colour (linear)", light.colour.data(), ImGuiColorEditFlags_Float);
    changed |= ImGui::SliderFloat(
        "Intensity", &light.intensity, 0, 16, "%.2f", ImGuiSliderFlags_AlwaysClamp
    );
    changed |= ImGui::DragFloat3(
        "Local position", light.position.data(), p.renderer.mesh().radius * .025f, 0, 0, "%.3f"
    );
    changed |= ImGui::DragFloat(
        "Range (game units)", &light.range, .05f, .01f, 10000, "%.3f", ImGuiSliderFlags_AlwaysClamp
    );

    if (light.spot) {
        changed |= spot_fields(light);
    }

    if (changed) {
        object_lights_changed();
    }
}

// Duplicating and deleting affect only the authored additions, never the original game object.
void light_list() {
    auto& p = s.object_viewer;

    if (object_viewer_button("Add point light", "add_point")) {
        add_light(false);
    }

    ImGui::SameLine();

    if (object_viewer_button("Add spot light", "add_spot")) {
        add_light(true);
    }

    if (ImGui::BeginListBox("Lights", {0, dp(65)})) {
        for (std::size_t i = 0; i < p.lights.size(); ++i) {
            ImGui::PushID(int(i));

            if (ImGui::Selectable(p.lights[i].name.c_str(), p.selected_light == int(i))) {
                p.selected_light = int(i);
                p.drag_axis = -1;
            }

            ImGui::PopID();
        }

        ImGui::EndListBox();
    }

    const bool selected = p.selected_light >= 0 && std::size_t(p.selected_light) < p.lights.size();
    ImGui::BeginDisabled(!selected);

    if (object_viewer_button("Duplicate light", "duplicate")) {
        auto copy = p.lights[std::size_t(p.selected_light)];
        copy.name += " copy";
        p.lights.push_back(copy);
        p.selected_light = int(p.lights.size() - 1);
        object_lights_changed();
    }

    ImGui::SameLine();

    if (object_viewer_button("Remove light", "remove")) {
        p.lights.erase(p.lights.begin() + p.selected_light);
        p.selected_light = int(p.lights.size()) - 1;
        p.drag_axis = -1;

        if (p.lights.empty()) {
            p.preview_enhanced = false;
        }

        object_lights_changed();
    }

    ImGui::EndDisabled();
    selected_light_controls();
}
}  // namespace

void object_lights_changed() {
    auto& p = s.object_viewer;
    p.drafts[p.profile_path].objects[p.key] = p.lights;
    p.status = "Unsaved light additions";
}

void update_object_preview_lights() {
    auto& p = s.object_viewer;

    if (!p.lighting) {
        return;
    }

    p.light_error.clear();
    std::vector<enhanced::LocalLight> lights;
    const auto& mesh = p.renderer.mesh();

    for (const auto& attached : p.lights) {
        if (!attached.enabled) {
            continue;
        }

        const int joint = enhanced::attached_light_joint(attached, mesh.bones);
        const auto frame = joint == -2 ? std::nullopt : scene::preview_attachment(mesh, joint);

        if (!frame) {
            p.light_error = "Unknown or invalid attachment bone: " + attached.bone;
            continue;
        }

        auto light = attached.light;
        light.position = renderer::transform_point(*frame, light.position);
        light.direction = renderer::normalize(renderer::transform_vector(*frame, light.direction));
        light.range /= mesh.radius;
        lights.push_back(light);
    }

    p.lighting->enabled = p.preview_enhanced;
    const auto result = p.lighting->set_lights(lights);

    if (!result) {
        p.light_error = result.error().message;
    }
}

namespace {
// Reload only this profile/object's draft; edits to other profiles are retained.
void reload_lights(const Shown& shown) {
    auto& p = s.object_viewer;

    auto catalog = enhanced::load_object_lights(*shown.picker_level);

    if (!catalog) {
        p.error = catalog.error().message;
    } else {
        p.catalog = std::move(catalog.value());
        p.lights = p.catalog.objects[p.key];

        if (auto draft = p.drafts.find(p.profile_path); draft != p.drafts.end()) {
            draft->second.objects.erase(p.key);

            if (draft->second.objects.empty()) {
                p.drafts.erase(draft);
            }
        }

        p.selected_light = p.lights.empty() ? -1 : 0;
        enhanced::publish_object_lights(p.game, p.catalog);
        p.error.clear();
        p.status = "Reloaded saved object lights";
    }
}
}  // namespace

// All enhancement-specific properties stay below a single foldout in the general viewer.
void object_light_controls(const Shown& shown) {
    auto& p = s.object_viewer;

    if (!gl::enhanced_rendering_available()) {
        return;
    }

    if (ImGui::CollapsingHeader("Enhanced graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Preview enhanced lighting", &p.preview_enhanced);
        ImGui::Checkbox("Light gizmos", &p.light_gizmos);
        ImGui::SameLine();
        ImGui::Checkbox("Local axes", &p.local_axes);
        ImGui::Combo("Gizmo", &p.gizmo_mode, "Move (W)\0Rotate (E)\0Range (R)\0");
        light_list();

        if (object_viewer_button("Save profile", "save")) {
            save_lights(shown);
        }

        ImGui::SameLine();

        if (object_viewer_button("Reload saved lights", "reload")) {
            reload_lights(shown);
        }

        ImGui::TextWrapped(
            "Saved per object type, for every instance. These are additive unshadowed lights."
        );
        ImGui::TextWrapped("%s", p.profile_path.string().c_str());
    }
}
}  // namespace openrac::frontend::debug_overlay
#endif
