// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/level.h"

#include <algorithm>
#include <cgltf.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>
#include <map>
#include <stb_image.h>
#include <stdexcept>
#include <vector>

#include "common/json.h"
#include "common/log.h"
#include "common/profiler.h"
#include "renderer/texture_pack.h"

namespace openrac::scene {

namespace fs = std::filesystem;

const char* layer_name(Layer layer) {
    switch (layer) {
        case Layer::Sky:
            return "sky";
        case Layer::Terrain:
            return "terrain";
        case Layer::Ties:
            return "ties";
        case Layer::Shrubs:
            return "shrubs";
        case Layer::Animated:
            return "animated props";
        case Layer::Mobys:
            return "mobys";
        case Layer::Count:
            break;
    }
    return "?";
}

namespace {

// A level file's path, or a mod's file in its place: the same path under one of
// LevelData::mod_roots, then under LevelData::overrides, when the file is there. `overridden`
// counts the files taken.
fs::path override_of(const fs::path& path, const LevelData& level, int* overridden) {
    if (level.overrides.empty() && level.mod_roots.empty()) {
        return path;
    }
    std::vector<fs::path> mods = level.mod_roots;
    if (!level.overrides.empty()) {
        mods.push_back(level.overrides);
    }
    // The path within the level: from the level's folder, or from a mod's (a mod's model names
    // the textures beside it, and the level's it does not replace, by the same relative path).
    const fs::path normal = path.lexically_normal();
    const auto inside = [](const fs::path& rel) { return !rel.empty() && *rel.begin() != ".."; };
    fs::path rel = normal.lexically_relative(level.dir.lexically_normal());
    for (std::size_t i = 0; i < mods.size() && !inside(rel); ++i) {
        rel = normal.lexically_relative(mods[i].lexically_normal());
    }
    if (!inside(rel)) {
        return path;
    }
    for (const fs::path& root : mods) {
        std::error_code ignored;
        if (fs::is_regular_file(root / rel, ignored)) {
            if (overridden != nullptr) {
                ++*overridden;
            }
            return (root / rel).lexically_normal();
        }
    }
    return (level.dir / rel).lexically_normal();
}

int image_index(const fs::path& requested, LevelData& level) {
    int overridden = 0;
    const std::string key =
        override_of(requested, level, &overridden).lexically_normal().generic_string();
    auto it = std::find(level.image_paths.begin(), level.image_paths.end(), key);
    if (it != level.image_paths.end()) {
        const auto index = static_cast<int>(it - level.image_paths.begin());
        return level.images[static_cast<std::size_t>(index)].width > 0 ? index : -1;
    }
    level.image_paths.push_back(key);
    level.overridden_images += overridden;
    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(key.c_str(), &w, &h, &channels, 4);
    renderer::Rgba8Image image;
    if (pixels != nullptr) {
        image = renderer::Rgba8Image(w, h);
        std::copy(pixels, pixels + static_cast<std::ptrdiff_t>(w) * h * 4, image.pixels.begin());
        stbi_image_free(pixels);
    } else {
        log::warn("texture {}: {}", key, stbi_failure_reason());
        ++level.missing_images;
    }
    // A texture pack's picture of it (docs/port/TEXTURE_PACKS.md), at any size.
    renderer::texture_pack::replace(image);
    bool translucent = false;
    for (std::size_t i = 3; i < image.pixels.size(); i += 4) {
        if (image.pixels[i] < 250) {
            translucent = true;
            break;
        }
    }
    level.images.push_back(std::move(image));
    level.translucent_images.push_back(translucent);
    return pixels != nullptr ? static_cast<int>(level.images.size() - 1) : -1;
}

// Shares equivalent decoded materials while retaining original emission intent for offline review.
int material_index(const cgltf_material* source, const fs::path& base, LevelData& level) {
    Material m;
    if (source != nullptr) {
        m.name = source->name ? source->name : "";
        m.cutout = source->alpha_mode == cgltf_alpha_mode_mask;
        m.metal = source->name != nullptr && std::strncmp(source->name, "metal_", 6) == 0;
        m.blend = source->alpha_mode == cgltf_alpha_mode_blend;
        const float strength =
            source->has_emissive_strength ? source->emissive_strength.emissive_strength : 1.0f;
        for (unsigned c = 0; c < 3; ++c) {
            m.emission[c] = source->emissive_factor[c] * strength;
            // Audits retain explicit material intent even when visible strength is zero.
            m.authored_emission = m.authored_emission || source->emissive_factor[c] > 0;
        }
        const auto* emission = source->emissive_texture.texture;
        if (emission && emission->image && emission->image->uri) {
            if (source->emissive_texture.texcoord != 0 || source->emissive_texture.has_transform) {
                throw std::runtime_error("probe emission requires untransformed TEXCOORD_0");
            }
            m.emission_image = image_index(base / emission->image->uri, level);
        }
        if (source->has_pbr_metallic_roughness) {
            const auto& pbr = source->pbr_metallic_roughness;
            std::copy(pbr.base_color_factor, pbr.base_color_factor + 4, m.colour.begin());
            const cgltf_texture* texture = pbr.base_color_texture.texture;
            if (texture != nullptr && texture->image != nullptr && texture->image->uri != nullptr) {
                m.image = image_index(base / texture->image->uri, level);
            }
            if (texture != nullptr && texture->sampler != nullptr) {
                m.clamp_s = texture->sampler->wrap_s == cgltf_wrap_mode_clamp_to_edge;
                m.clamp_t = texture->sampler->wrap_t == cgltf_wrap_mode_clamp_to_edge;
            }
        }
    }
    // A texture with texels below full alpha is drawn by the GS as the cutout materials are (the
    // static layers' and mobys' alpha test, GEQUAL 0x80 for the static layers and TEST 0x5360b for
    // mobys: texels at 0x80 written with depth, the others blended, colour only), whatever the
    // export's alpha mode says: the export marks a texture MASK only when it has fully clear texels
    // (editor/formats.py, Texture.cutout), so a sheet of glass, part-clear all over, came out
    // opaque. Kalebo III's glass sheets (moby class 800, alpha 0x32..0x53) were solid teal walls,
    // Novalis's windows by the plumber (a tie, glass at alpha 0x1F in an opaque frame) solid
    // blue-grey panes; part-clear terrain, tie and shrub textures of other levels (Veldin's,
    // alpha 0x3A all over) drew solid as well. The sky and the collision overlay are BLEND
    // materials and keep their own passes.
    if (!m.cutout && !m.metal && !m.blend && m.image >= 0
        && static_cast<std::size_t>(m.image) < level.translucent_images.size()) {
        m.cutout = level.translucent_images[static_cast<std::size_t>(m.image)];
    }
    for (std::size_t i = 0; i < level.materials.size(); ++i) {
        const Material& o = level.materials[i];
        // Shared materials must also agree on the emission intent retained for offline auditing.
        if (o.image == m.image && o.cutout == m.cutout && o.blend == m.blend && o.metal == m.metal
            && o.clamp_s == m.clamp_s && o.clamp_t == m.clamp_t && o.colour == m.colour
            && o.emission == m.emission && o.emission_image == m.emission_image
            && o.authored_emission == m.authored_emission && (!m.metal || o.name == m.name)) {
            return static_cast<int>(i);
        }
    }
    level.materials.push_back(m);
    return static_cast<int>(level.materials.size() - 1);
}

const cgltf_accessor* attribute(const cgltf_primitive& p, cgltf_attribute_type type) {
    for (cgltf_size i = 0; i < p.attributes_count; ++i) {
        if (p.attributes[i].type == type && p.attributes[i].index == 0) {
            return p.attributes[i].data;
        }
    }
    return nullptr;
}

bool read_matrix(const json::Value& value, Mat4& out) {
    if (value.items().size() != 16) {
        return false;
    }
    for (std::size_t i = 0; i < 16; ++i) {
        out[i] = static_cast<float>(value[i].number());
    }
    return true;
}

// A colour per class number, for boxes: the golden-ratio hue walk the
// editor's Godot export uses for its markers.
std::array<float, 4> class_colour(int class_id) {
    const double hue = std::fmod(class_id * 0.618034, 1.0) * 6.0;
    const double s = 0.65;
    const double v = 0.95;
    const int sector = static_cast<int>(hue) % 6;
    const double f = hue - std::floor(hue);
    const auto p = static_cast<float>(v * (1 - s));
    const auto q = static_cast<float>(v * (1 - s * f));
    const auto t = static_cast<float>(v * (1 - s * (1 - f)));
    const auto vv = static_cast<float>(v);
    switch (sector) {
        case 0:
            return {vv, t, p, 1.0f};
        case 1:
            return {q, vv, p, 1.0f};
        case 2:
            return {p, vv, t, 1.0f};
        case 3:
            return {p, q, vv, 1.0f};
        case 4:
            return {t, p, vv, 1.0f};
        default:
            return {vv, p, q, 1.0f};
    }
}

void grow(Vec3& low, Vec3& high, const Vec3& p) {
    for (int i = 0; i < 3; ++i) {
        low[i] = std::min(low[i], p[i]);
        high[i] = std::max(high[i], p[i]);
    }
}

// A tie's or shrub's draw distance as Ratchet & Clank's loader and renderer leave it
// (LevelData::draw_distances): `stored` is the placement's, `fade` the shrub class's billboard fade
// distance (-1 none). A placement without one is drawn as far as any.
float game_draw_distance(Layer layer, const json::Value& stored, int fade) {
    constexpr float kTieCap = 720.0f;    // TieProc's (draw_data.h kRac1Constants)
    constexpr float kShrubCap = 500.0f;  // ShrubProc's
    constexpr float kShrubMin = 16.0f;   // the loader's minimum
    constexpr float kFadeBand = 24.0f;   // a billboard class's: its fade distance + 24
    if (!stored.is_number()) {
        return layer == Layer::Ties ? kTieCap : kShrubCap;
    }
    const auto d = static_cast<float>(stored.number());
    if (layer == Layer::Ties) {
        return std::min(d, kTieCap);
    }
    const float raised = std::max(d, fade >= 0 ? static_cast<float>(fade) + kFadeBand : kShrubMin);
    return std::min(std::max(raised, kShrubMin), kShrubCap);
}

}  // namespace

std::optional<std::uint32_t> load_model(
    const fs::path& path, LevelData& level, std::string& error
) {
    cgltf_options options{};
    cgltf_data* data = nullptr;
    const std::string name = path.string();
    if (cgltf_parse_file(&options, name.c_str(), &data) != cgltf_result_success) {
        error = std::format("cannot read glTF {}", name);
        return std::nullopt;
    }
    if (cgltf_load_buffers(&options, data, name.c_str()) != cgltf_result_success) {
        cgltf_free(data);
        error = std::format("cannot read the buffers of {}", name);
        return std::nullopt;
    }
    const fs::path base = path.parent_path();
    Model model;
    model.name = path.filename().string();
    constexpr float kHuge = 3.0e38f;
    model.min = {kHuge, kHuge, kHuge};
    model.max = {-kHuge, -kHuge, -kHuge};

    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        if (node.mesh == nullptr) {
            continue;
        }
        // A skinned mesh is stored in its bind pose: its vertices as stored, with their skin.
        Mat4 world = renderer::identity();
        if (node.skin == nullptr) {
            cgltf_node_transform_world(&node, world.data());
        }
        // The skin's joints as the game numbers them (the exporter names them joint_NNN).
        std::vector<std::uint8_t> skin_joints;
        if (node.skin != nullptr) {
            for (cgltf_size j = 0; j < node.skin->joints_count; ++j) {
                const cgltf_node* joint = node.skin->joints[j];
                int number = static_cast<int>(j);
                if (joint != nullptr && joint->name != nullptr
                    && std::strncmp(joint->name, "joint_", 6) == 0) {
                    number = std::atoi(joint->name + 6);
                }
                skin_joints.push_back(static_cast<std::uint8_t>(std::clamp(number, 0, 255)));
            }
        }
        for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
            const cgltf_primitive& prim = node.mesh->primitives[p];
            if (prim.type != cgltf_primitive_type_triangles) {
                continue;
            }
            const cgltf_accessor* positions = attribute(prim, cgltf_attribute_type_position);
            if (positions == nullptr) {
                continue;
            }
            const cgltf_accessor* normals = attribute(prim, cgltf_attribute_type_normal);
            const cgltf_accessor* uvs = attribute(prim, cgltf_attribute_type_texcoord);
            const cgltf_accessor* colours = attribute(prim, cgltf_attribute_type_color);
            const cgltf_accessor* light_slots = nullptr;
            for (cgltf_size a = 0; a < prim.attributes_count; ++a) {
                if (prim.attributes[a].name != nullptr
                    && std::strcmp(prim.attributes[a].name, "_LIGHT_SLOT") == 0) {
                    light_slots = prim.attributes[a].data;
                }
            }
            const cgltf_accessor* joints =
                skin_joints.empty() ? nullptr : attribute(prim, cgltf_attribute_type_joints);
            const cgltf_accessor* weights =
                skin_joints.empty() ? nullptr : attribute(prim, cgltf_attribute_type_weights);
            const auto base_vertex = static_cast<std::uint32_t>(level.vertices.size());
            for (cgltf_size i = 0; i < positions->count; ++i) {
                Vertex v{};
                Vec3 position{};
                cgltf_accessor_read_float(positions, i, position.data(), 3);
                position = renderer::transform_point(world, position);
                std::copy(position.begin(), position.end(), v.position);
                grow(model.min, model.max, position);
                Vec3 normal{0.0f, 0.0f, 1.0f};
                if (normals != nullptr) {
                    cgltf_accessor_read_float(normals, i, normal.data(), 3);
                    Mat4 rotation = world;
                    rotation[12] = rotation[13] = rotation[14] = 0.0f;
                    normal = renderer::normalize(renderer::transform_point(rotation, normal));
                }
                std::copy(normal.begin(), normal.end(), v.normal);
                if (uvs != nullptr) {
                    cgltf_accessor_read_float(uvs, i, v.uv, 2);
                }
                v.colour[0] = v.colour[1] = v.colour[2] = v.colour[3] = 1.0f;
                if (colours != nullptr) {
                    cgltf_accessor_read_float(
                        colours, i, v.colour, colours->type == cgltf_type_vec3 ? 3 : 4
                    );
                }
                v.light_slot = -1.0f;
                if (light_slots != nullptr) {
                    cgltf_accessor_read_float(light_slots, i, &v.light_slot, 1);
                }
                if (joints != nullptr && weights != nullptr) {
                    cgltf_uint js[4] = {0, 0, 0, 0};
                    cgltf_accessor_read_uint(joints, i, js, 4);
                    cgltf_accessor_read_float(weights, i, v.weights, 4);
                    for (int k = 0; k < 4; ++k) {
                        v.joints[k] = js[k] < skin_joints.size() ? skin_joints[js[k]] : 0;
                        if (js[k] >= skin_joints.size()) {
                            v.weights[k] = 0.0f;
                        } else if (v.weights[k] > 0.0f) {
                            model.joints = std::max(model.joints, v.joints[k] + 1);
                        }
                    }
                }
                level.vertices.push_back(v);
            }
            Primitive out;
            out.first_index = static_cast<std::uint32_t>(level.indices.size());
            if (prim.indices != nullptr) {
                for (cgltf_size i = 0; i < prim.indices->count; ++i) {
                    level.indices.push_back(
                        base_vertex
                        + static_cast<std::uint32_t>(cgltf_accessor_read_index(prim.indices, i))
                    );
                }
            } else {
                for (cgltf_size i = 0; i < positions->count; ++i) {
                    level.indices.push_back(base_vertex + static_cast<std::uint32_t>(i));
                }
            }
            out.index_count = static_cast<std::uint32_t>(level.indices.size()) - out.first_index;
            try {
                out.material = material_index(prim.material, base, level);
            } catch (const std::runtime_error& failure) {
                error = std::format("{}: {}", name, failure.what());
                cgltf_free(data);
                return std::nullopt;
            }
            if (node.name != nullptr && std::strncmp(node.name, "Sky_", 4) == 0) {
                out.shell = std::atoi(node.name + 4);
            }
            if (node.name != nullptr && std::strncmp(node.name, "Terrain_", 8) == 0) {
                char* end = nullptr;
                const long fragment = std::strtol(node.name + 8, &end, 10);
                if (end != node.name + 8 && *end == 0 && fragment >= 0 && fragment < 1000000) {
                    out.terrain_fragment = static_cast<int>(fragment);
                }
            }
            model.primitives.push_back(out);
        }
    }
    cgltf_free(data);
    if (model.primitives.empty()) {
        model.min = model.max = Vec3{};
    }
    level.models.push_back(std::move(model));
    return static_cast<std::uint32_t>(level.models.size() - 1);
}

