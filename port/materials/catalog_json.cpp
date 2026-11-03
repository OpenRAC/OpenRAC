// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Strict material-catalog JSON parsing; no game assets, GPU state or per-frame filesystem work.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>

#include "common/json.h"
#include "materials/catalog.h"
#include "materials/metadata_internal.h"

namespace openrac::materials {
namespace {
// Metadata ceilings bound catalog parsing independently of mod contents.
constexpr std::size_t kMaximumRules = 4096;
constexpr std::uintmax_t kMaximumBytes = 2ull * 1024 * 1024;

// Reports structured authoring context; exceptions are caught by the public parser.
[[noreturn]] void fail(const std::string& context, const std::string& message) {
    throw std::runtime_error(context + ": " + message);
}

// Rejects unknown fields instead of silently losing misspelled settings during a save.
void keys(
    const json::Value& object,
    std::initializer_list<std::string_view> allowed,
    const std::string& context
) {
    if (!object.is_object()) {
        fail(context, "expected an object");
    }
    for (const auto& [key, value] : object.members()) {
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            fail(std::format("{}.{}", context, key), "unknown field");
        }
    }
}

// IDs remain small, stable authoring keys, independent of display labels and object addresses.
std::string identifier(const std::string& text, const std::string& context) {
    constexpr std::size_t kMaximumId = 128;
    if (text.empty() || text.size() > kMaximumId
        || text.find_first_not_of(
               "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-"
           ) != text.npos) {
        fail(
            context,
            "expected a nonempty material id using letters, digits, underscore, dot or hyphen"
        );
    }
    return text;
}

// Explicit hashes distinguish original assets from unrelated replacement artwork.
std::string digest(const json::Value& value, const std::string& context) {
    constexpr std::size_t kDigestLength = 64;
    const auto& text = value.string();
    if (!value.is_string() || text.size() != kDigestLength
        || text.find_first_not_of("0123456789abcdef") != text.npos) {
        fail(context, "expected a lowercase SHA-256 digest");
    }
    return text;
}

// Strings are checked before JSON's permissive default accessors can hide an invalid type.
std::string text(const json::Value& value, const std::string& context) {
    if (!value.is_string()) {
        fail(context, "expected text");
    }
    return value.string();
}

// Reads exactly three numeric channels before shared colour-range validation.
std::array<double, 3> colour(const json::Value& value, const std::string& context) {
    if (!value.is_array() || value.items().size() != 3) {
        fail(context, "expected three colour channels");
    }
    std::array<double, 3> channels{};
    for (std::size_t channel = 0; channel < channels.size(); ++channel) {
        if (!value[channel].is_number()) {
            fail(context, "expected numeric colour channels");
        }
        channels[channel] = value[channel].number();
    }
    return channels;
}

// The schema owns the accepted value kinds and all numeric ranges.
Value property(const json::Value& value, const Field& spec, const std::string& context) {
    switch (spec.kind) {
        // Authored checkboxes cannot be encoded as 0 or 1.
        case Kind::Boolean:
            if (value.type() != json::Value::Type::Bool) {
                fail(context, "expected a boolean");
            }
            return value.boolean();
        // Nonfinite and out-of-range values are rejected by shared validation below.
        case Kind::Number:
            if (!value.is_number()) {
                fail(context, "expected a number");
            }
            return value.number();
        // Linear RGB metadata has exactly three independently validated channels.
        case Kind::Colour:
            return colour(value, context);
        // Shader families and local references receive further schema validation.
        case Kind::Text:
            return text(value, context);
    }
    fail(context, "unsupported property type");
}

// Flattens the public two-level JSON blocks into stable property keys used by the resolver.
Properties properties(const json::Value& object, const std::string& context) {
    if (!object.is_object()) {
        fail(context, "expected material properties");
    }
    Properties result;
    for (const auto& [group, value] : object.members()) {
        const auto* direct = field(group);
        if (direct) {
            result.emplace(group, property(value, *direct, std::format("{}.{}", context, group)));
            continue;
        }
        if (!value.is_object()) {
            fail(std::format("{}.{}", context, group), "unknown material property group");
        }
        // Empty unknown groups are errors too, not silently accepted future settings.
        const std::string prefix = group + ".";
        if (std::none_of(fields().begin(), fields().end(), [&](const Field& candidate) {
                return candidate.path.starts_with(prefix);
            })) {
            fail(std::format("{}.{}", context, group), "unknown material property group");
        }
        for (const auto& [name, item] : value.members()) {
            const std::string key = prefix + name;
            const auto* spec = field(key);
            if (!spec) {
                fail(std::format("{}.{}", context, key), "unknown material property");
            }
            result.emplace(key, property(item, *spec, std::format("{}.{}", context, key)));
        }
    }
    const auto valid = validate(result);
    if (!valid) {
        fail(context, valid.error().message);
    }
    return result;
}

// Reads scene-layer and object-type constraints without introducing game-specific class constants.
void object_selector(Selector& result, const json::Value& object, const std::string& context) {
    if (!object["layer"].is_null()) {
        result.layer = text(object["layer"], context + ".layer");
        if (result.layer != "terrain" && result.layer != "tie" && result.layer != "shrub"
            && result.layer != "moby" && result.layer != "sky") {
            fail(context + ".layer", "expected terrain, tie, shrub, moby or sky");
        }
    }
    if (!object["class_id"].is_null()) {
        const auto& value = object["class_id"];
        const double number = value.number(-1);
        if (!value.is_number() || !std::isfinite(number) || number < 0
            || number > std::numeric_limits<int>::max() || std::floor(number) != number) {
            fail(context + ".class_id", "expected a nonnegative integer object type");
        }
        result.class_id = static_cast<int>(number);
    }
    if (!object["material_name"].is_null()) {
        result.material_name = text(object["material_name"], context + ".material_name");
    }
    // A slot index is meaningful within an explicitly named export.
    if (!object["model"].is_null()) {
        result.model = text(object["model"], context + ".model");
    }
    // The implicit glTF material uses -1 and is distinct from every authored slot.
    if (!object["material_slot"].is_null()) {
        const auto& value = object["material_slot"];
        const double number = value.number(-2);
        if (result.model.empty() || !value.is_number() || !std::isfinite(number) || number < -1
            || number > std::numeric_limits<int>::max() || std::floor(number) != number) {
            fail(
                context + ".material_slot", "expected a model and an integer slot of -1 or greater"
            );
        }
        result.material_slot = static_cast<int>(number);
    }
}

// Reads explicit texture identities; legacy paths remain logical names rather than resolved host paths.
void texture_selector(Selector& result, const json::Value& object, const std::string& context) {
    if (!object["texture_rgba_sha256"].is_null()) {
        result.texture_hash =
            digest(object["texture_rgba_sha256"], context + ".texture_rgba_sha256");
    }
    if (!object["texture_file_sha256"].is_null()) {
        result.texture_file_hash =
            digest(object["texture_file_sha256"], context + ".texture_file_sha256");
    }
    if (!object["texture_path"].is_null()) {
        result.texture_path = text(object["texture_path"], context + ".texture_path");
        const auto path = std::filesystem::path(
            std::u8string(result.texture_path.begin(), result.texture_path.end())
        );
        if (path.empty() || path.has_root_path()
            || result.texture_path.find("..") != std::string::npos
            || result.texture_path.find(':') != std::string::npos) {
            fail(context + ".texture_path", "expected a level-relative texture path");
        }
    }
}

// Coverage restrictions describe existing geometry, rather than overriding its depth or alpha policy.
void pass_selector(Selector& result, const json::Value& object, const std::string& context) {
    if (!object["pass"].is_null()) {
        result.pass = text(object["pass"], context + ".pass");
        if (result.pass != "metal" && result.pass != "colour") {
            fail(context + ".pass", "expected colour or metal");
        }
    }
    if (!object["cutout_only"].is_null()) {
        if (object["cutout_only"].type() != json::Value::Type::Bool) {
            fail(context + ".cutout_only", "expected a boolean");
        }
        result.cutout_only = object["cutout_only"].boolean();
    }
}

// Restricts layer/pass vocabulary while retaining exact exported material names.
Selector selector(const json::Value& object, const std::string& context) {
    keys(
        object,
        {"layer",
         "class_id",
         "material_name",
         "model",
         "material_slot",
         "texture_rgba_sha256",
         "texture_file_sha256",
         "texture_path",
         "pass",
         "cutout_only"},
        context
    );
    Selector result;
    object_selector(result, object, context);
    texture_selector(result, object, context);
    pass_selector(result, object, context);
    if (result.layer.empty() && !result.class_id && result.material_name.empty()
        && result.model.empty() && result.texture_hash.empty() && result.texture_file_hash.empty()
        && result.texture_path.empty() && result.pass.empty() && !result.cutout_only) {
        fail(context, "a material rule needs an explicit selector");
    }
    return result;
}

// Parses one stable rule; disabled tombstones may omit match/preset/properties.
Rule rule(const json::Value& object, const std::string& context, const std::string& source) {
    keys(object, {"id", "name", "match", "preset", "set", "enabled"}, context);
    Rule result;
    result.id = identifier(text(object["id"], context + ".id"), context + ".id");
    result.name = object["name"].is_null() ? result.id : text(object["name"], context + ".name");
    result.source = source;
    if (!object["enabled"].is_null()) {
        if (object["enabled"].type() != json::Value::Type::Bool) {
            fail(context + ".enabled", "expected a boolean");
        }
        result.enabled = object["enabled"].boolean();
    }
    if (result.enabled || !object["match"].is_null()) {
        result.match = selector(object["match"], context + ".match");
    }
    if (!object["preset"].is_null()) {
        result.preset =
            identifier(text(object["preset"], context + ".preset"), context + ".preset");
    }
    if (!object["set"].is_null()) {
        result.properties = properties(object["set"], context + ".set");
    }
    for (const auto& [key, value] : result.properties) {
        result.property_sources[key] = source;
    }
    return result;
}

// Reads the complete document before returning any externally visible authoring state.
Catalog document(const json::Value& root, const std::string& source) {
    keys(root, {"version", "presets", "overrides"}, "$");
    if (!root["version"].is_number() || root["version"].number() != 1) {
        fail("$.version", "expected material catalog version 1");
    }
    Catalog result;
    if (!root["presets"].is_null()) {
        if (!root["presets"].is_object() || root["presets"].members().size() > kMaximumRules) {
            fail("$.presets", "expected a bounded preset object");
        }
        for (const auto& [id, value] : root["presets"].members()) {
            identifier(id, "$.presets." + id);
            result.presets[id] = properties(value, "$.presets." + id);
            for (const auto& [key, property] : result.presets[id]) {
                result.preset_sources[id][key] = source;
            }
        }
    }
    if (!root["overrides"].is_null()) {
        if (!root["overrides"].is_array() || root["overrides"].items().size() > kMaximumRules) {
            fail("$.overrides", "expected a bounded rule array");
        }
        for (std::size_t index = 0; index < root["overrides"].items().size(); ++index) {
            auto entry = rule(
                root["overrides"][index], "$.overrides[" + std::to_string(index) + "]", source
            );
            if (!result.rules.emplace(entry.id, entry).second) {
                fail("$.overrides", "duplicate rule id " + entry.id);
            }
        }
    }
    return result;
}
}  // namespace

