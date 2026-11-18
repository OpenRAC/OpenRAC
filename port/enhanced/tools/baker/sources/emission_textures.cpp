// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Composes and publishes generated material textures before the bake checks its cache.
#include "enhanced/tools/baker/sources/emission_textures.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <initializer_list>
#include <map>
#include <memory>
#include <span>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "assets/disc/checksum.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"

namespace openrac::lighting {
namespace {
struct Image {
    int width = 0;
    int height = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels{nullptr, stbi_image_free};
};

using TextureOutputs = std::map<std::string, std::string>;

std::string_view byte_view(std::span<const std::uint8_t> bytes) {
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void check_fields(const json::Value& object, std::initializer_list<std::string_view> allowed) {
    for (const auto& [key, value] : object.members()) {
        if (std::ranges::find(allowed, key) == allowed.end()) {
            throw std::runtime_error("unknown emission recipe field: " + key);
        }
    }
}

// Authored paths are names within known folders, including on case-insensitive filesystems.
std::string png_name(std::string_view name) {
    if (!name.ends_with(".png") || name.size() <= 4
        || name.substr(0, name.size() - 4)
                   .find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")
               != std::string_view::npos) {
        throw std::runtime_error("expected a lowercase PNG filename in emission texture recipe");
    }
    return std::string(name);
}

std::filesystem::path source_texture(std::string_view path) {
    constexpr std::string_view folder = "textures/";
    if (!path.starts_with(folder)) {
        throw std::runtime_error("emission source must be a local exported texture");
    }
    return std::filesystem::path(folder) / png_name(path.substr(folder.size()));
}

Image read_image(const std::filesystem::path& path, int requested_channels) {
    const auto bytes = bake_json::source_bytes(path);
    if (requested_channels == 1) {
        // PNG's IHDR stores sample depth and colour type at bytes 24 and 25.
        const auto encoded = byte_view(bytes);
        if (!encoded.starts_with("\x89PNG\r\n\x1a\n") || encoded.size() < 26 || encoded[24] != 8
            || encoded[25] != 0) {
            throw std::runtime_error(
                "emission mask must be an 8-bit grayscale PNG: " + path.string()
            );
        }
    }
    const int size = static_cast<int>(bytes.size());
    Image image;
    int channels = 0;
    constexpr int kMaximumExtent = 4096;
    if (!stbi_info_from_memory(bytes.data(), size, &image.width, &image.height, &channels)
        || image.width <= 0 || image.height <= 0 || image.width > kMaximumExtent
        || image.height > kMaximumExtent) {
        throw std::runtime_error("invalid emission image: " + path.string());
    }
    image.pixels.reset(stbi_load_from_memory(
        bytes.data(), size, &image.width, &image.height, &channels, requested_channels
    ));
    if (!image.pixels) {
        throw std::runtime_error("cannot decode emission image: " + path.string());
    }
    return image;
}

std::array<double, 3> recipe_colour(const json::Value& value) {
    if (value.items().size() != 3) {
        throw std::runtime_error("emission colour requires three channels in the range 0 to 255");
    }
    std::array<double, 3> result{};
    for (std::size_t channel = 0; channel < result.size(); ++channel) {
        const auto component = value[channel].number(-1);
        if (!std::isfinite(component) || component < 0 || component > 255) {
            throw std::runtime_error("invalid emission colour channel");
        }
        result[channel] = component;
    }
    return result;
}

// Round ties to even regardless of the baker's configured floating-point rounding mode.
std::uint8_t colour_byte(double value) {
    const double clamped = std::clamp(value, 0.0, 255.0);
    const auto lower = static_cast<unsigned>(clamped);
    const double fraction = clamped - lower;
    return static_cast<std::uint8_t>(lower + (fraction > .5 || (fraction == .5 && lower % 2)));
}

void append_png(void* context, void* bytes, int size) {
    static_cast<std::string*>(context)
        ->append(static_cast<const char*>(bytes), static_cast<std::size_t>(size));
}

std::string generate_png(
    const std::filesystem::path& level,
    const std::filesystem::path& profile,
    const json::Value& recipe
) {
    check_fields(recipe, {"output", "mask", "mode", "source", "colour", "gain"});
    const auto& mode = recipe["mode"].string();
    if (mode != "source" && mode != "tint" && mode != "solid") {
        throw std::runtime_error("unknown emission texture mode");
    }
    if ((mode == "source" && !recipe["colour"].is_null())
        || (mode == "solid" && !recipe["source"].is_null())) {
        throw std::runtime_error("emission colour or source does not apply to this mode");
    }
    const auto mask = read_image(profile / "emission-masks" / png_name(recipe["mask"].string()), 1);
    const auto colour =
        mode == "source" ? std::array<double, 3>{255, 255, 255} : recipe_colour(recipe["colour"]);
    const double gain = recipe["gain"].is_null() ? 1.0 : recipe["gain"].number(-1);
    if (!std::isfinite(gain) || gain < 0 || gain > 16) {
        throw std::runtime_error("emission texture gain must be in the range 0 to 16");
    }

    Image source;
    if (mode != "solid") {
        source = read_image(level / source_texture(recipe["source"].string()), 3);
        if (source.width != mask.width || source.height != mask.height) {
            throw std::runtime_error("emission source and mask dimensions differ");
        }
    }

    const auto count = static_cast<std::size_t>(mask.width) * static_cast<std::size_t>(mask.height);
    std::vector<std::uint8_t> rgb(count * 3);
    for (std::size_t pixel = 0; pixel < count; ++pixel) {
        const double weight = mask.pixels.get()[pixel] / 255.0;
        for (std::size_t channel = 0; channel < colour.size(); ++channel) {
            const std::size_t at = pixel * 3 + channel;
            const double base = source.pixels ? source.pixels.get()[at] * (colour[channel] / 255.0)
                                              : colour[channel];
            rgb[at] = colour_byte(base * gain * weight);
        }
    }
    std::string png;
    if (!stbi_write_png_to_func(
            append_png, &png, mask.width, mask.height, 3, rgb.data(), mask.width * 3
        )) {
        throw std::runtime_error("cannot encode emission texture");
    }
    return png;
}

const json::Value& recipe_list(const json::Value& document, const std::filesystem::path& path) {
    check_fields(document, {"version", "textures"});
    const auto& recipes = document["textures"];
    if (document["version"].number() != 1 || !recipes.is_array() || recipes.items().size() > 64) {
        throw std::runtime_error("invalid emission texture recipe list: " + path.string());
    }
    return recipes;
}

TextureOutputs generate_textures(
    const std::filesystem::path& level, const std::filesystem::path& profile
) {
    const auto path = profile / "emission-textures.json";
    if (!std::filesystem::exists(path)) {
        return {};
    }
    const auto document = bake_json::document(path);
    const auto& recipes = recipe_list(document, path);

    // Decode every recipe before publishing any output from this profile.
    TextureOutputs textures;
    std::size_t bytes = 0;
    for (const auto& recipe : recipes.items()) {
        auto name = png_name(recipe["output"].string());
        if (textures.contains(name)
            || std::filesystem::exists(profile / "material-textures" / name)) {
            throw std::runtime_error("duplicate emission texture output: " + name);
        }
        auto png = generate_png(level, profile, recipe);
        bytes += png.size();
        if (bytes > 64ull * 1024 * 1024) {
            throw std::runtime_error("generated emission textures exceed the profile size limit");
        }
        textures.emplace(std::move(name), std::move(png));
    }
    return textures;
}

void install_texture(const std::filesystem::path& destination, std::string_view png) {
    if (std::filesystem::is_regular_file(destination)) {
        const auto previous = bake_json::source_bytes(destination);
        if (byte_view(previous) == png) {
            return;
        }
    }
    std::filesystem::create_directories(destination.parent_path());
    if (const auto written = write_stage_file(destination, png); !written) {
        throw std::runtime_error(written.error().message);
    }
}
}  // namespace

void prepare_material_textures(
    const std::filesystem::path& level,
    const std::filesystem::path& profile,
    const std::filesystem::path& output
) {
    for (const auto& [name, png] : generate_textures(level, profile)) {
        install_texture(output / name, png);
    }
    const auto authored = profile / "material-textures";
    if (!std::filesystem::is_directory(authored)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(authored)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            const auto bytes = bake_json::source_bytes(entry.path());
            install_texture(output / entry.path().filename(), byte_view(bytes));
        }
    }
}

std::string emission_texture_fingerprint(
    const std::filesystem::path& level, const std::filesystem::path& profiles
) {
    const auto manifest = bake_json::document(level / "manifest.json");
    const auto& game = manifest["game"].string();
    const auto& version = manifest["version"].string();
    const auto valid = [](std::string_view component) {
        return !component.empty()
               && component.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")
                      == std::string_view::npos;
    };
    const auto number = manifest["level"].number(-1);
    if (!valid(game) || !valid(version) || !(number >= 0 && number <= 999)) {
        return {};
    }
    const auto version_root = profiles / (game + '-' + version);
    const std::pair<std::string_view, std::filesystem::path> layers[] = {
        {"shared", profiles / game},
        {"version", version_root},
        {"level", version_root / std::format("level_{:02}", static_cast<int>(number))},
    };
    assets::disc::Sha256 fingerprint;
    bool has_recipes = false;
    for (const auto& [layer, profile] : layers) {
        const auto path = profile / "emission-textures.json";
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        has_recipes = true;
        const auto installed = level / "materials" / layer;
        fingerprint.update(layer);
        fingerprint.update(bake_json::file_hash(path));
        fingerprint.update(bake_json::file_hash(profile / "material-overrides.json"));
        fingerprint.update(bake_json::file_hash(installed / "material-overrides.json"));
        const auto document = bake_json::document(path);
        for (const auto& recipe : recipe_list(document, path).items()) {
            fingerprint.update(
                bake_json::file_hash(profile / "emission-masks" / png_name(recipe["mask"].string()))
            );
            if (recipe["mode"].string() != "solid") {
                fingerprint.update(
                    bake_json::file_hash(level / source_texture(recipe["source"].string()))
                );
            }
            fingerprint.update(
                bake_json::file_hash(installed / "textures" / png_name(recipe["output"].string()))
            );
        }
    }
    return has_recipes ? assets::disc::to_hex(fingerprint.finish()) : std::string{};
}

}  // namespace openrac::lighting
