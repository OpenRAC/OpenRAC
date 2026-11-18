// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material Studio's schema-backed controls and compatible finish library.
#include "debug/material_widgets.h"

#include <algorithm>
#include <array>
#include <limits>

#include "debug/material_presets.h"
#include "debug/menu_ui.h"
#include "enhanced/renderer/material_runtime.h"
#include "frontend.h"

namespace openrac::frontend::debug_overlay {
namespace {
// Field provenance distinguishes unsaved edits from inherited and saved values.
bool unsaved_property(const std::map<std::string, std::string>& sources, const std::string& key) {
    const auto found = sources.find(key);
    return found != sources.end() && found->second == "Unsaved material edit";
}

// Edits the Lit surface option without enabling unsupported shader families.
bool family_widget(std::string& value, const char* label) {
    int selected = value == "water" ? 1 : 0;
    if (!ImGui::Combo(label, &selected, "Regular\0Water\0")) {
        return false;
    }
    value = selected == 1 ? "water" : "standard";
    return true;
}

// Applies the reviewed roughness floor to foliage controls.
materials::Field applicable_field(materials::Field field, const materials::Properties& properties) {
    if (field.path == "surface.roughness"
        && materials::text_value(properties, "shading") == "foliage") {
        field.minimum = .35;
    }
    return field;
}

// Edits bounded scalars while keeping HDR emission controls unbounded by slider precision.
bool number_widget(const materials::Field& field, materials::Value& value, const char* label) {
    double number = std::get<double>(value);
    const bool hdr = field.maximum > std::numeric_limits<float>::max() / 2.0;
    bool changed = false;
    if (hdr) {
        changed = ImGui::InputDouble(label, &number);
        number = std::clamp(number, field.minimum, field.maximum);
    } else {
        changed = ImGui::SliderScalar(
            label,
            ImGuiDataType_Double,
            &number,
            &field.minimum,
            &field.maximum,
            "%.3f",
            ImGuiSliderFlags_AlwaysClamp
        );
    }
    if (changed) {
        value = number;
    }
    return changed;
}

// Dispatches schema fields to typed controls; asset paths remain read-only.
bool property_widget(const materials::Field& field, materials::Value& value, bool labelled = true) {
    const std::string label = labelled ? std::string(field.label) : "##value";
    switch (field.kind) {
        case materials::Kind::Boolean:
            return ImGui::Checkbox(label.c_str(), &std::get<bool>(value));
        case materials::Kind::Number:
            return number_widget(field, value, label.c_str());
        case materials::Kind::Colour: {
            auto& colour = std::get<std::array<double, 3>>(value);
            std::array<float, 3> edit{
                static_cast<float>(colour[0]),
                static_cast<float>(colour[1]),
                static_cast<float>(colour[2])
            };
            if (!ImGui::ColorEdit3(label.c_str(), edit.data())) {
                return false;
            }
            colour = {edit[0], edit[1], edit[2]};
            return true;
        }
        case materials::Kind::Text: {
            auto& text = std::get<std::string>(value);
            if (field.path == "type") {
                return type_widget(text, label.c_str());
            }
            if (field.path == "shading") {
                return family_widget(text, label.c_str());
            }
            if (labelled) {
                ImGui::Text("%s: %s", label.c_str(), text.c_str());
            } else {
                ImGui::TextWrapped("%s", text.c_str());
            }
            return false;
        }
    }
    return false;
}

// Groups compatible finishes for the library selector.
const char* preset_group(PresetSurface surface) {
    switch (surface) {
        case PresetSurface::Opaque:
            return "Opaque surfaces";
        case PresetSurface::Metal:
            return "Bare metals";
        case PresetSurface::Glass:
            return "Thin glass";
        case PresetSurface::Water:
        case PresetSurface::Ocean:
            return "Water";
        case PresetSurface::Foliage:
            return "Foliage";
        case PresetSurface::DryFur:
        case PresetSurface::WetFur:
            return "Fur";
    }
    return "";
}

// Reports why the current surface cannot use a library finish.
const char* preset_requirement(PresetSurface surface, const materials::Identity& identity) {
    const bool water = identity.material_name == "@water";
    switch (surface) {
        case PresetSurface::Opaque:
        case PresetSurface::Foliage:
        case PresetSurface::Metal:
            return !water && !identity.metal && identity.layer != "sky"
                       ? nullptr
                       : "Select a regular colour material.";
        case PresetSurface::Glass:
            return !water && identity.layer != "sky" ? nullptr
                                                     : "Select an object or world surface.";
        case PresetSurface::Water:
        case PresetSurface::Ocean:
            return water ? nullptr : "Select the level's water material.";
        case PresetSurface::DryFur:
        case PresetSurface::WetFur:
            for (const auto* profile :
                 {&enhanced::rac1_fur_profile(), &enhanced::rac2_fur_profile()}) {
                if (identity.layer == "moby" && !identity.metal
                    && identity.class_id == profile->class_id
                    && identity.material_name == profile->material) {
                    return nullptr;
                }
            }
            return "Select Ratchet's fur material. The renderer verifies the coat coverage.";
    }
    return "Select a compatible material.";
}

// Reverts only fields whose provenance identifies an unsaved edit.
void revert_control(
    const std::string& material,
    const std::string& key,
    const std::map<std::string, std::string>& sources
) {
    ImGui::TableSetColumnIndex(2);
    ImGui::BeginDisabled(!unsaved_property(sources, key));
    if (ImGui::SmallButton("Revert")) {
        const bool restored = frontend::undo_material_preset(material, key, s.material_error);
        if (!restored) {
            ImGui::TextWrapped("%s", s.material_error.c_str());
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Revert an unsaved edit to this property. Saved values are kept.");
    }
}

// Draws a typed value and its field-level Revert action.
void property_row(
    const std::string& material,
    const std::string& key,
    const materials::Value& current,
    const materials::Properties& properties,
    const std::map<std::string, std::string>& sources
) {
    const auto* field = materials::field(key);
    if (!field) {
        return;
    }
    auto value = current;
    ImGui::PushID(key.c_str());
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextWrapped("%.*s", static_cast<int>(field->label.size()), field->label.data());
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.f);
    if (property_widget(applicable_field(*field, properties), value, false)) {
        const bool applied =
            frontend::patch_material_preset(material, key, std::move(value), s.material_error);
        if (!applied) {
            ImGui::TextWrapped("%s", s.material_error.c_str());
        }
    }
    revert_control(material, key, sources);
    ImGui::PopID();
}

// Shows inherited defaults without authoring them and filters controls to the selected type.
void properties_table(
    const std::string& material,
    const materials::Properties& properties,
    const enhanced::MaterialEditor& editor,
    const std::string& type
) {
    const auto& origins = editor.bindings->catalog.preset_sources;
    const auto origin = origins.find(material);
    const std::map<std::string, std::string> empty;
    const auto& sources = origin == origins.end() ? empty : origin->second;
    if (!ImGui::BeginTable(
            "Shared material properties",
            3,
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg
                | ImGuiTableFlags_BordersInnerH
        )) {
        return;
    }
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.35f);
    ImGui::TableSetupColumn("Revert", ImGuiTableColumnFlags_WidthFixed, dp(48.f));
    scene::Material inherited;
    for (const auto& binding : editor.bindings->bindings) {
        if (binding.values.material == material) {
            inherited = binding.surface();
            break;
        }
    }
    const auto controls = material_control_properties(properties, inherited);
    for (const auto& [key, current] : controls) {
        if (key == "type" || (key == "shading" && type != "lit")
            || ((key.starts_with("glass.") || key == "reflection.opacity") && type != "glass-lit")
            || (key.starts_with("foliage.") && type != "foliage")
            || (key.starts_with("fur.") && type != "lit-fur" && type != "lit-wet-fur")
            || (key.starts_with("wetness.") && type != "lit-wet-fur")
            || (key.starts_with("ocean.")
                && materials::text_value(properties, "shading") != "water")) {
            continue;
        }
        property_row(material, key, current, properties, sources);
    }
    ImGui::EndTable();
}

}  // namespace

