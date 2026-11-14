// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material catalog source merging and deterministic resolution; called at load/edit boundaries.
#include <algorithm>
#include <tuple>

#include "materials/catalog.h"

namespace openrac::materials {
namespace {
// More-specific conditions win only inside their source layer; file priority always comes first.
int specificity(const Selector& match) {
    // Object slots outrank type defaults, which outrank global texture and pass defaults.
    return (match.material_slot ? 256 : 0) + (!match.model.empty() ? 128 : 0)
           + (!match.material_name.empty() ? 64 : 0) + (match.class_id ? 32 : 0)
           + (!match.texture_hash.empty() || !match.texture_file_hash.empty() ? 16 : 0)
           + (!match.texture_path.empty() ? 8 : 0) + (!match.pass.empty() ? 4 : 0)
           + (match.cutout_only ? 2 : 0) + (!match.layer.empty() ? 1 : 0);
}

// Every supplied identity constraint must match; no filename or colour heuristics are used.
bool matches(const Selector& match, const Identity& identity) {
    return (match.layer.empty() || match.layer == identity.layer)
           && (!match.class_id || *match.class_id == identity.class_id)
           && (match.material_name.empty() || match.material_name == identity.material_name)
           && (match.model.empty() || match.model == identity.model)
           && (!match.material_slot || match.material_slot == identity.material_slot)
           && (match.texture_hash.empty() || match.texture_hash == identity.texture_hash)
           && (match.texture_file_hash.empty()
               || match.texture_file_hash == identity.texture_file_hash)
           && (match.texture_path.empty() || match.texture_path == identity.texture_path)
           && (match.pass.empty() || (match.pass == "metal") == identity.metal)
           && (!match.cutout_only || identity.cutout);
}

// One effective rule contribution, with its preset already combined with explicit properties.
struct Contribution {
    // Allows allocation failures from the owned containers to reach the caller's error boundary.
    Contribution() noexcept(false) = default;
    Contribution(const Contribution&) noexcept(false) = default;
    Contribution(Contribution&&) noexcept(false) = default;
    Contribution& operator=(const Contribution&) noexcept(false) = default;
    Contribution& operator=(Contribution&&) noexcept(false) = default;
    ~Contribution() = default;
    Properties properties;                       // Preset followed by rule overrides.
    std::map<std::string, std::string> sources;  // Exact source of each effective property.
    std::map<std::string, int> layers;  // Preset-reference or explicit-field source priority.
};

// Expands one preset without inheritance, rejecting unresolved references explicitly.
Result<Contribution> contribution(const Catalog& catalog, const Rule& rule) {
    Contribution result;
    // Rules without a preset can supply an ordinary sparse property override.
    const auto& reference = rule.material.empty() ? rule.preset : rule.material;
    if (!reference.empty()) {
        const auto preset = catalog.presets.find(reference);
        if (preset == catalog.presets.end()) {
            return unexpected(Error{rule.source + ": unknown material preset " + reference});
        }
        result.properties = preset->second;
        for (const auto& [key, value] : preset->second) {
            const auto layers = catalog.preset_layers.find(reference);
            const int origin = layers != catalog.preset_layers.end() && layers->second.contains(key)
                                   ? layers->second.at(key)
                                   : 0;
            result.layers[key] = std::max(rule.preset_layer, origin);
        }
        const auto sources = catalog.preset_sources.find(reference);
        if (sources != catalog.preset_sources.end()) {
            result.sources = sources->second;
        }
    }
    for (const auto& [key, value] : rule.properties) {
        result.properties[key] = value;
        const auto origin = rule.property_sources.find(key);
        result.sources[key] = origin == rule.property_sources.end() ? rule.source : origin->second;
        const auto layer = rule.property_layers.find(key);
        result.layers[key] = layer == rule.property_layers.end() ? rule.layer : layer->second;
    }
    return result;
}
}  // namespace

void merge_catalog(Catalog& target, const Catalog& overlay, int layer) {
    // A partial preset edit changes only its named properties.
    for (const auto& [id, properties] : overlay.presets) {
        for (const auto& [key, value] : properties) {
            target.presets[id][key] = value;
            target.preset_layers[id][key] = layer;
            const auto sources = overlay.preset_sources.find(id);
            if (sources != overlay.preset_sources.end() && sources->second.contains(key)) {
                target.preset_sources[id][key] = sources->second.at(key);
            }
        }
    }
    // A selector is replaced as one unit; sparse material properties retain lower-layer values.
    for (const auto& [id, source] : overlay.rules) {
        Rule replacement = source;
        replacement.preset_layer = layer;
        for (const auto& [key, value] : source.properties) {
            replacement.property_layers[key] = layer;
            replacement.property_sources[key] = source.source;
        }
        const auto previous = target.rules.find(id);
        if (previous != target.rules.end() && source.material.empty()) {
            // A disabled tombstone retains the selector needed to display or re-enable the original rule.
            if (!source.enabled && specificity(source.match) == 0) {
                replacement.match = previous->second.match;
            }
            Properties combined = previous->second.properties;
            auto field_layers = previous->second.property_layers;
            auto field_sources = previous->second.property_sources;
            for (const auto& [key, value] : replacement.properties) {
                combined[key] = value;
                field_layers[key] = layer;
                field_sources[key] = source.source;
            }
            replacement.properties = std::move(combined);
            replacement.property_layers = std::move(field_layers);
            replacement.property_sources = std::move(field_sources);
            if (replacement.preset.empty()) {
                replacement.preset = previous->second.preset;
                replacement.preset_layer = previous->second.preset_layer;
            }
            replacement.material = previous->second.material;
        }
        replacement.layer = layer;
        target.rules[id] = std::move(replacement);
    }
}

Result<Resolved> resolve(const Catalog& catalog, const Identity& identity) {
    std::vector<const Rule*> selected;
    // Disabled overrides remain catalog entries, so a mod can explicitly disable a shared rule by id.
    for (const auto& [id, rule] : catalog.rules) {
        if (rule.enabled && matches(rule.match, identity)) {
            selected.push_back(&rule);
        }
    }
    std::sort(selected.begin(), selected.end(), [](const Rule* a, const Rule* b) {
        return std::tuple(a->layer, specificity(a->match), a->id)
               < std::tuple(b->layer, specificity(b->match), b->id);
    });
    Resolved result;
    std::map<std::string, std::tuple<int, int, std::string>> writers;
    std::optional<std::pair<int, int>> assignment_rank;
    // Equal-precedence conflicting writes are authoring errors, never JSON-order-dependent choices.
    for (const Rule* rule : selected) {
        const auto expanded = contribution(catalog, *rule);
        if (!expanded) {
            return unexpected(expanded.error());
        }
        if (!rule->material.empty()) {
            const auto rank = std::pair(rule->layer, specificity(rule->match));
            if (assignment_rank == rank && result.material != rule->material) {
                return unexpected(Error{"ambiguous material assignment: " + rule->id});
            }
            // Assignments select one complete material instead of retaining a previous finish.
            result = {};
            result.material = rule->material;
            assignment_rank = rank;
            writers.clear();
        }
        for (const auto& [key, value] : expanded->properties) {
            const auto rank = std::pair(expanded->layers.at(key), specificity(rule->match));
            const auto prior = writers.find(key);
            // An unchanged inherited field cannot acquire the priority of a newer unrelated edit.
            if (prior != writers.end()
                && rank < std::pair(std::get<0>(prior->second), std::get<1>(prior->second))) {
                continue;
            }
            if (prior != writers.end()
                && rank == std::pair(std::get<0>(prior->second), std::get<1>(prior->second))
                && result.properties.at(key) != value) {
                return unexpected(
                    Error{
                        "ambiguous material property " + key + ": " + std::get<2>(prior->second)
                        + " and " + rule->id
                    }
                );
            }
            result.properties[key] = value;
            const auto origin = expanded->sources.find(key);
            result.sources[key] = origin == expanded->sources.end() ? rule->source : origin->second;
            writers[key] = {rank.first, rank.second, rule->id};
        }
        result.rules.push_back(rule->id);
    }
    const auto valid = validate(result.properties);
    if (!valid) {
        return unexpected(valid.error());
    }
    return result;
}

Result<std::string> transport_catalog_stamp(const Catalog& catalog) {
    Catalog projected;
    bool projects_transport = false;
    // Runtime-only catalog edits must not enter offline preparation fingerprints.
    for (const auto& [id, rule] : catalog.rules) {
        if (!rule.enabled) {
            continue;
        }
        const auto expanded = contribution(catalog, rule);
        if (!expanded) {
            return unexpected(expanded.error());
        }
        auto properties = transport_properties(expanded->properties);
        projects_transport |= !properties.empty();
        if (properties.empty() && rule.material.empty()) {
            continue;
        }
        Rule item = rule;
        item.preset.clear();
        item.material.clear();
        item.properties = std::move(properties);
        if (!rule.material.empty()) {
            // Keep assignment boundaries: they can remove earlier diffuse/emission overrides.
            item.material = rule.material;
            projected.presets[rule.material] =
                transport_properties(catalog.presets.at(rule.material));
            item.properties = transport_properties(rule.properties);
        }
        item.name.clear();
        // Priority changes can change transport even when selectors and numeric values are unchanged.
        for (const auto& [key, value] : transport_properties(expanded->properties)) {
            item.name += key + "=" + std::to_string(expanded->layers.at(key)) + ";";
        }
        projected.rules.emplace(id, std::move(item));
    }
    // Generated surface materials can be saved as shared definitions without explicit slot rules.
    // Include their transport contribution so preparation cannot reuse a bake with stale geometry.
    for (const auto& [name, definition] : catalog.presets) {
        const bool referenced = std::ranges::any_of(catalog.rules, [&](const auto& item) {
            return item.second.enabled
                   && (item.second.material == name || item.second.preset == name);
        });
        if (!referenced) {
            auto properties = transport_properties(definition);
            if (!properties.empty()) {
                projects_transport = true;
                projected.presets[name] = std::move(properties);
            }
        }
    }
    if (!projects_transport) {
        return std::string{};
    }
    return catalog_text(projected);
}
}  // namespace openrac::materials
