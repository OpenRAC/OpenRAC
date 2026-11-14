// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/textures/interface_run.h"

#include <algorithm>
#include <atomic>
#include <mutex>

#include "renderer/textures/texture_pack.h"
#include "renderer/textures/texture_pool.h"

namespace openrac::renderer::interface_run {

namespace {

struct State {
    std::mutex lock;
    std::map<std::uint64_t, Recipe> recipes;
    std::map<std::uint64_t, std::vector<std::uint8_t>> blobs;
};

State& state() {
    static State s;
    return s;
}

std::atomic<bool> g_recording{false};

std::uint64_t field(std::uint64_t value, int at) {
    return value << at;
}

// Keeps a copy of `bytes` under its hash; returns the hash.
std::uint64_t keep(State& s, std::span<const std::uint8_t> bytes) {
    const std::uint64_t hash = blob_hash(bytes);
    if (!s.blobs.contains(hash)) {
        s.blobs.emplace(hash, std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
    }
    return hash;
}

Upload shape_of(const ImageUpload& upload, std::uint64_t blob) {
    return Upload{
        upload.dbp, upload.dbw, upload.dpsm, upload.x, upload.y, upload.width, upload.height, blob
    };
}

}  // namespace

std::string_view category_id(Category category) {
    switch (category) {
        case Category::Font:
            return "font";
        case Category::Interface:
            return "interface";
        case Category::Effects:
            return "effects";
    }
    return "interface";
}

std::optional<Category> parse_category(std::string_view id) {
    for (const Category category : kCategories) {
        if (category_id(category) == id) {
            return category;
        }
    }
    return std::nullopt;
}

std::uint64_t encode(const gs::Tex0& t) {
    return field(t.tbp0 & 0x3fff, 0) | field(t.tbw & 0x3f, 14) | field(t.psm & 0x3f, 20)
           | field(t.tw & 0xf, 26) | field(t.th & 0xf, 30) | field(t.tcc ? 1 : 0, 34)
           | field(static_cast<std::uint64_t>(t.tfx) & 3, 35) | field(t.cbp & 0x3fff, 37)
           | field(t.cpsm & 0xf, 51) | field(t.csm ? 1 : 0, 55) | field(t.csa & 0x1f, 56)
           | field(t.cld & 7, 61);
}

std::uint64_t encode(const gs::Texa& t) {
    return field(t.ta0, 0) | field(t.aem ? 1 : 0, 15) | field(t.ta1, 32);
}

std::uint64_t blob_hash(std::span<const std::uint8_t> bytes) {
    // The size first, so that two blobs of which one starts the other differ.
    const std::uint64_t size = bytes.size();
    return hash_bytes(bytes.data(), bytes.size()) ^ (size * 0x9E3779B97F4A7C15ull);
}

void start() {
    state();  // made before any atexit handler is registered, so it outlives them
    g_recording = true;
}

bool recording() {
    return g_recording.load(std::memory_order_relaxed);
}

void note_pool(
    const Rgba8Image& image,
    const gs::Tex0& tex0,
    const gs::Texa& texa,
    std::span<const PoolUpload> uploads,
    const ImageUpload* clut
) {
    if (!recording() || image.pixels.empty()) {
        return;
    }
    const std::uint64_t name = texture_pack::image_hash(image);
    State& s = state();
    const std::lock_guard guard(s.lock);
    if (s.recipes.contains(name)) {
        return;
    }
    Recipe recipe;
    recipe.name = name;
    recipe.category = Category::Interface;
    recipe.kind = Kind::Pool;
    recipe.tex0 = encode(tex0);
    recipe.tbp = tex0.tbp0 > 0x3fff ? tex0.tbp0 : 0;
    recipe.texa = encode(texa);
    // The CLUT first: the texture and anything it reaches over come after it, so that where they
    // overlap they are what the conversion read, as in the game.
    if (clut != nullptr) {
        recipe.uploads.push_back(shape_of(*clut, keep(s, clut->data)));
    }
    std::vector<PoolUpload> ordered(uploads.begin(), uploads.end());
    std::stable_sort(ordered.begin(), ordered.end(), [](const PoolUpload& a, const PoolUpload& b) {
        return a.sequence < b.sequence;
    });
    for (const PoolUpload& u : ordered) {
        if (u.upload == nullptr) {
            continue;
        }
        const Upload shape = shape_of(*u.upload, keep(s, u.upload->data));
        const bool again =
            std::any_of(recipe.uploads.begin(), recipe.uploads.end(), [&](const Upload& had) {
                return had.dbp == shape.dbp && had.blob == shape.blob && had.x == shape.x
                       && had.y == shape.y;
            });
        if (!again) {
            recipe.uploads.push_back(shape);
        }
    }
    s.recipes.emplace(name, std::move(recipe));
}

void note_indexed8(
    const Rgba8Image& image,
    std::span<const std::uint8_t> indices,
    std::span<const std::uint8_t> palette,
    int width,
    int height,
    const gs::Texa& texa
) {
    if (!recording() || image.pixels.empty()) {
        return;
    }
    const std::uint64_t name = texture_pack::image_hash(image);
    State& s = state();
    const std::lock_guard guard(s.lock);
    if (s.recipes.contains(name)) {
        return;
    }
    Recipe recipe;
    recipe.name = name;
    recipe.category = Category::Effects;
    recipe.kind = Kind::Indexed8;
    recipe.texa = encode(texa);
    recipe.indices = keep(
        s,
        indices.subspan(
            0, std::min<std::size_t>(indices.size(), static_cast<std::size_t>(width) * height)
        )
    );
    recipe.palette = keep(s, palette);
    recipe.width = width;
    recipe.height = height;
    s.recipes.emplace(name, std::move(recipe));
}

void mark_font(std::uint64_t name) {
    if (!recording() || name == 0) {
        return;
    }
    State& s = state();
    const std::lock_guard guard(s.lock);
    if (auto found = s.recipes.find(name);
        found != s.recipes.end() && found->second.kind == Kind::Pool) {
        found->second.category = Category::Font;
    }
}

Noted take() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    Noted out;
    out.recipes = std::move(s.recipes);
    out.blobs = std::move(s.blobs);
    s.recipes.clear();
    s.blobs.clear();
    return out;
}

std::optional<Rgba8Image> rebuild(const Recipe& recipe, const BlobSource& blobs) {
    const gs::Texa texa = gs::Texa::decode(recipe.texa);
    if (recipe.kind == Kind::Indexed8) {
        const std::vector<std::uint8_t>* indices = blobs(recipe.indices);
        const std::vector<std::uint8_t>* palette = blobs(recipe.palette);
        if (indices == nullptr || palette == nullptr || recipe.width <= 0 || recipe.height <= 0
            || recipe.width > kMaxTextureSide || recipe.height > kMaxTextureSide
            || indices->size() < static_cast<std::size_t>(recipe.width)
                                     * static_cast<std::size_t>(recipe.height)
            || palette->size() < 1024) {
            return std::nullopt;
        }
        // As world.cpp converts them: the CLUT in CSM1, in the chip's alpha units.
        const std::vector<std::uint32_t> colours = clut_csm1(*palette, 0x00, 16, 256, texa);
        Rgba8Image image =
            convert_t8(*indices, recipe.width, recipe.height, colours, AlphaScale::Gs);
        if (image.pixels.empty()) {
            return std::nullopt;
        }
        return image;
    }
    // The 2D layer's: the same uploads into a pool of its own, and the same draw.
    TexturePool pool;
    pool.set_noting(false);
    for (const Upload& u : recipe.uploads) {
        const std::vector<std::uint8_t>* bytes = blobs(u.blob);
        if (bytes == nullptr) {
            return std::nullopt;
        }
        pool.upload(ImageUpload{u.dbp, u.dbw, u.dpsm, u.x, u.y, u.width, u.height, *bytes});
    }
    gs::Tex0 tex0 = gs::Tex0::decode(recipe.tex0);
    if (recipe.tbp != 0) {
        tex0.tbp0 = recipe.tbp;
    }
    const TextureHandle h = pool.resolve(tex0, texa);
    if (h == 0 || h == pool.placeholder()) {
        return std::nullopt;
    }
    return pool.image(h);
}

}  // namespace openrac::renderer::interface_run