std::uint32_t add_box_model(LevelData& level) {
    Model model;
    model.name = "box";
    model.min = {-0.25f, -0.25f, 0.0f};
    model.max = {0.25f, 0.25f, 0.5f};
    Material plain;
    level.materials.push_back(plain);
    const int material = static_cast<int>(level.materials.size() - 1);
    // Six faces, each its own four corners so every face has its normal.
    const float faces[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    Primitive prim;
    prim.first_index = static_cast<std::uint32_t>(level.indices.size());
    prim.material = material;
    for (const auto& n : faces) {
        // Two axes across the face.
        const Vec3 normal{n[0], n[1], n[2]};
        const Vec3 a = std::abs(n[2]) > 0.5f ? Vec3{1, 0, 0} : Vec3{0, 0, 1};
        const Vec3 b = renderer::cross(normal, a);
        const auto first = static_cast<std::uint32_t>(level.vertices.size());
        for (int corner = 0; corner < 4; ++corner) {
            const float u = (corner & 1) != 0 ? 1.0f : -1.0f;
            const float w = (corner & 2) != 0 ? 1.0f : -1.0f;
            Vertex v{};
            v.light_slot = -1.0f;
            for (int i = 0; i < 3; ++i) {
                v.position[i] = 0.25f * (normal[i] + u * a[i] + w * b[i]);
                v.normal[i] = normal[i];
            }
            v.position[2] += 0.25f;
            v.uv[0] = (u + 1.0f) / 2.0f;
            v.uv[1] = (w + 1.0f) / 2.0f;
            v.colour[0] = v.colour[1] = v.colour[2] = v.colour[3] = 1.0f;
            level.vertices.push_back(v);
        }
        for (std::uint32_t i : {0u, 1u, 2u, 1u, 3u, 2u}) {
            level.indices.push_back(first + i);
        }
    }
    prim.index_count = static_cast<std::uint32_t>(level.indices.size()) - prim.first_index;
    model.primitives.push_back(prim);
    level.models.push_back(std::move(model));
    return static_cast<std::uint32_t>(level.models.size() - 1);
}

std::vector<renderer::Rgba8Image> load_sky_textures(
    const fs::path& dir, const fs::path& overrides
) {
    std::vector<renderer::Rgba8Image> out;
    LevelData level;
    level.dir = dir;
    level.overrides = overrides;
    for (int i = 0; i < 64; ++i) {
        const fs::path original = dir / "textures" / std::format("sky_{:04d}.png", i);
        if (!fs::exists(original)) {
            break;
        }
        // The sprites' corners take the whole picture (0..1), so a replacement may be any size.
        const fs::path path = override_of(original, level, nullptr);
        int w = 0;
        int h = 0;
        int channels = 0;
        renderer::Rgba8Image image;
        if (unsigned char* pixels = stbi_load(path.string().c_str(), &w, &h, &channels, 4)) {
            image = renderer::Rgba8Image(w, h);
            std::copy(
                pixels, pixels + static_cast<std::ptrdiff_t>(w) * h * 4, image.pixels.begin()
            );
            stbi_image_free(pixels);
        }
        out.push_back(std::move(image));
    }
    return out;
}

Vec3 bounds_centre(const Model& model, const Mat4& matrix) {
    return renderer::transform_point(
        matrix,
        {(model.min[0] + model.max[0]) * 0.5f,
         (model.min[1] + model.max[1]) * 0.5f,
         (model.min[2] + model.max[2]) * 0.5f}
    );
}

bool load_level(
    const fs::path& dir,
    LevelData& out,
    std::string& error,
    const fs::path& edited,
    const std::vector<fs::path>& mod_roots
) {
    const profiling::Zone trace("Load level assets");

    out = LevelData{};
    out.dir = dir;
    for (const fs::path& root : mod_roots) {
        std::error_code ignored;
        if (fs::is_directory(root, ignored)) {
            out.mod_roots.push_back(fs::absolute(root, ignored));
        }
    }
    if (!edited.empty()) {
        std::error_code ignored;
        if (fs::is_directory(edited.parent_path(), ignored)) {
            out.overrides = fs::absolute(edited.parent_path(), ignored);
        }
    }
    json::Value manifest;
    if (!json::parse_file((dir / "manifest.json").string(), manifest, error)) {
        return false;
    }
    out.game = manifest["game"].string();
    const int format = static_cast<int>(manifest["format"].number(-1));
    if (format != kManifestFormat && format != kManifestFormatNewest) {
        // Newer levels than the game: the game is the old part, and reading the levels again
        // would not help.
        error = format > kManifestFormatNewest ? std::format(
                                                     "{}: manifest format {}, newer than this game "
                                                     "reads ({}): build the game again",
                                                     dir.string(),
                                                     format,
                                                     kManifestFormatNewest
                                                 )
                                               : std::format(
                                                     "{}: manifest format {} (this viewer reads {})",
                                                     dir.string(),
                                                     format,
                                                     kManifestFormatNewest
                                                 );
        return false;
    }
    out.level = static_cast<int>(manifest["level"].number(-1));
    // Optional extension: older exports remain readable and draw conservatively.
    json::Value pvs;
    std::string pvs_error;
    if (manifest["game"].string() == "rac1"
        && json::parse_file((dir / "pvs.json").string(), pvs, pvs_error)
        && pvs["format"].number() == 1 && pvs["game"].string() == "rac1"
        && pvs["version"].string() == manifest["version"].string()
        && pvs["level"].number(-1) == out.level && pvs["grid"].string() == "pvs.bin") {
        std::ifstream file(dir / "pvs.bin", std::ios::binary | std::ios::ate);
        const auto size = file.tellg();
        if (file && size > 0 && size <= 64 * 1024 * 1024) {
            out.pvs.grid.resize(static_cast<std::size_t>(size));
            file.seekg(0);
            if (!file.read(reinterpret_cast<char*>(out.pvs.grid.data()), size)) {
                out.pvs.grid.clear();
            }
        }
    }

    auto model_at = [&](const json::Value& file) -> std::optional<std::uint32_t> {
        if (!file.is_string()) {
            return std::nullopt;
        }
        return load_model(
            override_of(dir / file.string(), out, &out.overridden_models), out, error
        );
    };

    for (const json::Value& set : manifest["lights"].items()) {
        for (const json::Value& v : set.items()) {
            for (const json::Value& x : v.items()) {
                out.light_sets.push_back(static_cast<float>(x.number(0)));
            }
        }
    }
    if (out.light_sets.size() != 16 * 16) {
        out.light_sets.clear();
    }
    const json::Value& sky = manifest["sky"];
    if (sky.is_object()) {
        if (sky["panorama"].is_string()) {
            const auto path =
                override_of(dir / sky["panorama"].string(), out, &out.overridden_images);
            int width = 0, height = 0, channels = 0;
            if (auto* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4)) {
                out.sky_panorama = renderer::Rgba8Image(width, height);
                std::copy_n(
                    pixels, out.sky_panorama.pixels.size(), out.sky_panorama.pixels.begin()
                );
                stbi_image_free(pixels);
            } else {
                ++out.missing_images;
                log::warn("sky panorama {}: {}", path.string(), stbi_failure_reason());
            }
        }
        for (std::size_t i = 0; i < 3; ++i) {
            out.background[i] = static_cast<float>(sky["background"][i].number()) / 255.0f;
        }
        if (sky["mesh"].is_string()) {
            auto model = model_at(sky["mesh"]);
            if (!model) {
                return false;
            }
            out.instances[static_cast<std::size_t>(Layer::Sky)].push_back(
                {*model, renderer::identity(), {1, 1, 1, 1}}
            );
        }
    }
    if (manifest["terrain"].is_string()) {
        auto model = model_at(manifest["terrain"]);
        if (!model) {
            return false;
        }
        for (auto& primitive : out.models[*model].primitives) {
            if (primitive.terrain_fragment >= 0) {
                primitive.occlusion = static_cast<std::uint16_t>(
                    pvs["terrain"][static_cast<std::size_t>(primitive.terrain_fragment)].number(
                        Pvs::always
                    )
                );
            }
        }
        out.instances[static_cast<std::size_t>(Layer::Terrain)].push_back(
            {*model, renderer::identity(), {1, 1, 1, 1}}
        );
    }

    // Classes: one model each.
    struct ClassModel {
        std::uint32_t model;
        float scale;
        bool box;
    };

    std::map<int, ClassModel> ties;
    std::map<int, ClassModel> shrubs;
    std::map<int, ClassModel> mobys;
    // The shrub classes' billboard fade distances ("billboard_fade", -1 none), when every class
    // has one; and the game's caps, known for Ratchet & Clank's (renderer/world/draw_data.h).
    std::map<int, int> billboard_fades;
    bool fades_known = true;
    const bool known_caps = manifest["game"].string() == "rac1";
    std::optional<std::uint32_t> box;
    const json::Value& classes = manifest["classes"];
    const std::pair<const char*, std::map<int, ClassModel>*> families[] =
        {{"tie", &ties}, {"shrub", &shrubs}, {"moby", &mobys}};
    for (const auto& [family, table] : families) {
        for (const auto& [key, cls] : classes[family].members()) {
            const int id = std::atoi(key.c_str());
            const auto scale = static_cast<float>(cls["scale"].number(1.0));
            if (table == &shrubs) {
                fades_known = fades_known && cls["billboard_fade"].is_number();
                billboard_fades[id] = static_cast<int>(cls["billboard_fade"].number(-1));
            }
            if (cls["mesh"].is_string()) {
                auto model = model_at(cls["mesh"]);
                if (!model) {
                    return false;
                }
                (*table)[id] = {*model, scale, false};
            } else {
                if (!box) {
                    box = add_box_model(out);
                }
                (*table)[id] = {*box, 1.0f, true};
            }
        }
    }

    // A mod's classes.json (its assets' level_NN, MODS.md section 3) adds classes, or replaces the
    // level's of the same id, in the manifest's "classes" form; its meshes are read from the mod's
    // folder by the same path. The last mod in the list first, so the first wins.
    for (auto root = out.mod_roots.rbegin(); root != out.mod_roots.rend(); ++root) {
        std::error_code ignored;
        const fs::path file = *root / "classes.json";
        if (!fs::is_regular_file(file, ignored)) {
            continue;
        }
        json::Value added;
        if (!json::parse_file(file.string(), added, error)) {
            error = std::format("{}: {}", file.string(), error);
            return false;
        }
        for (const auto& [family, table] : families) {
            for (const auto& [key, cls] : added[family].members()) {
                if (!cls["mesh"].is_string()) {
                    error = std::format("{}: {} class {} has no mesh", file.string(), family, key);
                    return false;
                }
                auto model = model_at(cls["mesh"]);
                if (!model) {
                    return false;
                }
                (*table)[std::atoi(key.c_str())] = {
                    *model, static_cast<float>(cls["scale"].number(1.0)), false
                };
                if (table == &shrubs) {
                    billboard_fades[std::atoi(key.c_str())] =
                        static_cast<int>(cls["billboard_fade"].number(-1));
                    fades_known = fades_known && cls["billboard_fade"].is_number();
                }
            }
        }
    }

    for (const auto& [id, cls] : mobys) {
        out.moby_classes[id] = {cls.model, cls.box};
    }

    json::Value emission_edits;
    const auto emission_file = override_of(dir / "probe-emission.json", out, nullptr);
    if (fs::exists(emission_file)
        && !json::parse_file(emission_file.string(), emission_edits, error)) {
        return false;
    }
    std::vector<int> emission_statics;
    const auto& static_classes = emission_edits["static_mobys"];
    // Source profiles may opt reviewed scenery into the bake, but cannot invent missing classes.
    if (!static_classes.is_null() && !static_classes.is_array()) {
        error = "static_mobys must be an array of loaded class IDs";
        return false;
    }
    // Only reviewed classes present in this export can opt their scenery into transport.
    for (const auto& item : static_classes.items()) {
        const double number = item.number(-1);
        // Reject malformed IDs before converting to the instance class type.
        if (!item.is_number() || !std::isfinite(number) || number < 0
            || number > std::numeric_limits<int>::max() || std::floor(number) != number
            || !mobys.contains(static_cast<int>(number))) {
            error = "invalid loaded class ID in static_mobys";
            return false;
        }
        emission_statics.push_back(static_cast<int>(number));
    }

    const std::string placements_file =
        manifest["placements"].is_string() ? manifest["placements"].string() : "placements.json";
    json::Value placements;
    if (!json::parse_file((dir / placements_file).string(), placements, error)) {
        return false;
    }
    // A mod's placements: the lists it has stand for the level's own.
    json::Value edits;
    if (!edited.empty() && fs::exists(edited) && !json::parse_file(edited.string(), edits, error)) {
        return false;
    }
    // Creator-authored spinning scenery lives in the mod folder. The retail manifests and
    // placements do not know about this optional, mod-only stream.
    if (!out.overrides.empty()) {
        const fs::path animation_file = out.overrides / "animated.json";
        std::error_code file_error;
        if (fs::is_regular_file(animation_file, file_error)) {
            json::Value animation;
            if (!json::parse_file(animation_file.string(), animation, error)) {
                return false;
            }
            if (animation["format"].number(-1) != 1 || !animation["objects"].is_array()) {
                error = "animated.json must use format 1 and contain an objects array";
                return false;
            }
            if (animation["objects"].items().size()
                > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                error = "animated.json exceeds the renderer's signed instance-count limit";
                return false;
            }
            std::map<std::string, std::uint32_t> model_cache;
            const fs::path mod_root = fs::canonical(out.overrides, file_error);
            if (file_error) {
                error = "cannot resolve the mod folder for animated.json";
                return false;
            }
            for (const json::Value& object : animation["objects"].items()) {
                if (!object.is_object()) {
                    error = "animated.json objects must be objects";
                    return false;
                }
                const std::string& model_uri = object["model"].string();
                const fs::path relative(model_uri);
                if (model_uri.empty() || relative.is_absolute() || relative.has_root_name()
                    || std::any_of(relative.begin(), relative.end(), [](const fs::path& part) {
                           return part == "..";
                       })) {
                    error = "animated object model must be a relative path inside the mod folder";
                    return false;
                }
                const fs::path model_path = fs::canonical(mod_root / relative, file_error);
                if (file_error || !fs::is_regular_file(model_path, file_error)) {
                    error = std::format("animated object model {} is missing", model_uri);
                    return false;
                }
                const fs::path contained = model_path.lexically_relative(mod_root);
                if (contained.empty() || contained.is_absolute()
                    || std::any_of(contained.begin(), contained.end(), [](const fs::path& part) {
                           return part == "..";
                       })) {
                    error = "animated object model resolves outside the mod folder";
                    return false;
                }
                std::uint32_t model_index = 0;
                const std::string cache_key = model_path.string();
                if (const auto found = model_cache.find(cache_key); found != model_cache.end()) {
                    model_index = found->second;
                } else {
                    const auto loaded = load_model(model_path, out, error);
                    if (!loaded) {
                        return false;
                    }
                    model_index = *loaded;
                    model_cache.emplace(cache_key, model_index);
                }
                const json::Value& matrix = object["matrix"];
                if (!matrix.is_array() || matrix.items().size() != 16) {
                    error = "animated object matrix must contain 16 numbers";
                    return false;
                }
                Instance instance;
                instance.model = model_index;
                for (std::size_t i = 0; i < instance.matrix.size(); ++i) {
                    const json::Value& value = matrix[i];
                    const double number = value.number(std::numeric_limits<double>::quiet_NaN());
                    if (!value.is_number() || !std::isfinite(number) || std::abs(number) > 1.0e6) {
                        error = "animated object matrix contains an invalid value";
                        return false;
                    }
                    instance.matrix[i] = static_cast<float>(number);
                }
                const json::Value& axis = object["spin_axis"];
                if (!axis.is_array() || axis.items().size() != 3
                    || !object["spin_speed"].is_number()) {
                    error = "animated object needs a three-number spin_axis and spin_speed";
                    return false;
                }
                Vec3 spin_axis{};
                for (std::size_t i = 0; i < spin_axis.size(); ++i) {
                    spin_axis[i] = static_cast<float>(
                        axis[i].number(std::numeric_limits<double>::quiet_NaN())
                    );
                    if (!std::isfinite(spin_axis[i])) {
                        error = "animated object spin_axis contains an invalid value";
                        return false;
                    }
                }
                const float axis_length = renderer::dot(spin_axis, spin_axis);
                if (!std::isfinite(axis_length) || axis_length < 1.0e-8f) {
                    error = "animated object spin_axis must have non-zero length";
                    return false;
                }
                spin_axis = renderer::normalize(spin_axis);
                Vec3 move_axis{};
                const json::Value& move_axis_json = object["move_axis"];
                if (!move_axis_json.is_null()
                    && (!move_axis_json.is_array() || move_axis_json.items().size() != 3)) {
                    error = "animated object move_axis must contain three numbers";
                    return false;
                }
                for (std::size_t i = 0; i < move_axis.size(); ++i) {
                    move_axis[i] =
                        move_axis_json.is_array()
                            ? static_cast<float>(
                                  move_axis_json[i].number(std::numeric_limits<double>::quiet_NaN())
                              )
                            : 0.0f;
                    if (!std::isfinite(move_axis[i])) {
                        error = "animated object move_axis contains an invalid value";
                        return false;
                    }
                }
                const float move_axis_length = renderer::dot(move_axis, move_axis);
                if (!std::isfinite(move_axis_length)) {
                    error = "animated object move_axis contains an invalid value";
                    return false;
                }
                const double move_distance = object["move_distance"].number(0.0);
                const double move_speed = object["move_speed"].number(1.0);
                const double move_phase = object["move_phase"].number(0.0);
                if (!std::isfinite(move_distance) || std::abs(move_distance) > 1000000.0
                    || !std::isfinite(move_speed) || std::abs(move_speed) > 100.0
                    || !std::isfinite(move_phase) || std::abs(move_phase) > 1000000.0
                    || (move_axis_length < 1.0e-8f && move_distance != 0.0)) {
                    error = "animated object movement values are outside their supported range";
                    return false;
                }
                if (move_axis_length >= 1.0e-8f) {
                    move_axis = renderer::normalize(move_axis);
                }
                const double speed = object["spin_speed"].number();
                const double phase = object["phase"].number(0.0);
                if (!std::isfinite(speed) || std::abs(speed) > 100.0 || !std::isfinite(phase)
                    || std::abs(phase) > 1000000.0) {
                    error = "animated object spin_speed or phase is outside its supported range";
                    return false;
                }
                auto& instances = out.instances[static_cast<std::size_t>(Layer::Animated)];
                out.spins.push_back(
                    {instances.size(),
                     spin_axis,
                     static_cast<float>(speed),
                     static_cast<float>(phase),
                     move_axis,
                     static_cast<float>(move_distance),
                     static_cast<float>(move_speed),
                     static_cast<float>(move_phase)}
                );
                instances.push_back(instance);
            }
        }
    }
    const std::tuple<const char*, Layer, std::map<int, ClassModel>*> groups[] =
        {{"ties", Layer::Ties, &ties},
         {"shrubs", Layer::Shrubs, &shrubs},
         {"mobys", Layer::Mobys, &mobys}};
    for (const auto& [key, layer, table] : groups) {
        const json::Value& list = edits[key].is_array() ? edits[key] : placements[key];
        if (edits[key].is_array()) {
            log::info("{} {} from {}", list.items().size(), key, edited.string());
        }
        for (const json::Value& p : list.items()) {
            const int id = static_cast<int>(p["class"].number(-1));
            auto cls = table->find(id);
            if (cls == table->end()) {
                // A placement of a class the level does not have; the editor
                // refuses those, so a hand-edited file.
                log::warn("{} placement {} has unknown class {}", key, p["index"].number(-1), id);
                continue;
            }
            Instance instance;
            instance.model = cls->second.model;
            if (layer == Layer::Ties && !edits[key].is_array()
                && p["occlusion_index"].is_number()) {
                const auto occlusion_key =
                    std::to_string(static_cast<int>(p["occlusion_index"].number(-1)));
                instance.occlusion =
                    static_cast<std::uint16_t>(pvs["ties"][occlusion_key].number(Pvs::always));
                const auto placement_index = p["index"].number(-1);
                if (placement_index >= 0
                    && placement_index < static_cast<double>(pvs["tie_words"].items().size())) {
                    instance.occlusion = static_cast<std::uint16_t>(
                        pvs["tie_words"][static_cast<std::size_t>(placement_index)].number(
                            Pvs::always
                        )
                    );
                }
            } else if (layer == Layer::Mobys && !edits[key].is_array()
                       && p["fields"]["occlusion"].number(1) == 0
                       && p["fields"]["spawn_id"].is_number() && !cls->second.box) {
                const auto occlusion_key =
                    std::to_string(static_cast<int>(p["fields"]["spawn_id"].number(-1)));
                instance.occlusion =
                    static_cast<std::uint16_t>(pvs["mobys"][occlusion_key].number(Pvs::always));
            }
            instance.stand_in = cls->second.box;
            if (layer == Layer::Mobys) {
                // Reviewed initial-state scenery. Both classes can break; no
                // characters, crates, traffic, rotating signs or platforms.
                // Classes and level numbers are the same on the PAL and NTSC-U discs.
                const bool metropolis_scenery = manifest["game"].string() == "rac1"
                                                && out.level == 3 && (id == 817 || id == 1827);
                const bool orxon_lamp =
                    manifest["game"].string() == "rac1" && out.level == 10 && id == 1855;
                const auto& selection =
                    manifest["classes"]["moby"][std::to_string(id)]["bake_static"];
                const bool emission_static =
                    std::find(emission_statics.begin(), emission_statics.end(), id)
                    != emission_statics.end();
                instance.bake_static = p["bake_static"].boolean(
                    selection.boolean(metropolis_scenery || orxon_lamp || emission_static)
                );
            }
            if (!read_matrix(p["matrix"], instance.matrix)) {
                error = std::format(
                    "{}: {} placement {} has no 4 x 4 matrix",
                    placements_file,
                    key,
                    p["index"].number(-1)
                );
                return false;
            }
            if (cls->second.scale != 1.0f) {
                instance.matrix =
                    renderer::multiply(instance.matrix, renderer::scale(cls->second.scale));
            }
            if (cls->second.box) {
                instance.tint = class_colour(id);
            }
            const std::size_t lit = p["colours"].items().size();
            if ((layer == Layer::Ties && lit == 64) || (layer == Layer::Shrubs && lit == 24)) {
                instance.lights = static_cast<int>(out.light_colours.size());
                for (const json::Value& c : p["colours"].items()) {
                    out.light_colours.push_back(static_cast<std::uint32_t>(c.number(0)));
                }
            }
            out.instances[static_cast<std::size_t>(layer)].push_back(instance);
            if (known_caps && (layer == Layer::Ties || (layer == Layer::Shrubs && fades_known))) {
                out.draw_distances[static_cast<std::size_t>(layer)].push_back(
                    game_draw_distance(layer, p["draw_distance"], billboard_fades[id])
                );
            }
        }
    }

    // Explicit bake radiance overrides keyed by the base texture's relative path.
    // This is kept outside generated GLBs so an artistic intensity can be tuned.
    for (const auto& surface : emission_edits["surfaces"].items()) {
        const fs::path mesh = fs::path(surface["mesh"].string()).lexically_normal();
        if (mesh.empty() || mesh.has_root_path() || *mesh.begin() == "..") {
            error = "bake emission surface mesh must be level-relative";
            return false;
        }
        const auto model = load_model(override_of(dir / mesh, out, nullptr), out, error);
        if (!model) {
            return false;
        }
        Instance instance;
        instance.model = *model;
        out.bake_surfaces.push_back(instance);
    }
    for (auto& material : out.materials) {
        if (material.image < 0) {
            continue;
        }
        // image_paths use resolved source paths; mods retain the same logical name.
        auto image_path =
            fs::absolute(fs::path(out.image_paths.at(static_cast<std::size_t>(material.image))))
                .lexically_normal();
        const auto mod_relative =
            out.overrides.empty() ? fs::path{} : image_path.lexically_relative(out.overrides);
        const auto root = !mod_relative.empty() && *mod_relative.begin() != ".."
                              ? out.overrides
                              : fs::absolute(dir).lexically_normal();
        const auto relative = image_path.lexically_relative(root).generic_string();
        const auto& edit = emission_edits[relative];
        if (!edit.is_object()) {
            continue;
        }
        // The old whole-material brightness bypass is retired. A legacy flag by
        // itself must not silently turn an ordinary surface into an emitter.
        const bool legacy_only =
            edit.members().size() == 1 && !edit["preserve_authored_lighting"].is_null();
        if (legacy_only) {
            continue;
        }
        // Reject the retired ambiguous key rather than silently losing an authored light.
        if (!edit["intensity"].is_null()) {
            error = "rename emission intensity to runtime_intensity in probe-emission.json";
            return false;
        }
        // Runtime intensity scales the texture's colour and supplies the default bake radiance.
        if (!edit["runtime_intensity"].is_null()) {
            const float runtime_intensity =
                static_cast<float>(edit["runtime_intensity"].number(-1));

            // Invalid intensities cannot define emission in either runtime or the bake.
            if (!std::isfinite(runtime_intensity) || runtime_intensity < 0) {
                error = "invalid runtime_intensity in probe-emission.json";
                return false;
            }

            material.emission.fill(runtime_intensity);
            material.emission_image = edit["use_base_texture"].boolean(true) ? material.image : -1;
        }

        // A bake override controls transport independently of the visible material's glow.
        if (!edit["bake_intensity"].is_null()) {
            const float intensity = static_cast<float>(edit["bake_intensity"].number(-1));

            // Reject invalid overrides instead of producing negative or non-finite radiance.
            if (!std::isfinite(intensity) || intensity < 0) {
                error = "invalid bake_intensity in probe-emission.json";
                return false;
            }

            material.bake_intensity = intensity;
            material.emission_image = edit["use_base_texture"].boolean(true) ? material.image : -1;
        }

        if (!edit["use_base_texture"].is_null()) {
            material.emission_image = edit["use_base_texture"].boolean() ? material.image : -1;
        }
        if (!edit["texture"].is_null()) {
            const fs::path texture = fs::path(edit["texture"].string()).lexically_normal();
            if (texture.empty() || texture.has_root_path() || *texture.begin() == "..") {
                error = "emission texture must be level-relative";
                return false;
            }
            material.emission_image = image_index(dir / texture, out);
            if (material.emission_image < 0) {
                error = "missing emission texture in probe-emission.json";
                return false;
            }
        }
        material.emission_contrast =
            edit["contrast"].is_null() ? 1.f : static_cast<float>(edit["contrast"].number(-1));
        if (!std::isfinite(material.emission_contrast) || material.emission_contrast < 1
            || material.emission_contrast > 4) {
            error = "emission contrast must be between one and four";
            return false;
        }
        material.emission_vertex_colour =
            edit["vertex_colour"].boolean(material.emission_vertex_colour);
        material.diffuse_scale =
            static_cast<float>(edit["diffuse_scale"].number(material.diffuse_scale));
        if (!std::isfinite(material.diffuse_scale) || material.diffuse_scale < 0
            || material.diffuse_scale > 1) {
            error = "diffuse_scale must be between zero and one";
            return false;
        }
    }

    // Bounds of everything but the sky, in game axes.
    constexpr float kHuge = 3.0e38f;
    out.bounds_min = {kHuge, kHuge, kHuge};
    out.bounds_max = {-kHuge, -kHuge, -kHuge};
    for (std::size_t layer = static_cast<std::size_t>(Layer::Terrain); layer < kLayerCount;
         ++layer) {
        for (const Instance& instance : out.instances[layer]) {
            const Model& m = out.models[instance.model];
            if (m.primitives.empty()) {
                continue;
            }
            for (int corner = 0; corner < 8; ++corner) {
                const Vec3 p{
                    (corner & 1) != 0 ? m.max[0] : m.min[0],
                    (corner & 2) != 0 ? m.max[1] : m.min[1],
                    (corner & 4) != 0 ? m.max[2] : m.min[2]
                };
                grow(out.bounds_min, out.bounds_max, renderer::transform_point(instance.matrix, p));
            }
        }
    }
    if (out.bounds_min[0] > out.bounds_max[0]) {
        out.bounds_min = out.bounds_max = Vec3{};
    }
    if (out.overridden_models > 0 || out.overridden_images > 0) {
        log::info(
            "level {}: {} meshes and {} textures from {}",
            out.level,
            out.overridden_models,
            out.overridden_images,
            out.overrides.string()
        );
    }
    // Authored visibility is not valid after arbitrary geometry/placement edits.
    if (out.overridden_models || edits["ties"].is_array() || edits["mobys"].is_array()
        || !out.spins.empty()) {
        out.pvs.grid.clear();
    }
    log::info(
        "level {}: baked PVS {} ({} bytes)",
        out.level,
        out.pvs.grid.empty() ? "unavailable" : "loaded",
        out.pvs.grid.size()
    );
    return true;
}

}  // namespace openrac::scene
