// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The fonts: glyph tables found through the printing wrappers' call sites in
// a synthetic level program, and text widths.

#include "assets/world/font.h"

#include <cstring>
#include <string>

#include "tests/check.h"

using namespace openrac::assets;
using namespace openrac::assets::rac1;

namespace {

constexpr u32 kEntry = 0x0020'1000;
constexpr u32 kText = 0x0020'0000;
constexpr u32 kData = 0x0030'0000;

void section(ByteWriter& w, u32 dest, const std::vector<u8>& data) {
    w.put(dest);
    w.put(static_cast<u32>(data.size()));
    w.put<u32>(1);  // PROGBITS
    w.put(kEntry);
    w.put_bytes(data);
}

// A wrapper: call the FX texture getter with `face` in the delay slot, load
// `table` into t2, call the printer with the low half in its delay slot.
void wrapper(std::vector<u32>& code, u32 face, u32 table) {
    const u32 lo = table & 0xffff;
    const u32 hi = (table >> 16) + (lo >= 0x8000 ? 1 : 0);  // the addiu sign-extends
    code.insert(
        code.end(),
        {0x0c00'0100,
         0x2404'0000 | face,
         0x3c0a'0000 | hi,
         0x0000'0000,
         0x0c00'0200,
         0x254a'0000 | lo}
    );
}

std::vector<u8> words_to_bytes(const std::vector<u32>& code) {
    std::vector<u8> b(code.size() * 4);
    std::memcpy(b.data(), code.data(), b.size());
    return b;
}

GlyphTable table_with(u8 advance_of_a) {
    GlyphTable t{};
    t['A'] = {16, 32, 1, static_cast<s8>(advance_of_a)};
    t['b'] = {32, 32, 2, 9};
    t[0x0a] = {0, 0, 0, 0};  // a colour code draws nothing
    return t;
}

std::vector<u8> program(const std::array<u32, 3>& tables, std::optional<u32> second_regular = {}) {
    std::vector<u32> code(8, 0);
    for (u32 f = 0; f < 3; ++f) {
        wrapper(code, f + 1, tables[f]);
        code.push_back(0);
    }
    if (second_regular) {
        wrapper(code, 1, *second_regular);
    }
    std::vector<u8> data(0x1'0000, 0);
    for (std::size_t f = 0; f < 3; ++f) {
        if (tables[f] < kData || tables[f] + sizeof(GlyphTable) > kData + data.size()) {
            continue;  // a table the test puts outside the program
        }
        const GlyphTable t = table_with(static_cast<u8>(10 + f));
        std::memcpy(data.data() + (tables[f] - kData), t.data(), sizeof t);
    }
    ByteWriter w;
    section(w, kText, words_to_bytes(code));
    section(w, kData, data);
    return w.bytes();
}

template <typename F>
bool throws(F&& f) {
    try {
        f();
    } catch (const AssetError&) {
        return true;
    }
    return false;
}

void finds_tables() {
    // The large face's table needs the high half carried (low half 0xf000).
    const std::array<u32, 3> at = {kData + 0x100, kData + 0x800, kData + 0xf000};
    const LevelFonts fonts = read_level_fonts(program(at));
    CHECK(fonts.addresses == at);
    CHECK(fonts[FontFace::Regular]['A'].advance == 10);
    CHECK(fonts[FontFace::Small]['A'].advance == 11);
    CHECK(fonts[FontFace::Large]['A'].advance == 12);
    CHECK((fonts[FontFace::Large]['b'] == GlyphCell{32, 32, 2, 9}));
    CHECK(font_fx_texture(FontFace::Small) == 2);
}

void refuses() {
    const std::array<u32, 3> at = {kData + 0x100, kData + 0x800, kData + 0x1000};
    // Two wrappers of the regular face that load different tables.
    CHECK(throws([&] { read_level_fonts(program(at, kData + 0x2000)); }));
    // The same table twice is fine.
    CHECK(!throws([&] { read_level_fonts(program(at, kData + 0x100)); }));
    // A table outside the program.
    CHECK(throws([&] { read_level_fonts(program({kData + 0x100, kData + 0x800, 0x0040'0000})); }));
    // No wrapper at all.
    ByteWriter w;
    section(w, kText, std::vector<u8>(64, 0));
    CHECK(throws([&] { read_level_fonts(w.bytes()); }));
}

void widths() {
    const GlyphTable t = table_with(10);
    CHECK(text_width("Ab", t) == 19);
    // Colour codes add nothing.
    const std::string coloured = std::string("A") + '\x0a' + "b";
    CHECK(text_width(coloured, t) == 19);
    CHECK(text_width("AbAb", t, 3) == 29);                    // a limit
    CHECK(text_width(std::string_view("A\0b", 3), t) == 10);  // stops at the NUL
    CHECK(text_width("\xff", t) == 0);                        // past the table
}

}  // namespace

int main() {
    finds_tables();
    refuses();
    widths();
    return openrac::test::result();
}
