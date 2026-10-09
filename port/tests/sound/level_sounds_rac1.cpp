// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/sound_bank.rs
// (its tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// RAC1's sound remap, the gadget defs, the lookup by owner and the two sound
// sections of the gameplay file, on synthetic data built from the layouts.

#include "assets/sound/level_sounds_rac1.h"

#include <vector>

#include "tests/check.h"

using namespace openrac::assets;

namespace {

SoundDef def_with_index(u16 index, f32 far = 24.0f) {
    SoundDef d;
    d.far = far;
    d.volume_near = 0x400;
    d.index = index;
    return d;
}

void remap() {
    constexpr std::size_t base = 0x10;
    ByteWriter index;
    index.resize(0x100);
    index.put_at<s16>(base + 0, 0x20);  // defs
    index.put_at<s16>(base + 2, 4);
    index.put_at<s16>(base + 4, 0x80);  // map
    index.put_at<s16>(base + 6, 2);
    index.put_at<s16>(base + 8, 0x90);  // class 0: two ids
    index.put_at<s16>(base + 10, 2);
    index.put_at<s16>(base + 12, 0xa0);  // class 1 (a gadget): one id
    index.put_at<s16>(base + 14, 1);
    const u16 def_indices[] = {0, 1, 5, 0xffff};
    for (std::size_t k = 0; k < 4; ++k) {
        index.put_at<SoundDef>(base + 0x20 + 0x20 * k, def_with_index(def_indices[k]));
    }
    index.put_at<u16>(base + 0x7c, 0x1234);  // the u16 before the map
    index.put_at<u16>(base + 0x80, 21);
    index.put_at<u16>(base + 0x84, 7);
    index.put_at<u16>(base + 0x90, 220);
    index.put_at<u16>(base + 0x94, 221);
    index.put_at<u16>(base + 0xa0, 30);

    ByteWriter core;
    core.resize(0x40);
    core.put_at<u8>(0x40 + 0x0d, 2);
    core.put_at<s32>(0x40 + 0x28, 0x30);
    core.put_at<SoundDef>(0x70, def_with_index(99, 128.0f));
    core.put_at<SoundDef>(0x90, def_with_index(99, 100.0f));

    const SoundRemapClass classes[] = {{688, 0x40}, {71, 0}};
    LevelSounds s =
        parse_level_sounds_rac1(ByteView(index.bytes()), base, classes, ByteView(core.bytes()));
    CHECK(s.map.size() == 2 && s.map[0] == 21 && s.map[1] == 7);
    CHECK(s.level_defs.size() == 4);
    CHECK(s.level_defs[0].index == 21 && s.level_defs[1].index == 7);
    CHECK(s.level_defs[2].index == 0xffff);
    CHECK(s.level_defs[3].index == 0x1234);  // -1 passes the signed compare
    CHECK(s.classes.size() == 2);
    CHECK(s.classes[0].bank_ids == std::vector<u16>({220, 221}));
    CHECK(s.classes[0].defs.size() == 2 && s.classes[0].header_count == u8{2});
    CHECK(s.classes[0].defs[0].index == 220 && s.classes[0].defs[1].index == 221);
    CHECK(s.classes[0].defs[0].far == 128.0f);
    CHECK(s.classes[1].defs.empty() && !s.classes[1].header_count);

    // The gadget's blob gets its parked id.
    ByteWriter gadget;
    gadget.resize(0x30);
    gadget.put_at<u8>(0x0d, 1);
    gadget.put_at<s32>(0x28, 0x30);
    gadget.put_at<SoundDef>(0x30, def_with_index(5));
    const GadgetBlob blobs[] = {{71, ByteView(gadget.bytes())}, {688, ByteView(gadget.bytes())}};
    apply_gadget_sounds_rac1(s, blobs);
    CHECK(s.classes[1].defs.size() == 1 && s.classes[1].defs[0].index == 30);
    CHECK(s.classes[1].header_count == u8{1});
    CHECK(s.classes[0].defs[0].far == 128.0f);  // a class with a level blob is left alone

    CHECK(
        parse_level_sounds_rac1(ByteView(index.bytes()), 0, classes, ByteView()).level_defs.empty()
    );
}

void resolve() {
    LevelSounds s;
    s.level_defs = {def_with_index(0), def_with_index(21), def_with_index(0xffff)};
    s.map = {0, 21};
    ClassSounds c;
    c.o_class = 688;
    c.bank_ids = {220};
    c.defs = {def_with_index(220)};
    c.header_count = 1;
    s.classes.push_back(c);
    CHECK(resolve_sound(s, SoundOwner::of_level(), 1) == u16{21});
    CHECK(!resolve_sound(s, SoundOwner::of_level(), 2));
    CHECK(!resolve_sound(s, SoundOwner::of_level(), 3));
    CHECK(resolve_sound(s, SoundOwner::of_class(688), 0) == u16{220});
    CHECK(!resolve_sound(s, SoundOwner::of_class(1), 0));
    const SoundDef* d = find_sound_def(s, SoundOwner::of_class(688), 0);
    CHECK(d != nullptr && d->index == 220);
    CHECK(find_sound_def(s, SoundOwner::of_class(688), 1) == nullptr);
}

void gameplay_sections() {
    ByteWriter g;
    g.resize(0x100);
    g.put_at<u32>(kSoundInstancesPointerRac1, 0x100);
    g.put_at<u32>(kEnvSamplePointsPointerRac1, 0x300);
    g.put_at<s32>(0x100, 2);
    for (std::size_t i = 0; i < 2; ++i) {
        const std::size_t o = 0x110 + 0x90 * i;
        g.put_at<s16>(o, static_cast<s16>(3 * i));
        g.put_at<s32>(o + 8, static_cast<s32>(10 + i));
        g.put_at<f32>(o + 0x0c, 24.0f);
        g.put_at<f32>(o + 0x40, 162.5f);  // matrix row 3: the position
        g.put_at<f32>(o + 0x44, 136.0f);
        g.put_at<f32>(o + 0x48, 60.5f);
        g.put_at<f32>(o + 0x50, 0.5f);  // inverse rows: a scale of 1/2
        g.put_at<f32>(o + 0x64, 0.5f);
        g.put_at<f32>(o + 0x78, 0.5f);
    }
    g.put_at<s32>(0x300, 1);
    g.put_at<f32>(0x310, 249.3f);
    g.put_at<s32>(0x330, 1500);
    g.put_at<u8>(0x334, 4);
    g.put_at<u8>(0x337, 1);
    g.put_at<s32>(0x338, 0);
    const auto instances = parse_sound_instances_rac1(ByteView(g.bytes()));
    CHECK(instances.size() == 2);
    CHECK(
        instances[1].o_class == 3 && instances[1].pvar_index == 11 && instances[1].range == 24.0f
    );
    CHECK(instances[0].position()[0] == 162.5f && instances[0].position()[2] == 60.5f);
    const auto local = instances[0].to_local({2.0f, -4.0f, 1.0f});
    CHECK(local[0] == 1.0f && local[1] == -2.0f && local[2] == 0.5f);
    const auto points = parse_env_sample_points_rac1(ByteView(g.bytes()));
    CHECK(points.size() == 1);
    CHECK(points[0].position[0] == 249.3f && points[0].reverb_depth == 1500);
    CHECK(points[0].reverb_type == 4 && points[0].reverb_enable == 1 && points[0].music_track == 0);
}

}  // namespace

int main() {
    remap();
    resolve();
    gameplay_sections();
    return openrac::test::result();
}
