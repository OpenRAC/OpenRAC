// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sector map (sector_map.h): its runs read once, each read served from
// the files they name, a few of them kept open.

#include "assets/disc/sector_map.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <format>
#include <string>

namespace openrac::assets::disc {

namespace {

// Files kept open at once: a level load reads a few lumps in turn, the music its own.
constexpr std::size_t kOpenFiles = 8;

// The next tab-separated field of `line` from `at`, `at` moved past it.
std::string_view field(std::string_view line, std::size_t& at, bool last) {
    if (at > line.size()) {
        return {};
    }

    const std::size_t end = last ? line.size() : line.find('\t', at);
    const std::size_t stop = end == std::string_view::npos ? line.size() : end;
    const std::string_view out = line.substr(at, stop - at);
    at = stop + 1;
    return out;
}

template <typename T>
bool number(std::string_view text, T& out) {
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), out);
    return error == std::errc{} && end == text.data() + text.size();
}

class MapSource final : public ImageSource {
public:
    explicit MapSource(SectorMap map) : m_map(std::move(map)) {}

    u64 size() const override { return m_map.size(); }

    void read(u64 offset, std::span<u8> dst) override { m_map.read(offset, dst); }

private:
    SectorMap m_map;
};

}  // namespace

bool SectorMap::present(const std::filesystem::path& folder) {
    std::error_code error;
    return std::filesystem::is_regular_file(folder / kSectorMapName, error);
}

SectorMap SectorMap::load(const std::filesystem::path& folder) {
    const std::filesystem::path path = folder / kSectorMapName;
    std::ifstream input(path, std::ios::binary);

    // No map: a set-up made before the port read files, or none at all.
    if (!input) {
        throw IoError(std::format("cannot open {}", path.string()));
    }

    SectorMap map;
    std::string line;
    std::size_t number_of_line = 0;
    u64 next = 0;

    while (std::getline(input, line)) {
        ++number_of_line;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // The first line says what the file is; later comments and blank lines are skipped.
        if (number_of_line == 1) {
            if (line != kSectorMapHeader) {
                throw IoError(std::format("{} is not a sector map of version 1", path.string()));
            }
            continue;
        }

        if (line.empty() || line.front() == '#') {
            continue;
        }

        std::size_t at = 0;
        SectorRun run;
        const std::string_view text = line;
        const bool parsed = number(field(text, at, false), run.first)
                            && number(field(text, at, false), run.sectors)
                            && number(field(text, at, false), run.bytes);
        const std::string_view file = field(text, at, true);

        // A run must follow the one before it: the map covers the disc in order.
        if (!parsed || file.empty() || run.sectors == 0 || run.first != next
            || run.bytes > static_cast<u64>(run.sectors) * kSectorSize) {
            throw IoError(
                std::format("{}:{}: not a run of the map", path.string(), number_of_line)
            );
        }

        // Paths are "/"-separated and relative to the folder; none climbs out of it.
        const std::filesystem::path relative = std::filesystem::path(file).lexically_normal();
        if (relative.is_absolute() || relative.empty() || *relative.begin() == "..") {
            throw IoError(
                std::format("{}:{}: a file outside the folder", path.string(), number_of_line)
            );
        }

        run.file = folder / relative;
        next = static_cast<u64>(run.first) + run.sectors;

        if (next > 0xFFFFFFFFull) {
            throw IoError(
                std::format("{}:{}: past the last sector", path.string(), number_of_line)
            );
        }

        map.m_runs.push_back(std::move(run));
    }

    if (map.m_runs.empty()) {
        throw IoError(std::format("{} has no runs", path.string()));
    }

    map.m_sectors = static_cast<u32>(next);
    return map;
}

std::size_t SectorMap::run_at(u32 sector) const {
    // The last run starting at or before the sector.
    const auto it =
        std::upper_bound(m_runs.begin(), m_runs.end(), sector, [](u32 s, const SectorRun& run) {
            return s < run.first;
        });
    return static_cast<std::size_t>(it - m_runs.begin()) - 1;
}

void SectorMap::read_run(std::size_t run, u64 at, std::span<u8> dst) const {
    const SectorRun& r = m_runs[run];
    // The bytes of the file, then zeros: a file of the file system ends before its last sector.
    const u64 from_file = at < r.bytes ? std::min<u64>(dst.size(), r.bytes - at) : 0;

    if (from_file > 0) {
        std::lock_guard lock(*m_lock);
        auto found =
            std::find_if(m_open.begin(), m_open.end(), [&](const Open& o) { return o.run == run; });

        if (found == m_open.end()) {
            if (m_open.size() == kOpenFiles) {
                m_open.erase(m_open.begin());
            }

            auto file = std::make_unique<std::ifstream>(r.file, std::ios::binary);

            if (!*file) {
                throw IoError(std::format("cannot open {}", r.file.string()));
            }

            m_open.push_back({run, std::move(file)});
            found = m_open.end() - 1;
        }

        std::ifstream& file = *found->file;
        file.clear();
        file.seekg(static_cast<std::streamoff>(at));
        file.read(reinterpret_cast<char*>(dst.data()), static_cast<std::streamsize>(from_file));

        if (!file || static_cast<u64>(file.gcount()) != from_file) {
            throw IoError(
                std::format(
                    "cannot read {:#x} bytes at {:#x} of {}", from_file, at, r.file.string()
                )
            );
        }
    }

    std::memset(dst.data() + from_file, 0, dst.size() - from_file);
}

std::size_t SectorMap::read_some(u64 offset, std::span<u8> dst) const {
    const u64 end = size();

    if (offset >= end) {
        return 0;
    }

    const std::size_t total = static_cast<std::size_t>(std::min<u64>(dst.size(), end - offset));
    std::size_t done = 0;

    // Run by run: a read may cross from one file into the next.
    while (done < total) {
        const u64 at = offset + done;
        const std::size_t run = run_at(static_cast<u32>(at / kSectorSize));
        const SectorRun& r = m_runs[run];
        const u64 start = static_cast<u64>(r.first) * kSectorSize;
        const u64 length = static_cast<u64>(r.sectors) * kSectorSize;
        const std::size_t part =
            static_cast<std::size_t>(std::min<u64>(total - done, start + length - at));
        read_run(run, at - start, dst.subspan(done, part));
        done += part;
    }

    return total;
}

void SectorMap::read(u64 offset, std::span<u8> dst) const {
    if (offset > size() || dst.size() > size() - offset) {
        throw IoError(
            std::format("read of {:#x} bytes at {:#x} past the end of the disc", dst.size(), offset)
        );
    }

    read_some(offset, dst);
}

std::unique_ptr<ImageSource> open_sector_map(const std::filesystem::path& folder) {
    return std::make_unique<MapSource>(SectorMap::load(folder));
}

}  // namespace openrac::assets::disc
