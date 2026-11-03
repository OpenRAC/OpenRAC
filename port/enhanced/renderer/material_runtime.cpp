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
        if (!bindings || !author) {
            auto editor = std::make_shared<MaterialEditor>();
            editor->path = m_path;
            editor->error = !bindings ? bindings.error().message : author.error().message;
            m_editor = std::move(editor);
            return unexpected(Error{m_editor->error});
        }
        m_author = *author;
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
    m_author = {};
    m_edits = {};
    m_path.clear();
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
    editor->dirty = !m_edits.rules.empty() || !m_edits.presets.empty();
    editor->transport_dirty = editor->bindings->transport_stamp != m_transport_baseline;
    // Existing water rendering consumes one shared appearance; simulation state remains independently owned.
    for (std::size_t n = 0; n < responses.size(); ++n) {
        if (editor->bindings->assets->sources[n].identity.material_name == "@water") {
            set_reflection_material_settings(ReflectionSurface::Water, responses[n].reflection);
        }
    }
    m_responses = std::move(responses);
    m_editor = std::move(editor);
}

Result<void> MaterialRuntime::patch(std::size_t binding, std::string key, materials::Value value) {
    try {
        if (!m_editor || !m_editor->bindings || binding >= m_editor->bindings->bindings.size()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        materials::Properties property{{key, value}};
        const auto valid = materials::validate(property);
        if (!valid) {
            return unexpected(valid.error());
        }
        auto edits = m_edits;
        auto rule = edit_rule(m_editor->bindings->assets->sources[binding]);
        const auto id = rule.id;
        edits.rules.try_emplace(id, std::move(rule));
        edits.rules.at(id).properties[key] = std::move(value);
        return preview(std::move(edits));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::patch_preset(
    const std::string& preset, std::string key, materials::Value value
) {
    try {
        if (!m_baseline || !m_baseline->catalog.presets.contains(preset)) {
            return unexpected(Error{"material preset is unavailable"});
        }
        const auto valid = materials::validate({{key, value}});
        if (!valid) {
            return unexpected(valid.error());
        }
        auto edits = m_edits;
        edits.presets[preset][key] = std::move(value);
        edits.preset_sources[preset][key] = "Unsaved material edit";
        return preview(std::move(edits));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> MaterialRuntime::undo(std::size_t binding, const std::string& key) {
    try {
        if (!m_editor || !m_editor->bindings || binding >= m_editor->bindings->bindings.size()) {
            return unexpected(Error{"material selection is unavailable"});
        }
        auto edits = m_edits;
        const auto id = rule_id(m_editor->bindings->assets->sources[binding]);
        const auto rule = edits.rules.find(id);
        if (rule == edits.rules.end()) {
            return {};
        }
        rule->second.properties.erase(key);
        if (rule->second.properties.empty()) {
            edits.rules.erase(rule);
        }
        return preview(std::move(edits));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<std::shared_ptr<const scene::MaterialBindings>> MaterialRuntime::resolve_author(
    const materials::Catalog& author
) const {
    auto catalog = m_baseline->catalog;
    materials::merge_catalog(catalog, author, std::numeric_limits<int>::max());
    return scene::resolve_materials(m_baseline->assets, catalog);
}

Result<void> MaterialRuntime::preview(materials::Catalog edits) {
    // Edits share a file with the existing local rules and must preview at the same priority.
    auto author = m_author;
    materials::merge_catalog(author, edits, 0);
    const auto resolved = resolve_author(author);
    if (!resolved) {
        return unexpected(resolved.error());
    }
    m_edits = std::move(edits);
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
        fields->second.erase(key);
        if (fields->second.empty()) {
            edits.presets.erase(fields);
        }
        return preview(std::move(edits));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

void MaterialRuntime::reset() {
    if (!m_baseline) {
        return;
    }
    m_edits = {};
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
        materials::merge_catalog(author, m_edits, 0);
        const auto loaded = authored_catalog(author, m_path);
        if (!loaded) {
            return unexpected(loaded.error());
        }
        const auto resolved = resolve_author(*loaded);
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
        publish(m_baseline);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::enhanced
