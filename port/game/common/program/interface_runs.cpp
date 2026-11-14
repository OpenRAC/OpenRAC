// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "program/interface_runs.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <system_error>
#include <unordered_map>

#include "common/json.h"
#include "common/log.h"
#include "common/wad.h"
#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "program/options.h"

#ifdef OPENRAC_FRONTEND
#include <stb_image_write.h>

#include "renderer/textures/interface_run.h"
#include "renderer/textures/texture_pack.h"
#endif

namespace fs = std::filesystem;

namespace {

constexpr std::uint64_t kSector = 2048;
// How much of the disc is read at once while looking for blobs.
constexpr std::uint64_t kPart = std::uint64_t{32} << 20;

// The sectors the game read during the run, as cdvd.c reported them.
struct Reads {
    std::mutex lock;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;  // first sector, count
};

Reads& reads() {
    static Reads r;
    return r;
}

std::atomic<bool> g_noting{false};

// `count` sectors from `sector` were read off the disc.
void note_disc_read(unsigned int sector, unsigned int count) {
    if (!g_noting.load(std::memory_order_relaxed) || count == 0) {
        return;
    }
    Reads& r = reads();
    const std::lock_guard guard(r.lock);
    // A streamed file reads on where it stopped: one range grows rather than thousands.
    if (!r.ranges.empty() && r.ranges.back().first + r.ranges.back().second == sector) {
        r.ranges.back().second += count;
        return;
    }
    r.ranges.emplace_back(sector, count);
}

}  // namespace

namespace openrac::program {

#ifdef OPENRAC_FRONTEND

namespace {

namespace run = renderer::interface_run;

fs::path g_run_file;

// A JSON string literal of `text` (UTF-8 kept as it is).
std::string json_quote(std::string_view text) {
    std::string out = "\"";
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += std::format("\\u{:04x}", static_cast<unsigned>(c));
                } else {
                    out += c;
                }
        }
    }
    out += '"';
    return out;
}

std::string hex(std::uint64_t value) {
    return std::format("{:016x}", value);
}

std::optional<std::uint64_t> parse_hex(const std::string& text) {
    if (text.empty() || text.size() > 16) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (const char c : text) {
        value <<= 4;
        if (c >= '0' && c <= '9') {
            value |= static_cast<std::uint64_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value |= static_cast<std::uint64_t>(c - 'a' + 10);
        } else {
            return std::nullopt;
        }
    }
    return value;
}

std::string_view folder_of(run::Category category) {
    switch (category) {
        case run::Category::Font:
            return "Font";
        case run::Category::Interface:
            return "HUD and menus";
        case run::Category::Effects:
            return "Effects";
    }
    return "HUD and menus";
}

// Where a blob's bytes are on the disc: at a byte of it, or at a byte inside the WAD stream that
// starts at `wad`.
struct Place {
    std::uint64_t size = 0;
    std::uint64_t at = 0;
    std::optional<std::uint64_t> wad;
};

// Reads `size` bytes at byte `at` of the disc; empty when the disc is shorter.
std::vector<std::uint8_t> read_at(std::ifstream& disc, std::uint64_t at, std::uint64_t size) {
    std::vector<std::uint8_t> out(size);
    disc.clear();
    disc.seekg(static_cast<std::streamoff>(at));
    disc.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
    if (static_cast<std::uint64_t>(disc.gcount()) != size) {
        return {};
    }
    return out;
}

// The decompressed WAD stream that starts at byte `at` of the disc, or nothing.
std::optional<std::vector<std::uint8_t>> read_wad(std::ifstream& disc, std::uint64_t at) {
    const std::vector<std::uint8_t> header = read_at(disc, at, wad::kHeaderBytes);
    const auto size = wad::stream_size(header);
    if (!size || *size > wad::kDefaultLimit) {
        return std::nullopt;
    }
    const std::vector<std::uint8_t> stream = read_at(disc, at, *size);
    if (stream.empty()) {
        return std::nullopt;
    }
    return wad::decompress(stream);
}

// ---- Finding the blobs on the disc ----

// One blob to find: its hash and key.
struct Needle {
    std::uint64_t hash;
    run::BlobKey key;
};

class Finder {
public:
    explicit Finder(const std::map<std::uint64_t, run::BlobKey>& blobs) {
        for (const auto& [hash, key] : blobs) {
            if (key.size < kMinBlob) {
                continue;
            }
            m_needles[key.anchor].push_back({hash, key});
            m_longest = std::max<std::uint64_t>(m_longest, key.size);
            m_filter[filter_bit(key.anchor) / 64] |= std::uint64_t{1}
                                                     << (filter_bit(key.anchor) % 64);
            ++m_left;
        }
    }

