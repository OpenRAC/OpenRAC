// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "disc.h"

#include <algorithm>
#include <cctype>

namespace sys {

using namespace ps2;

namespace {

std::string canonical(std::string name) {
  std::size_t semicolon = name.find(';');
  if (semicolon != std::string::npos) {
    name.erase(semicolon);
  }
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
  return name;
}

}  // namespace

Disc::~Disc() {
  if (file_) {
    std::fclose(file_);
  }
}

bool Disc::open(const std::string& path) {
  if (file_) {
    std::fclose(file_);
  }
  file_ = std::fopen(path.c_str(), "rb");
  if (!file_) {
    return false;
  }
  fseeko(file_, 0, SEEK_END);
  sectors_ = static_cast<u64>(ftello(file_)) / kSector;
  return true;
}

bool Disc::read(u32 sector, u32 count, u8* out) {
  if (!file_ || static_cast<u64>(sector) + count > sectors_) {
    return false;
  }
  fseeko(file_, static_cast<off_t>(sector) * static_cast<off_t>(kSector), SEEK_SET);
  return std::fread(out, kSector, count, file_) == count;
}

std::optional<Disc::File> Disc::find(const std::string& path) {
  // The primary volume descriptor is sector 16; its root directory record is
  // at offset 156. A record holds the extent's sector at 2, its length at 10,
  // flags at 25 (bit 1: directory), the name's length at 32 and the name at 33.
  std::vector<u8> sector(kSector);
  if (!read(16, 1, sector.data()) || std::string(sector.begin() + 1, sector.begin() + 6) != "CD001") {
    return std::nullopt;
  }
  File current{load<u32>(&sector[156 + 2]), load<u32>(&sector[156 + 10])};

  std::size_t at = 0;
  while (at < path.size()) {
    while (at < path.size() && (path[at] == '/' || path[at] == '\\')) {
      at++;
    }
    std::size_t end = path.find_first_of("/\\", at);
    if (end == std::string::npos) {
      end = path.size();
    }
    if (end == at) {
      break;
    }
    std::string want = canonical(path.substr(at, end - at));
    at = end;

    std::vector<u8> dir((current.bytes + kSector - 1) / kSector * kSector);
    if (!read(current.sector, static_cast<u32>(dir.size() / kSector), dir.data())) {
      return std::nullopt;
    }
    bool found = false;
    for (std::size_t r = 0; r + 34 <= dir.size();) {
      unsigned length = dir[r];
      if (length == 0) {
        r = (r / kSector + 1) * kSector;  // records do not cross sectors
        continue;
      }
      std::string name(dir.begin() + static_cast<long>(r) + 33, dir.begin() + static_cast<long>(r) + 33 + dir[r + 32]);
      if (canonical(name) == want) {
        current = File{load<u32>(&dir[r + 2]), load<u32>(&dir[r + 10])};
        found = true;
        break;
      }
      r += length;
    }
    if (!found) {
      return std::nullopt;
    }
  }
  return current;
}

std::vector<u8> Disc::read_file(const File& file) {
  std::vector<u8> bytes((file.bytes + kSector - 1) / kSector * kSector);
  if (!read(file.sector, static_cast<u32>(bytes.size() / kSector), bytes.data())) {
    return {};
  }
  bytes.resize(file.bytes);
  return bytes;
}

}  // namespace sys
