// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Foliage profile validation, original-texture matching and compact per-draw parameters.
#include "enhanced/renderer/foliage.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <set>
#include <vector>

#include "common/float_encoding.h"
#include "common/json.h"
#include "materials/catalog.h"
#include "renderer/gl.h"
#include "renderer/shader.h"

namespace openrac::enhanced {
namespace {
// Overrides follow the same priority as meshes and object-light profiles.
Result<std::filesystem::path> profile_path(const scene::LevelData& level) {
    std::vector<std::filesystem::path> roots = level.mod_roots;
    roots.push_back(level.overrides);
    roots.push_back(level.dir);
    // An empty root must not turn the process's working folder into an asset source.
    for (const auto& directory : roots) {
        if (directory.empty()) {
            continue;
        }
        std::error_code error;
        const auto path = directory / "foliage-materials.json";
        const bool found = std::filesystem::is_regular_file(path, error);
        // Missing optional profiles retain ordinary shading; other I/O errors must be explicit.
        if (error && error != std::errc::no_such_file_or_directory) {
            return unexpected(Error{"cannot inspect foliage-materials.json: " + error.message()});
        }
        if (found) {
            return path;
        }
    }
    return std::filesystem::path{};
}

// Full content hashes prevent unrelated cutouts and replacement textures from inheriting leaf shading.
bool valid_hash(const std::string& hash) {
    constexpr std::size_t kHashCharacters = 64;
    return hash.size() == kHashCharacters
           && std::all_of(hash.begin(), hash.end(), [](char character) {
                  return (character >= '0' && character <= '9')
                         || (character >= 'a' && character <= 'f');
              });
}

// A bounded optical field; the pointer belongs to the material passed to material_fields.
struct MaterialField {
    const char* name;  // JSON field name used in diagnostics.
    float* output;     // Material component being inspected.
    float minimum;     // Inclusive lower bound.
    float maximum;     // Inclusive upper bound.
};

// Keeps profile parsing and live-edit validation on the same four ranges.
std::array<MaterialField, 4> material_fields(FoliageMaterial& material) {
    return {
        {{"transmission", &material.transmission, 0, 1},
         {"thickness", &material.thickness, 0, 4},
         {"normal_bend", &material.normal_bend, 0, 1},
         {"roughness", &material.roughness, .35f, 1}}
    };
}

// Rejects the complete material before an edit can reach packed GPU parameters.
Result<void> validate_material(FoliageMaterial material) {
    // The shader requires finite, bounded optical energy and nonzero roughness.
    for (const auto& field : material_fields(material)) {
        const float number = *field.output;
        if (!std::isfinite(number) || number < field.minimum || number > field.maximum) {
            return unexpected(Error{std::string("invalid foliage ") + field.name});
        }
    }
    return {};
}

// Resolves every profile entry before activating any matches, so invalid data cannot partially apply.
Result<std::map<std::string, FoliageMaterial>> read_materials(const json::Value& document) {
    std::map<std::string, FoliageMaterial> result;
    // The version and array are required even for an intentionally empty catalog.
    if (!document.is_object() || document["version"].number() != 1
        || !document["materials"].is_array()) {
        return unexpected(Error{"foliage-materials.json requires version 1 and a materials array"});
    }
    for (const auto& entry : document["materials"].items()) {
        const std::string& hash = entry["texture_rgba_sha256"].string();
        const auto material = read_foliage_material(entry);
        // Duplicate entries are ambiguous even when their current values happen to agree.
        if (!valid_hash(hash) || result.contains(hash)) {
            return unexpected(
                Error{"invalid or duplicate texture_rgba_sha256 in foliage-materials.json"}
            );
        }
        if (!material) {
            return unexpected(material.error());
        }
        result.emplace(hash, *material);
    }
    return result;
}

// Matches distinct eligible images after the complete catalog has passed validation.
std::map<int, std::string> match_materials(
    const scene::LevelData& level, const std::map<std::string, FoliageMaterial>& materials
) {
    std::map<int, std::string> result;
    std::set<int> visited;
    // Hash only distinct cutout images; trunks, metal, particles and missing textures stay unchanged.
    for (const auto& material : level.materials) {
        if (!material.cutout || material.metal || material.blend || material.image < 0
            || static_cast<std::size_t>(material.image) >= level.images.size()
            || !visited.insert(material.image).second) {
            continue;
        }
        const auto hash =
            foliage_texture_hash(level.images[static_cast<std::size_t>(material.image)]);
        const auto found = materials.find(hash);
        if (found != materials.end()) {
            result.emplace(material.image, hash);
        }
    }
    return result;
}

// Retains author labels and per-texture reset values without keeping decoded image data.
std::vector<FoliageMaterialControls> profile_controls(
    const json::Value& document,
    const std::map<std::string, FoliageMaterial>& materials,
    const std::map<int, std::string>& images
) {
    std::vector<FoliageMaterialControls> result;
    // Labels and original controls travel with each texture so resets retain fern-specific normals.
    for (const auto& entry : document["materials"].items()) {
        const auto& hash = entry["texture_rgba_sha256"].string();
        const auto& material = materials.at(hash);
        const int matches =
            static_cast<int>(std::count_if(images.begin(), images.end(), [&](const auto& image) {
                return image.second == hash;
            }));
        const std::string name =
            entry["name"].string().empty() ? hash.substr(0, 8) : entry["name"].string();
        result.push_back({name, hash, material, material, matches});
    }
    return result;
}
}  // namespace

std::string foliage_texture_hash(const renderer::Rgba8Image& image) {
    return materials::texture_identity(image.width, image.height, image.pixels);
}

Result<FoliageMaterial> read_foliage_material(const json::Value& value) {
    // Scalars and arrays cannot define material parameters.
    if (!value.is_object()) {
        return unexpected(Error{"foliage material must be an object"});
    }
    FoliageMaterial material;

    // Defaults apply only to absent values, never to mistyped or nonfinite fields.
    for (const auto& field : material_fields(material)) {
        const auto& input = value[field.name];
        if (input.is_null()) {
            continue;
        }
        const float number = static_cast<float>(input.number());
        if (!input.is_number()) {
            return unexpected(Error{std::string("invalid foliage ") + field.name});
        }
        *field.output = number;
    }
    const auto valid = validate_material(material);
    if (!valid) {
        return unexpected(valid.error());
    }
    return material;
}

std::array<std::uint32_t, 2> pack_foliage_material(const FoliageMaterial& material) {
    // Each component is a bounded, nonnegative half with ten mantissa bits.
    const auto half = [](float value) {
        return float_encoding::pack_small_float(value, 10, false);
    };
    return {
        half(material.transmission) | (half(material.thickness) << 16),
        half(material.normal_bend) | (half(material.roughness) << 16)
    };
}

Result<void> FoliageMaterials::load(const scene::LevelData& level) {
    clear();
    const auto path = profile_path(level);
    if (!path) {
        return unexpected(path.error());
    }
    if (path->empty()) {
        return {};
    }
    // Material authoring is small metadata, not an unbounded data source.
    constexpr std::uintmax_t kMaximumProfileBytes = 1024ull * 1024ull;
    std::error_code size_error;
    const std::uintmax_t bytes = std::filesystem::file_size(*path, size_error);
    if (size_error || bytes > kMaximumProfileBytes) {
        return unexpected(Error{"foliage-materials.json is unreadable or exceeds 1 MiB"});
    }
    json::Value document;
    std::string error;
    if (!json::parse_file(path->string(), document, error)) {
        return unexpected(Error{"cannot read foliage-materials.json: " + error});
    }
    const auto materials = read_materials(document);
    if (!materials) {
        return unexpected(materials.error());
    }
    // Debug values must be integers; a missing master switch means enabled.
    const double debug = document["debug"].number();
    if ((!document["enabled"].is_null() && document["enabled"].type() != json::Value::Type::Bool)
        || (!document["debug"].is_null() && !document["debug"].is_number()) || !std::isfinite(debug)
        || debug < 0 || debug > 3 || std::floor(debug) != debug) {
        return unexpected(Error{"invalid enabled/debug control in foliage-materials.json"});
    }
    m_image_hashes = match_materials(level, *materials);
    m_controls.default_enabled = document["enabled"].boolean(true);
    m_controls.enabled = m_controls.default_enabled;
    m_controls.default_debug = static_cast<int>(debug);
    m_controls.debug = m_controls.default_debug;
    m_controls.materials = profile_controls(document, *materials, m_image_hashes);
    update_parameters();
    return {};
}

std::array<std::uint32_t, 2> FoliageMaterials::parameters(
    const scene::Material& material, bool active
) const {
    std::array<std::uint32_t, 2> words{};
    const auto found = m_materials.find(material.image);
    // Shared textures may also appear on non-leaf materials; coverage flags remain part of selection.
    if (active && m_controls.enabled && material.cutout && !material.metal && !material.blend
        && found != m_materials.end()) {
        words = found->second;
    }
    return words;
}

void FoliageMaterials::bind(
    const renderer::Shader& shader, const scene::Material& material, bool active
) const {
    const auto words = parameters(material, active);
    gl::glUniform1f(shader.uniform("foliage_reflectivity"), .04f);
    gl::glUniform1i(shader.uniform("foliage_parameters0"), std::bit_cast<std::int32_t>(words[0]));
    gl::glUniform1i(shader.uniform("foliage_parameters1"), std::bit_cast<std::int32_t>(words[1]));
    gl::glUniform1i(shader.uniform("foliage_debug"), active ? m_controls.debug : 0);
}

void FoliageMaterials::update_parameters() {
    m_materials.clear();
    // Texture hashing stays at level load; edits only repack matched image records.
    for (const auto& entry : m_controls.materials) {
        const auto words = pack_foliage_material(entry.material);
        for (const auto& [image, hash] : m_image_hashes) {
            if (hash == entry.texture_hash) {
                m_materials.emplace(image, words);
            }
        }
    }
}

Result<void> FoliageMaterials::set_controls(const FoliageControls& controls) {
    // Level changes must not apply edits to a different material catalog.
    if (controls.materials.size() != m_controls.materials.size() || controls.debug < 0
        || controls.debug > 3) {
        return unexpected(Error{"stale or invalid foliage controls"});
    }
    for (std::size_t n = 0; n < controls.materials.size(); ++n) {
        if (controls.materials[n].texture_hash != m_controls.materials[n].texture_hash) {
            return unexpected(Error{"stale foliage material identity"});
        }
        const auto valid = validate_material(controls.materials[n].material);
        if (!valid) {
            return unexpected(valid.error());
        }
    }
    m_controls.enabled = controls.enabled;
    m_controls.debug = controls.debug;
    // Names and reset values remain authoritative, even if a caller edited its snapshot metadata.
    for (std::size_t n = 0; n < controls.materials.size(); ++n) {
        m_controls.materials[n].material = controls.materials[n].material;
    }
    update_parameters();
    return {};
}

void FoliageMaterials::reset_controls() {
    m_controls.enabled = m_controls.default_enabled;
    m_controls.debug = m_controls.default_debug;
    // Each reviewed texture restores its own profile values, including zero-bend fern cards.
    for (auto& entry : m_controls.materials) {
        entry.material = entry.defaults;
    }
    update_parameters();
}

void FoliageMaterials::clear() {
    m_materials.clear();
    m_image_hashes.clear();
    m_controls = {};
}
}  // namespace openrac::enhanced