    // Looks for every blob not found yet in `haystack`; a find is placed by `place` from its
    // offset in `haystack`.
    template <typename PlaceAt>
    void search(std::span<const std::uint8_t> haystack, PlaceAt place) {
        if (m_left == 0 || haystack.size() < 8) {
            return;
        }
        const std::uint8_t* data = haystack.data();
        const std::size_t end = haystack.size() - 7;
        for (std::size_t i = 0; i < end && m_left > 0; ++i) {
            std::uint64_t window = 0;
            std::memcpy(&window, data + i, sizeof window);
            const std::uint32_t bit = filter_bit(window);
            if ((m_filter[bit / 64] & (std::uint64_t{1} << (bit % 64))) == 0) {
                continue;
            }
            const auto found = m_needles.find(window);
            if (found == m_needles.end()) {
                continue;
            }
            for (const Needle& needle : found->second) {
                if (m_found.contains(needle.hash) || i < needle.key.anchor_at) {
                    continue;
                }
                const std::size_t start = i - needle.key.anchor_at;
                const std::size_t size = needle.key.size;
                if (start + size > haystack.size() || !probes_match(data + start, needle.key)
                    || run::blob_hash(haystack.subspan(start, size)) != needle.hash) {
                    continue;
                }
                m_found.emplace(needle.hash, place(start, needle.key.size));
                --m_left;
            }
        }
    }

    bool done() const { return m_left == 0; }

    // The longest blob looked for: how far a piece read in parts must reach into the next one.
    std::uint64_t longest() const { return m_longest; }

    const std::map<std::uint64_t, Place>& found() const { return m_found; }

private:
    // Blobs shorter than this are not looked for: their bytes are found everywhere.
    static constexpr std::size_t kMinBlob = 32;
    static constexpr std::uint32_t kFilterBits = 1u << 20;

    static std::uint64_t window_at(const std::uint8_t* at) {
        std::uint64_t window = 0;
        std::memcpy(&window, at, sizeof window);
        return window;
    }

    static bool probes_match(const std::uint8_t* blob, const run::BlobKey& key) {
        for (std::size_t n = 0; n < run::BlobKey::kProbes; ++n) {
            if (window_at(blob + run::probe_at(key.size, n)) != key.probes[n]) {
                return false;
            }
        }
        return true;
    }

    static std::uint32_t filter_bit(std::uint64_t key) {
        return static_cast<std::uint32_t>((key * 0x9E3779B97F4A7C15ull) >> 44);
    }

    std::unordered_map<std::uint64_t, std::vector<Needle>> m_needles;
    std::vector<std::uint64_t> m_filter = std::vector<std::uint64_t>(kFilterBits / 64);
    std::map<std::uint64_t, Place> m_found;
    std::size_t m_left = 0;
    std::uint64_t m_longest = 0;
};

// Where each blob is on the disc, from the sectors the game read: as they are, then inside each
// WAD stream that starts in them.
std::map<std::uint64_t, Place> locate(
    const fs::path& disc_path,
    const std::map<std::uint64_t, run::BlobKey>& blobs,
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges
) {
    Finder finder(blobs);
    std::ifstream disc(disc_path, std::ios::binary);
    if (!disc) {
        log::warn(
            "interface run: {} cannot be read: no texture is placed on the disc", disc_path.string()
        );
        return {};
    }
    // The sectors read, joined where they touch or overlap.
    std::sort(ranges.begin(), ranges.end());
    std::vector<std::pair<std::uint64_t, std::uint64_t>> joined;
    for (const auto& [first, count] : ranges) {
        if (!joined.empty() && first <= joined.back().first + joined.back().second) {
            auto& last = joined.back();
            last.second = std::max(last.second, first + count - last.first);
        } else {
            joined.emplace_back(first, count);
        }
    }
    // Read in parts (a long route reads hundreds of megabytes), each reaching past its end by the
    // longest blob so that one lying across the cut is still whole in one part.
    disc.seekg(0, std::ios::end);
    const std::uint64_t disc_size =
        static_cast<std::uint64_t>(std::max<std::streamoff>(0, disc.tellg()));
    const std::uint64_t reach = std::max<std::uint64_t>(finder.longest(), wad::kHeaderBytes);
    std::set<std::uint64_t> streams;
    for (const auto& [first, count] : joined) {
        const std::uint64_t begin = std::min(first * kSector, disc_size);
        const std::uint64_t end = std::min((first + count) * kSector, disc_size);
        for (std::uint64_t base = begin; base < end; base += kPart) {
            const std::uint64_t own = std::min(kPart, end - base);
            const std::vector<std::uint8_t> bytes =
                read_at(disc, base, std::min(own + reach, disc_size - base));
            if (bytes.empty()) {
                continue;
            }
            finder.search(bytes, [&](std::uint64_t at, std::uint64_t size) {
                return Place{size, base + at, std::nullopt};
            });
            // The WAD streams that start here (the magic and a size that is plausible).
            for (std::size_t i = 0; i < own && i + wad::kHeaderBytes <= bytes.size(); ++i) {
                if (bytes[i] != 'W' || bytes[i + 1] != 'A' || bytes[i + 2] != 'D') {
                    continue;
                }
                const auto size = wad::stream_size(std::span(bytes).subspan(i));
                if (size && *size <= wad::kDefaultLimit) {
                    streams.insert(base + i);
                }
            }
        }
    }
    for (const std::uint64_t at : streams) {
        if (finder.done()) {
            break;
        }
        const auto decompressed = read_wad(disc, at);
        if (!decompressed) {
            continue;
        }
        finder.search(*decompressed, [&](std::uint64_t inside, std::uint64_t size) {
            return Place{size, inside, at};
        });
    }
    return finder.found();
}

