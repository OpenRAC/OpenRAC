// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// material_overrides.h's identity discovery and source-layer loading, without graphics API calls.
#include "scene/material_overrides.h"

#include <algorithm>
#include <charconv>
#include <exception>
#include <limits>
#include <set>

namespace openrac::scene {
namespace {
// Resolves model names written by the exporter; class numbers come from assets rather than game constants.
int class_number(std::string_view name) {
    name = name.substr(0, name.find_last_of('.'));
    const auto split = name.find('_');
    if (split == name.npos) {
        return -1;
    }
    int number = -1;
    const auto parsed = std::from_chars(name.data() + split + 1, name.data() + name.size(), number);
    return parsed.ec == std::errc{} && parsed.ptr == name.data() + name.size() ? number : -1;
}

// Uses stable layer vocabulary rather than the renderer's draw-order numbers.
std::string model_layer(std::string_view name) {
    if (name.starts_with("tie_")) {
        return "tie";
    }
    if (name.starts_with("shrub_")) {
        return "shrub";
    }
    if (name.starts_with("moby_")) {
        return "moby";
    }
    return name.starts_with("terrain") ? "terrain" : "sky";
}

// Records only models the existing bake geometry exporter can include in static transport.
std::set<std::uint32_t> transport_models(const LevelData& level) {
    std::set<std::uint32_t> result;
    for (std::size_t layer = static_cast<std::size_t>(Layer::Terrain);
         layer <= static_cast<std::size_t>(Layer::Mobys);
         ++layer) {
        for (const auto& instance : level.instances[layer]) {
            const bool moby = layer == static_cast<std::size_t>(Layer::Mobys);
            if (!moby
                || (instance.bake_static && !instance.stand_in && instance.palette < 0
                    && instance.blend == 0 && instance.tint[3] >= .99f
                    && level.models.at(instance.model).joints == 0)) {
                result.insert(instance.model);
            }
        }
    }
    for (const auto& instance : level.bake_surfaces) {
        result.insert(instance.model);
    }
    return result;
}

// Recovers a logical exported path independently of mod selection and the player's absolute folder.
std::string logical_texture_path(const LevelData& level, const std::filesystem::path& path) {
    auto roots = level.mod_roots;
    roots.push_back(level.overrides);
    roots.push_back(level.dir);
    for (const auto& root : roots) {
        if (root.empty()) {
            continue;
        }
        const auto relative = path.lexically_relative(root);
        if (!relative.empty() && *relative.begin() != "..") {
            const auto utf8 = relative.generic_u8string();
            return {utf8.begin(), utf8.end()};
        }
    }
    return {};
}

// Copies one surface identity without exposing renderer-local indices in authored rules.
materials::Identity material_identity(
    const LevelData& level,
    std::size_t model,
    int material,
    const std::vector<materials::Identity>& images
) {
    const auto& mesh = level.models[model];
    const auto& surface = level.materials[static_cast<std::size_t>(material)];
    materials::Identity identity;
    if (surface.image >= 0 && static_cast<std::size_t>(surface.image) < images.size()) {
        identity = images[static_cast<std::size_t>(surface.image)];
    }
    identity.layer = model_layer(mesh.name);
    identity.class_id = class_number(mesh.name);
    // The explicit table also covers aliases and replacement model filenames.
    for (const auto& [class_id, entry] : level.moby_classes) {
        if (!entry.second && entry.first == model) {
            identity.layer = "moby";
            identity.class_id = class_id;
            break;
        }
    }
    identity.material_name = surface.name;
    identity.model = mesh.name;
    identity.material_slot = surface.source_slot.value_or(material);
    identity.metal = surface.metal;
    identity.cutout = surface.cutout;
    return identity;
}

// Visits unique model slots once, excluding invalid geometry indices before caching identities.
void append_model_materials(
    const LevelData& level,
    std::size_t model,
    const std::vector<materials::Identity>& images,
    const std::set<std::uint32_t>& transport,
    MaterialAssets& assets
) {
    const auto& mesh = level.models[model];
    std::set<int> visited;
    for (const auto& primitive : mesh.primitives) {
        if (primitive.material < 0
            || static_cast<std::size_t>(primitive.material) >= level.materials.size()
            || !visited.insert(primitive.material).second) {
            continue;
        }
        assets.sources.push_back(
            {static_cast<std::uint32_t>(model),
             primitive.material,
             mesh.name,
             material_identity(level, model, primitive.material, images),
             transport.contains(static_cast<std::uint32_t>(model))}
        );
    }
}

// A synthetic slot exposes the existing shared water appearance without merging simulation geometry or state.
void append_water_material(MaterialAssets& assets) {
    Material water;
    water.name = "@water";
    const int slot = static_cast<int>(assets.originals.size());
    assets.originals.push_back(water);
    materials::Identity identity;
    identity.material_name = "@water";
    assets.sources.push_back(
        {std::numeric_limits<std::uint32_t>::max(),
         slot,
         "Water appearance",
         std::move(identity),
         false}
    );
}

// Fingerprints only requested texture representations, avoiding source PNG reads for decoded-only rules.
Result<std::vector<materials::Identity>> image_identities(
    const LevelData& level, const materials::Catalog& catalog
) {
    std::vector<materials::Identity> result(level.images.size());
    const bool legacy =
        std::any_of(catalog.rules.begin(), catalog.rules.end(), [](const auto& entry) {
            return !entry.second.match.texture_file_hash.empty();
        });
    for (std::size_t image = 0; image < level.images.size(); ++image) {
        const auto& pixels = level.images[image];
        result[image].texture_hash =
            materials::texture_identity(pixels.width, pixels.height, pixels.pixels);
        // Procedural images can be identified by decoded content without an exported file.
        if (image >= level.image_paths.size()) {
            continue;
        }
        const auto path = std::filesystem::path(
            std::u8string(level.image_paths[image].begin(), level.image_paths[image].end())
        );
        result[image].texture_path = logical_texture_path(level, path);
        if (!legacy) {
            continue;
        }
        const auto digest = materials::file_identity(path);
        if (!digest) {
            return unexpected(digest.error());
        }
        result[image].texture_file_hash = *digest;
    }
    return result;
}

// Loads only selected emission references; ordinary profiles reuse their immutable assets without a second material copy.
Result<std::shared_ptr<const MaterialAssets>> emission_images(
    LevelData& level,
    std::shared_ptr<const MaterialAssets> assets,
    const materials::Catalog& catalog
) {
    std::shared_ptr<MaterialAssets> extended;
    for (const auto& source : assets->sources) {
        const auto selected = materials::resolve(catalog, source.identity);
        if (!selected) {
            return unexpected(selected.error());
        }
        const auto texture = materials::text_value(selected->properties, "emission.texture");
        if (texture.empty() || assets->images.contains(std::string(texture))
            || (extended && extended->images.contains(std::string(texture)))) {
            continue;
        }
        const auto image = load_material_image(level, std::string(texture));
        if (!image) {
            return unexpected(image.error());
        }
        if (!extended) {
            extended = std::make_shared<MaterialAssets>(*assets);
        }
        extended->images[std::string(texture)] = *image;
        const auto& pixels = level.images[static_cast<std::size_t>(*image)];
        extended->image_hashes[*image] =
            materials::texture_identity(pixels.width, pixels.height, pixels.pixels);
    }
    return extended ? std::shared_ptr<const MaterialAssets>(std::move(extended))
                    : std::move(assets);
}

}  // namespace

Result<materials::Catalog> load_material_catalog(
    const LevelData& level, const std::filesystem::path& excluded
) {
    try {
        const auto name = "material-overrides.json";
        std::vector<std::filesystem::path> paths{
            level.dir / "materials/shared" / name,
            level.dir / "materials/version" / name,
            level.dir / "materials/level" / name,
            level.dir.parent_path() / name,
            level.dir / name
        };
        if (!level.overrides.empty()) {
            paths.push_back(level.overrides / name);
        }
        for (auto root = level.mod_roots.rbegin(); root != level.mod_roots.rend(); ++root) {
            paths.push_back(*root / name);
        }
        materials::Catalog result;
        int layer = 0;
        const auto legacy = legacy_material_edits(level);
        if (!legacy) {
            return unexpected(legacy.error());
        }
        for (const auto& path : paths) {
            // Imported legacy edits sit above tracked defaults and below explicit material authoring.
            if (path == level.dir.parent_path() / name) {
                materials::merge_catalog(result, *legacy, layer++);
            }
            if (path == excluded) {
                continue;
            }
            const auto source = materials::read_catalog(path);
            if (!source) {
                return unexpected(source.error());
            }
            materials::merge_catalog(result, *source, layer++);
        }
        return result;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

std::filesystem::path material_author_path(const LevelData& level) {
    if (!level.mod_roots.empty()) {
        return level.mod_roots.front() / "material-overrides.json";
    }
    if (!level.overrides.empty()) {
        return level.overrides / "material-overrides.json";
    }
    return level.dir / "material-overrides.json";
}

Result<std::shared_ptr<const MaterialAssets>> identify_materials(
    const LevelData& level, const materials::Catalog& catalog
) {
    try {
        const auto images = image_identities(level, catalog);
        if (!images) {
            return unexpected(images.error());
        }
        auto assets = std::make_shared<MaterialAssets>();
        assets->originals = level.materials;
        const auto transport = transport_models(level);
        for (std::size_t image = 0; image < images->size(); ++image) {
            assets->image_hashes[static_cast<int>(image)] = (*images)[image].texture_hash;
            if (!(*images)[image].texture_path.empty() && level.images[image].width > 0
                && level.images[image].height > 0) {
                assets->images[(*images)[image].texture_path] = static_cast<int>(image);
            }
        }
        for (std::size_t model = 0; model < level.models.size(); ++model) {
            append_model_materials(level, model, *images, transport, *assets);
        }
        append_water_material(*assets);
        return std::shared_ptr<const MaterialAssets>(std::move(assets));
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

void prepare_material_overrides(LevelData& level) {
    level.material_error.clear();
    level.material_assets.reset();
    level.material_bindings.reset();
    const auto catalog = load_material_catalog(level);
    if (!catalog) {
        level.material_error = catalog.error().message;
        return;
    }
    // Legacy exports and ordinary fixtures retain their original path without hashing every image.
    if (catalog->rules.empty() && catalog->presets.empty()) {
        return;
    }
    const auto assets = identify_materials(level, *catalog);
    if (!assets) {
        level.material_error = assets.error().message;
        return;
    }
    const auto extended = emission_images(level, *assets, *catalog);
    if (!extended) {
        level.material_error = extended.error().message;
        return;
    }
    const auto bindings = resolve_materials(*extended, *catalog);
    if (!bindings) {
        level.material_error = bindings.error().message;
        return;
    }
    level.material_assets = *extended;
    level.material_bindings = *bindings;
}

const MaterialBinding* MaterialBindings::find(std::uint32_t model, int material) const {
    const auto found = lookup.find({model, material});
    return found == lookup.end() ? nullptr : &bindings[found->second];
}

const Material& resolved_material(const LevelData& level, std::uint32_t model, int material) {
    if (level.material_bindings) {
        const auto* binding = level.material_bindings->find(model, material);
        if (binding) {
            return binding->surface();
        }
    }
    return level.materials.at(static_cast<std::size_t>(material));
}

bool MaterialBindings::original(std::uint32_t model, int material) const {
    const auto* binding = find(model, material);
    return binding && materials::text_value(binding->values.properties, "type") == "original";
}
}  // namespace openrac::scene
