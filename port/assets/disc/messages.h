// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/strings.rs
// (spec: docs/plan/hud_text.md section 5): ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// RAC1's level messages (help boxes, banners, subtitles) in a decompressed
// gameplay file. The header word at +0x10 + 4 * language points to a text
// block (NTSC-U's level loader 0x255958 copies the current language's to the
// level heap):
//
//   u32 count, u32 size
//   count x {s32 text offset (from the block), s32 id, s32 help audio (-1
//            none), s32 0}
//   NUL-terminated strings
//
// NTSC-U msg_string__Fi (0x2259e8) finds an id with a linear scan, first
// match, and shows "Paradox! This message does not exist" when it is absent.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

// Languages as RAC1 numbers them; 1 is unused (its block is empty).
enum class Language : u32 {
    English = 0,
    French = 2,
    German = 3,
    Spanish = 4,
    Italian = 5,
};

inline constexpr u32 kLanguageSlots = 8;

inline constexpr std::string_view kMissingMessage = "Paradox! This message does not exist";

struct Message {
    s32 id;
    std::string text;  // up to the NUL, control codes kept
    s32 help_audio;    // -1: none

    bool operator==(const Message&) const = default;
};

// One text block starting at byte 0 of `block`.
std::vector<Message> parse_text_block(ByteView block);

// The messages of `language` (a slot number, 0..7) of a decompressed gameplay file.
std::vector<Message> parse_messages(ByteView gameplay, u32 language);

// The index of the first message with `id`.
std::optional<std::size_t> find_message(const std::vector<Message>& messages, s32 id);

// The text of `id`, or the "Paradox!" fallback.
std::string_view message_text(const std::vector<Message>& messages, s32 id);

// Printable ASCII as is, everything else (colour codes 0x08..0x0f, pad icons
// 0x10..0x19, the newline 0x01, accented letters 0x80..) as \xNN.
std::string printable(std::string_view text);

}  // namespace openrac::assets::disc
