// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "ps2/types.h"

namespace sys {

// The user's own disc image: sectors of 2,048 bytes, and the ISO 9660
// directory for the few files the games open by name.
class Disc {
public:
    static constexpr std::size_t kSector = 2048;

    struct File {
        ps2::u32 sector = 0;
        ps2::u32 bytes = 0;
    };

    ~Disc();
    bool open(const std::string& path);

    bool is_open() const { return file_ != nullptr; }

    ps2::u64 sectors() const { return sectors_; }

    // Reads whole sectors; false if any lies outside the image.
    bool read(ps2::u32 sector, ps2::u32 count, ps2::u8* out);
    // A file by its path from the root, in any case, with or without ";1".
    std::optional<File> find(const std::string& path);
    std::vector<ps2::u8> read_file(const File& file);

private:
    std::FILE* file_ = nullptr;
    ps2::u64 sectors_ = 0;
};

}  // namespace sys
