// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/level_overlay.rs
// and crates/rc-formats/src/font.rs (the overlay's sections): ISC License, Copyright (c)
// 2026 ReRAC contributors.
//
// RAC1's level overlay: the level's program (levels/NN/overlay.bin), its
// sections, its class tables, and a relocator that finds, in any level's
// overlay, the copy of a function or the data address a reference level's
// address names, by matching the code.
//
// Sections: {u32 dest, u32 size, u32 type, u32 entry} then `size` bytes; the
// list ends at the first header whose entry point differs from the first
// one (the game's own rule). On NTSC-U every overlay has seven sections in
// the same order: .lit (0x15ef00), .bss, .data, lvl.vtbl, lvl.camvtbl,
// lvl.sndvtbl, .text, as ReRAC checked against the NTSC decomp's section
// tables.
//
// Code identity: the engine code is the same object code in every overlay,
// linked at other addresses, so a function differs from its copy only in
// relocated fields. mask() hides exactly those: j/jal targets, lui immediates
// of main-RAM addresses (0x0010..0x003f, so float and small-integer luis are
// still compared) and the 16-bit immediates that complete such a lui (%lo,
// tracked per register, through moves too) or address $gp. Two functions are
// the same when their masked words agree over the reference function's
// extent (to the next known function start). A false negative only loses a
// port; a false positive would need two functions identical up to their
// relocations.

#pragma once

#include <map>
#include <optional>
#include <set>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

struct OverlaySection {
    u32 dest;
    u32 kind;  // ELF sh_type: 1 PROGBITS, 8 NOBITS
    u32 entry;
    std::vector<u8> data;

    bool operator==(const OverlaySection&) const = default;
};

// Splits a raw overlay lump into its sections.
std::vector<OverlaySection> parse_overlay_sections(ByteView overlay);

// `length` bytes at EE address `address` of the loaded sections, or nothing.
std::optional<ByteView> read_overlay(
    std::span<const OverlaySection> sections, u32 address, std::size_t length
);

// NTSC-U: the section indices of lvl.vtbl, lvl.camvtbl and .text, the same
// on all 19 levels.
inline constexpr std::size_t kVtblSection = 3;
inline constexpr std::size_t kCamVtblSection = 4;
inline constexpr std::size_t kTextSection = 6;

// NTSC-U: $gp in every level (the boot executable sets it once).
inline constexpr u32 kRac1NtscGp = 0x166c00;

// Longest function extent compared, in bytes.
inline constexpr u32 kMaxExtent = 0x8000;

// One lvl.vtbl record: a class and its update function (0: none).
struct VtblEntry {
    s32 o_class;
    u32 update;
    u32 w8;

    bool operator==(const VtblEntry&) const = default;
};

// One lvl.camvtbl record (0x14 bytes): a camera class and its four functions
// (NTSC-U UpdateAllCameras 0x20d620 reads them by mode).
struct CamVtblEntry {
    s32 klass;
    u32 activate;
    u32 init;
    u32 update;
    u32 pre;

    bool operator==(const CamVtblEntry&) const = default;
};

// The masked words of `words`, walked from a function start.
std::vector<u32> mask_code(std::span<const u32> words, u32 gp);

// (word index, address) of every lui/%lo pair completed in `words`, and of
// every $gp-relative access.
std::vector<std::pair<std::size_t, u32>> address_refs(std::span<const u32> words, u32 gp);

class LevelOverlay {
public:
    LevelOverlay(std::vector<OverlaySection> sections, u32 gp);
    static LevelOverlay parse(ByteView overlay, u32 gp);

    const std::vector<OverlaySection>& sections() const { return m_sections; }

    u32 gp() const { return m_gp; }

    // .text as words, and its address.
    std::span<const u32> text() const { return m_text; }

    u32 text_start() const { return m_text_start; }

    std::optional<ByteView> read(u32 address, std::size_t length) const;
    std::optional<u32> word(u32 address) const;

    // `count` code words at `address` of .text.
    std::optional<std::span<const u32>> code(u32 address, std::size_t count) const;

    // The lvl.vtbl records up to the -1 end marker.
    std::vector<VtblEntry> vtbl() const;

    // The lvl.camvtbl records up to the -1 end marker.
    std::vector<CamVtblEntry> camvtbl() const;

    // The extent of the function at `address`, in words: up to the next known
    // function start, at most kMaxExtent bytes and the end of .text.
    std::optional<std::size_t> extent(u32 address) const;

    const std::set<u32>& function_starts() const { return m_starts; }

    bool same_text(const LevelOverlay& other) const;

private:
    u32 text_end() const { return m_text_start + 4 * static_cast<u32>(m_text.size()); }

    std::vector<OverlaySection> m_sections;
    u32 m_gp;
    u32 m_text_start = 0;
    std::vector<u32> m_text;
    std::set<u32> m_starts;
};

// Translates reference (e.g. level 01) addresses to a target overlay's.
class Relocation {
public:
    Relocation(const LevelOverlay& reference, const LevelOverlay& target);

    // The target is the reference itself.
    bool is_identity() const { return m_identity; }

    // The target code at `target_fn` is the reference function `ref_fn`.
    bool same_code(u32 ref_fn, u32 target_fn) const;

    // Every copy of the reference function in the target's .text.
    std::vector<u32> copies(u32 ref_fn) const;

    // The target's copy (nothing when absent or found more than once).
    std::optional<u32> function(u32 ref_fn) const;

    // The target's address for a reference data address, found through a
    // reference function that forms it with lui/%lo and the same instruction
    // in that function's copy.
    std::optional<u32> data(u32 ref_address) const;

private:
    using Key = std::array<u32, 4>;

    struct KeyHash {
        std::size_t operator()(const Key& k) const;
    };

    std::optional<std::vector<u32>> reference_masked(u32 ref_fn) const;
    const std::unordered_map<u32, std::vector<std::pair<u32, std::size_t>>>& xrefs() const;

    const LevelOverlay* m_reference;
    const LevelOverlay* m_target;
    bool m_identity;
    std::unordered_map<Key, std::vector<u32>, KeyHash> m_index;
    mutable std::optional<std::unordered_map<u32, std::vector<std::pair<u32, std::size_t>>>>
        m_xrefs;
};

}  // namespace openrac::assets::disc
