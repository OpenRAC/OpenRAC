// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Interface texture runs (renderer/textures/interface_run.h) and the WAD streams they find blobs
// in (common/wad.h): a texture noted while the pool converts it is rebuilt from its recipe into the
// same picture under the same name, and a recipe with a missing or changed blob makes none.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

#include "check.h"
#include "common/wad.h"
#include "renderer/textures/interface_run.h"
#include "renderer/textures/texture_pack.h"
#include "renderer/textures/texture_pool.h"

namespace {

using namespace openrac;
using namespace openrac::renderer;

constexpr gs::Texa kTexa{0x80, false, 0x80};

std::vector<std::uint8_t> wad_stream(std::vector<std::uint8_t> body) {
    std::vector<std::uint8_t> out(wad::kHeaderBytes, 0);
    out[0] = 'W';
    out[1] = 'A';
    out[2] = 'D';
    out.insert(out.end(), body.begin(), body.end());
    const auto size = static_cast<std::uint32_t>(out.size());
    for (int i = 0; i < 4; ++i) {
        out[3 + i] = static_cast<std::uint8_t>(size >> (i * 8));
    }
    return out;
}

void wad_streams() {
    // Four literals, then a match of 3 bytes 4 back: ABCD + ABC.
    const auto stream = wad_stream({0x11 + 4, 'A', 'B', 'C', 'D', 0x4c, 0x00});
    CHECK(wad::looks_like_stream(stream));
    CHECK(wad::stream_size(stream) == stream.size());
    const auto out = wad::decompress(stream);
    CHECK(out && *out == std::vector<std::uint8_t>({'A', 'B', 'C', 'D', 'A', 'B', 'C'}));

    // Cut short, or reaching back before its start, or past the limit: nothing.
    auto cut = stream;
    cut.pop_back();
    CHECK(!wad::decompress(cut));
    CHECK(!wad::decompress(wad_stream({0x11 + 1, 'A', 0x5c, 0x00})));
    CHECK(!wad::decompress(stream, 5));
    CHECK(!wad::stream_size(std::vector<std::uint8_t>{'W', 'A', 'X', 0, 0, 0, 0}));

    // A repeat: a match of 7 bytes 2 back goes on copying the bytes it makes.
    const auto repeat = wad::decompress(wad_stream({0x11 + 2, 'A', 'B', 0x25, 0x04, 0x00}));
    CHECK(
        repeat
        && *repeat == std::vector<std::uint8_t>({'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A'})
    );

    // Enough: the stream's start only (here its first packet), the rest not read.
    const auto start = wad::decompress(stream, wad::kDefaultLimit, 2);
    CHECK(start && *start == std::vector<std::uint8_t>({'A', 'B', 'C', 'D'}));
    auto bad_tail = stream;
    bad_tail.back() = 0xff;
    CHECK(!wad::decompress(bad_tail));
    CHECK(wad::decompress(bad_tail, wad::kDefaultLimit, 4));
}

std::vector<std::uint8_t> clut() {
    std::vector<std::uint8_t> out(1024);
    for (std::size_t i = 0; i < 256; ++i) {
        out[i * 4] = static_cast<std::uint8_t>(i);
        out[i * 4 + 1] = static_cast<std::uint8_t>(255 - i);
        out[i * 4 + 2] = 7;
        out[i * 4 + 3] = static_cast<std::uint8_t>(i / 2);
    }
    return out;
}

void pool_round_trip() {
    interface_run::start();
    TexturePool pool;
    std::vector<std::uint8_t> pixels(16 * 16);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<std::uint8_t>(i * 7 % 256);
    }
    pool.upload({0x200, 1, gs::kPsmct32, 0, 0, 16, 16, clut()});
    pool.upload({0x300, 1, gs::kPsmt8, 0, 0, 16, 16, pixels});
    gs::Tex0 tex0{};
    tex0.tbp0 = 0x300;
    tex0.tbw = 1;
    tex0.psm = gs::kPsmt8;
    tex0.tw = 4;
    tex0.th = 4;
    tex0.tcc = true;
    tex0.cbp = 0x200;
    tex0.cpsm = gs::kPsmct32;
    const TextureHandle h = pool.resolve(tex0, kTexa);
    CHECK(h != 0 && h != pool.placeholder());
    const std::uint64_t name = texture_pack::image_hash(pool.image(h));
    CHECK(pool.pack_hash(h) == name);

    // The registers survive their encoding.
    const gs::Tex0 back = gs::Tex0::decode(interface_run::encode(tex0));
    CHECK(
        back.tbp0 == tex0.tbp0 && back.psm == tex0.psm && back.tw == 4 && back.cbp == 0x200
        && back.tcc
    );

    interface_run::mark_font(name);
    interface_run::Noted noted = interface_run::take();
    CHECK(noted.recipes.size() == 1);
    CHECK(noted.recipes.contains(name));
    const interface_run::Recipe& recipe = noted.recipes[name];
    CHECK(recipe.category == interface_run::Category::Font);
    CHECK(recipe.kind == interface_run::Kind::Pool);
    CHECK(recipe.uploads.size() == 2);

    // The run keeps no bytes, only each blob's key; an export reads them back (here, the uploads').
    std::map<std::uint64_t, std::vector<std::uint8_t>> bytes;
    for (const auto& b : {clut(), pixels}) {
        bytes[interface_run::blob_hash(b)] = b;
    }
    for (const auto& u : recipe.uploads) {
        CHECK(bytes.contains(u.blob));
        CHECK(noted.blobs.contains(u.blob) && noted.blobs[u.blob].size == bytes[u.blob].size());
    }
    const auto source = [&](std::uint64_t blob) -> const std::vector<std::uint8_t>* {
        const auto found = bytes.find(blob);
        return found == bytes.end() ? nullptr : &found->second;
    };
    const auto rebuilt = interface_run::rebuild(recipe, source);
    CHECK(rebuilt && texture_pack::image_hash(*rebuilt) == name);

