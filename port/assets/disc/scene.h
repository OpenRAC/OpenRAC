// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/scene.rs
// (spec: docs/plan/cutscenes_transitions.md sections 1 and 2): ISC License, Copyright (c)
// 2026 ReRAC contributors.
//
// RAC1's in-engine scenes (cutscenes): the level header's scene table and
// the WAD-compressed scene chunks.
//
// Table (level header +0x184, toc.h's SceneRecord): 15 records, each the
// speech VAG sector per language and the NTSC and PAL chunk sectors. A
// region's chunks are contiguous, so a region is one file
// (levels/NN/scene/KK_ntsc.bin, chunks and sentinel).
//
// Chunk (NTSC-U decompresses it into 0x16cd38 and parses it with
// FUN_00259288): 96 ticks on NTSC, 80 on PAL. Header {s16 end tick, s16 0,
// s32 subtitle table offset (< 0x400: none), s16 audio start tick, s16 -1,
// u16 actor count, u16 0, s32 camera table offset, s32 actor offsets[count]}.
// The camera table has one 0x20-byte record per tick plus one (the last
// equals the next chunk's first). An actor record is {s32 class, s32, s32,
// s32 position track offset} followed by a moby sequence (header at +0x10,
// frame offsets relative to +0x10) whose frames run at 30 Hz; the position
// track is one vec4 per frame. Subtitles are 16-byte {s16 start, s16 end, s16
// text[5] (English, French, German, Spanish, Italian, offsets from the table),
// s16 0} ended by start = -1, then the strings.
//
// The actors' sequences are decoded by the moby animation reader; a chunk
// keeps its decompressed bytes and gives each actor's sequence offset.

#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "assets/bytes.h"
#include "assets/disc/toc.h"

namespace openrac::assets::disc {

enum class SceneRegion : u8 {
    Ntsc,
    Pal,
};

// Ticks per chunk (NTSC-U CutsceneModeUpdate 0x2aca80).
u32 ticks_per_chunk(SceneRegion region);
std::string_view scene_region_name(SceneRegion region);  // "ntsc", "pal"

inline constexpr s32 kSubtitleMinOffset = 0x400;
inline constexpr std::size_t kSubtitleLanguages = 5;

struct ChunkRange {
    u32 sector;
    u32 sectors;

    bool operator==(const ChunkRange&) const = default;
};

// One region's chunk list: the chunks and the sentinel sector after them.
struct RegionTable {
    std::vector<ChunkRange> chunks;
    u32 sentinel;

    // From the non-zero leading entries of a record's list (strictly
    // increasing). Nothing for an empty list.
    static std::optional<RegionTable> from_sectors(std::span<const s32> sectors);

    u32 first_sector() const;
    // The region file's size: every chunk and the sentinel sector.
    std::size_t file_bytes() const;
    // Chunk i's byte range {offset, size} in the region file.
    std::optional<std::pair<std::size_t, std::size_t>> chunk_in_file(std::size_t i) const;
};

struct SceneEntry {
    std::array<std::optional<u32>, 6> speech;  // by the game's language number
    std::optional<RegionTable> ntsc;
    std::optional<RegionTable> pal;

    static SceneEntry from_record(const SceneRecord& r);
    const std::optional<RegionTable>& region(SceneRegion r) const;
    bool empty() const;
};

std::vector<SceneEntry> scene_table(const LevelHeader& h);

// The chunks of a global scene lump: a 0x800-byte table of {s32 offset, s32
// size} ended by size 0 (at most 70), chunk data at 0x800 + offset.
std::vector<ByteView> lump_chunks(ByteView lump);

struct ChunkHeader {
    s16 end_tick;  // the scene's end tick, the same in every chunk
    s16 unknown_02;
    s32 subtitle_offset;
    s16 audio_start;
    s16 unknown_0a;
    u16 actor_count;
    u16 unknown_0e;
    s32 camera_offset;
};

static_assert(sizeof(ChunkHeader) == 0x14);

// One camera record per 60 Hz tick (NTSC-U FUN_002ac8d8).
struct CamRecord {
    std::array<f32, 3> eye;
    u32 cut;                    // the low byte is the cut flag
    std::array<f32, 3> angles;  // radians, for rotations about X, Y, Z
    f32 tan_half_fov;

    bool is_cut() const { return (cut & 0xff) != 0; }
};

static_assert(sizeof(CamRecord) == 0x20);

struct SceneActor {
    u32 offset;  // record offset in the chunk
    s32 klass;   // the moby class
    s32 unknown_04;
    s32 unknown_08;
    s32 track_offset;
    u32 sequence_offset;  // the moby sequence's header: offset + 0x10
    u8 frame_count;
    // The sequence header's trigger-count byte as stored: scene sequences
    // store 0xff there and no trigger words follow the frame offsets.
    u8 trigger_count;
    std::vector<u32> frame_offsets;             // as stored, relative to the sequence
    std::vector<std::array<f32, 4>> positions;  // one per frame, w = 0
};

struct Subtitle {
    s16 start;  // scene ticks, inclusive
    s16 end;
    std::array<s16, kSubtitleLanguages> text_offsets;
    s16 pad;
    std::array<std::string, kSubtitleLanguages> text;

    bool covers(s32 tick) const { return start <= tick && tick <= end; }
};

struct SceneChunk {
    std::vector<u8> bytes;  // decompressed
    ChunkHeader header;
    std::vector<CamRecord> camera;
    std::vector<SceneActor> actors;
    std::vector<Subtitle> subtitles;

    ByteView sequence(const SceneActor& actor) const;
};

// Camera records of chunk `index` of a scene ending at `end_tick`.
std::size_t camera_records(s32 end_tick, std::size_t index, SceneRegion region);

// Chunk `index` of its scene, decompressed.
SceneChunk parse_scene_chunk(std::vector<u8> decompressed, std::size_t index, SceneRegion region);

struct Scene {
    std::size_t index;  // record index (the id the game's dialog start takes)
    SceneRegion region;
    std::vector<SceneChunk> chunks;

    // Scene k of a level from its header and region file, every chunk decompressed.
    static Scene load(
        const LevelHeader& header, ByteView region_file, std::size_t k, SceneRegion region
    );
    // Checks the chunks agree on the end tick and cover it.
    static Scene from_chunks(std::size_t index, SceneRegion region, std::vector<SceneChunk> chunks);
    // A global scene lump.
    static Scene from_lump(ByteView lump, std::size_t index, SceneRegion region);

    s32 end_tick() const { return chunks.front().header.end_tick; }

    s32 audio_start() const { return chunks.front().header.audio_start; }

    std::vector<s32> actor_classes() const;

    // {chunk, chunk tick} at scene tick `tick`.
    std::pair<std::size_t, u32> chunk_at(s32 tick) const;

    const CamRecord* camera_at(s32 tick) const;

    std::vector<s32> cut_ticks() const;

    const Subtitle* subtitle_at(s32 tick) const;
};

}  // namespace openrac::assets::disc