void write_run(const fs::path& file, const fs::path& disc) {
    run::Noted noted = run::take();
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
    {
        Reads& r = reads();
        const std::lock_guard guard(r.lock);
        ranges = r.ranges;
    }
    const auto located_from = std::chrono::steady_clock::now();
    const std::map<std::uint64_t, Place> places = locate(disc, noted.blobs, std::move(ranges));
    log::info(
        "interface run: {} blobs looked for on the disc in {} ms",
        noted.blobs.size(),
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - located_from
        )
            .count()
    );
    // The textures whose bytes were not all found on the disc, by name: run.py drops those another
    // part has.
    std::map<std::string, std::string> missing{{"font", ""}, {"interface", ""}, {"effects", ""}};
    std::size_t missed = 0;
    std::set<std::uint64_t> used;
    std::string textures;
    for (const auto& [name, r] : noted.recipes) {
        std::vector<std::uint64_t> needs;
        if (r.kind == run::Kind::Pool) {
            for (const auto& u : r.uploads) {
                needs.push_back(u.blob);
            }
        } else {
            needs = {r.indices, r.palette};
        }
        if (!std::all_of(needs.begin(), needs.end(), [&](std::uint64_t b) {
                return places.contains(b);
            })) {
            std::string& list = missing[std::string(run::category_id(r.category))];
            list += std::format("{}\"{}\"", list.empty() ? "" : ", ", hex(name));
            ++missed;
            continue;
        }
        used.insert(needs.begin(), needs.end());
        if (!textures.empty()) {
            textures += ",\n";
        }
        textures += std::format(
            R"(  {{"name": "{}", "category": "{}", )", hex(name), run::category_id(r.category)
        );
        if (r.kind == run::Kind::Pool) {
            textures += std::format(
                R"("kind": "pool", "tex0": "{}", "texa": "{}", )", hex(r.tex0), hex(r.texa)
            );
            if (r.tbp != 0) {
                textures += std::format(R"("tbp": {}, )", r.tbp);
            }
            textures += R"("uploads": [)";
            for (std::size_t i = 0; i < r.uploads.size(); ++i) {
                const auto& u = r.uploads[i];
                textures += std::format(
                    R"({}{{"dbp": {}, "dbw": {}, "dpsm": {}, "x": {}, "y": {}, "w": {}, "h": {}, "blob": "{}"}})",
                    i == 0 ? "" : ", ",
                    u.dbp,
                    u.dbw,
                    u.dpsm,
                    u.x,
                    u.y,
                    u.width,
                    u.height,
                    hex(u.blob)
                );
            }
            textures += "]}";
        } else {
            textures += std::format(
                R"("kind": "indexed8", "texa": "{}", "w": {}, "h": {}, "indices": "{}", "palette": "{}"}})",
                hex(r.texa),
                r.width,
                r.height,
                hex(r.indices),
                hex(r.palette)
            );
        }
    }
    std::string blobs;
    for (const std::uint64_t hash : used) {
        const Place& p = places.at(hash);
        if (!blobs.empty()) {
            blobs += ",\n";
        }
        blobs += p.wad
                     ? std::format(
                           R"(  "{}": {{"size": {}, "wad": {}, "at": {}}})",
                           hex(hash),
                           p.size,
                           *p.wad,
                           p.at
                       )
                     : std::format(R"(  "{}": {{"size": {}, "at": {}}})", hex(hash), p.size, p.at);
    }
    const std::string text = std::format(
        "{{\"format\": {}, \"game\": \"{}\", \"converter\": {},\n\"missing\": {{\"font\": [{}], "
        "\"interface\": [{}], "
        "\"effects\": [{}]}},\n\"blobs\": {{\n{}\n}},\n\"textures\": [\n{}\n]}}\n",
        kInterfaceRunFormat,
        openrac_game.id,
        run::kConverter,
        missing["font"],
        missing["interface"],
        missing["effects"],
        blobs,
        textures
    );
    std::error_code e;
    if (file.has_parent_path()) {
        fs::create_directories(file.parent_path(), e);
    }
    const fs::path partial = file.string() + ".partial";
    {
        std::ofstream out(partial, std::ios::binary);
        out << text;
        if (!out) {
            log::warn("interface run: {} could not be written", file.string());
            return;
        }
    }
    fs::rename(partial, file, e);
    if (e) {
        log::warn("interface run: {} could not be written: {}", file.string(), e.message());
        return;
    }
    log::info(
        "interface run: {} textures written to {} ({} not found on the disc)",
        noted.recipes.size() - missed,
        file.string(),
        missed
    );
}