    // A blob that cannot be read: nothing. One that changed: another picture (the caller's check).
    CHECK(!interface_run::rebuild(recipe, [](std::uint64_t) { return nullptr; }));
    auto changed = bytes;
    for (auto& [hash, bytes] : changed) {
        if (bytes.size() == pixels.size()) {
            bytes[5] ^= 0x55;
        }
    }
    const auto other =
        interface_run::rebuild(recipe, [&](std::uint64_t blob) -> const std::vector<std::uint8_t>* {
            const auto found = changed.find(blob);
            return found == changed.end() ? nullptr : &found->second;
        });
    CHECK(other && texture_pack::image_hash(*other) != name);
}

void past_fourteen_bits() {
    // The 2D layer keys a transfer drawn as itself past the chip's blocks (direct.cpp): the recipe
    // keeps the whole base.
    interface_run::start();
    TexturePool pool;
    std::vector<std::uint8_t> pixels(16 * 4 * 4, 0x40);
    pool.upload({0x10000u + 0x1a00, 1, gs::kPsmct32, 0, 0, 16, 4, pixels});
    gs::Tex0 tex0{};
    tex0.tbp0 = 0x10000u + 0x1a00;
    tex0.tbw = 1;
    tex0.psm = gs::kPsmct32;
    tex0.tw = 4;
    tex0.th = 2;
    const TextureHandle h = pool.resolve(tex0, kTexa);
    CHECK(h != pool.placeholder());
    interface_run::Noted noted = interface_run::take();
    CHECK(noted.recipes.size() == 1);
    const interface_run::Recipe& recipe = noted.recipes.begin()->second;
    CHECK(recipe.tbp == 0x11a00);
    const auto rebuilt = interface_run::rebuild(recipe, [&](std::uint64_t blob) {
        return blob == interface_run::blob_hash(pixels) ? &pixels : nullptr;
    });
    CHECK(rebuilt && texture_pack::image_hash(*rebuilt) == recipe.name);
}

void indexed8_round_trip() {
    std::vector<std::uint8_t> indices(8 * 8);
    for (std::size_t i = 0; i < indices.size(); ++i) {
        indices[i] = static_cast<std::uint8_t>(i * 3);
    }
    const std::vector<std::uint8_t> palette = clut();
    const Rgba8Image image =
        convert_t8(indices, 8, 8, clut_csm1(palette, 0x00, 16, 256, kTexa), AlphaScale::Gs);
    interface_run::note_indexed8(image, indices, palette, 8, 8, kTexa);
    // Noted once, however often it is made.
    interface_run::note_indexed8(image, indices, palette, 8, 8, kTexa);
    interface_run::Noted noted = interface_run::take();
    CHECK(noted.recipes.size() == 1);
    const std::uint64_t name = texture_pack::image_hash(image);
    CHECK(noted.recipes.contains(name));
    CHECK(noted.recipes[name].category == interface_run::Category::Effects);
    std::map<std::uint64_t, std::vector<std::uint8_t>> bytes{
        {interface_run::blob_hash(indices), indices}, {interface_run::blob_hash(palette), palette}
    };
    const auto read = [&](std::uint64_t blob) -> const std::vector<std::uint8_t>* {
        const auto found = bytes.find(blob);
        return found == bytes.end() ? nullptr : &found->second;
    };
    const auto rebuilt = interface_run::rebuild(noted.recipes[name], read);
    CHECK(rebuilt && texture_pack::image_hash(*rebuilt) == name);

    // A short palette or absurd size: nothing.
    interface_run::Recipe bad = noted.recipes[name];
    bad.width = 1 << 20;
    CHECK(!interface_run::rebuild(bad, read));
}

void blob_keys() {
    // The anchor is a varied window, not a run of one byte.
    std::vector<std::uint8_t> bytes(4096, 0);
    for (std::size_t i = 100; i < 108; ++i) {
        bytes[i] = static_cast<std::uint8_t>(i);
    }
    const interface_run::BlobKey key = interface_run::blob_key(bytes);
    CHECK(
        key.size == 4096 && key.anchor_at == 99
    );  // the first window of 8 different bytes: 0, 100..106
    std::uint64_t window = 0;
    std::memcpy(&window, bytes.data() + 99, 8);
    CHECK(key.anchor == window);
    // The windows checked before a hash, spread up to the end.
    bytes[4095] = 9;
    const interface_run::BlobKey probed = interface_run::blob_key(bytes);
    CHECK(interface_run::probe_at(4096, interface_run::BlobKey::kProbes - 1) == 4088);
    CHECK(probed.probes.back() == std::uint64_t{9} << 56);
    CHECK(interface_run::blob_key(std::vector<std::uint8_t>(4)).size == 4);
}

void categories() {
    for (const auto category : interface_run::kCategories) {
        CHECK(interface_run::parse_category(interface_run::category_id(category)) == category);
    }
    CHECK(!interface_run::parse_category("hud"));
    // Blobs that start alike differ by their size.
    const std::vector<std::uint8_t> a = {1, 2, 3};
    const std::vector<std::uint8_t> b = {1, 2, 3, 0};
    CHECK(interface_run::blob_hash(a) != interface_run::blob_hash(b));
}

}  // namespace

int main() {
    wad_streams();
    pool_round_trip();
    past_fourteen_bits();
    indexed8_round_trip();
    blob_keys();
    categories();
    return openrac::test::result();
}