Result<Catalog> parse_catalog(std::string_view text, const std::string& source) {
    try {
        if (text.size() > kMaximumBytes) {
            return unexpected(Error{source + ": material catalog exceeds 2 MiB"});
        }
        const MetadataRounding rounding;
        json::Value root;
        std::string error;
        if (!json::parse(text, root, error, json::DuplicateKeys::Reject)) {
            return unexpected(Error{source + ": " + error});
        }
        return document(root, source);
    } catch (const std::exception& error) {
        return unexpected(Error{source + ": " + error.what()});
    }
}

Result<Catalog> read_catalog(const std::filesystem::path& path) {
    const auto utf8 = path.generic_u8string();
    const std::string source(utf8.begin(), utf8.end());
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        return unexpected(Error{source + ": " + error.message()});
    }
    if (!exists) {
        return Catalog{};
    }
    const std::uintmax_t bytes = std::filesystem::file_size(path, error);
    if (error || bytes > kMaximumBytes) {
        return unexpected(Error{source + ": unreadable catalog or larger than 2 MiB"});
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return unexpected(Error{source + ": cannot read material catalog"});
    }
    // Read at most the inspected length, detecting concurrent growth instead of unbounded allocation.
    std::string text(static_cast<std::size_t>(bytes), '\0');
    file.read(text.data(), static_cast<std::streamsize>(bytes));
    if (!file || file.peek() != std::char_traits<char>::eof()) {
        return unexpected(Error{source + ": material catalog changed during read"});
    }
    return parse_catalog(text, source);
}
}  // namespace openrac::materials
