// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/spatial_link_tiles.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#endif

#include "enhanced/lighting/spatial_watershed.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned kPageBytes = 4 * 1024;
constexpr unsigned kWindowBytes = 64 * 1024;
constexpr unsigned kPagesPerWindow = kWindowBytes / kPageBytes;
constexpr unsigned kWindowsPerSegment = 128;
constexpr unsigned kSegmentBytes = kWindowBytes * kWindowsPerSegment;

#ifndef _WIN32
bool reserve_scratch_file(std::FILE* file) {
#if defined(__linux__) && !defined(__ANDROID__)
    const int error = posix_fallocate(fileno(file), 0, kSegmentBytes);
    if (error == 0) {
        return true;
    }
    if (error != EOPNOTSUPP && error != ENOSYS && error != EINVAL) {
        return false;
    }
#endif
    const std::array<char, 64 * 1024> zeros{};
    for (unsigned offset = 0; offset < kSegmentBytes; offset += zeros.size()) {
        if (std::fwrite(zeros.data(), 1, zeros.size(), file) != zeros.size()) {
            return false;
        }
    }
    return std::fflush(file) == 0;
}
#endif

class ScratchMapping {
public:
    ScratchMapping() {
#ifdef _WIN32
        static std::atomic_uint64_t serial{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto file = std::filesystem::temp_directory_path()
                          / ("openrac-watershed-" + std::to_string(GetCurrentProcessId()) + "-"
                             + std::to_string(stamp) + "-" + std::to_string(serial++) + ".tmp");
        m_file = CreateFileW(
            file.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
            nullptr
        );
        if (m_file != INVALID_HANDLE_VALUE) {
            m_mapping =
                CreateFileMappingW(m_file, nullptr, PAGE_READWRITE, 0, kSegmentBytes, nullptr);
            if (!m_mapping) {
                CloseHandle(m_file);
            }
        }
        if (!m_mapping) {
            throw std::runtime_error("cannot allocate watershed tile scratch storage");
        }
#else
        m_file = std::tmpfile();
        // Reserve backing space before exposing pages to avoid a fault on a full disk.
        if (!m_file || !reserve_scratch_file(m_file)) {
            if (m_file) {
                std::fclose(m_file);
            }
            throw std::runtime_error("cannot allocate watershed tile scratch storage");
        }
#endif
    }

    ~ScratchMapping() {
#ifdef _WIN32
        CloseHandle(m_mapping);
        CloseHandle(m_file);
#else
        std::fclose(m_file);
#endif
    }

    ScratchMapping(const ScratchMapping&) = delete;
    ScratchMapping& operator=(const ScratchMapping&) = delete;

    char* map(unsigned window) const {
        const unsigned offset = window * kWindowBytes;
#ifdef _WIN32
        auto* view =
            static_cast<char*>(MapViewOfFile(m_mapping, FILE_MAP_WRITE, 0, offset, kWindowBytes));
        if (view) {
            return view;
        }
#else
        void* view =
            mmap(nullptr, kWindowBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(m_file), offset);
        if (view != MAP_FAILED) {
            return static_cast<char*>(view);
        }
#endif
        throw std::runtime_error("cannot map watershed tile scratch window");
    }

    static void unmap(char* view) {
#ifdef _WIN32
        UnmapViewOfFile(view);
#else
        munmap(view, kWindowBytes);
#endif
    }

private:
#ifdef _WIN32
    HANDLE m_file = INVALID_HANDLE_VALUE;
    HANDLE m_mapping = nullptr;
#else
    std::FILE* m_file = nullptr;
#endif
};

}  // namespace

struct TiledSpatialLinks::Storage {
    static constexpr unsigned links_per_page = kPageBytes / sizeof(SpatialLink);
    static constexpr unsigned locations_per_page = kPageBytes / sizeof(unsigned);

    struct Tile {
        unsigned page = 0, next = links_per_page;
    };

    static constexpr unsigned max_ways = 4;

    struct Window {
        unsigned id = ~0u;
        char* view = nullptr;
    };

    struct CacheSet {
        std::array<Window, max_ways> windows;
        mutable std::shared_mutex mutex;
        unsigned next = 0;
    };

    Storage(double size, std::size_t cache_bytes)
        : tile_size(size),
          ways(unsigned(std::min<std::size_t>(max_ways, cache_bytes / kWindowBytes))),
          cache(cache_bytes / (ways * kWindowBytes)) {}

    ~Storage() {
        for (const auto& set : cache) {
            for (const auto& window : set.windows) {
                if (window.view) {
                    ScratchMapping::unmap(window.view);
                }
            }
        }
    }

    unsigned allocate_page() {
        if (page_count % (kPagesPerWindow * kWindowsPerSegment) == 0) {
            segments.push_back(std::make_unique<ScratchMapping>());
        }
        return page_count++;
    }

