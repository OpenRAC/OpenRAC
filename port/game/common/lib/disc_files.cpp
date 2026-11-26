// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The disc for the game's libraries (game_host.h: openrac_disc_read and the rest): the files the
// extractor wrote in openrac_game_disc_files, read through its sector map
// (assets/disc/sector_map.h). The CD library (cdvd.c), the sound engine's streams and banks, the
// movies and Up Your Arsenal's programs all read here; none opens the disc image.

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <span>

#include "assets/disc/sector_map.h"
#include "common/log.h"
#include "openrac/game_host.h"

namespace {

namespace disc = openrac::assets::disc;

std::mutex g_load_lock;
std::optional<disc::SectorMap> g_map;
bool g_tried = false;

// The map, read the first time it is asked for (the folder is set before the game starts).
const disc::SectorMap* sector_map() {
    std::lock_guard lock(g_load_lock);

    if (!g_tried && openrac_game_disc_files != nullptr) {
        g_tried = true;

        try {
            g_map.emplace(disc::SectorMap::load(openrac_game_disc_files));
        } catch (const std::exception& e) {
            openrac::log::error("the disc's files: {}", e.what());
        }
    }

    return g_map ? &*g_map : nullptr;
}

}  // namespace

extern "C" int openrac_disc_available(void) {
    return sector_map() != nullptr;
}

extern "C" int openrac_disc_read(uint64_t offset, void* dst, uint64_t size) {
    const disc::SectorMap* map = sector_map();

    if (map == nullptr) {
        return 0;
    }

    try {
        map->read(
            offset, std::span(static_cast<std::uint8_t*>(dst), static_cast<std::size_t>(size))
        );
        return 1;
    } catch (const std::exception& e) {
        openrac::log::error("disc read: {}", e.what());
        return 0;
    }
}

extern "C" uint64_t openrac_disc_read_some(uint64_t offset, void* dst, uint64_t size) {
    const disc::SectorMap* map = sector_map();

    if (map == nullptr) {
        return 0;
    }

    try {
        return map->read_some(
            offset, std::span(static_cast<std::uint8_t*>(dst), static_cast<std::size_t>(size))
        );
    } catch (const std::exception& e) {
        openrac::log::error("disc read: {}", e.what());
        return 0;
    }
}

extern "C" uint64_t openrac_disc_file_size(const char* path) {
    if (openrac_game_disc_files == nullptr || path == nullptr) {
        return UINT64_MAX;
    }

    std::error_code error;
    const auto size =
        std::filesystem::file_size(std::filesystem::path(openrac_game_disc_files) / path, error);
    return error ? UINT64_MAX : static_cast<uint64_t>(size);
}

extern "C" int openrac_disc_read_file(const char* path, void* dst, uint64_t size) {
    if (openrac_game_disc_files == nullptr || path == nullptr) {
        return 0;
    }

    std::ifstream input(std::filesystem::path(openrac_game_disc_files) / path, std::ios::binary);
    input.read(static_cast<char*>(dst), static_cast<std::streamsize>(size));
    return input && static_cast<uint64_t>(input.gcount()) == size ? 1 : 0;
}