bool type_widget(std::string& value, const char* label) {
    bool changed = false;
    const auto current = std::ranges::find_if(materials::kMaterialTypes, [&](const auto& type) {
        return type.id == value;
    });
    const auto current_label = current == std::end(materials::kMaterialTypes)
                                   ? std::string("Lit")
                                   : std::string(current->label);
    if (ImGui::BeginCombo(label, current_label.c_str())) {
        for (const auto& type : materials::kMaterialTypes) {
            const std::string type_label(type.label);
            if (ImGui::Selectable(type_label.c_str(), type.id == value)) {
                value = type.id;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

void foliage_diagnostics() {
    auto controls = frontend::foliage_controls();
    if (ImGui::Combo(
            "Foliage view", &controls.debug, "Shaded\0Material mask\0Transmission\0Normals\0"
        )) {
        const bool applied = frontend::set_foliage_controls(controls, s.material_error);
        if (!applied) {
            ImGui::TextWrapped("%s", s.material_error.c_str());
        }
    }
}

void library_controls(const scene::MaterialBindings& bindings, std::span<const std::size_t> slots) {
    auto& selection = s.material_library_preset;
    if (ImGui::BeginCombo(
            "Preset library", selection < 0 ? "Choose a finish" : kMaterialPresets[selection].name
        )) {
        std::string_view last_group;
        for (int i = 0; i < static_cast<int>(std::size(kMaterialPresets)); ++i) {
            const auto& preset = kMaterialPresets[i];
            const std::string_view group = preset_group(preset.surface);
            if (group != last_group) {
                ImGui::TextDisabled("%.*s", static_cast<int>(group.size()), group.data());
                last_group = group;
            }
            if (ImGui::Selectable(preset.name, i == selection)) {
                selection = i;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", preset.description);
            }
        }
        ImGui::EndCombo();
    }
    if (selection < 0) {
        return;
    }
    const auto& preset = kMaterialPresets[selection];
    ImGui::TextWrapped("%s", preset.description);
    const char* requirement = nullptr;
    for (const auto slot : slots) {
        requirement = preset_requirement(preset.surface, bindings.assets->sources[slot].identity);
        if (requirement) {
            break;
        }
    }
    if (requirement) {
        ImGui::TextDisabled("%s", requirement);
    }
    ImGui::BeginDisabled(requirement != nullptr);
    if (ImGui::Button(slots.size() == 1 ? "Assign finish" : "Assign finish to selected surfaces")) {
        std::string name = "library.";
        for (const char c : std::string_view(preset.name)) {
            if (c >= 'A' && c <= 'Z') {
                name += static_cast<char>(c + 'a' - 'A');
            } else {
                name += c == ' ' ? '-' : c;
            }
        }
        frontend::assign_material(slots, name, preset_properties(preset), s.material_error);
    }
    ImGui::EndDisabled();
}

void preset_controls(
    const std::string& material, const materials::Properties& properties, bool heading
) {
    const auto editor = frontend::material_editor();
    if (heading) {
        ImGui::TextWrapped("%s", material.c_str());
        ImGui::TextWrapped("Editing this material changes every surface using it.");
    }
    auto type = std::string(materials::text_value(properties, "type", "lit"));
    ImGui::TextUnformatted("Material type");
    ImGui::SetNextItemWidth(-1.f);
    if (type_widget(type, "##material_type")) {
        frontend::patch_material_preset(material, "type", type, s.material_error);
    }
    if (type == "original") {
        ImGui::TextWrapped(
            "Uses the original surface appearance without enhanced material lighting."
        );
        return;
    }
    if (type == "lit-fur" || type == "lit-wet-fur") {
        ImGui::TextWrapped(
            "Fur detail uses verified coat coverage. Wetness uses the existing contact simulation."
        );
    }
    properties_table(material, properties, *editor, type);
}

}  // namespace openrac::frontend::debug_overlay
