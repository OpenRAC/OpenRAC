// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture packs (docs/port/TEXTURE_PACKS.md): a folder of PNGs, each named by the hash of the
// texture it replaces (its size and RGBA texels as the port converted them), at any size. The
// level's textures (scene/level.cpp) and the 2D layer's uploads (texture_pool.cpp) both ask here.
// With a dump folder, every texture is written there once, by the same name, for pack makers,
// sorted into folders a player can find their way in: Interface/ (the 2D layer: HUD, menus, font),
// Planets/<planet>/ (a level's own textures) and Shared/ (those met on more than one planet).
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "renderer/texture.h"

namespace openrac::renderer::texture_pack {

// Reading and writing PNGs, from the program (the renderer itself links no image library).
struct Io {
    std::optional<Rgba8Image> (*load)(const std::filesystem::path& file) = nullptr;
    bool (*save)(const std::filesystem::path& file, const Rgba8Image& image) = nullptr;
};

// Where a texture was met, which picks its dump folder.
struct Origin {
    bool interface = false;  // the 2D layer's uploads (texture_pool.cpp)
    int level = -1;          // a level's own texture (scene/level.cpp): the game's level number
};

// A level's name for its dump folder (the game's planet table); nullptr or an empty name gives
// "Level <number>".
using LevelNamer = std::string (*)(int level);

// The pack's folder (empty or missing: none) and the dump folder (empty: no dump). The pack's
// file names, and those already in the dump folder, are read once here.
void configure(
    const std::filesystem::path& pack,
    const std::filesystem::path& dump,
    Io io,
    LevelNamer level_name = nullptr
);

// The dump's folder, relative to the dump folder, for a texture met there: "Interface",
// "Planets/<name>" or, with no level, "Shared". Characters no file system takes become spaces.
std::filesystem::path dump_folder(const Origin& origin);

// The 64-bit hash a texture is known by (FNV-1a over its width, height and texels).
std::uint64_t image_hash(const Rgba8Image& image);

// Its file name: 16 lowercase hex digits and ".png".
std::string file_name(std::uint64_t hash);

// `image` replaced by the pack's picture of it, when the pack has one (true); dumped first when
// dumping, into the folder of `origin`. A level's texture already dumped under another planet
// moves to Shared/. A pack file that cannot be read leaves the image as it was.
bool replace(Rgba8Image& image, const Origin& origin = {});

// How many textures the pack replaced so far.
unsigned replaced_count();

}  // namespace openrac::renderer::texture_pack
