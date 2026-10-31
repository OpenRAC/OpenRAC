// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Real disc-read replacement on synthetic sectors: switching off a map must drop cached patches.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "check.h"
#include "openrac/game_host.h"
#include "openrac/memory.h"

extern "C" {
const char* openrac_game_disc_image;
const char* openrac_game_mods_dir;
unsigned openrac_game_mods_generation;
int openrac_lib_sceCdRead(unsigned sector, unsigned count, gaddr buffer, gaddr mode);
}

int main() {
    namespace fs = std::filesystem;
    openrac::runtime::Memory::create();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() / ("openrac-mount-" + std::to_string(stamp));
    CHECK(fs::create_directory(root));
    const std::string disc = (root / "disc.bin").string(), folder = root.string();
    std::ofstream(disc, std::ios::binary) << std::string(4096, 'A');
    std::ofstream(root / "patch.bin", std::ios::binary) << std::string(2048, 'B');
    std::ofstream(root / "patches.tsv") << "1\t1\tpatch.bin\n";
    openrac_game_disc_image = disc.c_str();
    for (int pass = 0; pass < 4; ++pass) {
        const bool active = pass % 2 != 0;
        openrac_game_mods_dir = active ? folder.c_str() : nullptr;
        ++openrac_game_mods_generation;
        CHECK(openrac_lib_sceCdRead(0, 2, 0x10000, 0) == 1);
        CHECK(GREF(char, 0x10000) == 'A');
        CHECK(GREF(char, 0x10800) == (active ? 'B' : 'A'));
    }
    // Release cached patch handles before cleanup. The disc handle lives until process exit.
    openrac_game_mods_dir = nullptr;
    ++openrac_game_mods_generation;
    CHECK(openrac_lib_sceCdRead(1, 1, 0x10000, 0) == 1);
    CHECK(GREF(char, 0x10000) == 'A');
    // A read may cross from a patched physical sector into mod-owned sectors.
    std::ofstream(root / "virtual.bin", std::ios::binary) << std::string(4096, 'V');
    {
        std::ofstream virtuals(root / "virtual.tsv");
        virtuals << "2\t1\tvirtual.bin\n";
        for (unsigned i = 0; i < 1025; ++i) {
            virtuals << 0x10000000u + i << "\t1\tvirtual.bin\n";
        }
    }
    openrac_game_mods_dir = folder.c_str();
    ++openrac_game_mods_generation;
    CHECK(openrac_lib_sceCdRead(1, 2, 0x10000, 0) == 1);
    CHECK(GREF(char, 0x10000) == 'B');
    CHECK(GREF(char, 0x10800) == 'V');
    // The 1,025th mapping is loaded, and high offsets use 64-bit file seeks.
    CHECK(openrac_lib_sceCdRead(0x10000400u, 1, 0x10000, 0) == 1);
    CHECK(GREF(char, 0x10000) == 'V');
    openrac_game_mods_dir = nullptr;
    ++openrac_game_mods_generation;
    CHECK(openrac_lib_sceCdRead(0, 1, 0x10000, 0) == 1);
    CHECK(GREF(char, 0x10000) == 'A');
    std::error_code ignored;
    fs::remove(root / "virtual.bin", ignored);
    fs::remove(root / "virtual.tsv", ignored);
    fs::remove(root / "patch.bin", ignored);
    fs::remove(root / "patches.tsv", ignored);
    fs::remove(root / "disc.bin", ignored);
    fs::remove(root, ignored);
    return openrac::test::result();
}
