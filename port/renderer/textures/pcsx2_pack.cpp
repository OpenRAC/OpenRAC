// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/textures/pcsx2_pack.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cctype>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <format>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#define XXH_INLINE_ALL
#include <xxhash.h>

#include "common/joining_thread.h"
#include "common/log.h"

namespace openrac::renderer::pcsx2_pack {
namespace {

namespace fs = std::filesystem;
std::unordered_map<std::string, fs::path> files;
LoadPng read_png = nullptr;

struct Crop {
    int left, top, right, bottom;
    std::uint64_t hash;
    fs::path file;
};

std::unordered_map<std::string, std::vector<Crop>> crops;
std::unordered_set<std::uint32_t> crop_formats;

// Pack names and diagnostics use UTF-8 without passing through the Windows ANSI code page.
std::string path_text(const fs::path& path) {
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

std::string lowercase(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

std::string key(std::uint64_t texture, std::uint64_t palette, std::uint32_t bits) {
    return std::format("{:x}-{:x}-{:08x}", texture, palette, bits);
}

template <class Integer>
bool hex(std::string_view part, Integer& value) {
    const auto end = part.data() + part.size();
    const auto result = std::from_chars(part.data(), end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

std::uint32_t texture_bits(const gs::Tex0& tex0) {
    return tex0.psm | (tex0.tw << 6) | (tex0.th << 10);
}

std::optional<std::string> parse_key(std::string_view name, const fs::path& path) {
    const auto first = name.find('-');
    const auto last = name.rfind('-');
    if (first == std::string_view::npos || first == last || name.size() - last != 9) {
        return {};
    }
    std::uint64_t texture = 0, palette = 0;
    std::uint32_t bits = 0;
    const auto region = name.find("-r", first + 1);
    const auto palette_end = region == std::string_view::npos ? last : region;
    if (!hex(name.substr(0, first), texture)
        || !hex(name.substr(first + 1, palette_end - first - 1), palette)
        || !hex(name.substr(last + 1), bits)) {
        return {};
    }
    // Older packs recorded TCC in bit 14. Modern PCSX2 ignores it.
    bits &= ~0x4000u;
    if (region == std::string_view::npos) {
        return key(texture, palette, bits);
    }
    // Legacy names store min/max (exclusive), X in the low word and Y in the high word.
    std::uint64_t area = 0;
    if (region == last || !hex(name.substr(region + 2, last - region - 2), area) || !area) {
        return {};
    }
    const auto side = [area](int shift) {
        return static_cast<int>((area >> shift) & 0xffff);
    };
    Crop crop{side(0), side(32), side(16), side(48), texture, path};
    const auto tw = (bits >> 6) & 15u;
    const auto th = (bits >> 10) & 15u;
    if (tw > 10 || th > 10 || (bits & 63u) != gs::kPsmt8) {
        return {};
    }
    if (crop.left == 0 && crop.right == 0) {
        crop.right = 1 << tw;
    }
    if (crop.top == 0 && crop.bottom == 0) {
        crop.bottom = 1 << th;
    }
    if (crop.right > crop.left && crop.bottom > crop.top && crop.right <= kMaxTextureSide
        && crop.bottom <= kMaxTextureSide) {
        crops[key(0, palette, bits)].push_back(std::move(crop));
        crop_formats.insert(bits);
    }
    return {};
}

std::uint64_t texture_hash(std::span<const std::uint8_t> raster, const gs::Tex0& tex0) {
    const int width = tex0.width();
    const int height = tex0.height();
    // PSMT8 hashes 16x16 GS blocks in raster block order. Sub-block textures and
    // high-byte textures hash their expanded indices without unrelated VRAM bytes.
    if (tex0.psm != gs::kPsmt8 || width < 16 || height < 16) {
        return XXH3_64bits(raster.data(), raster.size());
    }
    static const auto order = [] {
        std::array<std::size_t, 256> result{};
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                result[swizzle::nibble_address(gs::kPsmt8, x, y, 0, 2) / 2] =
                    static_cast<std::size_t>(y * 16 + x);
            }
        }
        return result;
    }();
    XXH3_state_t state;
    XXH3_64bits_reset(&state);
    std::array<std::uint8_t, 256> block{};
    for (int y = 0; y < height; y += 16) {
        for (int x = 0; x < width; x += 16) {
            for (std::size_t i = 0; i < block.size(); ++i) {
                const auto offset = order[i];
                block[i] = raster
                    [static_cast<std::size_t>(y + static_cast<int>(offset / 16))
                         * static_cast<std::size_t>(width)
                     + static_cast<std::size_t>(x) + offset % 16];
            }
            XXH3_64bits_update(&state, block.data(), block.size());
        }
    }
    return XXH3_64bits_digest(&state);
}

std::optional<Rgba8Image> load(const fs::path& path) {
    if (lowercase(path_text(path.extension())) == ".dds") {
        return load_dds(path);
    }
    return read_png ? read_png(path) : std::nullopt;
}

// Pictures already decoded, so that one is not decoded twice. The limit is a few hundred MB;
// precaching raises it (set_loading).
#if defined(__ANDROID__)
constexpr std::size_t kCacheBytes = std::size_t{256} << 20;
constexpr std::size_t kPrecacheBytes = std::size_t{512} << 20;
#else
constexpr std::size_t kCacheBytes = std::size_t{512} << 20;
constexpr std::size_t kPrecacheBytes = std::size_t{2048} << 20;
#endif

struct CachedPicture {
    std::shared_ptr<const Rgba8Image> image;
    std::size_t bytes = 0;
    std::uint64_t used = 0;
};

std::mutex cache_lock;
std::unordered_map<std::string, CachedPicture> cached_pictures;
std::size_t cached_bytes = 0;
std::uint64_t cached_uses = 0;
std::size_t cache_limit = kCacheBytes;
// Bumped by configure(): a picture read for an older pack is not kept.
std::uint64_t pack_generation = 0;
std::atomic<bool> async_loading{true};
std::atomic<bool> precaching{false};

std::string cache_key(const fs::path& path, AlphaScale alpha) {
    return path_text(path) + (alpha == AlphaScale::Full ? "\x1f" : "\x1e");
}

// Read from the cache or load and cache the picture.
std::shared_ptr<const Rgba8Image> read_cached(const fs::path& path, AlphaScale alpha) {
    const std::string key = cache_key(path, alpha);
    std::uint64_t generation = 0;
    {
        const std::lock_guard guard(cache_lock);
        if (const auto found = cached_pictures.find(key); found != cached_pictures.end()) {
            found->second.used = ++cached_uses;
            return found->second.image;
        }
        generation = pack_generation;
    }
    auto picture = load(path);
    if (!picture || picture->pixels.empty()) {
        return nullptr;
    }
    if (alpha == AlphaScale::Full) {
        // PCSX2's PSMT8 textures have a 4-bit alpha channel. The pack's replacement pictures are RGBA8, so we double the alpha channel to fill the 8 bits.
        for (std::size_t a = 3; a < picture->pixels.size(); a += 4) {
            picture->pixels[a] = static_cast<std::uint8_t>(std::min(255, picture->pixels[a] * 2));
        }
    }
    auto shared = std::make_shared<const Rgba8Image>(std::move(*picture));
    const std::size_t bytes = shared->pixels.size();
    const std::lock_guard guard(cache_lock);
    if (bytes > cache_limit || generation != pack_generation) {
        return shared;
    }
    while (!cached_pictures.empty() && cached_bytes + bytes > cache_limit) {
        const auto oldest = std::min_element(
            cached_pictures.begin(), cached_pictures.end(), [](const auto& a, const auto& b) {
                return a.second.used < b.second.used;
            }
        );
        cached_bytes -= oldest->second.bytes;
        cached_pictures.erase(oldest);
    }
    const auto [found, added] =
        cached_pictures.try_emplace(key, CachedPicture{shared, bytes, ++cached_uses});
    if (added) {
        cached_bytes += bytes;
    }
    return found->second.image;
}

std::shared_ptr<const Rgba8Image> cached_only(const std::string& key) {
    const std::lock_guard guard(cache_lock);
    if (const auto found = cached_pictures.find(key); found != cached_pictures.end()) {
        found->second.used = ++cached_uses;
        return found->second.image;
    }
    return nullptr;
}

// Reads pictures on its own thread: the ones the game asked for first, then (when precaching) the
// whole pack, smallest files first, until the cache is full.
class Loader {
public:
    ~Loader() { stop(); }

    // The picture is not read yet: it is queued, and `pending` says the caller should ask again
    // once ready_version() changes.
    std::shared_ptr<const Rgba8Image> get(const fs::path& path, AlphaScale alpha, bool& pending) {
        const std::string key = cache_key(path, alpha);
        if (auto found = cached_only(key)) {
            return found;
        }
        {
            const std::lock_guard guard(m_lock);
            if (m_failed.contains(key)) {
                return nullptr;
            }
            if (m_wanted.insert(key).second) {
                m_urgent.push_back({path, alpha, key});
            }
            start();
        }
        m_wake.notify_one();
        pending = true;
        return nullptr;
    }

    void precache(std::vector<fs::path> list) {
        {
            const std::lock_guard guard(m_lock);
            m_ahead = std::move(list);
            m_next = 0;
            start();
        }
        m_wake.notify_one();
    }

    // Forgets what is queued and failed: a new pack.
    void reset() {
        const std::lock_guard guard(m_lock);
        m_urgent.clear();
        m_wanted.clear();
        m_failed.clear();
        m_ahead.clear();
        m_next = 0;
    }

    // Ends the thread, which finishes the picture it reads first.
    void stop() {
        {
            const std::lock_guard guard(m_lock);
            m_stop = true;
        }
        m_wake.notify_all();
        m_thread.join();
    }

    std::uint64_t version() const { return m_version.load(std::memory_order_relaxed); }

private:
    struct Job {
        fs::path path;
        AlphaScale alpha;
        std::string key;
    };

    void start() {
        if (!m_started) {
            m_started = true;
            m_thread = JoiningThread([this] { run(); });
        }
    }

    void run() {
        for (;;) {
            Job job;
            bool urgent = false;
            {
                std::unique_lock guard(m_lock);
                m_wake.wait(guard, [&] {
                    return m_stop || !m_urgent.empty() || (precaching && m_next < m_ahead.size());
                });
                if (m_stop) {
                    return;
                }
                if (!m_urgent.empty()) {
                    job = std::move(m_urgent.front());
                    m_urgent.pop_front();
                    urgent = true;
                } else {
                    job = {m_ahead[m_next++], AlphaScale::Gs, {}};
                    job.key = cache_key(job.path, job.alpha);
                }
            }
            if (!urgent && !room_left()) {
                const std::lock_guard guard(m_lock);
                m_next = m_ahead.size();
                continue;
            }
            const bool ok = cached_only(job.key) != nullptr || read_cached(job.path, job.alpha);
            const std::lock_guard guard(m_lock);
            if (!ok) {
                m_failed.insert(job.key);
            }
            // A picture a texture waits for: its conversion is made again. Once the queue is
            // empty, or at most every 50 ms, so that a burst of pictures does not have every
            // waiting texture made again for each one.
            const auto now = std::chrono::steady_clock::now();
            m_dirty = m_wanted.erase(job.key) != 0 || m_dirty;
            if (m_dirty
                && (m_urgent.empty() || now - m_last_bump >= std::chrono::milliseconds(50))) {
                m_version.fetch_add(1, std::memory_order_relaxed);
                m_last_bump = now;
                m_dirty = false;
            }
        }
    }

    static bool room_left() {
        const std::lock_guard guard(cache_lock);
        return cached_bytes + (std::size_t{16} << 20) <= cache_limit;
    }

    std::mutex m_lock;
    std::condition_variable m_wake;
    std::deque<Job> m_urgent;
    std::set<std::string> m_wanted;
    std::set<std::string> m_failed;
    std::vector<fs::path> m_ahead;
    std::size_t m_next = 0;
    std::atomic<std::uint64_t> m_version{0};
    bool m_dirty = false;
    std::chrono::steady_clock::time_point m_last_bump{};
    bool m_started = false;
    bool m_stop = false;
    JoiningThread m_thread;
};

Loader loader;

// A picture for a texture: read now, or (with `pending`, when loading in the background is on)
// queued, and null until it is ready.
std::shared_ptr<const Rgba8Image> picture_for(
    const fs::path& path, AlphaScale alpha, bool* pending
) {
    if (pending != nullptr && async_loading) {
        return loader.get(path, alpha, *pending);
    }
    return read_cached(path, alpha);
}

const std::vector<Crop>* crops_for(const gs::Tex0& tex0, std::span<const std::uint32_t> palette) {
    if (tex0.psm != gs::kPsmt8 || palette.size() != 256) {
        return nullptr;
    }
    const auto found =
        crops.find(key(0, XXH3_64bits(palette.data(), palette.size_bytes()), texture_bits(tex0)));
    return found != crops.end() ? &found->second : nullptr;
}

}  // namespace

void configure(const fs::path& replacements, LoadPng load_png) {
    read_png = load_png;
    loader.reset();
    {
        // Clear the cache before clearing the files and crops, so that any cached pictures are not kept around after the pack is cleared.
        const std::lock_guard guard(cache_lock);
        cached_pictures.clear();
        cached_bytes = 0;
        ++pack_generation;
        cache_limit = precaching ? kPrecacheBytes : kCacheBytes;
    }
    files.clear();
    crops.clear();
    crop_formats.clear();
    if (replacements.empty()) {
        return;
    }
    std::error_code error;
    for (fs::recursive_directory_iterator
             it(replacements, fs::directory_options::skip_permission_denied, error),
         end;
         it != end;
         it.increment(error)) {
        const auto& path = it->path();
        const auto ext = lowercase(path_text(path.extension()));
        if ((ext != ".dds" && ext != ".png") || !it->is_regular_file(error)) {
            continue;
        }
        if (auto name = parse_key(path_text(path.stem()), path)) {
            auto [found, inserted] = files.try_emplace(*name, path);
            if (!inserted && path < found->second) {
                found->second = path;
            }
        }
    }
    if (precaching && async_loading && !files.empty()) {
        // Smallest files first: most pictures fit the cache that way.
        std::vector<std::pair<std::uintmax_t, fs::path>> sized;
        sized.reserve(files.size());
        for (const auto& [name, file] : files) {
            std::error_code ignored;
            sized.emplace_back(fs::file_size(file, ignored), file);
        }
        std::ranges::sort(sized);
        std::vector<fs::path> list;
        list.reserve(sized.size());
        for (auto& [size, file] : sized) {
            list.push_back(std::move(file));
        }
        loader.precache(std::move(list));
    }
    std::size_t crop_count = 0;
    for (auto& [group, entries] : crops) {
        // Shared one-texel margins belong to the next cell, where that cell starts.
        std::ranges::sort(entries, {}, [](const Crop& crop) {
            return std::tie(crop.top, crop.left, crop.file);
        });
        crop_count += entries.size();
    }
    if (!files.empty() || crop_count) {
        log::info(
            "PCSX2 texture pack {}: {} indexed replacements, {} cropped",
            path_text(replacements),
            files.size(),
            crop_count
        );
    }
}

void set_loading(bool in_background, bool precache) {
    async_loading = in_background;
    precaching = precache;
}

std::uint64_t ready_version() {
    return loader.version();
}

bool enabled() {
    return !files.empty() || !crops.empty();
}

bool replace(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha,
    bool* pending
) {
    if (files.empty() || tex0.tw > 10 || tex0.th > 10
        || (tex0.psm != gs::kPsmt8 && tex0.psm != gs::kPsmt8h) || palette.size() != 256
        || raster.size() != static_cast<std::size_t>(tex0.width() * tex0.height())) {
        return false;
    }
    const auto bits = texture_bits(tex0);
    const auto found = files.find(
        key(texture_hash(raster, tex0), XXH3_64bits(palette.data(), palette.size_bytes()), bits)
    );
    if (found == files.end()) {
        return false;
    }
    // Get the replacement picture from the cache or load it from disk and cache it.
    const auto replacement = picture_for(found->second, alpha, pending);
    // Missing or invalid files leave the original texture intact.
    if (!replacement) {
        return false;
    }
    image = *replacement;
    return true;
}

bool may_have_crops(const gs::Tex0& tex0) {
    return crop_formats.contains(texture_bits(tex0));
}

std::array<int, 2> crop_extent(const gs::Tex0& tex0, std::span<const std::uint32_t> palette) {
    const auto* entries = crops_for(tex0, palette);
    if (!entries) {
        return {};
    }
    std::array extent{tex0.width(), tex0.height()};
    for (const auto& crop : *entries) {
        extent[0] = std::max(extent[0], crop.right);
        extent[1] = std::max(extent[1], crop.bottom);
    }
    return extent;
}

bool replace_crops(
    Rgba8Image& image,
    const gs::Tex0& tex0,
    std::span<const std::uint8_t> raster,
    std::span<const std::uint32_t> palette,
    std::array<int, 2> extent,
    bool* pending
) {
    const auto* entries = crops_for(tex0, palette);
    const auto [width, height] = extent;
    if (!entries || width < tex0.width() || height < tex0.height()
        || raster.size() != static_cast<std::size_t>(width) * height || image.pixels.empty()) {
        return false;
    }
    Rgba8Image atlas;
    int scale_x = 0, scale_y = 0;
    for (const auto& crop : *entries) {
        if (crop.right > width || crop.bottom > height) {
            continue;
        }
        const int w = crop.right - crop.left;
        const int h = crop.bottom - crop.top;
        // PCSX2 hashes a region's expanded indices row by row, even for swizzled textures.
        XXH3_state_t hash;
        XXH3_64bits_reset(&hash);
        for (int y = crop.top; y < crop.bottom; ++y) {
            XXH3_64bits_update(&hash, raster.data() + y * width + crop.left, w);
        }
        if (XXH3_64bits_digest(&hash) != crop.hash) {
            continue;
        }
        const auto picture = picture_for(crop.file, AlphaScale::Gs, pending);
        if (!picture) {
            continue;
        }
        if (atlas.pixels.empty()) {
            scale_x = std::max({1, (picture->width + w - 1) / w, image.width / tex0.width()});
            scale_y = std::max({1, (picture->height + h - 1) / h, image.height / tex0.height()});
            if (scale_x > 8192 / width || scale_y > 8192 / height
                || std::size_t{16} * 1024 * 1024
                       < static_cast<std::size_t>(width * scale_x) * height * scale_y) {
                continue;
            }
            atlas = Rgba8Image(width * scale_x, height * scale_y);
            for (int y = 0; y < atlas.height; ++y) {
                for (int x = 0; x < atlas.width; ++x) {
                    const int sx = x / scale_x;
                    const int sy = y / scale_y;
                    const auto pixel =
                        sx < tex0.width() && sy < tex0.height()
                            ? image.texel(
                                  static_cast<int>(
                                      std::int64_t{x} * image.width / (tex0.width() * scale_x)
                                  ),
                                  static_cast<int>(
                                      std::int64_t{y} * image.height / (tex0.height() * scale_y)
                                  )
                              )
                            : palette[raster[sy * width + sx]];
                    atlas.set_texel(x, y, pixel);
                }
            }
        }
        for (int y = 0; y < h * scale_y; ++y) {
            for (int x = 0; x < w * scale_x; ++x) {
                atlas.set_texel(
                    crop.left * scale_x + x,
                    crop.top * scale_y + y,
                    picture->texel(
                        x * picture->width / (w * scale_x), y * picture->height / (h * scale_y)
                    )
                );
            }
        }
        log::verbose("PCSX2 cropped texture: {}", path_text(crop.file.filename()));
    }
    if (atlas.pixels.empty()) {
        return false;
    }
    image = std::move(atlas);
    return true;
}

bool replace_indexed8(
    Rgba8Image& image,
    std::span<const std::uint8_t> indices,
    std::span<const std::uint32_t> palette,
    AlphaScale alpha,
    bool high,
    bool* pending
) {
    const auto width = static_cast<unsigned>(image.width);
    const auto height = static_cast<unsigned>(image.height);
    if (!enabled() || width > kMaxTextureSide || height > kMaxTextureSide
        || !std::has_single_bit(width) || !std::has_single_bit(height)) {
        return false;
    }
    gs::Tex0 tex0{};
    tex0.psm = high ? gs::kPsmt8h : gs::kPsmt8;
    tex0.tw = static_cast<std::uint32_t>(std::countr_zero(width));
    tex0.th = static_cast<std::uint32_t>(std::countr_zero(height));
    return replace(image, tex0, indices, palette, alpha, pending);
}

}  // namespace openrac::renderer::pcsx2_pack
