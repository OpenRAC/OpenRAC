// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Validated, versioned JSON authoring with atomic replacement and explicit path-qualified errors.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>

#include "common/json.h"
#include "enhanced/lighting/object_lights.h"

namespace openrac::enhanced {
// Keep diagnostics readable even when Windows cannot represent a folder in its ANSI code page.
std::string object_light_path_text(const std::filesystem::path& path) {
    const auto text = path.generic_u8string();
    return {text.begin(), text.end()};
}

namespace {
// Keep limits identical to the production authored-light table, including finite HDR radiance.
Result<void> validate_light(const AttachedLight& light) {
    LocalLights validator;
    auto result = validator.set_lights(std::span(&light.light, 1));

    if (!result) {
        return result;
    }

    if (!std::isfinite(light.light.inner_angle) || !std::isfinite(light.light.outer_angle)
        || light.light.inner_angle < 0 || light.light.outer_angle < light.light.inner_angle
        || light.light.outer_angle >= 1.5707963f) {
        return unexpected(Error{"saved cone angles must satisfy 0 <= inner <= outer < pi/2"});
    }

    if (light.name.size() > 128 || light.bone.size() > 128) {
        return unexpected(Error{"light or bone name exceeds 128 bytes"});
    }

    return {};
}

// Missing optional vectors keep defaults; malformed present values never silently become zero.
Result<renderer::Vec3> vector(
    const json::Value& root, std::string_view key, renderer::Vec3 fallback
) {
    const auto& value = root[key];

    if (value.is_null()) {
        return fallback;
    }

    if (!value.is_array() || value.items().size() != 3) {
        return unexpected(Error{std::string(key) + " must have three numbers"});
    }

    for (std::size_t i = 0; i < 3; ++i) {
        if (!value[i].is_number() || !std::isfinite(value[i].number())) {
            return unexpected(Error{std::string(key) + " must be finite"});
        }

        fallback[i] = float(value[i].number());
    }

    return fallback;
}

// Strict optional numbers catch typos instead of accepting defaults for strings or booleans.
Result<float> number(const json::Value& root, std::string_view key, float fallback) {
    const auto& value = root[key];

    if (value.is_null()) {
        return fallback;
    }

    if (!value.is_number() || !std::isfinite(value.number())) {
        return unexpected(Error{std::string(key) + " must be a finite number"});
    }

    return float(value.number());
}

// Read numeric fields transactionally, with strict dimensional and finite-value checks.
Result<void> parse_light_fields(const json::Value& value, AttachedLight& out) {
    for (const auto& [key, target] : std::array<std::pair<const char*, renderer::Vec3*>, 3>{
             {{"position", &out.light.position},
              {"direction", &out.light.direction},
              {"colour", &out.light.colour}}
         }) {
        auto parsed = vector(value, key, *target);

        if (!parsed) {
            return unexpected(parsed.error());
        }

        *target = parsed.value();
    }

    for (const auto& [key, target] : std::array<std::pair<const char*, float*>, 4>{
             {{"intensity", &out.light.intensity},
              {"range", &out.light.range},
              {"inner_angle", &out.light.inner_angle},
              {"outer_angle", &out.light.outer_angle}}
         }) {
        auto parsed = number(value, key, *target);

        if (!parsed) {
            return unexpected(parsed.error());
        }

        *target = parsed.value();
    }

    return {};
}

// Reject misspelled fields before parsing authored values.
Result<void> validate_light_fields(const json::Value& value) {
    for (const auto& [key, unused] : value.members()) {
        const std::array<std::string_view, 11> known{
            "name",
            "bone",
            "type",
            "enabled",
            "position",
            "direction",
            "colour",
            "intensity",
            "range",
            "inner_angle",
            "outer_angle"
        };

        if (std::find(known.begin(), known.end(), key) == known.end()) {
            return unexpected(Error{"unknown light field: " + key});
        }
    }

    return {};
}

// Check identities and types before accepting a complete saved light.
Result<AttachedLight> parse_light(const json::Value& value) {
    if (!value.is_object()) {
        return unexpected(Error{"light must be a JSON object"});
    }

    const auto identity = validate_light_fields(value);

    if (!identity) {
        return unexpected(identity.error());
    }

    AttachedLight out;

    for (const char* key : {"name", "bone", "type"}) {
        if (!value[key].is_null() && !value[key].is_string()) {
            return unexpected(Error{std::string(key) + " must be a string"});
        }
    }

    if (!value["name"].is_null()) {
        out.name = value["name"].string();
    }

    out.bone = value["bone"].string();
    const auto type = value["type"].is_null() ? "point" : value["type"].string();

    if (type != "point" && type != "spot") {
        return unexpected(Error{"light type must be point or spot"});
    }

    out.light.spot = type == "spot";

    if (!value["enabled"].is_null() && value["enabled"].type() != json::Value::Type::Bool) {
        return unexpected(Error{"enabled must be boolean"});
    }

    out.enabled = value["enabled"].boolean(true);

    auto fields = parse_light_fields(value, out);

    if (!fields) {
        return unexpected(fields.error());
    }

    const auto valid = validate_light(out);

    if (!valid) {
        return unexpected(valid.error());
    }

    return out;
}

// JSON string escaping also handles control characters; std::quoted alone is not a JSON writer.
std::string json_quote(std::string_view text) {
    std::string out = "\"";
    constexpr char kHex[] = "0123456789abcdef";

    for (unsigned char c : text) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += char(c);
        } else if (c < 32) {
            out += "\\u00";
            out += kHex[c >> 4];
            out += kHex[c & 15];
        } else {
            out += char(c);
        }
    }

