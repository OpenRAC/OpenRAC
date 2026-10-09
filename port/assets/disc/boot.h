// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-extract/src/identify.rs
// (SYSTEM.CNF) and crates/rc-formats/src/tfrag_light.rs (elf_read): ISC License,
// Copyright (c) 2026 ReRAC contributors.
//
// What a PlayStation 2 disc boots: SYSTEM.CNF's lines (BOOT2 names the boot
// executable, whose file name is the product serial, VER the disc version,
// VMODE NTSC or PAL), and reads from the boot executable's loaded segments.

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "assets/bytes.h"

namespace openrac::assets::disc {

struct SystemCnf {
    std::string boot;     // the disc path BOOT2 names, "/SCUS_971.99"
    std::string serial;   // its file name, "SCUS_971.99"
    std::string version;  // VER, empty when absent
    std::string vmode;    // VMODE, empty when absent

    // Nothing when there is no BOOT2 line.
    static std::optional<SystemCnf> parse(std::string_view text);
};

// The value of `KEY = value` (up to ';', CR or LF), or nothing.
std::optional<std::string> cnf_value(std::string_view text, std::string_view key);

// `length` bytes at virtual address `vaddr` of a little-endian ELF32's PT_LOAD
// segments (only bytes the file holds). Throws when no segment holds them.
ByteView elf_read(ByteView elf, u32 vaddr, std::size_t length);

}  // namespace openrac::assets::disc
