// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The gameplay file of a small invented level: its section table, level
// settings, moby instances and classes, and the pvar blocks with the
// loader's moby links and shared data.

#include "assets/world/gameplay.h"

#include <cstring>

#include "assets/world/cameras.h"
#include "tests/check.h"

using namespace openrac::assets;

namespace {

using Section = GameplaySection;

// A gameplay file built section by section: each section's offset goes in
// its header slot.
class GameplayBuilder {
public:
    GameplayBuilder() { m_w.resize(0x100); }

    // Starts `section` here (16-aligned) and returns its offset.
    std::size_t begin(Section section) {
        m_w.pad_to(16);
        const auto at = static_cast<u32>(m_w.size());
        m_w.put_at(4 * static_cast<std::size_t>(section), at);
        return at;
    }

    ByteWriter& w() { return m_w; }

    std::vector<u8> bytes() {
        m_w.pad_to(16);
        return m_w.bytes();
    }

private:
    ByteWriter m_w;
};

void put_instance(ByteWriter& w, s32 class_id, s32 pvar, std::array<s32, 3> colour, f32 x) {
    const std::size_t r = w.size();
    w.resize(r + kMobyInstanceSize);
    w.put_at<s32>(r, static_cast<s32>(kMobyInstanceSize));
    w.put_at<s32>(r + 0x18, class_id);
    w.put_at<f32>(r + 0x1c, 0.5f);
    w.put_at<f32>(r + 0x30, x);
    w.put_at<s32>(r + 0x48, -1);
    w.put_at<s32>(r + 0x58, pvar);
    for (std::size_t k = 0; k < 3; ++k) {
        w.put_at<s32>(r + 0x64 + 4 * k, colour[k]);
    }
}

std::vector<u8> invented_level() {
    GameplayBuilder g;
    // Level settings: a blue background, fog from 20 to 150, a ship at (100, 200, 30).
    std::size_t s = g.begin(Section::LevelSettings);
    g.w().resize(s + 0x50);
    for (std::size_t k = 0; k < 3; ++k) {
        g.w().put_at<s32>(s + 4 * k, std::array<s32, 3>{10, 20, 200}[k]);
        g.w().put_at<s32>(s + 0x0c + 4 * k, std::array<s32, 3>{90, 90, 120}[k]);
        g.w().put_at<f32>(s + 0x2c + 4 * k, std::array<f32, 3>{100, 200, 30}[k]);
    }
    g.w().put_at<f32>(s + 0x18, 20.0f);
    g.w().put_at<f32>(s + 0x1c, 150.0f);
    g.w().put_at<f32>(s + 0x28, -40.0f);
    g.w().put_at<s32>(s + 0x3c, 2);

    // Two moby classes.
    s = g.begin(Section::MobyClasses);
    g.w().put<s32>(2);
    g.w().resize(s + 0x10);
    g.w().put<s32>(500);
    g.w().put<s32>(501);

    // Three instances: the second's colour carries past a byte.
    s = g.begin(Section::MobyInstances);
    g.w().put<s32>(3);
    g.w().resize(s + 0x10);
    put_instance(g.w(), 500, 0, {1, 2, 3}, 5.0f);
    put_instance(g.w(), 501, 1, {0x100, 0, 0}, 6.0f);
    put_instance(g.w(), 500, -1, {0, 0, 0}, 7.0f);

    // Two pvar blocks of 8 bytes: block 0's first word names instance 2,
    // block 1's second word takes a shared-data offset.
    s = g.begin(Section::PvarTable);
    for (const s32 v : {0, 8, 8, 8}) {
        g.w().put(v);
    }
    s = g.begin(Section::PvarData);
    for (const s32 v : {2, 77, 1, 0}) {
        g.w().put(v);
    }
    s = g.begin(Section::PvarMobyLinks);
    for (const s32 v : {0, 0, -1, 0}) {
        g.w().put(v);
    }
    // Shared data: 16 bytes, one record pointing block 1 field +4 at offset 8.
    s = g.begin(Section::SharedData);
    g.w().put<s32>(16);
    g.w().put<s32>(1);
    g.w().resize(s + 0x10);
    g.w().put_bytes(std::vector<u8>(16, 0xab));
    g.w().put<u16>(1);
    g.w().put<u16>(4);
    g.w().put<s32>(8);
    // One camera of class 3 at (1, 2, 3) with pvar block 1.
    s = g.begin(Section::Cameras);
    g.w().put<s32>(1);
    g.w().resize(s + 0x10);
    g.w().put<s32>(3);
    for (const f32 v : {1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.5f}) {
        g.w().put(v);
    }
    g.w().put<s32>(1);
    return g.bytes();
}

void sections() {
    const auto bytes = invented_level();
    const GameplayFile file(Game::Rac1, bytes);
    const auto ranges = file.sections();
    CHECK(ranges.size() == 8);
    for (std::size_t k = 1; k < ranges.size(); ++k) {
        CHECK(ranges[k - 1].end == ranges[k].start);
    }
    CHECK(ranges.back().end == bytes.size());
    CHECK(file.section(Section::MobyClasses)->size() == 0x20);
    CHECK(!file.section(Section::Cuboids));
    CHECK(gameplay_section_name(Section::PvarTable) != gameplay_section_name(Section::PvarData));
    bool refused = false;
    try {
        GameplayFile other(Game::Rac3, bytes);
    } catch (const AssetError&) {
        refused = true;
    }
    CHECK(refused);  // the sequels' layouts are not known yet
}

void settings_and_instances() {
    const auto bytes = invented_level();
    const GameplayFile file(Game::Rac1, bytes);
    const LevelSettings l = read_level_settings(file);
    CHECK((l.background_rgb == std::array<s32, 3>{10, 20, 200}));
    CHECK(l.fog_near == 20.0f && l.fog_far == 150.0f && l.death_height == -40.0f);
    CHECK(l.has_ship() && l.ship_path == 2 && l.ship_position[1] == 200.0f);

    CHECK((read_class_list(file, Section::MobyClasses) == std::vector<s32>{500, 501}));
    CHECK(read_class_list(file, Section::TieClasses).empty());
    bool refused = false;
    try {
        read_class_list(file, Section::Cameras);
    } catch (const AssetError&) {
        refused = true;
    }
    CHECK(refused);

    const auto mobys = read_moby_instances(file);
    CHECK(mobys.size() == 3);
    CHECK(mobys[1].class_id == 501 && mobys[2].position[0] == 7.0f && mobys[0].scale == 0.5f);
    CHECK((mobys[0].ambient_rgb() == std::array<u8, 3>{1, 2, 3}));
    // The loader adds the channels as words: 0x100 red carries into green.
    CHECK((mobys[1].ambient_rgb() == std::array<u8, 3>{0, 1, 0}));
}

void pvars() {
    const auto bytes = invented_level();
    const GameplayFile file(Game::Rac1, bytes);
    CHECK((*read_pvar_block(file, 1) == std::vector<u8>{1, 0, 0, 0, 0, 0, 0, 0}));
    CHECK(!read_pvar_block(file, -1));
    const PvarFixups fixups = read_pvar_fixups(file);
    CHECK(fixups.moby_links.size() == 1 && fixups.pointers.empty());
    CHECK(read_pvar_shared_data(file).size() == 16);
    const auto shared = read_pvar_shared_records(file);
    CHECK(
        shared.size() == 1 && shared[0].pvar_index == 1 && shared[0].offset == 4
        && shared[0].data_offset == 8
    );

    auto word = [](const std::vector<u8>& b, std::size_t at) {
        s32 v;
        std::memcpy(&v, b.data() + at, 4);
        return v;
    };
    // Everything created: instance 2 is runtime moby 2.
    const auto all = read_pvars(file);
    CHECK(all.size() == 2 && word(*all[0], 0) == 2 && word(*all[0], 4) == 77);
    CHECK(word(*all[1], 4) == 8);  // the shared-data offset
    // Instance 1 not created: instance 2 becomes runtime moby 1.
    const bool spawned[] = {true, false, true};
    const auto some = read_pvars(file, spawned);
    CHECK(word(*some[0], 0) == 1);
}

void cameras() {
    const auto bytes = invented_level();
    const GameplayFile file(Game::Rac1, bytes);
    const auto list = read_level_cameras(file);
    CHECK(list.size() == 1);
    CHECK(list[0].record.class_id == 3 && list[0].record.position[2] == 3.0f);
    CHECK(list[0].record.rotation[2] == 1.5f);
    CHECK(list[0].pvar && list[0].pvar->size() == 8);
    // An 8-byte block is too short for any camera's header.
    CHECK(!CameraHeader::read(*list[0].pvar));
    // A region tweak block: priority 2, distance 9, mode 4.
    ByteWriter w;
    w.resize(0x58);
    w.put_at<u8>(0x1c, 2);
    w.put_at<f32>(0x24, 9.0f);
    w.put_at<s16>(0x2c, 4);
    const auto tweak = CameraRegionTweak::read(w.bytes());
    CHECK(tweak && tweak->header.priority == 2 && tweak->distance == 9.0f && tweak->mode == 4);
}

}  // namespace

int main() {
    sections();
    settings_and_instances();
    pvars();
    cameras();
    return openrac::test::result();
}
