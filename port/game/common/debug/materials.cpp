// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Unified material authoring window, using the shared schema and explicit sparse property edits.
#include <algorithm>
#include <array>
#include <limits>

#include "debug/overlay.h"
#include "enhanced/renderer/foliage.h"
#include "enhanced/renderer/material_runtime.h"
#include "frontend.h"

namespace openrac::frontend::debug_overlay {
namespace {
// Retains inherited values until a widget changes; displayed defaults never become authored overrides implicitly.
materials::Properties displayed_properties(const scene::MaterialBinding& binding) {
    auto properties = binding.values.properties;
    properties.try_emplace("surface.tint", std::array<double, 3>{1, 1, 1});
    properties
        .try_emplace("surface.diffuse_scale", static_cast<double>(binding.surface.diffuse_scale));
    properties.try_emplace(
        "emission.runtime_intensity",
        static_cast<double>(
            *std::max_element(binding.surface.emission.begin(), binding.surface.emission.end())
        )
    );
    properties.try_emplace(
        "emission.bake_intensity",
        static_cast<double>(binding.surface.bake_intensity.value_or(
            *std::max_element(binding.surface.emission.begin(), binding.surface.emission.end())
        ))
    );
    properties
        .try_emplace("emission.contrast", static_cast<double>(binding.surface.emission_contrast));
    properties.try_emplace("emission.vertex_colour", binding.surface.emission_vertex_colour);
    properties.try_emplace(
        "emission.use_base_texture", binding.surface.emission_image == binding.surface.image
    );
    return properties;
}

// Presents only implemented families; changing a family cannot manufacture water geometry or reflective packets.
bool family_widget(std::string& value) {
    int selected = value == "foliage" ? 1 : 0;
    if (!ImGui::Combo("Shading", &selected, "Standard\0Foliage\0")) {
        return false;
    }
    value = selected == 1 ? "foliage" : "standard";
    return true;
}

// Narrows the shared roughness range for thin leaves without changing chrome/glass controls.
materials::Field applicable_field(materials::Field field, const materials::Properties& properties) {
    if (field.path == "surface.roughness"
        && materials::text_value(properties, "shading") == "foliage") {
        field.minimum = .35;
    }
    return field;
}

// Diagnostics remain session-only; authoring files contain material response rather than debug views.
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

// Edits one supported scalar, retaining the schema's bounds and HDR input range.
bool number_widget(const materials::Field& field, materials::Value& value) {
    double number = std::get<double>(value);
    const bool hdr = field.maximum > std::numeric_limits<float>::max() / 2.0;
    bool changed = false;
    if (hdr) {
        changed = ImGui::InputDouble(std::string(field.label).c_str(), &number);
        number = std::clamp(number, field.minimum, field.maximum);
    } else {
        changed = ImGui::SliderScalar(
            std::string(field.label).c_str(),
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

// Text asset references are read-only here; importing unverified coverage or missing images requires an explicit reload.
bool property_widget(const materials::Field& field, materials::Value& value) {
    const std::string label(field.label);
    switch (field.kind) {
        case materials::Kind::Boolean:
            return ImGui::Checkbox(label.c_str(), &std::get<bool>(value));
        case materials::Kind::Number:
            return number_widget(field, value);
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
            if (field.path == "shading" && (text == "standard" || text == "foliage")) {
                return family_widget(text);
            }
            ImGui::Text("%s: %s", label.c_str(), text.c_str());
            return false;
        }
    }
    return false;
}

// Keeps the click picker's model/slot selection stable when sorted authoring rows change between levels.
void pick_binding(const scene::MaterialBindings& bindings) {
    if (s.material_slot < 0) {
        return;
    }
    const auto found = bindings.lookup.find({s.material_model, s.material_slot});
    if (found != bindings.lookup.end()) {
        s.material_selection = found->second;
    }
    s.material_slot = -1;
    s.material_preset_edit = false;
}

// Lists loaded identities without matching selectors or hashing images in the frame loop.
void binding_combo(const scene::MaterialBindings& bindings) {
    const auto& sources = bindings.assets->sources;
    s.material_selection = std::min(s.material_selection, sources.size() - 1);
    const auto label = [](const scene::MaterialSource& source) {
        auto name = source.model_name + " / " + source.identity.material_name;
        // Repeated or empty glTF names still need visibly distinct choices.
        if (source.identity.material_slot) {
            name += " [slot " + std::to_string(*source.identity.material_slot) + "]";
        }
        return name;
    };
    if (!ImGui::BeginCombo("Material", label(sources[s.material_selection]).c_str())) {
        return;
    }
    for (std::size_t n = 0; n < sources.size(); ++n) {
        ImGui::PushID(static_cast<int>(n));
        if (ImGui::Selectable(label(sources[n]).c_str(), n == s.material_selection)) {
            s.material_selection = n;
        }
        ImGui::PopID();
    }
    ImGui::EndCombo();
}

// Shows exact provenance and sends only changed properties; validation failures leave all previous bindings intact.
void binding_controls(const scene::MaterialBindings& bindings) {
    const auto& binding = bindings.bindings[s.material_selection];
    const auto& source = bindings.assets->sources[s.material_selection];
    const bool water = source.identity.material_name == "@water";
    const bool sky = source.identity.layer == "sky";
    auto properties = water ? binding.values.properties : displayed_properties(binding);
    // Older exports have no authored water fields, but still use the renderer defaults.
    if (water) {
        const auto defaults =
            enhanced::default_reflection_material(enhanced::ReflectionSurface::Water);
        properties.try_emplace("surface.roughness", static_cast<double>(defaults.roughness));
        properties.try_emplace("surface.reflectivity", static_cast<double>(defaults.reflectivity));
        properties.try_emplace("reflection.strength", static_cast<double>(defaults.strength));
    }
    if (!source.identity.metal && !water && !sky) {
        properties.try_emplace("shading", std::string("standard"));
    }
    if (materials::text_value(properties, "shading") == "foliage") {
        properties.try_emplace("surface.roughness", .65);
        properties.try_emplace("surface.reflectivity", .04);
        properties.try_emplace("foliage.transmission", .35);
        properties.try_emplace("foliage.thickness", .7);
        properties.try_emplace("foliage.normal_bend", .2);
    }
    ImGui::Text("Layer: %s | Class: %d", source.identity.layer.c_str(), source.identity.class_id);
    ImGui::TextWrapped("Edits apply to every instance of this model and material slot.");
    for (const auto& field : materials::fields()) {
        // The sky shader consumes only tint.
        if (sky && field.path != "surface.tint") {
            continue;
        }
        const auto found = properties.find(field.path);
        if (found == properties.end()) {
            continue;
        }
        ImGui::PushID(field.path.data(), field.path.data() + field.path.size());
        auto value = found->second;
        if (property_widget(applicable_field(field, properties), value)) {
            const bool applied = frontend::patch_material(
                s.material_selection, std::string(field.path), std::move(value), s.material_error
            );
            if (!applied) {
                ImGui::TextWrapped("%s", s.material_error.c_str());
            }
        }
        if (ImGui::IsItemHovered()) {
            const auto origin = binding.values.sources.find(std::string(field.path));
            ImGui::SetTooltip(
                "%s",
                origin == binding.values.sources.end() ? "Original material/default"
                                                       : origin->second.c_str()
            );
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Undo")) {
            const bool restored = frontend::undo_material(
                s.material_selection, std::string(field.path), s.material_error
            );
            if (!restored) {
                ImGui::TextWrapped("%s", s.material_error.c_str());
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Discard this field's unsaved edit and inherit its loaded/saved value."
            );
        }
        ImGui::PopID();
    }
}

// Shared presets update every rule using them, while higher-priority object fields remain explicit exceptions.
void preset_controls(const scene::MaterialBindings& bindings) {
    const auto& presets = bindings.catalog.presets;
    if (presets.empty()) {
        ImGui::TextDisabled("No shared presets loaded.");
        return;
    }
    if (!presets.contains(s.material_preset)) {
        s.material_preset = presets.begin()->first;
    }
    if (ImGui::BeginCombo("Preset", s.material_preset.c_str())) {
        for (const auto& [id, properties] : presets) {
            if (ImGui::Selectable(id.c_str(), id == s.material_preset)) {
                s.material_preset = id;
            }
        }
        ImGui::EndCombo();
    }
    for (const auto& [key, current] : presets.at(s.material_preset)) {
        const auto* field = materials::field(key);
        if (!field) {
            continue;
        }
        auto value = current;
        ImGui::PushID(key.c_str());
        if (property_widget(applicable_field(*field, presets.at(s.material_preset)), value)) {
            const bool applied = frontend::patch_material_preset(
                s.material_preset, key, std::move(value), s.material_error
            );
            if (!applied) {
                ImGui::TextWrapped("%s", s.material_error.c_str());
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Undo")) {
            const bool restored =
                frontend::undo_material_preset(s.material_preset, key, s.material_error);
            if (!restored) {
                ImGui::TextWrapped("%s", s.material_error.c_str());
            }
        }
        ImGui::PopID();
    }
}

// Offers explicit saves and session reset, with a separate warning for transport edits.
void save_controls(const enhanced::MaterialEditor& editor) {
    const auto path = editor.path.u8string();
    ImGui::TextWrapped("Save to: %s", reinterpret_cast<const char*>(path.c_str()));
    ImGui::BeginDisabled(!editor.dirty);
    if (ImGui::Button("Save material overrides")) {
        const bool saved = frontend::save_materials(s.material_error);
        if (!saved) {
            ImGui::TextWrapped("%s", s.material_error.c_str());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard unsaved edits")) {
        frontend::reset_materials();
        s.material_error.clear();
    }
    ImGui::EndDisabled();
    if (editor.dirty) {
        ImGui::TextDisabled("Unsaved changes");
    }
    if (editor.transport_dirty) {
        ImGui::TextWrapped("Baked lighting changed. Rebake this level to update probe lighting.");
    }
    if (!s.material_error.empty()) {
        ImGui::TextWrapped("%s", s.material_error.c_str());
    }
}
}  // namespace

void material_settings_window() {
    if (!s.material_settings) {
        return;
    }
    if (!ImGui::Begin("Material inspector", &s.material_settings)) {
        ImGui::End();
        return;
    }
    const auto editor = frontend::material_editor();
    if (!editor) {
        ImGui::TextDisabled("Waiting for the render worker.");
    } else if (!editor->error.empty()) {
        ImGui::TextWrapped("%s", editor->error.c_str());
    } else if (editor->bindings && !editor->bindings->bindings.empty()) {
        pick_binding(*editor->bindings);
        ImGui::Checkbox("Edit shared preset", &s.material_preset_edit);
        if (!s.material_preset_edit) {
            binding_combo(*editor->bindings);
        }
        save_controls(*editor);
        foliage_diagnostics();
        ImGui::Separator();
        if (s.material_preset_edit) {
            preset_controls(*editor->bindings);
        } else {
            binding_controls(*editor->bindings);
        }
    }
    ImGui::End();
}
}  // namespace openrac::frontend::debug_overlay
