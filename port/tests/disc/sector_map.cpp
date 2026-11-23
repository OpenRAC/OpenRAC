// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sector map (assets/disc/sector_map.h) on a synthetic image cut into files the way
// tools/openrac_common/disc_files.py cuts one: the file system's files at their paths, every
// other sector a lump. IsoImage reads the folder as it reads the image, byte for byte.

#include "assets/disc/sector_map.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>

#include "tests/check.h"
#include "tests/disc/synthetic_disc.h"

using namespace openrac::assets;
using namespace openrac::assets::disc;
using namespace openrac::test;

namespace fs = std::filesystem;

namespace {

void write(const fs::path& path, std::span<const u8> bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary)
        .write(
            reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        );
}

fs::path scratch() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root =
        fs::temp_directory_path() / ("openrac-sector-map-" + std::to_string(stamp));
    fs::create_directories(root);
    return root;
}

// The image cut into files: its one file of the file system at its path (its padding left
// out), every sector before and after it a lump.
void folder_reads_as_image() {
    std::vector<u8> data(5000);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<u8>(i * 13 + 1);
    }
    std::vector<u8> image = make_iso(40, {{"DATA.BIN", data}});
    // Sectors no file holds, past the file system, as the games keep their data.
    for (std::size_t i = 0; i < 4 * kSectorSize; ++i) {
        image.push_back(static_cast<u8>(i * 7));
    }
    const IsoImage iso(image_in_memory(image));
    const IsoEntry* file = iso.find("DATA.BIN");
    CHECK(file != nullptr);
    if (file == nullptr) {
        return;
    }
    const u32 sectors = static_cast<u32>(image.size() / kSectorSize);
    const u32 first = file->lba, count = (file->size + kSectorSize - 1) / kSectorSize;
    const fs::path root = scratch();
    const auto bytes = [&](u32 from, u32 n) {
        return std::span<const u8>(image).subspan(from * kSectorSize, n * kSectorSize);
    };
    write(root / "lumps/sectors/0000000.bin", bytes(0, first));
    write(root / "DATA.BIN", data);
    write(root / "lumps/sectors/rest.bin", bytes(first + count, sectors - first - count));
    std::ofstream(root / "sectors.tsv")
        << kSectorMapHeader << "\n"
        << "0\t" << first << "\t" << first * kSectorSize << "\tlumps/sectors/0000000.bin\n"
        << first << "\t" << count << "\t" << data.size() << "\tDATA.BIN\n"
        << first + count << "\t" << sectors - first - count << "\t"
        << (sectors - first - count) * kSectorSize << "\tlumps/sectors/rest.bin\n";

    CHECK(SectorMap::present(root));
    const SectorMap map = SectorMap::load(root);
    CHECK(map.sector_count() == sectors);
    CHECK(map.runs().size() == 3);

    // Every byte as the image has it, across the runs' edges, padding as zeros.
    std::vector<u8> whole(image.size());
    map.read(0, whole);
    CHECK(whole == image);
    std::vector<u8> across(3 * kSectorSize);
    map.read((first + count - 1) * kSectorSize - 100, across);
    CHECK(
        std::equal(
            across.begin(), across.end(), image.begin() + (first + count - 1) * kSectorSize - 100
        )
    );

    // read_some stops at the end; read throws past it.
    std::vector<u8> tail(3 * kSectorSize);
    CHECK(map.read_some(map.size() - kSectorSize, tail) == kSectorSize);
    bool threw = false;
    try {
        map.read(map.size() - kSectorSize, tail);
    } catch (const IoError&) {
        threw = true;
    }
    CHECK(threw);

    // The folder in place of an image: the same file system, the same bytes.
    const IsoImage from_files = IsoImage::open(root);
    const IsoEntry* again = from_files.find("DATA.BIN");
    CHECK(again != nullptr);
    if (again != nullptr) {
        CHECK(from_files.read_file(*again) == data);
    }

    std::error_code ignored;
    fs::remove_all(root, ignored);
}

// A map that is not one: the wrong first line, a gap between runs, a path out of the folder.
void refused() {
    const fs::path root = scratch();
    const auto refuses = [&](const std::string& text) {
        std::ofstream(root / "sectors.tsv") << text;
        try {
            (void)SectorMap::load(root);
        } catch (const IoError&) {
            return true;
        }
        return false;
    };
    CHECK(!SectorMap::present(root / "none"));
    CHECK(refuses("0\t1\t2048\ta.bin\n"));
    CHECK(refuses(std::string(kSectorMapHeader) + "\n0\t1\t2048\ta.bin\n2\t1\t2048\tb.bin\n"));
    CHECK(refuses(std::string(kSectorMapHeader) + "\n0\t1\t2048\t../a.bin\n"));
    CHECK(refuses(std::string(kSectorMapHeader) + "\n0\t1\t4096\ta.bin\n"));
    CHECK(refuses(std::string(kSectorMapHeader) + "\n"));
    std::error_code ignored;
    fs::remove_all(root, ignored);
}

}  // namespace

int main() {
    folder_reads_as_image();
    refused();
    return result();
}