// ---- Exporting ----

struct Loaded {
    std::map<std::uint64_t, Place> blobs;
    std::map<std::uint64_t, run::Recipe> recipes;
};

// Reads one run file into `into`; false (with `why`) when it is not one this program can use.
bool load_run(const fs::path& file, const std::string& game, Loaded& into, std::string& why) {
    json::Value doc;
    if (!json::parse_file(file.string(), doc, why)) {
        return false;
    }
    if (doc["format"].number(-1) != kInterfaceRunFormat) {
        why = std::format("{}: a run file of another format", file.string());
        return false;
    }
    if (doc["game"].string() != game) {
        why = std::format("{}: a run of {}, not {}", file.string(), doc["game"].string(), game);
        return false;
    }
    if (doc["converter"].number(-1) != run::kConverter) {
        why = std::format(
            "{}: made by another version of the port (converter {}, this one {})",
            file.string(),
            doc["converter"].number(-1),
            run::kConverter
        );
        return false;
    }
    for (const auto& [key, value] : doc["blobs"].members()) {
        const auto hash = parse_hex(key);
        if (!hash || !value["size"].is_number() || !value["at"].is_number()) {
            continue;
        }
        Place p{
            static_cast<std::uint64_t>(value["size"].number()),
            static_cast<std::uint64_t>(value["at"].number()),
            std::nullopt
        };
        if (value["wad"].is_number()) {
            p.wad = static_cast<std::uint64_t>(value["wad"].number());
        }
        into.blobs.emplace(*hash, p);
    }
    for (const json::Value& t : doc["textures"].items()) {
        const auto name = parse_hex(t["name"].string());
        const auto category = run::parse_category(t["category"].string());
        if (!name || !category || into.recipes.contains(*name)) {
            continue;
        }
        run::Recipe r;
        r.name = *name;
        r.category = *category;
        r.texa = parse_hex(t["texa"].string()).value_or(0);
        if (t["kind"].string() == "indexed8") {
            r.kind = run::Kind::Indexed8;
            r.width = static_cast<int>(t["w"].number());
            r.height = static_cast<int>(t["h"].number());
            const auto indices = parse_hex(t["indices"].string());
            const auto palette = parse_hex(t["palette"].string());
            if (!indices || !palette) {
                continue;
            }
            r.indices = *indices;
            r.palette = *palette;
        } else if (t["kind"].string() == "pool") {
            r.kind = run::Kind::Pool;
            const auto tex0 = parse_hex(t["tex0"].string());
            if (!tex0) {
                continue;
            }
            r.tex0 = *tex0;
            if (t["tbp"].is_number()) {
                const double tbp = t["tbp"].number();
                if (tbp < 0x4000 || tbp >= 0x20000) {
                    continue;
                }
                r.tbp = static_cast<std::uint32_t>(tbp);
            }
            bool good = true;
            for (const json::Value& u : t["uploads"].items()) {
                const auto blob = parse_hex(u["blob"].string());
                if (!blob) {
                    good = false;
                    break;
                }
                r.uploads.push_back(
                    run::Upload{
                        static_cast<std::uint32_t>(u["dbp"].number()),
                        static_cast<std::uint32_t>(u["dbw"].number()),
                        static_cast<std::uint8_t>(u["dpsm"].number()),
                        static_cast<std::uint32_t>(u["x"].number()),
                        static_cast<std::uint32_t>(u["y"].number()),
                        static_cast<std::uint32_t>(u["w"].number()),
                        static_cast<std::uint32_t>(u["h"].number()),
                        *blob
                    }
                );
            }
            if (!good || r.uploads.empty()) {
                continue;
            }
        } else {
            continue;
        }
        into.recipes.emplace(*name, std::move(r));
    }
    return true;
}

}  // namespace

