// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/save_game.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The save file's sections, CRC and tables.

#include "assets/disc/save_game.h"

#include <algorithm>
#include <cstring>

namespace openrac::assets::disc {

namespace {

std::size_t align4(std::size_t n) {
    return (n + 3) & ~std::size_t{3};
}

}  // namespace

std::size_t section_size(std::span<const ChunkDesc> descs) {
    std::size_t n = 8 + 8;

    // Each chunk takes its size, padded to 4 bytes, plus an 8-byte header.
    for (const ChunkDesc& d : descs) {
        n += align4(d.size) + 8;
    }

    return n;
}

u16 save_crc16(std::span<const u8> data) {
    // A buffer longer than the checksum covers is not checksummed.
    if (data.size() > kCrcMaxLength) {
        return 0;
    }

    u32 r = 0xedb88320;

    // Feed the bytes one at a time into the checksum.
    for (const u8 b : data) {
        r ^= u32{b} << 8;

        // Eight shifts per byte, each one a step of the CRC.
        for (int i = 0; i < 8; ++i) {
            r = (r & 0x8000) != 0 ? (r << 1) ^ 0x1f45 : r << 1;
        }
    }

    return static_cast<u16>(r & 0xffff);
}

Chunk Chunk::make(s32 id, std::vector<u8> data) {
    std::vector<u8> pad(align4(data.size()) - data.size(), 0);
    return {id, std::move(data), std::move(pad)};
}

const Chunk* Section::chunk(s32 id) const {
    // Search the chunks for the one with this id.
    for (const Chunk& c : chunks) {
        // Chunk ids are unique, so the first match is the chunk.
        if (c.id == id) {
            return &c;
        }
    }

    return nullptr;
}

std::size_t Section::encoded_size() const {
    std::size_t n = 8 + 8;

    // Each chunk takes its size, padded to 4 bytes, plus an 8-byte header.
    for (const Chunk& c : chunks) {
        n += 8 + align4(c.data.size());
    }

    return n;
}

std::vector<u8> Section::encode() const {
    ByteWriter w;
    w.put<u32>(0);
    w.put<u32>(0);

    // Write each chunk as its id, size, data and padding.
    for (const Chunk& c : chunks) {
        w.put<s32>(c.id);
        w.put<u32>(static_cast<u32>(c.data.size()));
        w.put_bytes(c.data);
        const std::size_t n = align4(c.data.size()) - c.data.size();

        // Pad each chunk to 4 bytes, keeping the padding bytes the file had.
        for (std::size_t k = 0; k < n; ++k) {
            w.put<u8>(k < c.pad.size() ? c.pad[k] : 0);
        }
    }

    w.put<s32>(kChunkEndId);
    w.put<u32>(0);
    std::vector<u8>& v = w.bytes();
    const u32 size = static_cast<u32>(v.size() - 8);
    const u32 crc = save_crc16(std::span<const u8>(v).subspan(8));
    w.put_at<u32>(0, size);
    w.put_at<u32>(4, crc);
    return std::move(v);
}

Section Section::decode(ByteView bytes) {
    const u32 data_size = bytes.u32_at(0);
    const u32 stored = bytes.u32_at(4);
    const ByteView data = bytes.sub(8, data_size, "section data");
    Section s;
    s.crc_ok = stored != 0 && save_crc16(data.span()) == stored;
    std::size_t p = 0;

    // Read chunks until the end id; the loop exits from inside.
    while (true) {
        const s32 id = data.s32_at(p);
        const u32 size = data.u32_at(p + 4);

        // The end id closes the section.
        if (id == kChunkEndId) {
            break;
        }

        const ByteView body = data.sub(p + 8, align4(size), "chunk data");
        s.chunks.push_back({id, body.sub(0, size).to_vector(), body.tail(size).to_vector()});
        p += 8 + align4(size);
    }

    return s;
}

SaveFile SaveFile::parse(ByteView bytes) {
    const u32 gs = bytes.u32_at(0);
    const u32 ls = bytes.u32_at(4);

    // A section smaller than its own header is damaged.
    if (gs < 16 || ls < 16) {
        fail("save file: implausible section sizes {:#x}/{:#x}", gs, ls);
    }

    bytes.check(0, 8 + std::size_t{gs} + kSaveLevelSlots * ls, "save file");
    SaveFile f;
    f.global = Section::decode(bytes.sub(8, gs));

    // The global section is followed by one section per level slot.
    for (std::size_t l = 0; l < kSaveLevelSlots; ++l) {
        f.levels[l] = Section::decode(bytes.sub(8 + gs + l * ls, ls));
    }

    return f;
}

std::vector<u8> SaveFile::to_bytes() const {
    const std::vector<u8> g = global.encode();
    std::vector<std::vector<u8>> lv;

    // Encode every level section before the sizes are known.
    for (const Section& s : levels) {
        lv.push_back(s.encode());
    }

    ByteWriter w;
    w.put<u32>(static_cast<u32>(g.size()));
    w.put<u32>(static_cast<u32>(lv[0].size()));
    w.put_bytes(g);

    // Append the level sections in slot order.
    for (const auto& l : lv) {
        w.put_bytes(l);
    }

    return std::move(w.bytes());
}

bool SaveFile::all_crcs_ok() const {
    return global.crc_ok
           && std::all_of(levels.begin(), levels.end(), [](const Section& s) { return s.crc_ok; });
}

void SaveFile::write_incremental(std::size_t level, std::span<u8> card_file) const {
    // A level slot past the table does not exist.
    if (level >= kSaveLevelSlots) {
        fail("level slot {} out of range", level);
    }

    const std::vector<u8> g = global.encode();
    const std::vector<u8> l = levels[level].encode();
    const std::size_t at = 8 + g.size() + level * l.size();

    // The card file must already hold the level slot being rewritten.
    if (card_file.size() < at + l.size()) {
        fail("incremental save: the card file is shorter than a whole save");
    }

    std::memcpy(card_file.data() + 8, g.data(), g.size());
    std::memcpy(card_file.data() + at, l.data(), l.size());
}

std::string save_file_name(std::size_t slot) {
    return std::format("save{}.bin", slot);
}

std::string card_directory(ByteView system_cnf) {
    system_cnf.check(0, 27, "SYSTEM.CNF");
    const u8* s = system_cnf.data();
    std::string n = "/BA****-*****RATCHET";
    n[2] = s[0x12] == 'E' ? 'E' : 'A';

    // Copy the four characters of the boot name's serial number.
    for (std::size_t i = 0; i < 4; ++i) {
        n[3 + i] = static_cast<char>(s[16 + i]);
    }

    // Copy the three characters of the next field.
    for (std::size_t i = 0; i < 3; ++i) {
        n[8 + i] = static_cast<char>(s[21 + i]);
    }

    // Copy the two characters of the last field.
    for (std::size_t i = 0; i < 2; ++i) {
        n[11 + i] = static_cast<char>(s[25 + i]);
    }

    // Check the whole name for characters outside ASCII.
    for (const char c : n) {
        // A byte with the top bit set is not ASCII, so the name is not what the file system
        // expects.
        if (static_cast<unsigned char>(c) >= 0x80) {
            fail("SYSTEM.CNF: non-ASCII boot name");
        }
    }

    return n;
}

SaveGameLump SaveGameLump::parse(ByteView lump) {
    auto part = [&](std::size_t i, std::string_view what) {
        return lump.sub(lump.u32_at(i * 8), lump.u32_at(i * 8 + 4), what).to_vector();
    };
    return {part(0, "icon.sys"), part(1, "static.ico"), part(2, "save template")};
}

}  // namespace openrac::assets::disc
