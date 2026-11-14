// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Versioned material definitions shared by export, bake preparation and enhanced rendering.
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "common/result.h"

namespace openrac::json {
class Value;
}

namespace openrac::materials {
// Metadata value types; GPU consumers compile these into their existing typed binding records.
using Value = std::variant<bool, double, std::string, std::array<double, 3>>;
using Properties = std::map<std::string, Value, std::less<>>;

struct MaterialType {
    std::string_view id;
    std::string_view label;
};
inline constexpr MaterialType kMaterialTypes[] = {
    {"lit", "Lit"}, {"glass-lit", "Glass Lit"},
    {"lit-fur", "Lit Fur Surface"}, {"lit-wet-fur", "Lit Wet Fur Surface"},
    {"foliage", "Lit Foliage"}, {"original", "Original"},
};
// Expands a named definition with type-specific defaults; mesh textures and coverage stay separate.
[[nodiscard]] Properties material_definition(Properties properties);

// The editor widget and parser representation of a supported property.
enum class Kind {
    Boolean,
    Number,
    Colour,
    Text
};

// Shared validation and editor metadata; bounds apply to numbers and each colour channel.
struct Field {
    std::string_view path;   // Stable dotted JSON property name.
    std::string_view label;  // Human-readable editor label.
    Kind kind;               // Required value representation.
    double minimum;          // Inclusive numeric lower bound.
    double maximum;          // Inclusive numeric upper bound.
    bool transport;          // True when a bake consumer may use this property.
};

// Stable exported identity; no process-local image indices or instance addresses enter a profile.
struct Identity {
    std::string layer;              // terrain, tie, shrub or moby.
    int class_id = -1;              // Object type, or -1 for unclassified geometry.
    std::string material_name;      // Authored slot name within its model.
    std::string texture_hash;       // Decoded dimensions and RGBA SHA-256.
    std::string texture_file_hash;  // Legacy PNG SHA-256, retained during emission migration.
    std::string
        texture_path;     // Logical level-relative path, retained for explicit authored overrides.
    bool metal = false;   // Original reflective packet eligibility.
    bool cutout = false;  // Original alpha-tested coverage eligibility.
    std::string model;    // Exported model name, independent of its position in the level.
    std::optional<int> material_slot;  // glTF material index, or -1 for the implicit material.
};

// Conjunctive selector; unspecified fields impose no condition.
struct Selector {
    std::string layer;              // Empty applies across scene layers.
    std::optional<int> class_id;    // All instances of this object type.
    std::string material_name;      // Empty accepts every authored slot.
    std::string texture_hash;       // Empty accepts any decoded texture.
    std::string texture_file_hash;  // Legacy file identity, never guessed from colour.
    std::string
        texture_path;  // Optional exact exported path, independent of the resolved mod folder.
    std::string pass;  // Empty, colour or metal.
    bool cutout_only = false;          // Restricts the rule to original cutout materials.
    std::string model;                 // Optional exact exported model name.
    std::optional<int> material_slot;  // Exact glTF slot, including -1 for the implicit material.
};

// One named override; source and layer are assigned by the loader, never trusted from JSON.
struct Rule {
    std::string id;         // Stable authoring key used by merges and saves.
    std::string name;       // Human-readable label, falling back to id.
    Selector match;         // Every specified condition must match.
    std::string preset;     // Optional reusable property set.
    std::string material;   // Shared material assignment, replacing earlier shading properties.
    Properties properties;  // Field overrides applied after the preset.
    bool enabled = true;    // A higher-layer false disables a rule with the same id.
    std::string source;     // File responsible for this rule.
    std::map<std::string, int>
        property_layers;  // Independent source priority for sparse field edits.
    std::map<std::string, std::string> property_sources;  // Origins of inherited and edited fields.
    int preset_layer = 0;  // Layer that assigned the preset reference.
    int layer = 0;         // Increasing source precedence.
};

// CPU-only authoring catalog; every field retains its source for the inspector.
struct Catalog {
    std::map<std::string, Properties> presets;  // Named property sets; no inheritance chains.
    std::map<std::string, std::map<std::string, int>> preset_layers;  // Per-field source priority.
    std::map<std::string, std::map<std::string, std::string>> preset_sources;  // Field origins.
    std::map<std::string, Rule> rules;  // Stable rule ids, independent of JSON ordering.
};

// Fully resolved values and their provenance, compiled once per model/material/layer binding.
struct Resolved {
    std::string material;                         // Explicit shared material assigned to this slot.
    Properties properties;                       // Effective supported fields.
    std::map<std::string, std::string> sources;  // Effective field's source file.
    std::vector<std::string> rules;              // Matching enabled rule ids in precedence order.
};

// Hashes little-endian dimensions and decoded RGBA8 pixels; returns empty for malformed images.
[[nodiscard]] std::string texture_identity(
    int width, int height, std::span<const std::uint8_t> rgba
);
// Streams a source texture's legacy byte identity; reports missing and incomplete files explicitly.
[[nodiscard]] Result<std::string> file_identity(const std::filesystem::path& path);
// Reads a validated scalar as canonical FP32, independent of the calling thread's game rounding mode.
[[nodiscard]] float number_value(
    const Properties& properties, std::string_view key, float fallback
);
// Reads an optional validated boolean, preserving the supplied fallback when absent.
[[nodiscard]] bool bool_value(const Properties& properties, std::string_view key, bool fallback);
// Reads optional validated text; the returned view lives as long as the property map remains unchanged.
[[nodiscard]] std::string_view text_value(
    const Properties& properties, std::string_view key, std::string_view fallback = {}
);
// Reads optional linear colour metadata without changing missing channel defaults.
[[nodiscard]] std::array<float, 3> colour_value(
    const Properties& properties, std::string_view key, std::array<float, 3> fallback
);
// Returns the immutable supported property schema, shared with the inspector.
[[nodiscard]] std::span<const Field> fields();
// Returns metadata for one dotted field, or null for unknown properties.
[[nodiscard]] const Field* field(std::string_view path);
// Rejects mistyped, nonfinite, unsupported or out-of-range property values.
[[nodiscard]] Result<void> validate(const Properties& properties);
// Parses a complete catalog; errors include source and JSON field context.
[[nodiscard]] Result<Catalog> parse_catalog(std::string_view text, const std::string& source = {});
// Reads optional metadata; a missing file is an empty catalog, other errors are explicit.
[[nodiscard]] Result<Catalog> read_catalog(const std::filesystem::path& path);
// Merges a validated higher-priority source, preserving unspecified property fields.
void merge_catalog(Catalog& target, const Catalog& overlay, int layer);
// Resolves matching rules deterministically; equal-precedence conflicting writes are rejected.
[[nodiscard]] Result<Resolved> resolve(const Catalog& catalog, const Identity& identity);
// Returns only properties consumed by transport; runtime-only edits do not enter bake identity.
[[nodiscard]] Properties transport_properties(const Properties& properties);
// Writes a validated catalog atomically; a failed save preserves the old file.
[[nodiscard]] Result<void> write_catalog(const std::filesystem::path& path, const Catalog& catalog);
// Canonical transport-only authoring identity, including selector and per-field source precedence.
[[nodiscard]] Result<std::string> transport_catalog_stamp(const Catalog& catalog);
// Serializes a validated catalog in stable id/property order for saves and migration comparisons.
[[nodiscard]] Result<std::string> catalog_text(const Catalog& catalog);
}  // namespace openrac::materials
