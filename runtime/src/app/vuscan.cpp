// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-vuscan: finds the VU microprograms stored in a file (a game's
// executable, read from your own disc) and reports whether the interpreter
// knows every instruction in them. It prints counts and instruction words it
// could not decode, never the programs themselves.
//
// A stored program is looked for as the games store them for VIF1: a run of
// VIF codes that begins with FLUSH and MPG and goes on with MPG blocks.

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "ps2/gif.h"
#include "ps2/gs.h"
#include "ps2/vif.h"
#include "ps2/vu.h"

using namespace ps2;

namespace {

constexpr u32 kEmpty = 0xDEADC0DE;  // marks program memory nothing was loaded into

std::vector<u8> read_file(const char* path) {
  std::vector<u8> bytes;
  if (std::FILE* f = std::fopen(path, "rb")) {
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    bytes.resize(static_cast<std::size_t>(size));
    if (std::fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) {
      bytes.clear();
    }
    std::fclose(f);
  }
  return bytes;
}

// Does the interpreter decode this pair? Run it alone and see.
bool known(u32 upper, u32 lower) {
  static std::array<u8, 4096> micro, data;
  store<u32>(&micro[0], lower);
  store<u32>(&micro[4], upper);
  Vu vu(Vu::Memory{micro.data(), 4096, data.data(), 4096});
  vu.run(0, 1);
  return vu.unknown_ops == 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: openrac-vuscan FILE [FIRST_OFFSET LAST_OFFSET]\n");
    return 2;
  }
  std::vector<u8> file = read_file(argv[1]);
  if (file.empty()) {
    std::fprintf(stderr, "cannot read %s\n", argv[1]);
    return 1;
  }
  std::size_t first = argc > 2 ? std::strtoul(argv[2], nullptr, 0) : 0;
  std::size_t last = argc > 3 ? std::strtoul(argv[3], nullptr, 0) : file.size();
  last = std::min(last, file.size());

  int programs = 0;
  u64 total = 0, total_unknown = 0;
  for (std::size_t at = first & ~std::size_t{3}; at + 8 <= last; at += 4) {
    if (load<u32>(&file[at]) != 0x11000000u || (load<u32>(&file[at + 4]) >> 24) != 0x4A) {
      continue;
    }
    // The run: FLUSH, then MPG blocks (a count of instructions, eight bytes
    // each), with NOPs between them allowed.
    std::size_t end = at + 4;
    while (end + 4 <= last) {
      u32 code = load<u32>(&file[end]);
      if ((code >> 24) == 0x4A) {
        u32 count = (code >> 16) & 0xFF;
        std::size_t next = end + 4 + std::size_t{count ? count : 256} * 8;
        if (next > last) {
          break;
        }
        end = next;
      } else if (code == 0 && end + 8 <= last && (load<u32>(&file[end + 4]) >> 24) == 0x4A) {
        end += 4;
      } else {
        break;
      }
    }
    u32 quadwords = static_cast<u32>((end - at + 15) / 16);

    // Load it the way the game does, through VIF1, and see what landed.
    Gs gs;
    Gif gif(gs);
    Vif1 vif(gif);
    for (std::size_t i = 0; i < vif.micro.size(); i += 4) {
      store<u32>(&vif.micro[i], kEmpty);
    }
    vif.write(&file[at], end - at);

    u32 instructions = 0, immediates = 0, unknown = 0, lowest = 0xFFFFFFFF, highest = 0;
    std::map<u64, u32> unknown_words;
    for (u32 n = 0; n < Vif1::kMemoryBytes / 8; n++) {
      u32 lower = load<u32>(&vif.micro[n * 8]), upper = load<u32>(&vif.micro[n * 8 + 4]);
      if (lower == kEmpty && upper == kEmpty) {
        continue;
      }
      instructions++;
      lowest = std::min(lowest, n);
      highest = std::max(highest, n);
      if (upper & 0x80000000u) {
        immediates++;
      }
      if (!known(upper, lower)) {
        unknown++;
        unknown_words[(u64{upper} << 32) | lower]++;
      }
    }
    std::printf("program at 0x%zx: %u quadwords, %u instructions at %u-%u, %u with a number for I, %u not decoded, "
                "%llu codes VIF1 did not know\n",
                at, quadwords, instructions, lowest, highest, immediates, unknown,
                static_cast<unsigned long long>(vif.unknown_codes));
    int shown = 0;
    for (const auto& [word, count] : unknown_words) {
      if (shown++ == 12) {
        std::printf("    ...\n");
        break;
      }
      std::printf("    upper %08x lower %08x  x%u\n", static_cast<u32>(word >> 32), static_cast<u32>(word), count);
    }
    programs++;
    total += instructions;
    total_unknown += unknown;
    at = end - 4;
  }
  std::printf("%d programs, %llu instructions, %llu not decoded\n", programs, static_cast<unsigned long long>(total),
              static_cast<unsigned long long>(total_unknown));
  return total_unknown ? 1 : 0;
}
