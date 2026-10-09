// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src (the tests of
// strings.rs, volumes.rs, save_game.rs, scene.rs, level_overlay.rs): ISC License,
// Copyright (c) 2026 ReRAC contributors.
//
// The lump readers on synthetic data: messages, volumes, saves, scenes, the
// overlay's code masking, the core index, the transition and boot lumps.

#include <cstring>

#include "assets/disc/frontend.h"
#include "assets/disc/level.h"
#include "assets/disc/messages.h"
#include "assets/disc/overlay.h"
#include "assets/disc/save_game.h"
#include "assets/disc/scene.h"
#include "assets/disc/transition.h"
#include "assets/disc/volumes.h"
#include "assets/disc/wad.h"
#include "tests/check.h"

using namespace openrac::assets;
using namespace openrac::assets::disc;

namespace {

template <typename T>
bool throws(T&& f) {
    try {
        f();
    } catch (const AssetError&) {
        return true;
    }
    return false;
}

void messages() {
    struct E {
        s32 id;
        std::string text;
        s32 audio;
    };
    const std::vector<E> entries = {{1000, "Gadgetron \x0cInfobots\x08 give", 4}, {7, "x", -1}, {1000, "second", 0}};
    ByteWriter table;
    std::string strings;
    const std::size_t base = 8 + 16 * entries.size();
    for (const E& e : entries) {
        table.put<s32>(static_cast<s32>(base + strings.size()));
        table.put<s32>(e.id);
        table.put<s32>(e.audio);
        table.put<s32>(0);
        strings += e.text;
        strings.push_back('\0');
    }
    ByteWriter g;
    g.resize(0x40);
    g.put_at<u32>(0x10, 0x40);
    g.put<u32>(static_cast<u32>(entries.size()));
    g.put<u32>(static_cast<u32>(base + strings.size()));
    g.put_bytes(table.bytes());
    g.put_bytes({reinterpret_cast<const u8*>(strings.data()), strings.size()});
    const auto m = parse_messages(g.bytes(), static_cast<u32>(Language::English));
    CHECK(m.size() == 3);
    CHECK(m[0].help_audio == 4);
    CHECK(message_text(m, 1000) == "Gadgetron \x0cInfobots\x08 give");
    CHECK(message_text(m, 5) == kMissingMessage);
    CHECK(printable(m[0].text) == "Gadgetron \\x0cInfobots\\x08 give");
}

Shape shape(Vec3 c, Vec3 h) {
    Shape s{};
    for (std::size_t k = 0; k < 3; ++k) {
        s.matrix[k][k] = h[k];
        s.inverse[k][k] = 1.0f / h[k];
    }
    s.matrix[3] = {c[0], c[1], c[2], 1.0f};
    return s;
}

void volumes() {
    ByteWriter g;
    g.resize(0x800);
    const Shape cub = shape({10, 20, 5}, {4, 2, 3});
    g.put_at<u32>(0x60, 0x100);
    g.put_at<s32>(0x100, 1);
    g.put_at<Shape>(0x110, cub);
    g.put_at<u32>(0x64, 0x200);  // count 0
    g.put_at<u32>(0x68, 0x300);
    g.put_at<s32>(0x300, 2);
    g.put_at<Shape>(0x310, cub);
    g.put_at<Shape>(0x390, shape({0, 0, 0}, {1, 1, 1}));
    g.put_at<u32>(0x6c, 0x500);
    g.put_at<s32>(0x500, 1);
    g.put_at<Shape>(0x510, cub);
    g.put_at<f32>(0x590, 7.5f);
    const Volumes v = parse_volumes(g.bytes());
    CHECK(v.cuboids.size() == 1 && v.spheres.empty() && v.cylinders.size() == 2 && v.pills.size() == 1);
    CHECK(v.cuboids[0] == cub);
    CHECK(v.pill_cap_radius(0) == 7.5f);
    CHECK((v.cuboids[0].local({14, 20, 8}) == Vec3{1, 0, 1}));
    CHECK((v.cuboids[0].world({1, -1, 0}) == Vec3{14, 18, 5}));
    CHECK(!v.shape(ShapeKind::Cuboid, -1) && v.shape(ShapeKind::Cylinder, 1));
    CHECK(v.paths.empty() && v.grind_paths.empty());
    CHECK(shape_from_grid_type(6) == ShapeKind::Cylinder);
}

u16 reference_crc(std::span<const u8> data) {
    // The textbook MSB-first CRC-16 with init 0x8320, polynomial 0x1f45.
    u16 r = 0x8320;
    for (const u8 b : data) {
        r = static_cast<u16>(r ^ (b << 8));
        for (int i = 0; i < 8; ++i) {
            r = static_cast<u16>((r & 0x8000) != 0 ? (r << 1) ^ 0x1f45 : r << 1);
        }
    }
    return r;
}

void saves() {
    const std::string digits = "123456789";
    const std::span<const u8> msg(reinterpret_cast<const u8*>(digits.data()), digits.size());
    CHECK(save_crc16({}) == 0x8320);
    CHECK(save_crc16(msg) == reference_crc(msg));
    CHECK(save_crc16(msg) == 0xd989);
    const u8 zero = 0;
    CHECK(save_crc16({&zero, 1}) == 0x05bc);
    std::vector<u8> v(0x1800);
    for (std::size_t i = 0; i < v.size(); ++i) {
        v[i] = static_cast<u8>(i * 7 + 3);
    }
    CHECK(save_crc16(v) == reference_crc(v));
    v.push_back(1);
    CHECK(save_crc16(v) == 0);

    Section s;
    s.chunks = {Chunk::make(0, {0xff, 0xff, 0xff, 0xff}), Chunk::make(10, {1, 2, 3, 4, 5}), Chunk{28, {7}, {9, 8, 7}}};
    const auto e = s.encode();
    CHECK(e.size() == s.encoded_size());
    CHECK(e.size() == 8 + (8 + 4) + (8 + 8) + (8 + 4) + 8);
    const ByteView ev(e);
    CHECK(ev.u32_at(0) == e.size() - 8);
    CHECK(ev.u32_at(4) == save_crc16(std::span<const u8>(e).subspan(8)));
    CHECK(ev.s32_at(e.size() - 8) == -1 && ev.u32_at(e.size() - 4) == 0);
    CHECK(e[8 + 12 + 8 + 5] == 0 && e[8 + 12 + 8 + 7] == 0);  // fresh chunks pad with zero
    const Section d = Section::decode(e);
    CHECK(d.crc_ok);
    CHECK(d == s);
    CHECK(d.encode() == e);
    auto bad = e;
    bad[16] ^= 1;
    CHECK(!Section::decode(bad).crc_ok);
    auto zero_crc = e;
    std::memset(zero_crc.data() + 4, 0, 4);
    CHECK(!Section::decode(zero_crc).crc_ok);

    const ChunkDesc descs[] = {{1, 37, 10}, {2, 4, 0}};
    CHECK(section_size(descs) == 8 + (40 + 8) + (4 + 8) + 8);

    SaveFile file;
    file.global.chunks = {Chunk::make(0, {0, 0, 0, 0})};
    for (std::size_t l = 0; l < kSaveLevelSlots; ++l) {
        file.levels[l].chunks = {Chunk::make(3001, {static_cast<u8>(l)})};
    }
    std::vector<u8> card = file.to_bytes();
    const std::vector<u8> before = card;
    SaveFile next = file;
    next.global.chunks = {Chunk::make(0, {1, 0, 0, 0})};
    for (Section& l : next.levels) {
        l.chunks[0].data[0] = 0xaa;
    }
    next.write_incremental(3, card);
    const std::size_t gs = next.global.encoded_size();
    const std::size_t ls = next.levels[0].encoded_size();
    CHECK(std::equal(card.begin(), card.begin() + 8, before.begin()));
    for (std::size_t slot = 0; slot < kSaveLevelSlots; ++slot) {
        const std::size_t at = 8 + gs + slot * ls;
        const std::vector<u8> got(card.begin() + static_cast<std::ptrdiff_t>(at), card.begin() + static_cast<std::ptrdiff_t>(at + ls));
        const std::vector<u8> old(before.begin() + static_cast<std::ptrdiff_t>(at), before.begin() + static_cast<std::ptrdiff_t>(at + ls));
        CHECK(slot == 3 ? got == next.levels[3].encode() : got == old);
    }
    const SaveFile parsed = SaveFile::parse(card);
    CHECK(parsed.levels[3].chunks[0].data == std::vector<u8>{0xaa});
    CHECK(parsed.levels[4].chunks[0].data == std::vector<u8>{4});

    const std::string cnf = "BOOT2 = cdrom0:\\SCUS_971.99;1\r\nVER = 1.00\r\n";
    CHECK(card_directory({reinterpret_cast<const u8*>(cnf.data()), cnf.size()}) == "/BASCUS-97199RATCHET");
    const std::string pal = "BOOT2 = cdrom0:\\SCES_503.26;1\r\n";
    CHECK(card_directory({reinterpret_cast<const u8*>(pal.data()), pal.size()}) == "/BESCES-50326RATCHET");
    CHECK(save_file_name(2) == "save2.bin");
}

// A decompressed scene chunk: `actors` actors of `frames` frames; camera eye.x
// = the record's scene tick, the cut flag on `cuts`; one subtitle line.
std::vector<u8> synth_chunk(s16 end, std::size_t index, std::size_t actors, u8 frames, std::vector<s32> cuts, bool subtitle) {
    const std::size_t n_camera = camera_records(end, index, SceneRegion::Ntsc);
    ByteWriter d;
    d.resize(0x14 + 4 * actors);
    d.pad_to(16);
    const std::size_t camera_offset = d.size();
    for (std::size_t r = 0; r < n_camera; ++r) {
        const f32 tick = static_cast<f32>(index * 96 + r);
        const bool cut = std::find(cuts.begin(), cuts.end(), static_cast<s32>(tick)) != cuts.end();
        d.put(CamRecord{{tick, 1, 2}, cut ? 1u : 0u, {0.1f, 0.2f, 0.3f}, 0.414f});
    }
    for (std::size_t a = 0; a < actors; ++a) {
        const std::size_t at = d.size();
        d.put_at<u32>(0x14 + 4 * a, static_cast<u32>(at));
        d.put<s32>(std::array<s32, 3>{0, 10, 530}[a % 3]);
        d.put<s32>(0x10);
        d.put<s32>(0);
        d.put<s32>(0);
        // The sequence header: a sphere, frame count, loop sound, trigger count.
        d.resize(d.size() + 0x10);
        d.put<u8>(frames);
        d.put<u8>(0xff);
        d.put<u8>(0xff);
        d.put<u8>(0);
        d.put<u32>(0);
        d.put<f32>(0);
        for (u8 f = 0; f < frames; ++f) {
            d.put<u32>(0x100u + f);
        }
        d.put_at<s32>(at + 12, static_cast<s32>(d.size()));
        for (u8 f = 0; f < frames; ++f) {
            d.put(std::array<f32, 4>{static_cast<f32>(index * 48 + f) * 2, static_cast<f32>(a), 0, 0});
        }
    }
    s32 subtitle_offset = 0;
    if (subtitle) {
        d.resize(std::max<std::size_t>(d.size(), kSubtitleMinOffset));
        subtitle_offset = static_cast<s32>(d.size());
        for (const s16 v : {s16{100}, s16{150}, s16{32}, s16{32}, s16{32}, s16{32}, s16{32}, s16{0}, s16{-1}}) {
            d.put<s16>(v);
        }
        d.resize(d.size() + 14);
        d.put_bytes({reinterpret_cast<const u8*>("Hello"), 6});
    }
    d.put_at(0, ChunkHeader{end, 0, subtitle_offset, -6, -1, static_cast<u16>(actors), 0, static_cast<s32>(camera_offset)});
    return std::move(d.bytes());
}

void scenes() {
    const SceneChunk c = parse_scene_chunk(synth_chunk(200, 1, 2, 49, {107}, true), 1, SceneRegion::Ntsc);
    CHECK(c.camera.size() == 97);
    CHECK(c.camera[0].eye[0] == 96.0f);
    CHECK(c.camera[11].is_cut() && !c.camera[10].is_cut());
    CHECK(c.actors.size() == 2 && c.actors[1].klass == 10);
    CHECK(c.actors[0].frame_count == 49 && c.actors[0].trigger_count == 0xff);
    CHECK(c.actors[0].frame_offsets.size() == 49 && c.actors[0].frame_offsets[2] == 0x102);
    CHECK((c.actors[0].positions[3] == std::array<f32, 4>{(48 + 3) * 2.0f, 0, 0, 0}));
    CHECK(c.sequence(c.actors[0]).u8_at(0x10) == 49);
    CHECK(c.subtitles.size() == 1 && c.subtitles[0].text[2] == "Hello");
    CHECK(c.subtitles[0].covers(150) && !c.subtitles[0].covers(151));
    // The last chunk of a 200-tick scene: 200 - 2 * 96 = 8 ticks, 9 records.
    const SceneChunk last = parse_scene_chunk(synth_chunk(200, 2, 1, 6, {}, false), 2, SceneRegion::Ntsc);
    CHECK(last.camera.size() == 9 && last.subtitles.empty());
    CHECK(throws([] { parse_scene_chunk(synth_chunk(200, 2, 1, 6, {}, false), 3, SceneRegion::Ntsc); }));

    std::vector<SceneChunk> chunks;
    for (std::size_t i = 0; i < 3; ++i) {
        chunks.push_back(parse_scene_chunk(synth_chunk(200, i, 1, 49, {107}, false), i, SceneRegion::Ntsc));
    }
    const Scene s = Scene::from_chunks(5, SceneRegion::Ntsc, std::move(chunks));
    CHECK(s.end_tick() == 200);
    CHECK((s.chunk_at(95) == std::pair<std::size_t, u32>{0, 95}));
    CHECK((s.chunk_at(96) == std::pair<std::size_t, u32>{1, 0}));
    bool all = true;
    for (s32 t = 1; t < 200; ++t) {
        all = all && s.camera_at(t) && s.camera_at(t)->eye[0] == static_cast<f32>(t);
    }
    CHECK(all);
    CHECK(s.cut_ticks() == std::vector<s32>{107});
    CHECK(throws([] { Scene::from_chunks(5, SceneRegion::Ntsc, {}); }));

    const s32 sectors[] = {100, 103, 110, 111};
    const auto t = RegionTable::from_sectors(sectors);
    CHECK(t && t->chunks == (std::vector<ChunkRange>{{100, 3}, {103, 7}, {110, 1}}));
    CHECK(t && t->file_bytes() == 12 * 0x800);
    CHECK(t && t->chunk_in_file(1) == (std::pair<std::size_t, std::size_t>{3 * 0x800, 7 * 0x800}));
    CHECK(!RegionTable::from_sectors({}));
    const s32 backwards[] = {5, 4};
    CHECK(throws([&] { RegionTable::from_sectors(backwards); }));

    // A whole scene from a region file of WAD-compressed chunks.
    LevelHeader h{};
    std::vector<u8> region;
    std::vector<s32> list;
    for (std::size_t i = 0; i < 3; ++i) {
        list.push_back(static_cast<s32>(1000 + region.size() / 2048));
        std::vector<u8> w = wad_compress(synth_chunk(200, i, 1, 4, {}, false));
        w.resize((w.size() + 2047) / 2048 * 2048);
        region.insert(region.end(), w.begin(), w.end());
    }
    list.push_back(static_cast<s32>(1000 + region.size() / 2048));
    region.resize(region.size() + 2048);
    std::copy(list.begin(), list.end(), h.scenes[2].ntsc.begin());
    const Scene loaded = Scene::load(h, region, 2, SceneRegion::Ntsc);
    CHECK(loaded.chunks.size() == 3 && loaded.actor_classes() == std::vector<s32>{0});
}

void overlay_masking() {
    // lui v0,0x17; addiu s1,v0,0x42c0; move s2,v0; addiu s1,s2,0x42c0;
    // lui at,0x3f80; lw t0,8(a0); lw t1,-0x10(gp)
    const u32 w[] = {0x3c020017, 0x245142c0, 0x0040902d, 0x265142c0, 0x3c013f80, 0x8c880008, 0x8f89fff0};
    CHECK((mask_code(w, kRac1NtscGp) == std::vector<u32>{0x3c020000, 0x24510000, 0x0040902d, 0x26510000, 0x3c013f80, 0x8c880008, 0x8f890000}));
    CHECK((address_refs(w, kRac1NtscGp) == std::vector<std::pair<std::size_t, u32>>{{1, 0x1742c0}, {3, 0x1742c0}, {6, kRac1NtscGp - 0x10}}));
    const u32 neg[] = {0x3c04001b, 0x2484f000};
    CHECK((address_refs(neg, kRac1NtscGp) == std::vector<std::pair<std::size_t, u32>>{{1, 0x1af000}}));

    // Seven sections; .text holds a function that jals another. The second
    // overlay is the same code linked 0x1000 higher.
    auto build = [](u32 shift) {
        ByteWriter o;
        const u32 text = 0x300000 + shift;
        auto section = [&](u32 dest, std::vector<u32> words) {
            o.put<u32>(dest);
            o.put<u32>(static_cast<u32>(words.size() * 4));
            o.put<u32>(1);
            o.put<u32>(0x200000);
            for (const u32 v : words) {
                o.put<u32>(v);
            }
        };
        section(0x15ef00, {0});
        section(0x160000, {0});
        section(0x170000, {0});
        section(0x180000, {42, text + 0x20, 0, 0xffffffff, 0, 0});
        section(0x180100, {0xffffffff, 0, 0, 0, 0});
        section(0x180200, {0});
        const u32 jal = 0x0c000000 | ((text + 0x20) >> 2);
        section(text, {0x27bdfff0, jal, 0, 0x03e00008, 0, 0, 0, 0, 0x27bdffe0, 0x3c020017, 0x245142c0, 0x03e00008, 0});
        return std::move(o.bytes());
    };
    const LevelOverlay a = LevelOverlay::parse(build(0), kRac1NtscGp);
    const LevelOverlay b = LevelOverlay::parse(build(0x1000), kRac1NtscGp);
    CHECK(a.sections().size() == 7);
    CHECK(a.vtbl().size() == 1 && a.vtbl()[0].o_class == 42);
    CHECK(a.camvtbl().empty());
    CHECK(a.extent(0x300000) == 8u);
    const Relocation r(a, b);
    CHECK(!r.is_identity());
    CHECK(r.function(0x300000) == 0x301000u);
    CHECK(r.function(0x300020) == 0x301020u);
    CHECK(r.data(0x1742c0) == 0x1742c0u);
    CHECK(Relocation(a, a).is_identity());
}

void core_index() {
    ByteWriter idx;
    idx.resize(0x200);
    LevelCoreHeader h{};
    h.tfrags = 0x100;
    h.sky = 0x400;
    h.collision = 0x500;
    h.moby_classes = {2, 0x100};
    h.assets_decompressed_size = 0x1000;
    h.heightmap_offset = 0x900;
    idx.put_at(0, h);
    idx.put_at(0x100, ClassEntry{0x600, 42, 0, 0, {}});
    idx.put_at(0x120, ClassEntry{0x800, 7, 0, 0, {}});
    const LevelCore core = parse_level_core(idx.bytes(), 0x1000);
    CHECK(core.moby_classes.size() == 2);
    std::vector<std::string> names;
    for (const CoreBlock& b : core.blocks) {
        names.push_back(std::format("{}@{:x}+{:x}", b.name, b.offset, b.size));
    }
    CHECK((names == std::vector<std::string>{"tfrags@100+300", "sky@400+100", "collision@500+100", "moby_class/0042@600+200", "moby_class/0007@800+800"}));
    std::vector<u8> data(0x1000);
    data[0x900] = 2;
    data[0x904] = 1;
    const f32 low = 0, high = 255;
    std::memcpy(data.data() + 0x908, &low, 4);
    std::memcpy(data.data() + 0x90c, &high, 4);
    data[0x911] = 255;
    const auto grid = HeightGrid::parse(h, data);
    CHECK(grid && grid->cells.size() == 2);
    CHECK(grid && grid->height(1.5f, 0.2f) == 0.0f && grid->height(0, 0) == 255.0f && !grid->height(2, 0));
    CHECK(core.block(data, "sky") && core.block(data, "sky")->size() == 0x100);
}

void lumps() {
    // A transition lump: header words, data at 0x100, then the parts.
    ByteWriter t;
    t.resize(0x200);
    t.put_at<u32>(0, 0x80);   // GS image at 0x80
    t.put_at<u32>(4, 0x100);  // base
    t.put_at<u32>(4 * 0x12, 0x00);
    t.put_at<u32>(4 * 0x13, 0x20);
    for (u32 v = 0; v < 5; ++v) {
        t.put_at<u32>(4 * (0x14 + v), 0x40 + 0x10 * v);
    }
    t.put_at<u32>(4 * 0x19, 0x90);
    t.put_at<u32>(0x120, 2);   // two pictures
    t.put_at<u32>(0x124, 0x8);
    t.put_at<u32>(0x128, 0xc);
    t.put_at<u32>(0x128 + 4, 0xabcd);
    const auto lump = TransitionLump::parse(wad_compress(t.bytes()));
    CHECK(lump.base() == 0x100);
    CHECK(lump.variant(0).size() == 0x10 && lump.variant(4).size() == 0x10);
    CHECK(lump.sky_block().size() == 0x20);
    CHECK(lump.picture(1).u32_at(0) == 0xabcd);
    CHECK(lump.gs_image().size() == 0x80);
    CHECK(lump.sound_bank().size() == 0x200 - 0x190);
    CHECK(throws([&] { lump.picture(2); }));

    // Boot pictures: a header with one WAD-compressed NTSC still.
    std::vector<u8> frame(std::size_t{kBootPictureWidth} * kBootPictureHeight * 4, 0x40);
    const std::vector<u8> packed = wad_compress(frame);
    ByteWriter irx;
    irx.resize(0x80);
    irx.put_at<u32>(0, 0x80);
    irx.put_at<u32>(4, static_cast<u32>(packed.size()));
    irx.put_bytes(packed);
    const auto pictures = BootPictures::parse(irx.bytes());
    const GsImage still = pictures.picture(BootPictureKind::Still, false, 0);
    CHECK(still.width == 512 && still.rgba == frame);
}

}  // namespace

int main() {
    messages();
    volumes();
    saves();
    scenes();
    overlay_masking();
    core_index();
    lumps();
    return openrac::test::result();
}
