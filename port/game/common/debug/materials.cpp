// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Unified material authoring window, using the shared schema and explicit sparse property edits.
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <vector>

#include "debug/material_presets.h"
#include "debug/menu_ui.h"
#include "enhanced/renderer/foliage.h"
#include "enhanced/renderer/material_runtime.h"
#include "frontend.h"
#include "renderer/gl/gl.h"

namespace openrac::frontend::debug_overlay {
namespace {
// Retains inherited values until a widget changes; displayed defaults never become authored overrides implicitly.
materials::Properties displayed_properties(const scene::MaterialBinding& binding) {
    return material_control_properties(binding.values.properties, binding.surface());
}

bool type_widget(std::string& value, const char* label) {
    bool changed = false;
    const auto current = std::ranges::find_if(materials::kMaterialTypes,
        [&](const auto& type) { return type.id == value; });
    if (ImGui::BeginCombo(label, current == std::end(materials::kMaterialTypes) ? "Lit" : current->label.data())) {
        for (const auto& type : materials::kMaterialTypes) {
            if (ImGui::Selectable(type.label.data(), type.id == value)) {
                value = type.id;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool family_widget(std::string& value, const char* label) {
    int selected = value == "water" ? 1 : 0;
    if (!ImGui::Combo(label, &selected, "Regular\0Water\0")) {
        return false;
    }
    value = selected == 1 ? "water" : "standard";
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

// Text asset references are read-only here; importing unverified coverage or missing images requires an explicit reload.
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

bool surface_has_local_changes(
    const enhanced::MaterialEditor& editor, const scene::MaterialBindings& bindings, std::size_t index
) {
    const auto& identity = bindings.assets->sources[index].identity;
    const auto& applied = bindings.bindings[index].values.rules;
    return std::ranges::any_of(applied, [&](const auto& id) {
        const auto& rule = bindings.catalog.rules.at(id);
        return editor.local_rules.contains(id)
            && ((rule.match.model == identity.model && rule.match.material_slot
                 && rule.match.material_slot == identity.material_slot)
                || (identity.material_name == "@water" && rule.match.material_name == "@water"));
    });
}

std::string surface_material_label(
    const enhanced::MaterialEditor& editor, const scene::MaterialBindings& bindings, std::size_t index
) {
    const auto& material = bindings.bindings[index].values.material;
    if (!material.empty()) {
        return material;
    }
    return surface_has_local_changes(editor, bindings, index) ? "Custom changes" : "Level defaults";
}

bool unsaved_property(
    const std::map<std::string, std::string>& sources, const std::string& key
) {
    const auto found = sources.find(key);
    return found != sources.end() && found->second == "Unsaved material edit";
}

void select_surface(const scene::MaterialBindings&, std::size_t index) {
    s.material_selection = index;
    s.material_preset.clear();
    s.material_error.clear();
}

bool pick_binding(const scene::MaterialBindings& bindings, const Shown& shown) {
    s.material_extra_slots.clear();
    s.material_selection.reset();
    const auto& hit = s.picker_result.hit;
    if (!hit || s.picker_result.level != shown.picker_level_number) {
        s.material_error = "No surface picked. Click an object with the Moby picker.";
        return false;
    }
    const auto found = bindings.lookup.find({hit->model, hit->material});
    if (found != bindings.lookup.end()) {
        const auto& identity = bindings.assets->sources[found->second].identity;
        if (!scene::is_retail_chrome(identity.metal, identity.material_name)) {
            select_surface(bindings, found->second);
            return true;
        }
    }
    s.material_error = "The picked surface has no editable material binding.";
    return false;
}

std::string binding_label(const scene::MaterialSource& source) {
    auto name = source.model_name + " / " + source.identity.material_name;
    if (source.identity.material_slot) {
        name += " [slot " + std::to_string(*source.identity.material_slot) + "]";
    }
    return name;
}

// Older exports still expose the visor's compiled defaults as sparse material edits.
void show_visor_defaults(materials::Properties& properties, const scene::MaterialSource& source) {
    const auto response = enhanced::material_response(source, properties);
    const auto& reflection = response.reflection;
    const auto colour = [](const std::array<float, 3>& rgb) {
        return std::array<double, 3>{rgb[0], rgb[1], rgb[2]};
    };
    properties.try_emplace("surface.roughness", double(reflection.roughness));
    properties.try_emplace("surface.reflectivity", double(reflection.reflectivity));
    properties.try_emplace("reflection.strength", double(reflection.strength));
    properties.try_emplace("reflection.opacity", double(reflection.opacity));
    properties.try_emplace("reflection.tint", colour(reflection.tint));
    properties.try_emplace("glass.transmission", double(response.glass.transmission));
    properties.try_emplace("glass.tint", colour(response.glass.tint));
}

const char* preset_group(PresetSurface surface) {
    switch (surface) {
        case PresetSurface::Opaque: return "Opaque surfaces";
        case PresetSurface::Metal: return "Bare metals";
        case PresetSurface::Glass: return "Thin glass";
        case PresetSurface::Water:
        case PresetSurface::Ocean: return "Water";
        case PresetSurface::Foliage: return "Foliage";
        case PresetSurface::DryFur:
        case PresetSurface::WetFur: return "Fur";
    }
    return "";
}

const char* preset_requirement(PresetSurface surface, const materials::Identity& identity) {
    const bool water = identity.material_name == "@water";
    switch (surface) {
        case PresetSurface::Opaque:
        case PresetSurface::Foliage:
        case PresetSurface::Metal:
            return !water && !identity.metal && identity.layer != "sky"
                       ? nullptr : "Select a regular colour material.";
        case PresetSurface::Glass:
            return !water && identity.layer != "sky" ? nullptr : "Select an object or world surface.";
        case PresetSurface::Water:
        case PresetSurface::Ocean:
            return water ? nullptr : "Select the level's water material.";
        case PresetSurface::DryFur:
        case PresetSurface::WetFur:
            for (const auto* profile : {&enhanced::rac1_fur_profile(), &enhanced::rac2_fur_profile()}) {
                if (identity.layer == "moby" && !identity.metal && identity.class_id == profile->class_id
                    && identity.material_name == profile->material) {
                    return nullptr;
                }
            }
            return "Select Ratchet's fur material. The renderer verifies the coat coverage.";
    }
    return "Select a compatible material.";
}

void library_controls(const scene::MaterialBindings& bindings,
                      std::span<const std::size_t> slots) {
    auto& selection = s.material_library_preset;
    if (ImGui::BeginCombo("Preset library", selection < 0 ? "Choose a finish" : kMaterialPresets[selection].name)) {
        std::string_view last_group;
        for (int i = 0; i < static_cast<int>(std::size(kMaterialPresets)); ++i) {
            const auto& preset = kMaterialPresets[i];
            const std::string_view group = preset_group(preset.surface);
            if (group != last_group) {
                ImGui::TextDisabled("%s", group.data());
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
            name += c >= 'A' && c <= 'Z' ? char(c + 'a' - 'A') : c == ' ' ? '-' : c;
        }
        frontend::assign_material(slots, name, preset_properties(preset), s.material_error);
    }
    ImGui::EndDisabled();
}

scene::ObjectPreview studio_preview;
std::weak_ptr<const scene::MaterialAssets> preview_assets;
std::optional<std::size_t> preview_selection;
bool preview_sphere = false;
bool uploaded_sphere = false;
std::vector<bool> preview_visibility;

bool update_preview_materials(const scene::MaterialBindings& bindings, std::uint32_t model) {
    const auto& parts = studio_preview.mesh().parts;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto found = bindings.lookup.find({model, parts[index].source_material});
        if (found == bindings.lookup.end()) {
            continue;
        }
        const auto& binding = bindings.bindings[found->second];
        const auto& source = bindings.assets->sources[found->second];
        if (scene::is_retail_chrome(source.identity.metal, source.identity.material_name)) {
            continue;
        }
        const auto response = enhanced::material_response(source, binding.values.properties);
        const auto& optics = response.reflection;
        const float metallic = std::max(response.metallic, 0.f);
        if (!studio_preview.update_material(index, binding.surface(), {
                optics.tint, optics.roughness, optics.reflectivity, optics.strength,
                metallic
            })) {
            return false;
        }
    }
    return true;
}

void material_preview(const scene::MaterialBindings& bindings, const Shown& shown) {
    const auto selection = *s.material_selection;
    const auto& source = bindings.assets->sources[selection];
    const bool has_mesh = source.identity.layer == "moby";
    const float size = dp(132.f);
    const auto top = ImGui::GetCursorScreenPos();
    if (ImGui::InvisibleButton("Material preview", {size, size}, ImGuiButtonFlags_MouseButtonRight)
        && has_mesh) {
        preview_sphere = !preview_sphere;
    }
    if (has_mesh && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Right-click to switch between the object and UV sphere.");
    }
    const bool sphere = !has_mesh || preview_sphere;
    const auto high = ImVec2(top.x + size, top.y + size);
    const auto* level = shown.picker_level;
    if (!ImGui::IsItemVisible() || !level) {
        return;
    }

    const auto show_error = [&](const Error& error) {
        ImGui::GetWindowDrawList()->AddRectFilled(top, high, ui().ink_deep);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", error.message.c_str());
        }
    };
    std::vector<bool> visibility;
    if (!sphere) {
        for (const auto& primitive : level->models[source.model].primitives) {
            visibility.push_back(scene::material_pass_visible(
                primitive, level->materials[primitive.material], true,
                bindings.original(source.model, primitive.appearance_slot())));
        }
    }
    if (preview_assets.lock() != bindings.assets || preview_selection != selection
        || preview_visibility != visibility
        || uploaded_sphere != sphere || !update_preview_materials(bindings, source.model)) {
        preview_selection.reset();
        scene::Instance instance;
        instance.model = source.model;
        scene::PickHit hit{};
        hit.model = source.model;
        hit.layer = scene::Layer::Mobys;
        auto mesh = sphere
                        ? Result<scene::PreviewMesh>(scene::preview_uv_sphere(
                              *level, source.material, bindings.bindings[selection].surface()
                          ))
                        : scene::preview_mesh(*level, hit, instance, {}, &bindings);
        if (!mesh) {
            studio_preview.release();
            show_error(mesh.error());
            return;
        }
        const auto loaded = studio_preview.select(std::move(*mesh));
        if (!loaded) {
            show_error(loaded.error());
            return;
        }
        update_preview_materials(bindings, source.model);
        preview_assets = bindings.assets;
        preview_visibility = std::move(visibility);
        preview_selection = selection;
        uploaded_sphere = sphere;
    }

    scene::PreviewOrbit orbit;
    orbit.distance = sphere ? 2.5f : 2.25f;
    const auto result = studio_preview.draw(
        int(size * ImGui::GetIO().DisplayFramebufferScale.x),
        int(size * ImGui::GetIO().DisplayFramebufferScale.y), orbit
    );
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
    gl::glViewport(0, 0, shown.window_width, shown.window_height);
    if (!result) {
        show_error(result.error());
        return;
    }
    ImGui::GetWindowDrawList()->AddImage(
        ImTextureID(studio_preview.texture()), top, high, {0, 1}, {1, 0}
    );
}

void material_slot_list(
    const enhanced::MaterialEditor& editor, const scene::MaterialBindings& bindings, std::size_t selection
) {
    const auto& sources = bindings.assets->sources;
    const auto model = sources[selection].model;
    const auto first = bindings.lookup.lower_bound({model, std::numeric_limits<int>::min()});
    const auto end = bindings.lookup.upper_bound({model, std::numeric_limits<int>::max()});
    const auto visible = [&](std::size_t index) {
        const auto& identity = sources[index].identity;
        return !scene::is_retail_chrome(identity.metal, identity.material_name);
    };
    const auto label = [&](std::size_t index) {
        const auto& identity = sources[index].identity;
        const auto slot = identity.material_slot.value_or(-1);
        std::string text = slot < 0 ? "Default surface" : "Surface " + std::to_string(slot);
        if (!identity.material_name.empty()) {
            text += " / " + identity.material_name;
        }
        if (identity.metal) {
            text += " (metal)";
        }
        return text;
    };
    menu_heading("Surfaces on this object");
    ImGui::TextWrapped("Select a surface. Ctrl-click to select more; Shift-click selects a range.");
    for (auto entry = first; entry != end; ++entry) {
        const auto index = entry->second;
        if (!visible(index)) {
            continue;
        }
        auto& extras = s.material_extra_slots;
        const bool active = index == *s.material_selection;
        const bool selected = active || std::ranges::find(extras, index) != extras.end();
        ImGui::PushID(static_cast<int>(index));
        const auto row = label(index) + (active && !extras.empty() ? " (active)" : "") + "###slot";
        if (ImGui::Selectable(row.c_str(), selected)) {
            const auto& io = ImGui::GetIO();
            if (io.KeyShift) {
                extras.clear();
                const auto anchor = std::find_if(first, end, [&](const auto& slot) {
                    return slot.second == selection && visible(slot.second);
                });
                if (anchor == end) {
                    select_surface(bindings, index);
                } else {
                    const auto [low, high] = std::minmax(anchor, entry, [](auto a, auto b) {
                        return a->first < b->first;
                    });
                    for (auto slot = low; slot != std::next(high); ++slot) {
                        if (slot->second != selection && visible(slot->second)) {
                            extras.push_back(slot->second);
                        }
                    }
                }
            } else if (!io.KeyCtrl) {
                extras.clear();
                select_surface(bindings, index);
            } else if (!active) {
                if (std::erase(extras, index) == 0) {
                    extras.push_back(index);
                }
            } else if (!extras.empty()) {
                const auto next = extras.back();
                extras.pop_back();
                select_surface(bindings, next);
            }
        }
        ImGui::TextWrapped("%s", surface_material_label(editor, bindings, index).c_str());
        ImGui::Spacing();
        ImGui::PopID();
    }
}

void object_information(
    const enhanced::MaterialEditor& editor, const scene::MaterialBindings& bindings, const Shown& shown
) {
    const auto& source = bindings.assets->sources[*s.material_selection];
    ImGui::TextWrapped("%s", source.model_name.c_str());
    ImGui::TextWrapped(
        "%s",
        source.identity.material_name.empty() ? "Unnamed material"
                                              : source.identity.material_name.c_str()
    );
    if (ImGui::TreeNode("Source information")) {
        ImGui::TextWrapped("Layer: %s / Surface: %d", source.identity.layer.c_str(),
                           source.identity.material_slot.value_or(-1));
        if (!source.identity.texture_path.empty()) {
            ImGui::TextWrapped("Texture: %s", source.identity.texture_path.c_str());
        }
        ImGui::TreePop();
    }
    ImGui::Spacing();
    material_preview(bindings, shown);
    ImGui::TextDisabled("Studio preview");
    material_slot_list(editor, bindings, *s.material_selection);
}

materials::Properties slot_properties(const scene::MaterialBindings& bindings, std::size_t selection) {
    const auto& binding = bindings.bindings[selection];
    const auto& source = bindings.assets->sources[selection];
    const bool water = source.identity.material_name == "@water";
    const bool sky = source.identity.layer == "sky";
    const bool visor = source.identity.material_name == enhanced::kShipVisorMaterial;
    auto properties = water || visor ? binding.values.properties : displayed_properties(binding);
    // Transmission applies only to the thin pane's blended pass.
    if (visor) {
        show_visor_defaults(properties, source);
    }
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
    const auto response = enhanced::material_response(source, properties);
    if (!visor) {
        properties.erase("reflection.opacity");
    }
    if (!water && !sky && !visor) {
        properties.try_emplace("surface.metallic", 0.0);
        properties.try_emplace("surface.roughness", double(response.reflection.roughness));
        properties.try_emplace("surface.reflectivity", double(response.reflection.reflectivity));
        properties.try_emplace("reflection.strength", double(response.reflection.strength));
    }
    return properties;
}

void preset_controls(const std::string& material, const materials::Properties& properties, bool heading = true);

bool material_used_by(const scene::MaterialBindings& bindings, std::size_t index, const std::string& name) {
    const auto& values = bindings.bindings[index].values;
    if (!values.material.empty()) {
        return values.material == name;
    }
    return std::ranges::any_of(values.rules, [&](const auto& id) {
        return bindings.catalog.rules.at(id).preset == name;
    });
}

std::size_t material_users(const scene::MaterialBindings& bindings, const std::string& name) {
    std::size_t count = 0;
    for (std::size_t index = 0; index < bindings.bindings.size(); ++index) {
        count += material_used_by(bindings, index, name);
    }
    return count;
}

// Every surface references a shared material, including imported defaults and legacy edits.
void binding_controls(const enhanced::MaterialEditor& editor, const scene::MaterialBindings& bindings) {
    const auto selection = *s.material_selection;
    const auto& binding = bindings.bindings[selection];
    std::vector<std::size_t> slots{selection};
    slots.insert(slots.end(), s.material_extra_slots.begin(), s.material_extra_slots.end());
    const auto properties = slot_properties(bindings, selection);
    const auto& material = binding.values.material;
    const bool shared_material = std::ranges::all_of(slots, [&](std::size_t index) {
        return bindings.bindings[index].values.material == material;
    });
    const bool local_changes = std::ranges::any_of(slots, [&](std::size_t index) {
        return surface_has_local_changes(editor, bindings, index);
    });
    const auto state = !shared_material ? "Mixed materials" : material;
    ImGui::TextWrapped("Material: %s", state.c_str());
    if (slots.size() > 1) {
        ImGui::TextWrapped("%zu surfaces selected. Assignment and reset apply to this selection.", slots.size());
    }
    if (!shared_material) {
        ImGui::TextWrapped("These surfaces use different materials. Choose one material to link them.");
    } else {
        const auto users = material_users(bindings, material);
        ImGui::TextWrapped("Shared by %zu %s. Editing its properties changes all of them.",
                           users, users == 1 ? "surface" : "surfaces");
    }
    ImGui::Spacing();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::BeginCombo("##assigned_material", "Choose a shared material...")) {
        for (const auto& [name, definition] : bindings.catalog.presets) {
            if (ImGui::Selectable(name.c_str(), shared_material && material == name)) {
                frontend::assign_material(slots, name, {}, s.material_error);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::BeginDisabled(!local_changes);
    if (ImGui::Button(slots.size() == 1 ? "Restore level material" : "Restore selected level materials")) {
        frontend::clear_material_slots(slots, s.material_error);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Remove saved and unsaved changes from the selected surfaces. "
                          "Other surfaces and shared materials stay intact. Save to keep the reset.");
    }
    if (shared_material && !material.empty() && ImGui::Button("Make independent copy")) {
        frontend::make_material_unique(slots, s.material_error);
    }
    if (ImGui::Button("Create shared material...")) {
        ImGui::OpenPopup("Create shared material");
    }
    static std::array<char, 129> new_name{};
    static std::string new_type = "lit";
    if (ImGui::BeginPopup("Create shared material")) {
        ImGui::TextWrapped("Start with this surface's current properties, then reuse the material on other surfaces.");
        type_widget(new_type, "Material type");
        ImGui::SetNextItemWidth(dp(260));
        ImGui::InputTextWithHint("##new_material", "e.g. novalis-stone", new_name.data(), new_name.size());
        const bool exists = bindings.catalog.presets.contains(new_name.data());
        if (exists) {
            ImGui::TextWrapped("That name is already in use. Choose it from the material list instead.");
        }
        ImGui::BeginDisabled(new_name[0] == '\0' || exists);
        auto definition = properties;
        definition["type"] = new_type;
        if (ImGui::Button("Create and assign")
            && frontend::assign_material(slots, new_name.data(), materials::material_definition(std::move(definition)), s.material_error)) {
            new_name[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    if (ImGui::CollapsingHeader("Choose a finish")) {
        library_controls(bindings, slots);
    }
    if (!shared_material) {
        ImGui::TextWrapped("Select surfaces using the same material, or assign one material to this selection.");
        return;
    }
    ImGui::Separator();
    preset_controls(material, bindings.catalog.presets.at(material), false);
}

void surface_list(const enhanced::MaterialEditor& editor, bool reveal_pick) {
    const auto& bindings = *editor.bindings;
    static ImGuiTextFilter search;
    static bool changed_only = false;
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputTextWithHint("##surface_search", "Search surfaces...", search.InputBuf, sizeof(search.InputBuf))) {
        search.Build();
    }
    ImGui::Checkbox("Local changes only", &changed_only);
    ImGui::Separator();
    std::size_t shown = 0;
    for (std::size_t index = 0; index < bindings.bindings.size(); ++index) {
        const auto& source = bindings.assets->sources[index];
        if (scene::is_retail_chrome(source.identity.metal, source.identity.material_name)) {
            continue;
        }
        const bool selected = s.material_selection == index;
        const auto full_label = binding_label(source);
        if (!selected && ((!search.PassFilter(full_label.c_str()))
            || (changed_only && !surface_has_local_changes(editor, bindings, index)))) {
            continue;
        }
        ++shown;
        ImGui::PushID(static_cast<int>(index));
        std::string label = source.model_name;
        if (source.identity.material_slot && *source.identity.material_slot >= 0) {
            label += " / Surface " + std::to_string(*source.identity.material_slot);
        }
        if (ImGui::Selectable(label.c_str(), selected)) {
            s.material_extra_slots.clear();
            select_surface(bindings, index);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", full_label.c_str());
        }
        if (selected && reveal_pick) {
            ImGui::SetScrollHereY(.5f);
        }
        ImGui::TextWrapped("%s", surface_material_label(editor, bindings, index).c_str());
        ImGui::Spacing();
        ImGui::PopID();
    }
    if (shown == 0) {
        ImGui::TextWrapped("No matching surfaces. Turn off Local changes only to browse the level.");
    }
}
void shared_preset_list(const scene::MaterialBindings& bindings) {
    ImGui::PushID("Shared materials");
    if (bindings.catalog.presets.empty()) {
        ImGui::TextDisabled("No shared materials loaded.");
    }
    for (const auto& [id, properties] : bindings.catalog.presets) {
        const auto label = id + " (" + std::to_string(material_users(bindings, id)) + " surfaces)";
        if (ImGui::Selectable(label.c_str(), id == s.material_preset)) {
            s.material_preset = id;
            s.material_extra_slots.clear();
            s.material_selection.reset();
            s.material_error.clear();
        }
    }
    ImGui::PopID();
}

// Shared presets update every rule using them, while higher-priority object fields remain explicit exceptions.
void preset_controls(const std::string& material, const materials::Properties& properties, bool heading) {
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
        ImGui::TextWrapped("Uses the original surface appearance without enhanced material lighting.");
        return;
    }
    if (type == "lit-fur" || type == "lit-wet-fur") {
        ImGui::TextWrapped("Fur detail uses verified coat coverage. Wetness uses the existing contact simulation.");
    }
    if (!ImGui::BeginTable("Shared material properties", 3,
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
        return;
    }
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.35f);
    ImGui::TableSetupColumn("Revert", ImGuiTableColumnFlags_WidthFixed, dp(48.f));
    scene::Material inherited;
    for (const auto& binding : editor->bindings->bindings) {
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
            || (key.starts_with("ocean.") && materials::text_value(properties, "shading") != "water")) {
            continue;
        }
        const auto* field = materials::field(key);
        if (!field) {
            continue;
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
            const bool applied = frontend::patch_material_preset(
                material, key, std::move(value), s.material_error
            );
            if (!applied) {
                ImGui::TextWrapped("%s", s.material_error.c_str());
            }
        }
        ImGui::TableSetColumnIndex(2);
        const auto& sources = editor->bindings->catalog.preset_sources;
        const auto origin = sources.find(material);
        ImGui::BeginDisabled(origin == sources.end() || !unsaved_property(origin->second, key));
        if (ImGui::SmallButton("Revert")) {
            const bool restored =
                frontend::undo_material_preset(material, key, s.material_error);
            if (!restored) {
                ImGui::TextWrapped("%s", s.material_error.c_str());
            }
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Revert an unsaved edit to this property. Saved values are kept.");
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

void studio_footer(const enhanced::MaterialEditor& editor) {
    const bool picking = picker_enabled();
    if (ImGui::Button(picking ? "Stop picking" : "Open Moby picker")) {
        set_picker_enabled(!picking);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!editor.dirty);
    if (ImGui::Button("Save changes")) {
        frontend::save_materials(s.material_error);
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard unsaved changes")) {
        frontend::reset_materials();
        s.material_error.clear();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Details")) {
        ImGui::OpenPopup("Material save details");
    }
    if (ImGui::BeginPopup("Material save details")) {
        const auto path = editor.path.u8string();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + dp(420));
        ImGui::TextUnformatted(reinterpret_cast<const char*>(path.c_str()));
        if (s.material_selection && editor.bindings) {
            const auto& binding = editor.bindings->bindings[*s.material_selection];
            if (ImGui::TreeNode("Material sources")) {
                for (const auto& id : binding.values.rules) {
                    const auto& rule = editor.bindings->catalog.rules.at(id);
                    ImGui::TextWrapped("%s", rule.name.empty() ? rule.id.c_str() : rule.name.c_str());
                    ImGui::TextWrapped("%s", rule.source.c_str());
                }
                ImGui::TreePop();
            }
        }
        if (!s.material_error.empty()) {
            ImGui::TextUnformatted(s.material_error.c_str());
        }
        ImGui::PopTextWrapPos();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Diagnostics")) {
        ImGui::OpenPopup("Material diagnostics");
    }
    if (ImGui::BeginPopup("Material diagnostics")) {
        ImGui::SetNextItemWidth(dp(170));
        foliage_diagnostics();
        ImGui::EndPopup();
    }
    if (!s.material_error.empty()) {
        ImGui::TextUnformatted("Material action failed. See Details.");
    } else if (picking) {
        ImGui::TextDisabled("Click a surface in the level to inspect its material.");
    } else if (editor.transport_dirty) {
        ImGui::TextDisabled("Rebake this level to update probe lighting.");
    } else {
        ImGui::TextDisabled("%s", editor.dirty ? "Unsaved changes" : "Changes saved");
    }
}
}  // namespace

void release_material_preview() {
    studio_preview.release();
    preview_assets.reset();
    preview_selection.reset();
}

void material_settings_window(const Shown& shown) {
    if (!s.material_settings) {
        return;
    }
    const auto* viewport = ImGui::GetMainViewport();
    const float margin = dp(12);
    const ImVec2 available{
        std::max(1.f, viewport->WorkSize.x - 2 * margin),
        std::max(1.f, viewport->WorkSize.y - 2 * margin)
    };
    ImGui::SetNextWindowPos(
        {viewport->WorkPos.x + margin, viewport->WorkPos.y + margin}, ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSize(
        {std::min(dp(1080), available.x), std::min(dp(700), available.y)}, ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSizeConstraints(
        {std::min(dp(720), available.x), std::min(dp(320), available.y)}, available
    );
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(18), dp(14)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, dp(12));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui_colors::mix(ui().panel_hi, ui().ink_deep, 0.35f));
    const bool visible = ImGui::Begin("Material Studio###Material inspector", &s.material_settings);
    if (!visible) {
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        return;
    }
    const auto editor = frontend::material_editor();
    if (!editor) {
        ImGui::TextDisabled("Waiting for the render worker.");
    } else if (!editor->error.empty()) {
        ImGui::TextWrapped("%s", editor->error.c_str());
    } else if (editor->bindings && !editor->bindings->bindings.empty()) {
        const auto& bindings = *editor->bindings;
        if (s.material_selection_assets.lock() != bindings.assets) {
            s.material_selection_assets = bindings.assets;
            s.material_extra_slots.clear();
            s.material_selection.reset();
            s.material_error.clear();
        }
        const bool reveal_pick =
            std::exchange(s.material_pick_pending, false) && pick_binding(bindings, shown);
        if (!bindings.catalog.presets.contains(s.material_preset)) {
            s.material_preset.clear();
        }

        const float body_height = std::max(
            1.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()
                     - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y
        );
        const float width = ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemSpacing.x;
        const float list_width = width * .28f;
        const float object_width = width * .27f;
        constexpr auto pane_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        if (ImGui::BeginChild(
                "Browser", ImVec2(list_width, body_height), ImGuiChildFlags_Borders, pane_flags
            )) {
            menu_heading("Browse");
            if (ImGui::BeginTabBar("Material browser")) {
                if (ImGui::BeginTabItem("Materials")) {
                    if (ImGui::BeginChild("Material list")) {
                        shared_preset_list(bindings);
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Surfaces", nullptr, reveal_pick ? ImGuiTabItemFlags_SetSelected : 0)) {
                    if (ImGui::BeginChild("Surface list")) {
                        surface_list(*editor, reveal_pick);
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild(
                "Object", ImVec2(object_width, body_height), ImGuiChildFlags_Borders, pane_flags
            )) {
            menu_heading(s.material_selection ? "Selected object" : "Material users");
            if (ImGui::BeginChild("Object contents")) {
                if (s.material_selection) {
                    object_information(*editor, bindings, shown);
                } else if (!s.material_preset.empty()) {
                    ImGui::TextWrapped("Material: %s", s.material_preset.c_str());
                    ImGui::Text("Used by %zu surfaces", material_users(bindings, s.material_preset));
                    for (std::size_t index = 0; index < bindings.bindings.size(); ++index) {
                        if (!material_used_by(bindings, index, s.material_preset)) {
                            continue;
                        }
                        const auto& source = bindings.assets->sources[index];
                        const auto label = source.model_name + " / Surface "
                            + std::to_string(source.identity.material_slot.value_or(-1));
                        ImGui::PushID(static_cast<int>(index));
                        const bool selected = ImGui::Selectable(label.c_str());
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", binding_label(source).c_str());
                        }
                        ImGui::PopID();
                        if (selected) {
                            select_surface(bindings, index);
                            break;
                        }
                    }
                } else {
                    ImGui::TextWrapped("Pick a surface in the level, browse Surfaces, or choose a shared material.");
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild(
                "Properties", ImVec2(0, body_height), ImGuiChildFlags_Borders, pane_flags
            )) {
            menu_heading(s.material_selection ? "Surface material" : "Shared material");
            if (ImGui::BeginChild("Property contents")) {
                if (const auto preset = bindings.catalog.presets.find(s.material_preset);
                    preset != bindings.catalog.presets.end()) {
                    ImGui::PushID(preset->first.c_str());
                    preset_controls(preset->first, preset->second);
                    ImGui::PopID();
                }
                if (s.material_selection) {
                    binding_controls(*editor, bindings);
                } else if (s.material_preset.empty()) {
                    ImGui::TextDisabled("Select a shared material or pick a surface to assign one.");
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
        ImGui::Separator();
        studio_footer(*editor);
    } else {
        ImGui::TextDisabled("No editable materials are loaded for this level.");
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    if (!s.material_settings) {
        release_material_preview();
    }
}
}  // namespace openrac::frontend::debug_overlay
