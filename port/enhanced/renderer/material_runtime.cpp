// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// material_runtime.h's transactional preview and atomic local-overlay saves.
#include "enhanced/renderer/material_runtime.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "common/log.h"
#include "enhanced/renderer/water.h"

namespace openrac::enhanced {
namespace {
// Targets the exported slot shared by all instances of the selected model.
materials::Rule selection_rule(const scene::MaterialSource& source) {
    materials::Rule rule;
    rule.id = "selection";
    rule.match.layer = source.identity.layer;
    rule.match.pass = source.identity.metal ? "metal" : "colour";
    // The synthetic water binding has no glTF slot.
    if (source.identity.material_slot) {
        rule.match.model = source.identity.model;
        rule.match.material_slot = source.identity.material_slot;
    } else {
        rule.match.material_name = source.identity.material_name;
    }
    return rule;
}

// Hashes the canonical selector, independent of source order, display labels, renderer indices and escaped characters.
std::string rule_id(const scene::MaterialSource& source) {
    materials::Catalog selector;
    selector.rules.emplace("selection", selection_rule(source));
    const auto text = materials::catalog_text(selector);
    if (!text) {
        throw std::runtime_error(text.error().message);
    }
    const std::vector<std::uint8_t> bytes(text->begin(), text->end());
    assets::disc::Sha256 digest;
    digest.update(bytes);
    return "object-" + assets::disc::to_hex(digest.finish());
}

// Adds authoring metadata only after deriving the stable match identity.
materials::Rule edit_rule(const scene::MaterialSource& source) {
    auto rule = selection_rule(source);
    rule.id = rule_id(source);
    rule.source = "Unsaved material edit";
    rule.name = source.model_name + " / " + source.identity.material_name;
    return rule;
}

// Validates the exact saved representation and assigns disk provenance before replacing any author file.
Result<materials::Catalog> authored_catalog(
    const materials::Catalog& catalog, const std::filesystem::path& path
) {
    const auto text = materials::catalog_text(catalog);
    if (!text) {
        return unexpected(text.error());
    }
    const auto utf8 = path.u8string();
    return materials::parse_catalog(*text, std::string(utf8.begin(), utf8.end()));
}

// Legacy exports acquire inspector identities without changing their original material response.
Result<std::shared_ptr<const scene::MaterialBindings>> initial_bindings(
    const scene::LevelData& level
) {
    if (!level.material_error.empty()) {
        return unexpected(Error{level.material_error});
    }
    if (level.material_bindings) {
        return level.material_bindings;
    }
    const auto assets = scene::identify_materials(level, {});
    if (!assets) {
        return unexpected(assets.error());
    }
    return scene::resolve_materials(*assets, {});
}

}  // namespace

Result<void> MaterialRuntime::load(const scene::LevelData& level) {
    try {
        clear();
        m_path = scene::material_author_path(level);
        const auto bindings = initial_bindings(level);
        const auto author = materials::read_catalog(m_path);
        const auto inherited = scene::load_material_catalog(level, m_path);
        if (!bindings || !author || !inherited) {
            auto editor = std::make_shared<MaterialEditor>();
            editor->path = m_path;
            editor->error = !bindings ? bindings.error().message
                            : !author ? author.error().message
                                      : inherited.error().message;
            m_editor = std::move(editor);
            return unexpected(Error{m_editor->error});
        }
        m_author = *author;
        m_inherited = *inherited;
        m_baseline = *bindings;
        m_transport_baseline = (*bindings)->transport_stamp;
        publish(*bindings);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

void MaterialRuntime::clear() {
    m_baseline.reset();
    m_editor.reset();
    m_responses.clear();
    set_ocean_optics_defaults({}, {});
    m_author = {};
    m_edits = {};
    m_created_materials.clear();
    m_path.clear();
    m_inherited = {};
    m_deleted.clear();
    m_transport_baseline.clear();
}

const scene::MaterialBinding* MaterialRuntime::find(std::uint32_t model, int material) const {
    return m_editor && m_editor->bindings ? m_editor->bindings->find(model, material) : nullptr;
}

const MaterialResponse* MaterialRuntime::response(std::uint32_t model, int material) const {
    if (!m_editor || !m_editor->bindings) {
        return nullptr;
    }
    const auto found = m_editor->bindings->lookup.find({model, material});
    return found == m_editor->bindings->lookup.end() ? nullptr : &m_responses[found->second];
}

void MaterialRuntime::publish(std::shared_ptr<const scene::MaterialBindings> bindings) {
    std::vector<MaterialResponse> responses;
    responses.reserve(bindings->bindings.size());
    for (std::size_t n = 0; n < bindings->bindings.size(); ++n) {
        responses.push_back(
            material_response(bindings->assets->sources[n], bindings->bindings[n].values.properties)
        );
    }
    auto editor = std::make_shared<MaterialEditor>();
    editor->path = m_path;
    editor->foliage_bindings = static_cast<std::size_t>(
        std::count_if(responses.begin(), responses.end(), [](const MaterialResponse& response) {
            return response.foliage[1] != 0;
        })
    );
    editor->bindings = std::move(bindings);
    for (const auto& [id, rule] : m_author.rules) {
        if (!m_deleted.contains(id)) {
            editor->local_rules.insert(id);
        }
    }
    for (const auto& [id, rule] : m_edits.rules) {
        editor->local_rules.insert(id);
    }
    editor->dirty = !m_edits.rules.empty() || !m_edits.presets.empty() || !m_deleted.empty();
    editor->transport_dirty = editor->bindings->transport_stamp != m_transport_baseline;
    // Existing water rendering consumes one shared appearance; simulation state remains independently owned.
    std::optional<float> ocean_roughness;
    std::optional<float> ocean_reflectivity;
    for (std::size_t n = 0; n < responses.size(); ++n) {
        if (editor->bindings->assets->sources[n].identity.material_name == "@water") {
            set_reflection_material_settings(ReflectionSurface::Water, responses[n].reflection);
            const auto& properties = editor->bindings->bindings[n].values.properties;
            if (properties.contains("ocean.roughness")) {
                ocean_roughness = materials::number_value(properties, "ocean.roughness", .22f);
            }
            if (properties.contains("ocean.reflectivity")) {
                ocean_reflectivity =
                    materials::number_value(properties, "ocean.reflectivity", .02037f);
            }
        }
    }
    set_ocean_optics_defaults(ocean_roughness, ocean_reflectivity);
    m_responses = std::move(responses);
    m_editor = std::move(editor);
}

Result<void> MaterialRuntime::patch(std::size_t binding, std::string key, materials::Value value) {
    return patch(binding, {{std::move(key), std::move(value)}});
}

Result<void> MaterialRuntime::patch(std::size_t binding, materials::Properties properties) {
    return patch(std::span(&binding, 1), std::move(properties));
}

Result<void> MaterialRuntime::patch(
    std::span<const std::size_t> bindings, materials::Properties properties
) {
    try {
        if (!m_editor || !m_editor->bindings || bindings.empty()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        const auto valid = materials::validate(properties);
        if (!valid) {
            return unexpected(valid.error());
        }
        bool ocean_roughness = false, ocean_reflectivity = false;
        auto edits = m_edits;
        for (const auto binding : bindings) {
            if (binding >= m_editor->bindings->bindings.size()) {
                return unexpected(Error{"material selection is unavailable"});
            }
            const auto& source = m_editor->bindings->assets->sources[binding];
            if (source.identity.material_name == "@water") {
                ocean_roughness = properties.contains("ocean.roughness");
                ocean_reflectivity = properties.contains("ocean.reflectivity");
            }
            auto rule = edit_rule(source);
            const auto id = rule.id;
            auto& fields = edits.rules.try_emplace(id, std::move(rule)).first->second.properties;
            for (const auto& [key, value] : properties) {
                fields.insert_or_assign(key, value);
            }
        }
        auto result = preview(std::move(edits), m_deleted);
        if (result && (ocean_roughness || ocean_reflectivity)) {
            reset_ocean_optics(ocean_roughness, ocean_reflectivity);
        }
        return result;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::assign(
    std::span<const std::size_t> bindings,
    const std::string& material,
    materials::Properties definition
) {
    try {
        if (!m_editor || !m_editor->bindings || bindings.empty()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        auto edits = m_edits;
        std::optional<materials::Properties> created;
        if (!m_baseline->source_catalog.presets.contains(material)
            && m_editor->bindings->catalog.presets.contains(material)) {
            edits.presets.try_emplace(material, m_editor->bindings->catalog.presets.at(material));
        }
        if (!m_editor->bindings->catalog.presets.contains(material)) {
            if (definition.empty()) {
                return unexpected(Error{"shared material is unavailable"});
            }
            materials::Catalog checked;
            checked.presets.emplace(material, definition);
            const auto valid = materials::catalog_text(checked);
            if (!valid) {
                return unexpected(valid.error());
            }
            edits.presets[material] = materials::material_definition(std::move(definition));
            created = edits.presets[material];
            for (const auto& [key, value] : edits.presets[material]) {
                edits.preset_sources[material][key] = "Unsaved shared material";
            }
        }
        for (const auto binding : bindings) {
            if (binding >= m_editor->bindings->bindings.size()) {
                return unexpected(Error{"material selection is unavailable"});
            }
            auto rule = edit_rule(m_editor->bindings->assets->sources[binding]);
            rule.material = material;
            edits.rules.insert_or_assign(rule.id, std::move(rule));
        }
        const auto result = preview(std::move(edits), m_deleted);
        if (result && created) {
            m_created_materials.emplace(material, std::move(*created));
        }
        return result;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<std::string> MaterialRuntime::make_unique(std::span<const std::size_t> bindings) {
    try {
        if (!m_editor || !m_editor->bindings || bindings.empty()
            || bindings.front() >= m_editor->bindings->bindings.size()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        const auto& binding = m_editor->bindings->bindings[bindings.front()];
        const auto& source = m_editor->bindings->assets->sources[bindings.front()];
        auto definition = binding.values.properties;
        const auto response = material_response(source, definition);
        definition.try_emplace("surface.roughness", double(response.reflection.roughness));
        definition.try_emplace("surface.reflectivity", double(response.reflection.reflectivity));
        definition.try_emplace("reflection.strength", double(response.reflection.strength));
        const std::string prefix =
            binding.values.material.empty() ? "material" : binding.values.material;
        std::string name;
        for (unsigned suffix = 1;; ++suffix) {
            name = prefix.substr(0, 110) + "-copy-" + std::to_string(suffix);
            if (!m_editor->bindings->catalog.presets.contains(name)) {
                break;
            }
        }
        const auto result = assign(bindings, name, std::move(definition));
        if (!result) {
            return unexpected(result.error());
        }
        return name;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::patch_preset(
    const std::string& preset, std::string key, materials::Value value
) {
    try {
        if (!m_editor || !m_editor->bindings
            || (!m_editor->bindings->catalog.presets.contains(preset)
                && !m_baseline->source_catalog.presets.contains(preset))) {
            return unexpected(Error{"material preset is unavailable"});
        }
        const auto valid = materials::validate({{key, value}});
        if (!valid) {
            return unexpected(valid.error());
        }
        auto edits = m_edits;
        if (!m_baseline->source_catalog.presets.contains(preset)) {
            edits.presets.try_emplace(preset, m_editor->bindings->catalog.presets.at(preset));
        }
        edits.presets[preset][key] = std::move(value);
        if (key == "type" && materials::text_value(edits.presets[preset], "type") == "foliage") {
            edits.presets[preset]["surface.roughness"] = std::max(
                .35,
                double(materials::number_value(
                    m_editor->bindings->catalog.presets.at(preset), "surface.roughness", .65f
                ))
            );
        }
        edits.preset_sources[preset][key] = "Unsaved material edit";
        return preview(std::move(edits), m_deleted);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::delete_override(const std::string& id) {
    try {
        if (!m_editor || !m_editor->local_rules.contains(id)) {
            return unexpected(Error{"Only overrides in the current save file can be deleted."});
        }
        auto edits = m_edits;
        edits.rules.erase(id);
        auto deleted = m_deleted;
        if (m_author.rules.contains(id)) {
            deleted.insert(id);
        }
        return preview(std::move(edits), deleted);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::clear_slots(std::span<const std::size_t> bindings) {
    try {
        if (!m_editor || !m_editor->bindings || bindings.empty()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        const auto& current = *m_editor->bindings;
        auto candidate = current.source_catalog;
        candidate.rules.clear();
        std::set<std::string> removed;
        for (const auto binding : bindings) {
            if (binding >= current.bindings.size()) {
                return unexpected(Error{"material selection is unavailable"});
            }
            const auto& source = current.assets->sources[binding];
            const auto own_rule = rule_id(source);
            for (const auto& id : m_editor->local_rules) {
                const auto& rule = current.catalog.rules.at(id);
                // A broad rule may affect unrelated slots; only an exact slot is removed here.
                const bool exact = rule.match.model == source.identity.model
                                   && rule.match.material_slot
                                   && rule.match.material_slot == source.identity.material_slot;
                if (id != own_rule && !exact) {
                    continue;
                }
                candidate.rules.clear();
                candidate.rules.emplace(id, rule);
                const auto matched = materials::resolve(candidate, source.identity);
                if (!matched) {
                    return unexpected(matched.error());
                }
                if (id == own_rule || !matched->rules.empty()) {
                    removed.insert(id);
                }
            }
        }
        if (removed.empty()) {
            return {};
        }
        auto edits = m_edits;
        auto deleted = m_deleted;
        for (const auto& id : removed) {
            edits.rules.erase(id);
            if (m_author.rules.contains(id)) {
                deleted.insert(id);
            }
        }
        return preview(std::move(edits), deleted);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::undo(std::size_t binding, const std::string& key) {
    return undo(std::span(&binding, 1), key);
}

Result<void> MaterialRuntime::undo(std::span<const std::size_t> bindings, const std::string& key) {
    try {
        if (!m_editor || !m_editor->bindings || bindings.empty()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        auto edits = m_edits;
        bool changed = false;
        for (const auto binding : bindings) {
            if (binding >= m_editor->bindings->bindings.size()) {
                return unexpected(Error{"material selection is unavailable"});
            }
            const auto id = rule_id(m_editor->bindings->assets->sources[binding]);
            const auto rule = edits.rules.find(id);
            if (rule == edits.rules.end()) {
                continue;
            }
            changed |= rule->second.properties.erase(key) != 0;
            if (rule->second.properties.empty()) {
                edits.rules.erase(rule);
            }
        }
        return changed ? preview(std::move(edits), m_deleted) : Result<void>{};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<std::shared_ptr<const scene::MaterialBindings>> MaterialRuntime::resolve_author(
    const materials::Catalog& author, const std::set<std::string>& deleted
) const {
    auto catalog = m_baseline->source_catalog;
    for (const auto& id : deleted) {
        catalog.rules.erase(id);
        if (const auto inherited = m_inherited.rules.find(id);
            inherited != m_inherited.rules.end()) {
            catalog.rules.insert(*inherited);
        }
    }
    materials::merge_catalog(catalog, author, std::numeric_limits<int>::max());
    return scene::resolve_materials(m_baseline->assets, catalog);
}

Result<void> MaterialRuntime::preview(
    materials::Catalog edits, const std::set<std::string>& deleted
) {
    // Edits share a file with the existing local rules and must preview at the same priority.
    auto author = m_author;
    for (const auto& id : deleted) {
        author.rules.erase(id);
    }
    materials::merge_catalog(author, edits, 0);
    const auto resolved = resolve_author(author, deleted);
    if (!resolved) {
        return unexpected(resolved.error());
    }
    m_edits = std::move(edits);
    m_deleted = deleted;
    publish(*resolved);
    return {};
}

Result<void> MaterialRuntime::undo_preset(const std::string& preset, const std::string& key) {
    try {
        if (!m_baseline) {
            return unexpected(Error{"material editor unavailable"});
        }
        auto edits = m_edits;
        const auto fields = edits.presets.find(preset);
        if (fields == edits.presets.end()) {
            return {};
        }
        const auto created = m_created_materials.find(preset);
        if (created != m_created_materials.end() && created->second.contains(key)) {
            fields->second[key] = created->second.at(key);
            edits.preset_sources[preset][key] = "Unsaved shared material";
        } else {
            fields->second.erase(key);
            edits.preset_sources[preset].erase(key);
        }
        if (fields->second.empty()) {
            edits.presets.erase(fields);
        }
        return preview(std::move(edits), m_deleted);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

void MaterialRuntime::reset() {
    if (!m_baseline) {
        return;
    }
    m_edits = {};
    m_created_materials.clear();
    m_deleted.clear();
    publish(m_baseline);
}

Result<void> MaterialRuntime::save() {
    try {
        if (!m_baseline || !m_editor) {
            return unexpected(Error{"material editor unavailable"});
        }
        if (!m_editor->dirty) {
            return {};
        }
        const auto current = materials::read_catalog(m_path);
        if (!current) {
            return unexpected(current.error());
        }
        const auto disk_text = materials::catalog_text(*current);
        const auto loaded_text = materials::catalog_text(m_author);
        if (!disk_text || !loaded_text || *disk_text != *loaded_text) {
            return unexpected(
                Error{"material overrides changed on disk; reload the level before saving"}
            );
        }
        auto author = m_author;
        for (const auto& id : m_deleted) {
            author.rules.erase(id);
        }
        materials::merge_catalog(author, m_edits, 0);
        const auto loaded = authored_catalog(author, m_path);
        if (!loaded) {
            return unexpected(loaded.error());
        }
        const auto resolved = resolve_author(*loaded, m_deleted);
        if (!resolved) {
            return unexpected(resolved.error());
        }
        // Validate the persisted layer composition before the atomic replacement, not after it.
        const auto saved = materials::write_catalog(m_path, author);
        if (!saved) {
            return unexpected(saved.error());
        }
        m_author = *loaded;
        m_baseline = *resolved;
        m_edits = {};
        m_created_materials.clear();
        m_deleted.clear();
        publish(m_baseline);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::enhanced
