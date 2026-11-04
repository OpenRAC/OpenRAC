// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic material-catalog JSON serialization; publication belongs to the authoring file owner.
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "common/staged_file.h"
#include "materials/catalog.h"
#include "materials/metadata_internal.h"

namespace openrac::materials {
namespace {
// Escapes JSON control bytes while retaining authored UTF-8 labels verbatim.
std::string json_quote(std::string_view text) {
    constexpr char kHex[] = "0123456789abcdef";
    std::string out = "\"";
    for (const char character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if (character == '\\' || character == '"') {
            out += '\\';
            out += character;
        } else if (byte < 32) {
            out += "\\u00";
            out += kHex[byte >> 4];
            out += kHex[byte & 15];
        } else {
            out += character;
        }
    }
    return out + "\"";
}

// Round-trips finite authoring doubles independently of locale or floating-point rounding mode.
std::string number(double value) {
    const MetadataRounding rounding;
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    if (!output) {
        throw std::runtime_error("cannot serialize material number");
    }
    return output.str();
}

// Writes the schema's finite set of property representations.
std::string value_text(const Value& value) {
    if (const auto* boolean = std::get_if<bool>(&value)) {
        return *boolean ? "true" : "false";
    }
    if (const auto* scalar = std::get_if<double>(&value)) {
        return number(*scalar);
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        return json_quote(*text);
    }
    const auto& colour = std::get<std::array<double, 3>>(value);
    return "[" + number(colour[0]) + ", " + number(colour[1]) + ", " + number(colour[2]) + "]";
}

// Separates object entries without trailing commas.
void member(std::string& output, std::string_view key, const std::string& value) {
    if (!output.empty()) {
        output += ", ";
    }
    output += json_quote(key) + ": " + value;
}

// Reconstructs schema blocks from flat runtime keys, preserving stable property ordering.
std::string properties_text(const Properties& properties) {
    std::map<std::string, std::string> groups;
    std::string root;
    for (const auto& [key, value] : properties) {
        const auto split = key.find('.');
        if (split == key.npos) {
            member(root, key, value_text(value));
        } else {
            member(groups[key.substr(0, split)], key.substr(split + 1), value_text(value));
        }
    }
    for (const auto& [key, contents] : groups) {
        member(root, key, "{" + contents + "}");
    }
    return "{" + root + "}";
}

// Emits only supplied selector constraints; original alpha and packet flags remain external identity.
std::string selector_text(const Selector& match) {
    std::string out;
    if (!match.layer.empty()) {
        member(out, "layer", json_quote(match.layer));
    }
    if (match.class_id) {
        member(out, "class_id", std::to_string(*match.class_id));
    }
    // Exact slot selectors retain their exported model and numeric slot on disk.
    if (!match.model.empty()) {
        member(out, "model", json_quote(match.model));
    }
    if (match.material_slot) {
        member(out, "material_slot", std::to_string(*match.material_slot));
    }
    if (!match.material_name.empty()) {
        member(out, "material_name", json_quote(match.material_name));
    }
    if (!match.texture_hash.empty()) {
        member(out, "texture_rgba_sha256", json_quote(match.texture_hash));
    }
    if (!match.texture_file_hash.empty()) {
        member(out, "texture_file_sha256", json_quote(match.texture_file_hash));
    }
    if (!match.texture_path.empty()) {
        member(out, "texture_path", json_quote(match.texture_path));
    }
    if (!match.pass.empty()) {
        member(out, "pass", json_quote(match.pass));
    }
    if (match.cutout_only) {
        member(out, "cutout_only", "true");
    }
    return "{" + out + "}";
}

// Serializes author fields only; provenance and resolved layer ranks are never persisted as input.
std::string rule_text(const Rule& rule) {
    std::string out;
    member(out, "id", json_quote(rule.id));
    member(out, "name", json_quote(rule.name));
    member(out, "enabled", rule.enabled ? "true" : "false");
    const auto match = selector_text(rule.match);
    if (match != "{}") {
        member(out, "match", match);
    }
    if (!rule.preset.empty()) {
        member(out, "preset", json_quote(rule.preset));
    }
    if (!rule.properties.empty()) {
        member(out, "set", properties_text(rule.properties));
    }
    return "{" + out + "}";
}
}  // namespace

Result<std::string> catalog_text(const Catalog& catalog) {
    try {
        std::string presets;
        for (const auto& [id, properties] : catalog.presets) {
            const auto valid = validate(properties);
            if (!valid) {
                return unexpected(valid.error());
            }
            member(presets, id, properties_text(properties));
        }
        std::string rules;
        for (const auto& [id, rule] : catalog.rules) {
            const auto valid = validate(rule.properties);
            if (!valid) {
                return unexpected(valid.error());
            }
            if (!rules.empty()) {
                rules += ",\n    ";
            }
            rules += rule_text(rule);
        }
        const std::string text = "{\n  \"version\": 1,\n  \"presets\": {" + presets
                                 + "},\n  \"overrides\": [\n    " + rules + "\n  ]\n}\n";
        const auto parsed = parse_catalog(text);
        if (!parsed) {
            return unexpected(parsed.error());
        }
        return text;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<void> write_catalog(const std::filesystem::path& path, const Catalog& catalog) {
    const auto text = catalog_text(catalog);
    if (!text) {
        return unexpected(text.error());
    }
    auto staged = StagedFile::create(path);
    if (!staged) {
        return unexpected(staged.error());
    }
    std::ofstream output(staged->path(), std::ios::binary | std::ios::trunc);
    output << *text;
    output.flush();
    const bool written = static_cast<bool>(output);
    output.close();
    // Neither an open, write nor close failure may publish incomplete authoring data.
    if (!written || !output) {
        return unexpected(Error{"cannot write staged material catalog; previous file retained"});
    }
    return staged->publish();
}
}  // namespace openrac::materials
