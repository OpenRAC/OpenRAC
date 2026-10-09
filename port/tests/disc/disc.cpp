// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/disc.rs
// (tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The table of contents, the per-version layouts and the lump plan on a
// synthetic RAC1 disc.

#include "assets/disc/disc.h"

#include "tests/check.h"
#include "tests/disc/synthetic_disc.h"

using namespace openrac::assets;
using namespace openrac::assets::disc;
using namespace openrac::test;

namespace {

constexpr std::size_t kHeaderSector = 1510;  // the level header (5 sectors), data follows

// A disc with a table of contents at 1500 and one level (id 3) in slot 1.
std::vector<u8> disc_image(std::vector<u8>& elf) {
    elf = fake_elf("Ratchet & Clank", 3000);
    std::vector<u8> img = make_iso(1600, {{"DATA/A.BIN", bytes_of("hello")}, {"SCUS_971.99", elf}, {"SYSTEM.CNF", system_cnf("SCUS_971.99")}});
    const std::size_t toc = 1500 * kSs;
    put32(img, toc, 1);
    put32(img, toc + 4, 0x2960);
    put32(img, toc + 0x28c8 + 8, static_cast<s32>(kHeaderSector));  // table slot 1
    put32(img, toc + 0x28c8 + 12, 1);
    const std::size_t h = kHeaderSector * kSs;
    put32(img, h, 3);
    put32(img, h + 4, 0x2434);
    put32(img, h + 8, kHeaderSector + 5);  // data: 2 sectors
    put32(img, h + 12, 2);
    put32(img, h + 16, kHeaderSector + 7);  // gameplay_ntsc: 1 sector
    put32(img, h + 20, 1);
    put32(img, h + 0x148, kHeaderSector + 8);  // music[0]: a VAG of 0x30 + 0x20 bytes
    const std::size_t d = (kHeaderSector + 5) * kSs;
    // overlay, sound_bank, core_index, gs_ram, hud_header, hud_banks[5], core_data
    const std::pair<s32, s32> ranges[11] = {
        {0x80, 0x10}, {-1, 0}, {0x100, 0x44}, {0x200, 0x300}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {0x600, 0x123},
    };
    for (std::size_t i = 0; i < 11; ++i) {
        put32(img, d + i * 8, ranges[i].first);
        put32(img, d + i * 8 + 4, ranges[i].second);
    }
    for (std::size_t i = 0x80; i < 0x1000; ++i) {
        img[d + i] = static_cast<u8>(i * 13);
    }
    const std::size_t v = (kHeaderSector + 8) * kSs;
    std::memcpy(img.data() + v, "VAGp", 4);
    img[v + 0x0f] = 0x20;  // big-endian data size
    return img;
}

std::vector<u8> slice(const std::vector<u8>& img, std::size_t at, std::size_t n) {
    return {img.begin() + static_cast<std::ptrdiff_t>(at), img.begin() + static_cast<std::ptrdiff_t>(at + n)};
}

void layouts() {
    const GameVersion* ntsc = find_version("rac1-ntsc");
    const GameVersion* pal = find_version("rac1-pal");
    CHECK(ntsc && pal);
    if (ntsc && pal) {
        CHECK(disc_layout(*ntsc).known && disc_layout(*pal).known);
        CHECK(disc_layout(*pal).toc_sector == 1500 && disc_layout(*pal).level_header_size == 0x2434);
    }
    for (const GameVersion& v : versions()) {
        const DiscLayout& l = disc_layout(v);
        CHECK(l.game == v.game);
        CHECK(l.known == (v.game == Game::Rac1));
    }
    // The global fields lie inside the table, before the level table, in order.
    u32 end = 0;
    for (const GlobalField& f : rac1_layout().global_fields) {
        CHECK(f.offset >= end);
        end = f.offset + f.count * (f.kind == FieldKind::Sector ? 4 : 8);
    }
    CHECK(end <= 0x28c8);
}

void reads_toc_boot_and_level_lumps() {
    std::vector<u8> elf;
    const std::vector<u8> img = disc_image(elf);
    const Disc disc(IsoImage(image_in_memory(img)), rac1_layout());
    CHECK(disc.toc().size() == 0x2960);
    CHECK(disc.level_ids() == std::vector<u32>{3});
    CHECK(disc.levels()[0].table_index == 1);
    CHECK(disc.boot_executable_path() == "/SCUS_971.99");
    CHECK(disc.boot_executable() == elf);

    const LevelFiles l = disc.level(3);
    const std::size_t d = 1515 * kSs;
    CHECK(l.overlay == slice(img, d + 0x80, 0x10));
    CHECK(l.core_index == slice(img, d + 0x100, 0x44));
    CHECK(l.gs_ram == slice(img, d + 0x200, 0x300));
    CHECK(l.core_data == slice(img, d + 0x600, 0x123));
    CHECK(!l.sound_bank && !l.hud_header && !l.gameplay_pal && !l.occlusion);
    for (const auto& b : l.hud_banks) {
        CHECK(!b);
    }
    CHECK(l.gameplay_ntsc == slice(img, 1517 * kSs, kSs));
    std::vector<std::string> names;
    for (const auto& [n, b] : l.files()) {
        names.push_back(n);
    }
    CHECK((names == std::vector<std::string>{"level_header.bin", "overlay.bin", "core_index.bin", "gs_ram.bin", "core_data.bin", "gameplay_ntsc.bin"}));
    CHECK(l.file("gs_ram.bin") && l.file("gs_ram.bin")->size() == 0x300);

    const auto streams = disc.level_stream_lumps(3);
    CHECK(streams == std::vector<StreamLump>{{"music/000", 1518, 0x50}});
    CHECK(disc.read_lump(streams[0]) == slice(img, 1518 * kSs, 0x50));
    bool threw = false;
    try {
        disc.level(4);
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
}

void global_lumps_and_plan() {
    std::vector<u8> elf;
    std::vector<u8> img = disc_image(elf);
    const std::size_t toc = 1500 * kSs;
    put32(img, toc + 0x10, 1520);  // save_game: 2 sectors
    put32(img, toc + 0x14, 2);
    put32(img, toc + 0xf00, 1541);  // qwark_boss_audio[0]: a bare sector, neither VAG nor WAD
    put32(img, toc + 0x17f8 + 3 * 8, 1530);  // mpegs[3]: 100 bytes
    put32(img, toc + 0x17f8 + 3 * 8 + 4, 100);
    put32(img, toc + 0x1ab8 + 5 * 4, 1540);  // help_audio[5]: a VAG of 0x30 + 0x40 bytes
    std::memcpy(img.data() + 1540 * kSs, "VAGp", 4);
    img[1540 * kSs + 0x0f] = 0x40;
    const Disc disc(IsoImage(image_in_memory(img)), rac1_layout());

    const auto g = disc.global_lumps();
    CHECK((g == std::vector<StreamLump>{
        {"save_game", 1520, 0x1000}, {"qwark_boss_audio/000", 1541, 0x800}, {"mpegs/003", 1530, 100}, {"help_audio/005", 1540, 0x70}}));
    CHECK(disc.save_game_lump().size() == 0x1000);

    const auto plan = disc.archive_files();
    std::vector<std::string> paths;
    for (const DiscFile& f : plan) {
        paths.push_back(f.path);
    }
    CHECK((paths == std::vector<std::string>{
        "boot/DATA/A.BIN", "boot/SCUS_971.99", "boot/SYSTEM.CNF", "toc.bin",
        "global/save_game.bin", "global/qwark_boss_audio/000.bin", "global/mpegs/003.bin", "global/help_audio/005.bin",
        "levels/03/level_header.bin", "levels/03/overlay.bin", "levels/03/core_index.bin", "levels/03/gs_ram.bin",
        "levels/03/core_data.bin", "levels/03/gameplay_ntsc.bin", "levels/03/music/000.bin"}));
    // Every planned range reproduces what the member readers return.
    const LevelFiles l = disc.level(3);
    for (const DiscFile& f : plan) {
        if (f.path.starts_with("levels/03/")) {
            if (const auto m = l.file(f.path.substr(10))) {
                CHECK(disc.iso().read_bytes(f.offset, f.bytes) == m->to_vector());
            }
        }
    }
    CHECK((plan[3] == DiscFile{"toc.bin", 1500 * 2048, 0x2960}));
    CHECK(plan[9].offset == 1515 * 2048 + 0x80);
    const auto group = disc.level_group_files(3);
    CHECK(group.size() == 6 && group.front().path == "levels/03/level_header.bin");
}

void rac1_shaped_disc() {
    const std::vector<u8> img = rac1_disc("SCES_509.16", fake_elf("Ratchet & Clank"));
    const Disc disc(IsoImage(image_in_memory(img)), rac1_layout());
    std::vector<std::string> paths;
    for (const DiscFile& f : disc.archive_files()) {
        paths.push_back(f.path);
    }
    CHECK((paths == std::vector<std::string>{
        "boot/SCES_509.16", "boot/SYSTEM.CNF", "toc.bin", "global/save_game.bin", "global/credits_images_pal/000.bin",
        "global/mpegs/021.bin", "global/mpegs/040.bin", "levels/01/level_header.bin", "levels/01/overlay.bin",
        "levels/01/core_data.bin", "levels/01/gameplay_ntsc.bin", "levels/01/gameplay_pal.bin",
        "levels/01/scene/00_ntsc.bin", "levels/01/scene/00_pal.bin"}));
    CHECK(disc.scene_region(1, 0, false)->size() == 2 * kSs);
    CHECK(!disc.scene_region(1, 1, false));
    CHECK(!disc.scene_speech(1, 0, 0));
}

void unknown_layouts_refuse() {
    const GameVersion* rac2 = find_version("rac2-ntsc");
    CHECK(rac2 != nullptr);
    if (!rac2) {
        return;
    }
    std::vector<u8> elf;
    bool threw = false;
    try {
        Disc d(IsoImage(image_in_memory(disc_image(elf))), disc_layout(*rac2));
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
}

void probes() {
    CHECK(probe_lump_size(std::vector<u8>(64, 0)).bytes == 2048);
    CHECK(!probe_lump_size(std::vector<u8>(64, 0)).exact);
    std::vector<u8> w = {'W', 'A', 'D', 0x34, 0x12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    CHECK(probe_lump_size(w).bytes == 0x1234 && probe_lump_size(w).exact);
}

}  // namespace

int main() {
    layouts();
    reads_toc_boot_and_level_lumps();
    global_lumps_and_plan();
    rac1_shaped_disc();
    unknown_layouts_refuse();
    probes();
    return openrac::test::result();
}