    return out + '"';
}

// Deterministic model ordering keeps authored diffs small; no retail geometry enters this file.
std::string catalog_text(const ObjectLightCatalog& catalog) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(9) << std::boolalpha;
    const auto vec = [&](const renderer::Vec3& v) {
        out << '[' << v[0] << ", " << v[1] << ", " << v[2] << ']';
    };
    out << "{\n  \"version\": 1,\n  \"objects\": {";
    bool first = true;

    for (const auto& [key, lights] : catalog.objects) {
        out << (first ? "\n" : ",\n") << "    " << json_quote(key) << ": [";
        first = false;

        for (std::size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            const auto& l = light.light;
            out << (i ? ",\n" : "\n") << "      {\"name\": " << json_quote(light.name)
                << ", \"type\": " << json_quote(l.spot ? "spot" : "point")
                << ", \"bone\": " << json_quote(light.bone) << ", \"enabled\": " << light.enabled
                << ", \"position\": ";
            vec(l.position);
            out << ", \"direction\": ";
            vec(l.direction);
            out << ", \"colour\": ";
            vec(l.colour);
            out << ", \"intensity\": " << l.intensity << ", \"range\": " << l.range
                << ", \"inner_angle\": " << l.inner_angle << ", \"outer_angle\": " << l.outer_angle
                << '}';
        }

        out << (lights.empty() ? "" : "\n    ") << ']';
    }

    out << "\n  }\n}\n";
    return out.str();
}

// Read native filesystem paths without narrowing through the active Windows code page.
Result<json::Value> read_profile_root(const std::filesystem::path& path, std::uintmax_t size) {
    std::ifstream file(path, std::ios::binary);
    std::string text(std::size_t(size), '\0');
    // Reading the exact checked size also detects replacement or truncation during the load.
    if (!file || !file.read(text.data(), std::streamsize(text.size()))
        || file.peek() != std::char_traits<char>::eof()) {
        return unexpected(Error{object_light_path_text(path) + ": cannot read complete profile"});
    }
    json::Value root;
    std::string why;
    if (!json::parse(text, root, why)) {
        return unexpected(Error{object_light_path_text(path) + ": " + why});
    }
    return root;
}