    CacheSet& cache_set(unsigned page) const {
        return cache[(page / kPagesPerWindow) % cache.size()];
    }

    // Readers pin the set while copying a record. Different sets can remap in parallel.
    char* find_page(unsigned page) const {
        const auto window = page / kPagesPerWindow;
        const auto& set = cache_set(page);
        for (unsigned i = 0; i < ways; ++i) {
            if (set.windows[i].id == window) {
                return set.windows[i].view + (page % kPagesPerWindow) * kPageBytes;
            }
        }
        return nullptr;
    }

    char* load_page(unsigned page) const {
        if (auto* cached = find_page(page)) {
            return cached;
        }
        const auto window = page / kPagesPerWindow;
        auto& set = cache_set(page);
        auto& entry = set.windows[set.next];
        if (entry.view) {
            ScratchMapping::unmap(entry.view);
            entry = {};
        }
        auto* view = segments[window / kWindowsPerSegment]->map(window % kWindowsPerSegment);
        entry = {window, view};
        set.next = (set.next + 1) % ways;
        return view + (page % kPagesPerWindow) * kPageBytes;
    }

    int coordinate(float value) const {
        const double coordinate = std::floor(double(value) / tile_size);
        if (!std::isfinite(coordinate) || coordinate < std::numeric_limits<int>::min()
            || coordinate > std::numeric_limits<int>::max()) {
            throw std::runtime_error("spatial watershed tile coordinate out of bounds");
        }
        return int(coordinate);
    }

    double tile_size;
    std::map<std::array<int, 2>, Tile> tiles;
    std::vector<std::unique_ptr<ScratchMapping>> segments;
    std::vector<std::unique_ptr<unsigned[]>> index_pages;
    unsigned ways;
    mutable std::vector<CacheSet> cache;
    unsigned page_count = 0, count = 0;
};

TiledSpatialLinks::TiledSpatialLinks(double tile_size, std::size_t cache_bytes)
    : m_storage(std::make_unique<Storage>(tile_size, cache_bytes)) {}

TiledSpatialLinks::~TiledSpatialLinks() = default;

void TiledSpatialLinks::append(SpatialLink link, Float3 source) {
    auto& storage = *m_storage;
    // The source tile owns the link even when its other endpoint lies across a seam.
    auto& tile = storage.tiles[{storage.coordinate(source[0]), storage.coordinate(source[1])}];
    if (tile.next == Storage::links_per_page) {
        tile.page = storage.allocate_page();
        tile.next = 0;
    }
    const auto index_slot = storage.count % Storage::locations_per_page;
    if (!index_slot) {
        storage.index_pages.push_back(
            std::make_unique_for_overwrite<unsigned[]>(Storage::locations_per_page)
        );
    }
    if (tile.page > (std::numeric_limits<unsigned>::max() - tile.next) / Storage::links_per_page) {
        throw std::runtime_error("watershed tile scratch index is too large");
    }
    const auto slot = tile.next++;
    // The builder appends on one thread, before readers can access the graph.
    std::memcpy(storage.load_page(tile.page) + slot * sizeof(link), &link, sizeof(link));
    storage.index_pages.back()[index_slot] = tile.page * Storage::links_per_page + slot;
    ++storage.count;
}

SpatialLink TiledSpatialLinks::link(unsigned index) const {
    const auto& storage = *m_storage;
    const auto location =
        storage
            .index_pages[index / Storage::locations_per_page][index % Storage::locations_per_page];
    const auto page = location / Storage::links_per_page;
    const auto slot = location % Storage::links_per_page;
    const auto read_edge = [&](const char* bytes) {
        SpatialLink edge;
        std::memcpy(&edge, bytes + slot * sizeof(edge), sizeof(edge));
        return edge;
    };
    auto& mutex = storage.cache_set(page).mutex;
    {
        std::shared_lock lock(mutex);
        if (const auto* cached = storage.find_page(page)) {
            return read_edge(cached);
        }
    }
    std::unique_lock lock(mutex);
    return read_edge(storage.load_page(page));
}

std::size_t TiledSpatialLinks::size() const {
    return m_storage->count;
}

std::size_t TiledSpatialLinks::cache_capacity() const {
    return m_storage->cache.size() * m_storage->ways * kWindowBytes;
}

std::size_t TiledSpatialLinks::cache_size(std::uint64_t graph_budget) {
    constexpr std::uint64_t maximum_windows = 64 * 1024 * 1024 / kWindowBytes;
    return std::size_t(
               std::clamp<std::uint64_t>(graph_budget / 8 / kWindowBytes, 1, maximum_windows)
           )
           * kWindowBytes;
}
}  // namespace openrac::lighting::bake_detail