void start_interface_run(const fs::path& file) {
    g_run_file = file;
    run::start();
    g_noting = true;
    openrac_cdvd_read_hook = note_disc_read;
    log::info("interface run: noting the interface textures into {}", file.string());
}

void finish_interface_run(const fs::path& disc) {
    if (g_run_file.empty()) {
        return;
    }
    g_noting = false;
    openrac_cdvd_read_hook = nullptr;
    // Called at the exit: nothing may leave it (an exception would unwind into the game's tick).
    try {
        write_run(g_run_file, disc);
    } catch (const std::exception& e) {
        log::error("interface run: {} not written: {}", g_run_file.string(), e.what());
    }
    g_run_file.clear();
}

int export_interface(
    const fs::path& disc_path,
    const std::string& game,
    const fs::path& asked,
    const std::vector<fs::path>& runs,
    const std::string& only,
    const Say& say
) {
    // The categories asked for (all with none named).
    std::set<run::Category> wanted;
    for (std::size_t start = 0; start < only.size();) {
        const std::size_t comma = std::min(only.find(',', start), only.size());
        const std::string id = only.substr(start, comma - start);
        if (const auto category = run::parse_category(id)) {
            wanted.insert(*category);
        } else if (!id.empty()) {
            say(std::format(
                R"({{"event": "error", "message": {}}})", json_quote("unknown category " + id)
            ));
            return 1;
        }
        start = comma + 1;
    }
    if (wanted.empty()) {
        wanted.insert(std::begin(run::kCategories), std::end(run::kCategories));
    }
    Loaded loaded;
    int usable = 0;
    for (const fs::path& file : runs) {
        std::string why;
        if (load_run(file, game, loaded, why)) {
            ++usable;
        } else {
            say(std::format(R"({{"event": "skipped-run", "message": {}}})", json_quote(why)));
        }
    }
    if (usable == 0) {
        say(R"({"event": "error", "message": "no run file this version of the port can use"})");
        return 1;
    }
    std::ifstream disc(disc_path, std::ios::binary);
    if (!disc) {
        say(std::format(
            R"({{"event": "error", "message": {}}})",
            json_quote(disc_path.string() + " cannot be read: set the game up from your disc first")
        ));
        return 1;
    }
    std::vector<const run::Recipe*> todo;
    for (const auto& [name, r] : loaded.recipes) {
        if (wanted.contains(r.category)) {
            todo.push_back(&r);
        }
    }
    // A new folder each time: an earlier export (maybe edited since) is never written over.
    fs::path out = asked;
    std::error_code e;
    for (int n = 2; fs::exists(out, e); ++n) {
        out = asked.parent_path() / std::format("{}-{}", asked.filename().string(), n);
    }
    say(std::format(R"({{"event": "start", "total": {}}})", todo.size()));

    // The blobs, read from the disc stream by stream, each checked against its hash.
    std::map<std::uint64_t, std::vector<std::uint8_t>> blobs;
    std::set<std::uint64_t> bad;
    std::map<std::uint64_t, std::vector<std::uint64_t>> by_stream;
    std::set<std::uint64_t> needed;
    for (const run::Recipe* r : todo) {
        if (r->kind == run::Kind::Pool) {
            for (const auto& u : r->uploads) {
                needed.insert(u.blob);
            }
        } else {
            needed.insert(r->indices);
            needed.insert(r->palette);
        }
    }
    for (const std::uint64_t hash : needed) {
        const auto found = loaded.blobs.find(hash);
        if (found == loaded.blobs.end()) {
            bad.insert(hash);
            continue;
        }
        const Place& p = found->second;
        if (p.wad) {
            by_stream[*p.wad].push_back(hash);
            continue;
        }
        std::vector<std::uint8_t> bytes = read_at(disc, p.at, p.size);
        if (bytes.size() != p.size || run::blob_hash(bytes) != hash) {
            bad.insert(hash);
            continue;
        }
        blobs.emplace(hash, std::move(bytes));
    }
    for (const auto& [at, hashes] : by_stream) {
        const auto stream = read_wad(disc, at);
        for (const std::uint64_t hash : hashes) {
            const Place& p = loaded.blobs.at(hash);
            if (!stream || p.at + p.size > stream->size()) {
                bad.insert(hash);
                continue;
            }
            std::vector<std::uint8_t> bytes(
                stream->begin() + static_cast<std::ptrdiff_t>(p.at),
                stream->begin() + static_cast<std::ptrdiff_t>(p.at + p.size)
            );
            if (run::blob_hash(bytes) != hash) {
                bad.insert(hash);
                continue;
            }
            blobs.emplace(hash, std::move(bytes));
        }
    }
    const run::BlobSource source = [&](std::uint64_t hash) -> const std::vector<std::uint8_t>* {
        const auto found = blobs.find(hash);
        return found == blobs.end() ? nullptr : &found->second;
    };

    std::map<std::string, int> written;
    int from_other_disc = 0;
    int mismatched = 0;
    int unwritable = 0;
    std::size_t done = 0;
    for (const run::Recipe* r : todo) {
        ++done;
        if (done % 50 == 0) {
            say(std::format(
                R"({{"event": "progress", "done": {}, "total": {}}})", done, todo.size()
            ));
        }
        const auto image = run::rebuild(*r, source);
        if (!image) {
            // A blob that is not on this disc as the run found it, or (all there) a conversion
            // that makes no picture of them.
            const bool all_read =
                r->kind == run::Kind::Pool
                    ? std::all_of(
                          r->uploads.begin(),
                          r->uploads.end(),
                          [&](const run::Upload& u) { return blobs.contains(u.blob); }
                      )
                    : blobs.contains(r->indices) && blobs.contains(r->palette);
            ++(all_read ? mismatched : from_other_disc);
            log::verbose(
                "interface export: {} made no picture ({})",
                hex(r->name),
                all_read ? "its blobs were read" : "a blob is not on the disc"
            );
            continue;
        }
        if (renderer::texture_pack::image_hash(*image) != r->name) {
            ++mismatched;
            log::verbose("interface export: {} came out as another picture", hex(r->name));
            continue;
        }
        const fs::path folder = out / folder_of(r->category);
        fs::create_directories(folder, e);
        const fs::path file = folder / renderer::texture_pack::file_name(r->name);
        if (stbi_write_png(
                file.string().c_str(),
                image->width,
                image->height,
                4,
                image->pixels.data(),
                image->width * 4
            )
            == 0) {
            ++unwritable;
            if (unwritable == 1) {
                say(std::format(
                    R"({{"event": "error", "message": {}}})",
                    json_quote(file.string() + " could not be written (is the disk full?)")
                ));
            }
            continue;
        }
        ++written[std::string(run::category_id(r->category))];
    }
    say(std::format(
        R"({{"event": "done", "folder": {}, "written": {{"font": {}, "interface": {}, "effects": {}}}, "skipped": {{"disc": {}, "different": {}, "unwritable": {}}}}})",
        json_quote(out.string()),
        written["font"],
        written["interface"],
        written["effects"],
        from_other_disc,
        mismatched,
        unwritable
    ));
    const int total = written["font"] + written["interface"] + written["effects"];
    if (total == 0) {
        say(std::format(
            R"({{"event": "error", "message": {}}})",
            json_quote(
                todo.empty() ? "the runs hold no texture of the kinds asked for"
                             : "no texture came out as the run says: is the disc set up "
                               "the same version as the run's?"
            )
        ));
        return 1;
    }
    // The folder, for the launcher (launcher/core/src/jobs.rs).
    say(std::format("openrac-output: {}", out.string()));
    return 0;
}

#else  // no window: no renderer to note or rebuild with

void start_interface_run(const fs::path&) {
    log::warn("interface run: this program has no window, so it has no textures to note");
}

void finish_interface_run(const fs::path&) {}

int export_interface(
    const fs::path&,
    const std::string&,
    const fs::path&,
    const std::vector<fs::path>&,
    const std::string&,
    const Say& say
) {
    say(R"({"event": "error", "message": "this program has no window and cannot export"})");
    return 1;
}

#endif

}  // namespace openrac::program
