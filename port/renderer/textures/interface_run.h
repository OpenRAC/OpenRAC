// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Interface texture runs (docs/port/TEXTURE_PACKS.md, "Export without playing"): while the game
// plays a run, every interface texture it converts is noted with what it was made from (the image
// transfers the game sent, by the hash of their bytes, and the registers it was drawn with) and the
// name a texture pack knows it by. The program finds those bytes on the disc and writes the run
// file (game/common/program/interface_runs.h); a player's export reads them back from their own
// disc and rebuilds each texture here, through the same conversion as the game, so the picture and
// its name are the ones the game makes. A run file holds no byte of the game: only shapes,
// registers, hashes and places on the disc.
//
// Three kinds of texture are noted:
//   Font       the 2D layer's text (a texture drawn as a glyph);
//   Interface  the rest of the 2D layer: the HUD and the menus;
//   Effects    the effects' and particles' 8-bit textures (game/common/frontend/world.cpp).
// A level's own textures are not: the editor exports those from the disc already.
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "renderer/direct/gs.h"
#include "renderer/textures/texture.h"

namespace openrac::renderer {

struct ImageUpload;

namespace interface_run {

// The version of the conversion a run's names were made with. Raise it whenever a change to the
// conversion (texture.cpp, TexturePool's uploads and CLUTs, texture_pack::image_hash) changes a
// texture's picture or name: runs made before then are then shown as out of date and not used.
// tests/interface_run_test.cpp holds the pictures and names this version makes.
inline constexpr int kConverter = 1;

enum class Category : std::uint8_t {
    Font,
    Interface,
    Effects
};

// Every category, in the order the launcher lists them.
inline constexpr Category kCategories[] = {Category::Font, Category::Interface, Category::Effects};

// "font", "interface", "effects".
std::string_view category_id(Category category);
std::optional<Category> parse_category(std::string_view id);

// The 2D layer's texture is rebuilt by replaying its uploads into a TexturePool; an effect's from
// its 8-bit indices and its 256-colour CLUT.
enum class Kind : std::uint8_t {
    Pool,
    Indexed8
};

// One image transfer: where in the chip's memory it went, and its bytes by hash (`blob`).
struct Upload {
    std::uint32_t dbp = 0;
    std::uint32_t dbw = 1;
    std::uint8_t dpsm = 0;
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t blob = 0;
};

// How to make one texture again.
struct Recipe {
    std::uint64_t name = 0;  // texture_pack::image_hash of the picture
    Category category = Category::Interface;
    Kind kind = Kind::Pool;
    // Pool: the uploads in the order the game made them, and the draw's registers.
    std::vector<Upload> uploads;
    std::uint64_t tex0 = 0;
    std::uint64_t texa = 0;
    // TEX0's base when it is past the chip's 14 bits (the 2D layer keys a transfer drawn as itself
    // at 0x10000 + its block): tex0 holds only the low 14. 0 when tex0's is the whole of it.
    std::uint32_t tbp = 0;
    // Indexed8: the indices and the CLUT (PSMCT32, 16 x 16) by hash, and the picture's size.
    std::uint64_t indices = 0;
    std::uint64_t palette = 0;
    int width = 0;
    int height = 0;
};

// The registers as the chip takes them (gs.h's decode() turned round).
std::uint64_t encode(const gs::Tex0& tex0);
std::uint64_t encode(const gs::Texa& texa);

// The hash a blob is known by.
std::uint64_t blob_hash(std::span<const std::uint8_t> bytes);

// ---- Noting a run (the game's process) ----

// Starts noting. Until then every note_* call does nothing.
void start();
bool recording();

// A 2D layer texture `image`, converted for a draw with `tex0` and `texa` from `uploads`: the
// texture's own first, then any other the conversion read, each with its order (its sequence in
// the pool); the CLUT as the one it was read as (`clut`, null for a direct colour texture).
struct PoolUpload {
    const ImageUpload* upload = nullptr;
    std::uint64_t sequence = 0;
};

void note_pool(
    const Rgba8Image& image,
    const gs::Tex0& tex0,
    const gs::Texa& texa,
    std::span<const PoolUpload> uploads,
    const ImageUpload* clut
);

// An effect's or particle's texture `image`, made by convert_t8 from `indices` and `palette` (a
// PSMCT32 CLUT, 16 x 16 in CSM1) with `texa`.
void note_indexed8(
    const Rgba8Image& image,
    std::span<const std::uint8_t> indices,
    std::span<const std::uint8_t> palette,
    int width,
    int height,
    const gs::Texa& texa
);

// The texture named `name` was drawn as text: it is a font texture.
void mark_font(std::uint64_t name);

// What a blob is known by while a run looks for it on the disc, without its bytes (a run can see
// thousands of transfers, a full-screen picture each frame the game pauses): its size and one
// 8-byte window of it (`anchor`, at `anchor_at`), the most varied among its first 4 KB. A place
// where the window matches is then checked by the hash of `size` bytes from there.
struct BlobKey {
    std::uint64_t size = 0;
    std::uint64_t anchor_at = 0;
    std::uint64_t anchor = 0;
    // Windows spread over the blob up to its end (probe_at), checked before the whole blob is
    // hashed: an anchor can be found in many places where the rest differs.
    static constexpr std::size_t kProbes = 16;
    std::array<std::uint64_t, kProbes> probes{};
};

// Where probe `n` of a blob of `size` bytes (8 or more) starts.
constexpr std::uint64_t probe_at(std::uint64_t size, std::size_t n) {
    return (size - 8) * (n + 1) / BlobKey::kProbes;
}

BlobKey blob_key(std::span<const std::uint8_t> bytes);

// What was noted: the recipes, by name, and every blob they name.
struct Noted {
    std::map<std::uint64_t, Recipe> recipes;
    std::map<std::uint64_t, BlobKey> blobs;
};

Noted take();

// ---- Rebuilding (a player's export) ----

// The bytes of a blob, or null when they could not be read.
using BlobSource = std::function<const std::vector<std::uint8_t>*(std::uint64_t blob)>;

// The picture `recipe` makes, or nothing when a blob is missing or the conversion makes none.
// The caller checks its hash against recipe.name.
std::optional<Rgba8Image> rebuild(const Recipe& recipe, const BlobSource& blobs);

}  // namespace interface_run
}  // namespace openrac::renderer