// Validate a complete profile independently from filesystem reads and publication.
Result<ObjectLightCatalog> parse_catalog(
    const json::Value& root, const std::filesystem::path& path
) {
    if (!root.is_object() || root["version"].number(-1) != 1 || !root["objects"].is_object()) {
        return unexpected(
            Error{object_light_path_text(path) + ": expected version 1 and an objects dictionary"}
        );
    }

    ObjectLightCatalog out;
    std::size_t count = 0;

    for (const auto& [key, value] : root["objects"].members()) {
        if (key.empty() || key.size() > 512 || !value.is_array()) {
            return unexpected(
                Error{object_light_path_text(path) + ": invalid object key or light array"}
            );
        }

        auto& lights = out.objects[key];

        for (const auto& entry : value.items()) {
            if (++count > 4096) {
                return unexpected(
                    Error{object_light_path_text(path) + ": profile exceeds 4096 authored lights"}
                );
            }

            auto light = parse_light(entry);

            if (!light) {
                return unexpected(
                    Error{object_light_path_text(path) + ": " + key + ": " + light.error().message}
                );
            }

            lights.push_back(std::move(light.value()));
        }
    }

    return out;
}

// Validate before creating any staging file, preserving previous data for malformed edits.
Result<void> validate_catalog(const ObjectLightCatalog& catalog) {
    std::size_t count = 0;

    for (const auto& [key, lights] : catalog.objects) {
        if (key.empty() || key.size() > 512) {
            return unexpected(Error{"invalid object light key"});
        }

        for (const auto& light : lights) {
            const auto valid = validate_light(light);

            if (!valid) {
                return unexpected(valid.error());
            }

            if (++count > 4096) {
                return unexpected(Error{"profile exceeds 4096 authored lights"});
            }
        }
    }

    return {};
}

// An exclusively created staging directory has exactly one cleanup owner.
class StagedProfile {
public:
    StagedProfile(std::filesystem::path file, std::filesystem::path dir)
        : m_file(std::move(file)),
          m_dir(std::move(dir)) {}

    StagedProfile(const StagedProfile&) = delete;
    StagedProfile& operator=(const StagedProfile&) = delete;
    StagedProfile(StagedProfile&&) = delete;
    StagedProfile& operator=(StagedProfile&&) = delete;

    ~StagedProfile() {
        std::error_code error;
        std::filesystem::remove(m_file, error);
        std::filesystem::remove(m_dir, error);
    }

private:
    std::filesystem::path m_file, m_dir;
};

}  // namespace

Result<ObjectLightCatalog> read_object_lights(const std::filesystem::path& path) {
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);

    if (error) {
        return unexpected(Error{object_light_path_text(path) + ": " + error.message()});
    }

    if (!exists) {
        return ObjectLightCatalog{};
    }

    // A profile is metadata, never an unbounded asset blob.
    const auto size = std::filesystem::file_size(path, error);

    if (error || size > 16ull * 1024 * 1024) {
        return unexpected(
            Error{object_light_path_text(path) + ": profile is unreadable or larger than 16 MiB"}
        );
    }

    auto parsed = read_profile_root(path, size);
    if (!parsed) {
        return unexpected(parsed.error());
    }
    return parse_catalog(parsed.value(), path);
}

Result<void> write_object_lights(
    const std::filesystem::path& path, const ObjectLightCatalog& catalog
) {
    auto valid = validate_catalog(catalog);

    if (!valid) {
        return valid;
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);

    if (error) {
        return unexpected(Error{object_light_path_text(path) + ": " + error.message()});
    }

    // Exclusive staging directories prevent two editor windows sharing a temporary filename.
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto staging = path.parent_path() / (".object-lights-" + std::to_string(stamp));

    if (!std::filesystem::create_directory(staging, error)) {
        return unexpected(
            Error{object_light_path_text(path) + ": cannot create private save staging"}
        );
    }

    const auto fresh = staging / "profile.json";

    StagedProfile cleanup(fresh, staging);
    std::ofstream out(fresh, std::ios::binary | std::ios::trunc);
    out << catalog_text(catalog);
    out.flush();
    const bool written = bool(out);
    out.close();

    if (!written || !out) {
        return unexpected(
            Error{
                object_light_path_text(path)
                + ": cannot write staged profile; previous file retained"
            }
        );
    }

    std::filesystem::rename(fresh, path, error);

    if (error) {
        return unexpected(
            Error{
                object_light_path_text(path)
                + ": cannot replace profile; previous file retained: " + error.message()
            }
        );
    }

    return {};
}
}  // namespace openrac::enhanced
