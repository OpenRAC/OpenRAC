// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game's executable, read from the player's extracted disc. The port
// copies its loadable segments into game memory at their addresses, the way
// the console's loader did, so that the game's data (tables, constants,
// initialised globals) is where the decompiled code expects it. Its code
// bytes come along as data and are never run: every function the port runs
// is compiled from C.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "openrac/guest.h"

namespace openrac::runtime {

struct ElfSegment {
    gaddr address;
    std::uint32_t file_size;    // bytes copied from the file
    std::uint32_t memory_size;  // file_size plus the zeroed rest (.bss)
    std::uint32_t flags;        // PF_X 1, PF_W 2, PF_R 4
};

struct ElfImage {
    gaddr entry = 0;
    std::vector<ElfSegment> segments;
};

// Reads a 32-bit little-endian MIPS executable and describes its loadable
// segments. Returns false with a reason if the file is not one.
bool read_elf(const std::vector<std::uint8_t>& file, ElfImage* image, std::string* reason);

// Copies the loadable segments into game memory and zeroes the rest of each
// segment. Returns false with a reason if a segment falls outside mapped
// memory.
bool load_elf(const std::vector<std::uint8_t>& file, const ElfImage& image, std::string* reason);

// Reads the file at path whole.
bool read_file(const std::filesystem::path& path, std::vector<std::uint8_t>* out);

}  // namespace openrac::runtime
