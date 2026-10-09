// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The memory card library, answered from a directory of the host: the card
// in the first slot is a directory, a folder on it a directory in that, a
// file a file. What the functions take and give is what the games' own card
// code shows (it checks every result) and what is publicly documented of
// the library's interface.
//
// Every function only starts its work on the console; the program asks with
// sceMcSync whether it is done and what came of it. Here the work is done at
// once and the answer is kept for that call.

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "machine.h"

namespace sys {
namespace {

namespace fs = std::filesystem;
using ps2::s32;

// The numbers sceMcSync gives for the function that finished.
enum : int {
  kGetInfo = 1, kOpen = 2, kClose = 3, kSeek = 4, kRead = 5, kWrite = 6, kMkdir = 11, kGetDir = 13, kDelete = 15,
  kFormat = 16, kUnformat = 17,
};
// Results, where not a count or a file number.
enum : int { kDone = 0, kNewCard = -1, kNoSuchEntry = -4, kDenied = -5, kNotEmpty = -6, kTooManyOpen = -7, kNoCard = -10 };
// What a card holds, in the 1,024-byte clusters the library counts in.
constexpr int kClusters = 8000;

void finish(Machine& m, int function, int result) {
  m.mc_function = function;
  m.mc_result = result;
  m.result(0);
}

bool present(Machine& m, u32 port, u32 slot) {
  return !m.card.directory.empty() && port == 0 && slot == 0;
}

// A name on the card as a place in the host's directory, or empty when the
// name would lead out of it.
fs::path host_path(Machine& m, std::string name) {
  fs::path path = m.card.directory;
  std::size_t at = 0;
  while (at < name.size()) {
    std::size_t end = name.find('/', at);
    if (end == std::string::npos) {
      end = name.size();
    }
    std::string part = name.substr(at, end - at);
    at = end + 1;
    if (part.empty() || part == ".") {
      continue;
    }
    if (part == ".." || part.find('\\') != std::string::npos || part.find(':') != std::string::npos) {
      return {};
    }
    path /= part;
  }
  return path;
}

int clusters_used(const fs::path& directory) {
  std::error_code error;
  u64 clusters = 0;
  for (fs::recursive_directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
    std::error_code ignored;
    clusters += it->is_regular_file(ignored) ? (it->file_size(ignored) + 1023) / 1024 : 1;
  }
  return static_cast<int>(std::min<u64>(clusters, kClusters));
}

// One entry of a directory listing as the library gives it: 64 bytes, with
// the times it was made and changed, its size, what it is, and its name.
void put_entry(Machine& m, u32 at, const std::string& name, bool directory, u64 size, std::time_t changed) {
  std::tm when{};
  if (std::tm* t = std::gmtime(&changed)) {
    when = *t;
  }
  for (u32 stamp = 0; stamp < 16; stamp += 8) {
    m.ee.write8(at + stamp + 0, 0);
    m.ee.write8(at + stamp + 1, static_cast<u8>(when.tm_sec));
    m.ee.write8(at + stamp + 2, static_cast<u8>(when.tm_min));
    m.ee.write8(at + stamp + 3, static_cast<u8>(when.tm_hour));
    m.ee.write8(at + stamp + 4, static_cast<u8>(when.tm_mday));
    m.ee.write8(at + stamp + 5, static_cast<u8>(when.tm_mon + 1));
    m.ee.write16(at + stamp + 6, static_cast<u16>(when.tm_year + 1900));
  }
  m.ee.write32(at + 16, static_cast<u32>(size));
  // Exists, closed, readable, writable, executable; then a file or a directory.
  m.ee.write16(at + 20, static_cast<u16>(0x8487 | (directory ? 0x20 : 0x10)));
  m.ee.write16(at + 22, 0);
  m.ee.write32(at + 24, 0);
  m.ee.write32(at + 28, 0);
  for (u32 n = 0; n < 32; n++) {
    m.ee.write8(at + 32 + n, n < name.size() && n < 31 ? static_cast<u8>(name[n]) : 0);
  }
}

std::time_t changed_at(const fs::path& path) {
  std::error_code error;
  auto time = fs::last_write_time(path, error);
  if (error) {
    return 0;
  }
  // (The file clock's zero is not the calendar's on every system.)
  auto now_file = fs::file_time_type::clock::now();
  auto now_system = std::chrono::system_clock::now();
  return std::chrono::system_clock::to_time_t(
      now_system + std::chrono::duration_cast<std::chrono::system_clock::duration>(time - now_file));
}

}  // namespace

Machine::Card::~Card() {
  for (auto& [number, file] : open) {
    std::fclose(file);
  }
}

void add_memory_card_services(Machine& machine) {
  machine.add_service("sceMcInit", [](Machine& m) { m.result(0); });

  // sceMcSync(mode, function, result): 1 when the last function has
  // finished, -1 when none was started.
  machine.add_service("sceMcSync", [](Machine& m) {
    if (m.mc_function == 0) {
      m.result(static_cast<u64>(-1));
      return;
    }
    if (m.arg(1)) m.ee.write32(m.arg(1), static_cast<u32>(m.mc_function));
    if (m.arg(2)) m.ee.write32(m.arg(2), static_cast<u32>(m.mc_result));
    m.mc_function = 0;
    m.result(1);
  });

  // sceMcGetInfo(port, slot, type, free, format): what is in the slot (2: a
  // card for this console), how many clusters are free, whether it is
  // formatted. The first time a card is asked about it is a new card.
  machine.add_service("sceMcGetInfo", [](Machine& m) {
    bool there = present(m, m.arg(0), m.arg(1));
    if (m.arg(2)) m.ee.write32(m.arg(2), there ? 2 : 0);
    if (m.arg(3)) m.ee.write32(m.arg(3), there ? static_cast<u32>(kClusters - clusters_used(m.card.directory)) : 0);
    if (m.arg(4)) m.ee.write32(m.arg(4), there ? 1 : 0);
    int result = !there ? kNoCard : m.card.seen ? kDone : kNewCard;
    if (there) {
      m.card.seen = true;
    }
    finish(m, kGetInfo, result);
  });

  // sceMcOpen(port, slot, name, mode): mode has 1 to read, 2 to write and
  // 0x200 to make the file if it is not there. Gives a file number.
  machine.add_service("sceMcOpen", [](Machine& m) {
    if (!present(m, m.arg(0), m.arg(1))) {
      finish(m, kOpen, kNoCard);
      return;
    }
    fs::path path = host_path(m, m.string_at(m.arg(2)));
    u32 mode = m.arg(3);
    std::error_code error;
    if (path.empty() || fs::is_directory(path, error)) {
      finish(m, kOpen, kNoSuchEntry);
      return;
    }
    if (m.card.open.size() >= 3) {
      finish(m, kOpen, kTooManyOpen);
      return;
    }
    bool exists = fs::exists(path, error);
    if (!exists && !(mode & 0x200)) {
      finish(m, kOpen, kNoSuchEntry);
      return;
    }
    std::FILE* file = nullptr;
    if (!exists) {
      if (!fs::is_directory(path.parent_path(), error)) {
        finish(m, kOpen, kNoSuchEntry);
        return;
      }
      file = std::fopen(path.string().c_str(), "w+b");
    } else {
      file = std::fopen(path.string().c_str(), (mode & 2) ? "r+b" : "rb");
    }
    if (!file) {
      finish(m, kOpen, kDenied);
      return;
    }
    int number = 0;
    while (m.card.open.count(number)) {
      number++;
    }
    m.card.open[number] = file;
    m.log(1, "memory card: opened %s (mode %x) as %d", path.string().c_str(), mode, number);
    finish(m, kOpen, number);
  });

  machine.add_service("sceMcClose", [](Machine& m) {
    auto found = m.card.open.find(static_cast<int>(m.arg(0)));
    if (found == m.card.open.end()) {
      finish(m, kClose, kNoSuchEntry);
      return;
    }
    std::fclose(found->second);
    m.card.open.erase(found);
    finish(m, kClose, kDone);
  });

  // sceMcSeek(file, offset, from): from the start (0), the position (1) or
  // the end (2). Gives the new position.
  machine.add_service("sceMcSeek", [](Machine& m) {
    auto found = m.card.open.find(static_cast<int>(m.arg(0)));
    if (found == m.card.open.end()) {
      finish(m, kSeek, kNoSuchEntry);
      return;
    }
    u32 from = m.arg(2);
    std::fseek(found->second, static_cast<long>(static_cast<s32>(m.arg(1))), from == 1 ? SEEK_CUR : from == 2 ? SEEK_END : SEEK_SET);
    finish(m, kSeek, static_cast<int>(std::ftell(found->second)));
  });

  // sceMcRead(file, to, bytes) and sceMcWrite(file, from, bytes) give the
  // number of bytes moved.
  machine.add_service("sceMcRead", [](Machine& m) {
    auto found = m.card.open.find(static_cast<int>(m.arg(0)));
    if (found == m.card.open.end()) {
      finish(m, kRead, kNoSuchEntry);
      return;
    }
    std::vector<u8> bytes(m.arg(2));
    std::size_t got = bytes.empty() ? 0 : std::fread(bytes.data(), 1, bytes.size(), found->second);
    for (std::size_t n = 0; n < got; n++) {
      m.ee.write8(m.arg(1) + static_cast<u32>(n), bytes[n]);
    }
    finish(m, kRead, static_cast<int>(got));
  });
  machine.add_service("sceMcWrite", [](Machine& m) {
    auto found = m.card.open.find(static_cast<int>(m.arg(0)));
    if (found == m.card.open.end()) {
      finish(m, kWrite, kNoSuchEntry);
      return;
    }
    std::vector<u8> bytes(m.arg(2));
    for (std::size_t n = 0; n < bytes.size(); n++) {
      bytes[n] = m.ee.read8(m.arg(1) + static_cast<u32>(n));
    }
    std::size_t put = bytes.empty() ? 0 : std::fwrite(bytes.data(), 1, bytes.size(), found->second);
    std::fflush(found->second);
    finish(m, kWrite, static_cast<int>(put));
  });

  // sceMcMkdir(port, slot, name): a folder that is there already is not an
  // error the games mind, but it is told apart.
  machine.add_service("sceMcMkdir", [](Machine& m) {
    if (!present(m, m.arg(0), m.arg(1))) {
      finish(m, kMkdir, kNoCard);
      return;
    }
    fs::path path = host_path(m, m.string_at(m.arg(2)));
    std::error_code error;
    if (path.empty()) {
      finish(m, kMkdir, kDenied);
    } else if (fs::exists(path, error)) {
      finish(m, kMkdir, kNoSuchEntry);  // the library's answer for "already there"
    } else {
      finish(m, kMkdir, fs::create_directory(path, error) ? kDone : kDenied);
    }
  });

  // sceMcGetDir(port, slot, name, mode, most, table): the entries a name
  // matches. "folder/*" lists the folder, with its "." and ".." first; any
  // other name is that one entry. Gives how many, at most `most`; with no
  // table they are only counted.
  machine.add_service("sceMcGetDir", [](Machine& m) {
    if (!present(m, m.arg(0), m.arg(1))) {
      finish(m, kGetDir, kNoCard);
      return;
    }
    std::string name = m.string_at(m.arg(2));
    s32 most = static_cast<s32>(m.arg(4));
    u32 table = m.arg(5);
    if (m.arg(3) != 0) {
      finish(m, kGetDir, 0);  // "the rest" of a listing: all of it was given the first time
      return;
    }
    struct Entry {
      std::string name;
      bool directory;
      u64 size;
      std::time_t changed;
    };
    std::vector<Entry> entries;
    std::error_code error;
    bool all = name.size() >= 2 && name.compare(name.size() - 2, 2, "/*") == 0;
    if (all || name == "*") {
      fs::path folder = host_path(m, name.substr(0, name.size() - 1));
      if (folder.empty() || !fs::is_directory(folder, error)) {
        finish(m, kGetDir, kNoSuchEntry);
        return;
      }
      std::time_t when = changed_at(folder);
      entries.push_back({".", true, 0, when});
      entries.push_back({"..", true, 0, when});
      std::vector<Entry> found;
      for (fs::directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
        std::error_code ignored;
        bool directory = it->is_directory(ignored);
        std::string entry = it->path().filename().string();
        if (!entry.empty() && entry[0] != '.') {
          found.push_back({entry, directory, directory ? 0 : it->file_size(ignored), changed_at(it->path())});
        }
      }
      std::sort(found.begin(), found.end(), [](const Entry& a, const Entry& b) { return a.name < b.name; });
      entries.insert(entries.end(), found.begin(), found.end());
    } else {
      fs::path path = host_path(m, name);
      if (!path.empty() && fs::exists(path, error)) {
        bool directory = fs::is_directory(path, error);
        entries.push_back({path.filename().string(), directory, directory ? 0 : fs::file_size(path, error), changed_at(path)});
      }
    }
    std::size_t count = entries.size();
    if (most >= 0) {
      count = std::min<std::size_t>(count, static_cast<std::size_t>(most));
    }
    if (table) {
      for (std::size_t n = 0; n < count; n++) {
        put_entry(m, table + static_cast<u32>(n) * 64, entries[n].name, entries[n].directory, entries[n].size, entries[n].changed);
      }
    }
    finish(m, kGetDir, static_cast<int>(count));
  });

  // sceMcDelete(port, slot, name): a file, or a folder with nothing in it.
  machine.add_service("sceMcDelete", [](Machine& m) {
    if (!present(m, m.arg(0), m.arg(1))) {
      finish(m, kDelete, kNoCard);
      return;
    }
    fs::path path = host_path(m, m.string_at(m.arg(2)));
    std::error_code error;
    if (path.empty() || path == fs::path(m.card.directory) || !fs::exists(path, error)) {
      finish(m, kDelete, kNoSuchEntry);
    } else if (fs::is_directory(path, error) && !fs::is_empty(path, error)) {
      finish(m, kDelete, kNotEmpty);
    } else {
      finish(m, kDelete, fs::remove(path, error) ? kDone : kDenied);
    }
  });

  // Formatting a directory of the host is nothing to do, and nothing is
  // erased for being asked to: a card here is always formatted.
  machine.add_service("sceMcFormat", [](Machine& m) { finish(m, kFormat, present(m, m.arg(0), m.arg(1)) ? kDone : kNoCard); });
  machine.add_service("sceMcUnformat", [](Machine& m) { finish(m, kUnformat, present(m, m.arg(0), m.arg(1)) ? kDone : kNoCard); });
}

}  // namespace sys
