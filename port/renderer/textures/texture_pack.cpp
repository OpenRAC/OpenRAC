// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/textures/texture_pack.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <format>
#include <fstream>
#include <mutex>
#include <optional>
#include <set>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>

#include "common/joining_thread.h"
#include "common/json.h"
#include "common/log.h"

namespace openrac::renderer::texture_pack {
namespace {

namespace fs = std::filesystem;

struct State {
    std::mutex lock;  // levels load on another thread than the 2D layer converts on
    Io io;
    // The pack's still pictures by hash, the one to use first at the front (still_rank).
    std::unordered_map<std::uint64_t, std::vector<fs::path>> files;
    // The pack's animations by hash: a folder of frames or a GIF, and those read so far.
    std::unordered_map<std::uint64_t, fs::path> animated;
    std::unordered_map<std::uint64_t, std::shared_ptr<const Animation>> animations;
    std::size_t animation_bytes = 0;

    // Still pictures already read, by hash (kMaxKeptPictureBytes); `used` orders them by last use.
    struct Kept {
        std::shared_ptr<const Picture> picture;
        std::size_t bytes = 0;
        std::uint64_t used = 0;
        BlockFormat blocks = BlockFormat::None;  // the block format a KTX2 was read for
    };

    std::unordered_map<std::uint64_t, Kept> kept;
    std::size_t kept_bytes = 0;
    std::uint64_t uses = 0;
    std::uint64_t kept_version = 0;  // one more each picture kept
    // Pictures a texture waits for, read before the rest (Origin::later).
    std::vector<TextureId> urgent;
    // A loading level's pictures, read ahead together (read_level_pictures()).
    std::unordered_map<std::uint64_t, Picture> level_ahead;
    // Pictures met while playing, and the file that keeps them for the next run (set_play_list()).
    std::set<std::uint64_t> play_pictures;
    bool play_list_known = false;  // the file listed some when it was read
    fs::path play_list;
    // Pictures met as a level's texture: not read ahead (preload()).
    std::set<std::uint64_t> level_pictures;
    fs::path pack;
    // One more each configure(), so that a read ahead of an old pack is dropped.
    unsigned generation = 0;
    fs::path dump;
    LevelNamer level_name = nullptr;
    std::unordered_map<std::uint64_t, fs::path> dumped;  // where each dumped texture lies, by hash
    std::set<std::uint64_t> written;  // the dumped textures this run wrote (undump())
    std::set<std::uint64_t> grouped;  // first hashes of the cycles written under Animated/
    unsigned replaced = 0;
    // preload()'s thread, last so that it stops before the rest goes; `reading` while it works.
    bool reading = false;
    bool read_again = false;
    std::atomic<bool> stop_reading{false};
    JoiningThread reader;

    ~State() { stop_reading = true; }
};

State& state() {
    static State s;
    return s;
}

// Starts the reader thread, or has the one under way read again when it ends; the lock is held.
void start_reader(State& s);

constexpr std::uint64_t kMicrosecondsPerSecond = 1000000;
constexpr std::size_t kHashDigits = 16;  // a hash's name: 64 bits in hex

// The hash a name spells, or nothing when it is not 16 lowercase hex digits.
std::optional<std::uint64_t> parse_hash(const std::string& stem) {
    // Any other length is some other file.
    if (stem.size() != kHashDigits) {
        return {};
    }
    std::uint64_t value = 0;
    for (const char c : stem) {
        const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (digit < 0) {
            return {};
        }
        value = value << 4 | static_cast<std::uint64_t>(digit);
    }
    return value;
}

// A pack file's hash from its name, or nothing when it is not one.
std::optional<std::uint64_t> hash_of(const fs::path& file) {
    // Only PNGs are still pictures.
    if (file.extension() != ".png") {
        return {};
    }
    return parse_hash(file.stem().string());
}

// Where a still picture of a pack stands when a texture has several: an SVG first (made for the
// sharpest picture), then a KTX2, then a PNG (what a pack keeps for older versions of the port,
// which read only PNGs). Nothing for a file that is not a still picture.
std::optional<int> still_rank(const fs::path& file) {
    const fs::path extension = file.extension();
    if (extension == ".svg") {
        return 0;
    }
    if (extension == ".ktx2") {
        return 1;
    }
    if (extension == ".png") {
        return 2;
    }
    return std::nullopt;
}

// The pack's still pictures under a folder, by hash, each texture's in the order they are tried.
std::unordered_map<std::uint64_t, std::vector<fs::path>> pictures_under(const fs::path& root) {
    std::unordered_map<std::uint64_t, std::vector<fs::path>> pictures;
    std::error_code e;
    for (const auto& entry : fs::recursive_directory_iterator(root, e)) {
        if (!entry.is_regular_file(e) || !still_rank(entry.path())) {
            continue;
        }
        if (const auto hash = parse_hash(entry.path().stem().string())) {
            pictures[*hash].push_back(entry.path());
        }
    }
    // Two of one kind: the first met wins, as before there were several kinds.
    for (auto& [hash, files] : pictures) {
        std::stable_sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
            return *still_rank(a) < *still_rank(b);
        });
    }
    return pictures;
}

std::atomic<BlockFormat> g_block_format{BlockFormat::None};
// Whether the pack's pictures are shown (set_shown()), and one more each change.
std::atomic<bool> g_shown{true};
std::atomic<std::uint64_t> g_shown_version{0};

// Whether the picture kept for `hash` is the one to use now; the lock is held. A KTX2 is kept in
// the block format the GPU had when it was read, and the game learns that format only once its
// window is open, after reading ahead has begun. A PNG or SVG has no blocks and suits any format.
bool kept_for_now(const State& s, std::uint64_t hash) {
    const auto kept = s.kept.find(hash);
    if (kept == s.kept.end()) {
        return false;
    }
    if (kept->second.blocks == g_block_format.load()) {
        return true;
    }
    const auto files = s.files.find(hash);
    return files != s.files.end() && !files->second.empty()
           && files->second.front().extension() != ".ktx2";
}

// A still picture of the pack for a texture of width x height, read by its kind.
std::optional<Picture> load_still(const Io& io, const fs::path& file, int width, int height) {
    const fs::path extension = file.extension();
    const auto plain = [](std::optional<Rgba8Image> image) -> std::optional<Picture> {
        if (!image) {
            return std::nullopt;
        }
        return Picture{std::move(*image), nullptr};
    };
    if (extension == ".svg") {
        return io.load_svg != nullptr ? plain(io.load_svg(file, width, height)) : std::nullopt;
    }
    if (extension == ".ktx2") {
        return io.load_ktx2 != nullptr ? io.load_ktx2(file, g_block_format.load()) : std::nullopt;
    }
    return io.load != nullptr ? plain(io.load(file)) : std::nullopt;
}

// A frame's number from its file name (0001.png is 1), or nothing when it is not a frame.
std::optional<unsigned> frame_number(const fs::path& file) {
    const std::string stem = file.stem().string();
    unsigned number = 0;
    const auto [end, error] = std::from_chars(stem.data(), stem.data() + stem.size(), number);
    // A frame is a PNG named by digits alone, and not by a hash: a PNG named by a hash is always a
    // texture of its own, wherever it lies.
    if (file.extension() != ".png" || stem.empty() || error != std::errc{}
        || end != stem.data() + stem.size() || parse_hash(stem)) {
        return {};
    }
    return number;
}

// Whether `folder` holds an animation's frames.
bool has_frames(const fs::path& folder) {
    std::error_code e;
    for (const auto& entry : fs::directory_iterator(folder, e)) {
        // One numbered PNG makes it an animation.
        if (entry.is_regular_file(e) && frame_number(entry.path())) {
            return true;
        }
    }
    return false;
}

// Every texture file under a folder, by hash.
std::unordered_map<std::uint64_t, fs::path> files_under(const fs::path& root) {
    std::unordered_map<std::uint64_t, fs::path> files;
    std::error_code e;
    if (root.empty() || !fs::is_directory(root, e)) {
        return files;
    }
    for (const auto& entry : fs::recursive_directory_iterator(root, e)) {
        if (entry.is_regular_file(e)) {
            if (const auto hash = hash_of(entry.path())) {
                files.emplace(*hash, entry.path());
            }
        }
    }
    return files;
}

// A planet's name as a folder name: no separators or characters Windows refuses, no trailing
// dots or spaces.
std::string folder_name(std::string name) {
    for (char& c : name) {
        if (std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos
            || static_cast<unsigned char>(c) < 32) {
            c = ' ';
        }
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
        name.pop_back();
    }
    while (!name.empty() && name.front() == ' ') {
        name.erase(name.begin());
    }
    return name;
}

// Returns whether any texel has nonzero alpha, including faint effects.
bool has_visible_texels(const Rgba8Image& image) {
    // RGBA8 stores alpha in the fourth byte of each pixel.
    constexpr std::size_t kPixelBytes = 4;
    constexpr std::size_t kAlphaByte = 3;

    // The alpha bytes alone determine whether the RGBA picture is empty.
    for (std::size_t byte = kAlphaByte; byte < image.pixels.size(); byte += kPixelBytes) {
        // Any nonzero alpha carries artwork that a pack maker may want to keep.
        if (image.pixels[byte] != 0) {
            return true;
        }
    }

    return false;
}

// Writes or moves the texture's dump file; the lock is held.
void dump(State& s, std::uint64_t hash, const Rgba8Image& image, const Origin& origin) {
    std::error_code e;
    const fs::path folder = dump_folder(origin);
    const auto found = s.dumped.find(hash);

    // Only new exports need their pixels checked; existing files can still move between folders.
    if (found == s.dumped.end()) {
        // Changing RGB below empty alpha produces unique hashes without useful RGBA pictures.
        if (origin.skip_empty_alpha && !has_visible_texels(image)) {
            return;
        }

        fs::create_directories(s.dump / folder, e);
        const fs::path file = s.dump / folder / file_name(hash);
        if (!s.io.save(file, image)) {
            log::warn("texture {} could not be dumped", file_name(hash));
        }
        s.dumped.emplace(hash, file);
        s.written.insert(hash);
        return;
    }
    // A level's texture dumped under another planet is shared: it moves to Shared/.
    const fs::path where = found->second.parent_path().lexically_relative(s.dump);
    if (origin.interface || origin.level < 0 || where.empty() || *where.begin() != "Planets"
        || where == folder) {
        return;
    }
    fs::create_directories(s.dump / "Shared", e);
    const fs::path shared = s.dump / "Shared" / file_name(hash);
    fs::rename(found->second, shared, e);
    if (e) {
        log::warn("texture {} could not be moved to Shared: {}", file_name(hash), e.message());
        return;
    }
    found->second = shared;
}

// The bytes a picture holds: its texels and its blocks.
std::size_t picture_bytes(const Picture& picture) {
    std::size_t bytes = picture.image.pixels.size();
    if (picture.blocks != nullptr) {
        for (const auto& level : picture.blocks->levels) {
            bytes += level.size();
        }
    }
    return bytes;
}

// Keeps a picture just read for the next time its texture is met; the lock is held. The least
// recently used make room. One larger than the whole budget is not kept.
void keep(State& s, std::uint64_t hash, std::shared_ptr<const Picture> picture) {
    // One kept for another block format gives way.
    if (const auto old = s.kept.find(hash); old != s.kept.end()) {
        s.kept_bytes -= old->second.bytes;
        s.kept.erase(old);
    }
    const std::size_t bytes = picture_bytes(*picture);
    if (bytes > kMaxKeptPictureBytes) {
        return;
    }
    while (s.kept_bytes + bytes > kMaxKeptPictureBytes && !s.kept.empty()) {
        const auto oldest =
            std::min_element(s.kept.begin(), s.kept.end(), [](const auto& a, const auto& b) {
                return a.second.used < b.second.used;
            });
        s.kept_bytes -= oldest->second.bytes;
        s.kept.erase(oldest);
    }
    s.kept_bytes += bytes;
    ++s.kept_version;
    s.kept.emplace(hash, State::Kept{std::move(picture), bytes, ++s.uses, g_block_format.load()});
}

// Notes a pack picture met while playing in the play list; the lock is held.
void learn_play_picture(State& s, std::uint64_t hash) {
    if (s.play_list.empty() || !s.play_pictures.insert(hash).second) {
        return;
    }
    std::error_code e;
    fs::create_directories(s.play_list.parent_path(), e);
    std::ofstream(s.play_list, std::ios::app) << file_name(hash).substr(0, kHashDigits) << '\n';
}

// The animation of hash `hash` from `source` (a folder or a GIF), read on first use and kept.
// Null when it cannot be used: the log says why, and it is not tried again.
std::shared_ptr<const Animation> animation_of(
    State& s, std::uint64_t hash, const fs::path& source
) {
    if (const auto found = s.animations.find(hash); found != s.animations.end()) {
        return found->second;
    }
    std::string why;
    std::optional<Animation> animation;
    // A GIF is read by the program; anything else is a folder of frames.
    if (source.extension() == ".gif") {
        animation = s.io.load_gif != nullptr ? s.io.load_gif(source) : std::nullopt;
        why = "it cannot be read as a GIF";
        // A GIF's frames are one size by the format; a reader that gives none failed.
        if (animation && animation->frames.empty()) {
            animation.reset();
        }
    } else {
        animation = load_folder(source, s.io, why);
    }
    // Without the frames there is nothing to draw: the still picture, if any, is used.
    if (!animation) {
        log::warn("texture pack: {}: {}; it is not animated", source.string(), why);
        s.animations.emplace(hash, nullptr);
        return nullptr;
    }
    const std::size_t bytes = animation->bytes();
    // The cap keeps a large pack from filling the memory; the first frame still replaces it.
    if (s.animation_bytes + bytes > kMaxAnimationBytes) {
        log::warn(
            "texture pack: {}: {} MB of frames would pass the {} MB all animations may hold; only "
            "its first frame is used",
            source.string(),
            bytes >> 20,
            kMaxAnimationBytes >> 20
        );
        animation->frames.resize(1);
        animation->durations_us.resize(1);
    } else {
        s.animation_bytes += bytes;
    }
    log::info(
        "texture pack: {} animated, {} frame(s) of {}x{}; animations hold {} KB",
        file_name(hash),
        animation->frames.size(),
        animation->frames[0].width,
        animation->frames[0].height,
        s.animation_bytes >> 10
    );
    auto shared = std::make_shared<const Animation>(std::move(*animation));
    s.animations.emplace(hash, shared);
    return shared;
}

}  // namespace

std::size_t Animation::bytes() const {
    std::size_t total = 0;
    for (const Rgba8Image& frame : frames) {
        total += frame.pixels.size();
    }
    return total;
}

std::size_t frame_at(const Animation& animation, std::uint64_t ticks) {
    const std::span<const std::uint32_t> lengths = animation.durations_us;
    const std::size_t count = lengths.size();
    // One frame (or none) never changes.
    if (count <= 1) {
        return 0;
    }
    std::uint64_t forward = 0;
    for (const std::uint32_t length : lengths) {
        forward += length;
    }
    std::uint64_t t = ticks * kMicrosecondsPerSecond / kTicksPerSecond;
    // Once over, Once stays on its last frame.
    if (animation.mode == AnimationMode::Once && t >= forward) {
        return count - 1;
    }
    // PingPong runs forwards, then back through the frames between the two ends.
    if (animation.mode == AnimationMode::PingPong) {
        const std::uint64_t back = forward - lengths.front() - lengths.back();
        t %= forward + back;
        // Past the forward run: the way back, from the frame before the last.
        if (t >= forward) {
            t -= forward;
            for (std::size_t i = count - 2; i > 0; --i) {
                // The frame whose time holds t.
                if (t < lengths[i]) {
                    return i;
                }
                t -= lengths[i];
            }
            return 0;
        }
    } else {
        t %= forward;
    }
    for (std::size_t i = 0; i < count; ++i) {
        // The frame whose time holds t.
        if (t < lengths[i]) {
            return i;
        }
        t -= lengths[i];
    }
    return count - 1;
}

std::optional<Animation> load_folder(const fs::path& folder, const Io& io, std::string& why) {
    std::vector<std::pair<unsigned, fs::path>> numbered;
    std::error_code e;
    for (const auto& entry : fs::directory_iterator(folder, e)) {
        // Numbered PNGs are the frames; animation.json and anything else are not.
        if (const auto number = frame_number(entry.path()); number && entry.is_regular_file(e)) {
            numbered.emplace_back(*number, entry.path());
        }
    }
    std::sort(numbered.begin(), numbered.end());
    // A folder of no frames is no animation.
    if (numbered.empty() || io.load == nullptr) {
        why = "it has no numbered frames (0001.png, 0002.png...)";
        return std::nullopt;
    }

    // animation.json: fps and mode, each optional.
    double fps = kDefaultFps;
    Animation animation;
    if (const fs::path settings = folder / "animation.json"; fs::is_regular_file(settings, e)) {
        json::Value root;
        std::string error;
        // A file that is not JSON leaves the defaults, as an absent one does.
        if (!json::parse_file(settings.string(), root, error)) {
            log::warn("texture pack: {}; 10 frames a second, looped", error);
            root = json::Value{};
        }
        constexpr double kMostFps = 1000.0;  // a millisecond a frame
        fps = std::clamp(root["fps"].number(kDefaultFps), 1.0 / kMostFps, kMostFps);
        const std::string& mode = root["mode"].string();
        // The three modes by name; anything else loops.
        if (mode == "pingpong") {
            animation.mode = AnimationMode::PingPong;
        } else if (mode == "once") {
            animation.mode = AnimationMode::Once;
        } else if (!mode.empty() && mode != "loop") {
            log::warn(
                "texture pack: {}: mode \"{}\" is not loop, pingpong or once; it loops",
                settings.string(),
                mode
            );
        }
    }
    const auto length = static_cast<std::uint32_t>(
        std::max(1.0, static_cast<double>(kMicrosecondsPerSecond) / fps)
    );

    for (const auto& [number, file] : numbered) {
        auto frame = io.load(file);
        // An unreadable frame would leave a hole in the animation.
        if (!frame || frame->pixels.empty()) {
            why = std::format("{} cannot be read", file.filename().string());
            return std::nullopt;
        }
        // The frames are drawn over one area, so they must be one size.
        if (!animation.frames.empty()
            && (frame->width != animation.frames[0].width
                || frame->height != animation.frames[0].height)) {
            why = std::format(
                "{} is {}x{} but {} is {}x{}; all frames of an animation must be one size",
                file.filename().string(),
                frame->width,
                frame->height,
                numbered.front().second.filename().string(),
                animation.frames[0].width,
                animation.frames[0].height
            );
            return std::nullopt;
        }
        animation.frames.push_back(std::move(*frame));
        animation.durations_us.push_back(length);
    }
    return animation;
}

fs::path dump_folder(const Origin& origin) {
    // The frames of a cycle together, by its first frame's name.
    if (origin.cycle != 0) {
        return fs::path("Animated") / file_name(origin.cycle).substr(0, kHashDigits);
    }
    if (origin.interface) {
        return "Interface";
    }
    if (origin.level < 0) {
        return "Shared";
    }
    const State& s = state();
    std::string name = s.level_name != nullptr ? folder_name(s.level_name(origin.level)) : "";
    if (name.empty()) {
        name = std::format("Level {}", origin.level);
    }
    return fs::path("Planets") / name;
}

void configure(const fs::path& pack, const fs::path& dump, Io io, LevelNamer level_name) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    s.io = io;
    s.level_name = level_name;
    s.files.clear();
    s.animated.clear();
    s.animations.clear();
    s.animation_bytes = 0;
    s.kept.clear();
    s.kept_bytes = 0;
    s.urgent.clear();
    s.level_pictures.clear();
    s.level_ahead.clear();
    s.play_pictures.clear();
    s.play_list_known = false;
    s.play_list.clear();
    s.pack = pack;
    ++s.generation;
    s.dumped.clear();
    s.written.clear();
    s.grouped.clear();
    s.replaced = 0;
    s.dump = dump;
    std::error_code e;
    if (!pack.empty() && fs::is_directory(pack, e)) {
        s.files = pictures_under(pack);
        for (auto it = fs::recursive_directory_iterator(pack, e);
             it != fs::recursive_directory_iterator();
             it.increment(e)) {
            const fs::path& path = it->path();
            const auto named = parse_hash(path.stem().string());
            // A folder named by a hash with frames in it is an animation (the hash-named PNGs a
            // dump's Animated/ folder holds are textures of their own, found above).
            if (it->is_directory(e) && parse_hash(path.filename().string()) && has_frames(path)) {
                s.animated.insert_or_assign(*parse_hash(path.filename().string()), path);
            } else if (it->is_regular_file(e) && named && path.extension() == ".gif") {
                s.animated.insert_or_assign(*named, path);
            }
        }
        log::info(
            "texture pack {}: {} texture(s), {} animated",
            pack.string(),
            s.files.size(),
            s.animated.size()
        );
    }
    if (!dump.empty()) {
        fs::create_directories(dump, e);
        // Textures dumped by an earlier run are not written again, but may still move to Shared/.
        s.dumped = files_under(dump);
        log::info("textures dumped to {} ({} there already)", dump.string(), s.dumped.size());
    }
}

std::uint64_t image_hash(const Rgba8Image& image) {
    std::uint64_t h = 1469598103934665603ull;
    const auto mix = [&](std::uint8_t byte) {
        h ^= byte;
        h *= 1099511628211ull;
    };
    for (const int side : {image.width, image.height}) {
        for (int i = 0; i < 4; ++i) {
            mix(static_cast<std::uint8_t>(static_cast<std::uint32_t>(side) >> (8 * i)));
        }
    }
    for (const std::uint8_t byte : image.pixels) {
        mix(byte);
    }
    return h;
}

std::string file_name(std::uint64_t hash) {
    return std::format("{:016x}.png", hash);
}

bool replace(Rgba8Image& image, const Origin& origin, Replacement* found_out) {
    State& s = state();
    std::unique_lock guard(s.lock);
    if ((s.files.empty() && s.animated.empty() && s.dump.empty()) || image.pixels.empty()) {
        return false;
    }
    const std::uint64_t hash = image_hash(image);
    // The caller keeps the hash, to recognise the texture again (CycleWatch).
    if (found_out != nullptr) {
        *found_out = Replacement{hash, nullptr, nullptr};
    }
    if (!s.dump.empty() && s.io.save != nullptr && origin.dump) {
        dump(s, hash, image, origin);
    }
    if (!g_shown.load()) {
        return false;
    }
    // An animation comes before a still picture of the same hash.
    if (const auto animated = s.animated.find(hash); animated != s.animated.end()) {
        // One that cannot be used falls back to the still picture, if there is one.
        if (auto animation = animation_of(s, hash, animated->second)) {
            image = animation->frames[0];
            ++s.replaced;
            // A caller that draws the frames gets them all.
            if (found_out != nullptr) {
                found_out->animation = std::move(animation);
            }
            return true;
        }
        s.animated.erase(animated);
    }
    const auto found = s.files.find(hash);
    if (found == s.files.end()) {
        return false;
    }
    // Read before, for the GPU's block format: no file is read again.
    if (const auto kept = s.kept.find(hash); kept_for_now(s, hash)) {
        kept->second.used = ++s.uses;
        image = kept->second.picture->image;
        ++s.replaced;
        if (found_out != nullptr) {
            found_out->blocks = kept->second.picture->blocks;
        }
        if (origin.level < 0) {
            learn_play_picture(s, hash);
        }
        // A level's texture, read ahead before it was known as one: the level keeps its own copy.
        if (origin.level >= 0) {
            s.level_pictures.insert(hash);
            s.kept_bytes -= kept->second.bytes;
            s.kept.erase(kept);
        }
        return true;
    }
    // A level's picture read ahead with the others of its level.
    if (const auto ahead = s.level_ahead.find(hash); ahead != s.level_ahead.end()) {
        if (origin.level >= 0) {
            s.level_pictures.insert(hash);
        }
        image = std::move(ahead->second.image);
        if (found_out != nullptr) {
            found_out->blocks = std::move(ahead->second.blocks);
        }
        s.level_ahead.erase(ahead);
        ++s.replaced;
        return true;
    }
    // Read in the background, first of all; the original is drawn until then.
    if (origin.later && origin.level < 0) {
        learn_play_picture(s, hash);
        const bool queued = std::any_of(s.urgent.begin(), s.urgent.end(), [&](const TextureId& t) {
            return t.hash == hash;
        });
        if (!queued) {
            s.urgent.push_back({hash, image.width, image.height});
        }
        start_reader(s);
        return false;
    }
    // The files are read without the lock: a level loading on another thread must not stop the
    // 2D layer's conversions while it reads its pictures.
    const Io io = s.io;
    const unsigned generation = s.generation;
    for (;;) {
        const auto files = s.files.find(hash);
        if (files == s.files.end()) {
            return false;
        }
        if (files->second.empty()) {
            s.files.erase(files);
            return false;
        }
        const fs::path file = files->second.front();
        guard.unlock();
        auto picture = load_still(io, file, image.width, image.height);
        guard.lock();
        // Another pack was configured meanwhile: this one's picture is not used.
        if (s.generation != generation) {
            return false;
        }
        if (picture && !picture->image.pixels.empty()) {
            // A level's textures are read once, as it loads; the rest come back while playing.
            if (origin.level >= 0) {
                s.level_pictures.insert(hash);
                image = std::move(picture->image);
                if (found_out != nullptr) {
                    found_out->blocks = std::move(picture->blocks);
                }
            } else {
                learn_play_picture(s, hash);
                auto shared = std::make_shared<const Picture>(std::move(*picture));
                keep(s, hash, shared);
                image = shared->image;
                if (found_out != nullptr) {
                    found_out->blocks = shared->blocks;
                }
            }
            ++s.replaced;
            return true;
        }
        // A file that cannot be read is not tried again; the next picture of the texture is.
        const auto again = s.files.find(hash);
        if (again == s.files.end()) {
            return false;
        }
        std::vector<fs::path>& candidates = again->second;
        log::warn(
            "texture pack: {} cannot be read; {}",
            file.string(),
            candidates.size() > 1 ? "the next picture of it is used" : "the original is kept"
        );
        std::erase(candidates, file);
    }
}

namespace {

// Reads one picture without the lock and keeps it, unless another pack was configured meanwhile
// or (with `budget`) it would pass the budget. False when the pass should stop.
bool read_and_keep(
    State& s,
    const Io& io,
    unsigned generation,
    const TextureId& texture,
    const fs::path& file,
    bool budget,
    std::size_t& read,
    std::size_t& bytes
) {
    // One that cannot be read is left to replace(), which says why and tries the next picture.
    auto picture = load_still(io, file, texture.width, texture.height);
    if (!picture || picture->image.pixels.empty()) {
        return true;
    }
    auto shared = std::make_shared<const Picture>(std::move(*picture));
    const std::size_t size = picture_bytes(*shared);
    const std::lock_guard guard(s.lock);
    if (s.generation != generation) {
        return false;
    }
    // Pictures already kept are not pushed out by guesses (a picture waited for may).
    if (budget && s.kept_bytes + size > kMaxKeptPictureBytes) {
        return false;
    }
    if (!kept_for_now(s, texture.hash) && !s.level_pictures.contains(texture.hash)) {
        keep(s, texture.hash, std::move(shared));
        bytes += size;
        ++read;
    }
    return true;
}

// The next picture a texture waits for (Origin::later), taken from the queue; nothing when none
// waits.
std::optional<std::pair<TextureId, fs::path>> next_urgent(State& s) {
    const std::lock_guard guard(s.lock);
    while (!s.urgent.empty()) {
        const TextureId t = s.urgent.front();
        s.urgent.erase(s.urgent.begin());
        const auto files = s.files.find(t.hash);
        if (files != s.files.end() && !files->second.empty() && !kept_for_now(s, t.hash)) {
            return std::pair{t, files->second.front()};
        }
    }
    return std::nullopt;
}

// One pass of the reader: the pictures textures wait for first, then the pack's pictures for
// textures met while playing, smallest first, all read without the lock so that the game goes on.
void read_ahead(const std::atomic<bool>& stop) {
    State& s = state();
    std::vector<std::tuple<std::uintmax_t, std::uint64_t, fs::path>> wanted;
    Io io;
    unsigned generation = 0;
    {
        const std::lock_guard guard(s.lock);
        io = s.io;
        generation = s.generation;
        for (const auto& [hash, candidates] : s.files) {
            // Read already, a level's own, or an SVG (drawn at the size of a texture not met yet).
            if (candidates.empty() || kept_for_now(s, hash) || s.level_pictures.contains(hash)
                || candidates.front().extension() == ".svg") {
                continue;
            }
            // Once the play list knows some, only those: the rest are levels' textures.
            if (s.play_list_known && !s.play_pictures.contains(hash)) {
                continue;
            }
            // A dump's Planets/ folder holds levels' textures (dump_folder()).
            const fs::path where = candidates.front().lexically_relative(s.pack);
            if (!where.empty() && *where.begin() == "Planets") {
                continue;
            }
            std::error_code e;
            wanted.emplace_back(fs::file_size(candidates.front(), e), hash, candidates.front());
        }
    }
    // Small pictures first: HUD icons and effects are mostly small, and many fit the budget.
    std::sort(wanted.begin(), wanted.end());
    std::size_t read = 0;
    std::size_t bytes = 0;
    std::size_t next = 0;
    bool full = false;
    while (!stop) {
        // A picture a texture waits for comes before the next guess.
        if (const auto urgent = next_urgent(s)) {
            if (!read_and_keep(
                    s, io, generation, urgent->first, urgent->second, false, read, bytes
                )) {
                break;
            }
            continue;
        }
        if (full || next == wanted.size()) {
            break;
        }
        const auto& [file_bytes, hash, file] = wanted[next++];
        full = !read_and_keep(s, io, generation, {hash, 0, 0}, file, true, read, bytes);
    }
    if (read > 0) {
        log::info("texture pack: {} picture(s) read ahead ({} MB)", read, bytes >> 20);
    }
}

void start_reader(State& s) {
    if (s.reading) {
        s.read_again = true;
        return;
    }
    // The last pass has ended (reading is false), so its thread only has to leave.
    s.reader.join();
    s.reading = true;
    s.reader = JoiningThread([] {
        State& st = state();
        for (;;) {
            read_ahead(st.stop_reading);
            const std::lock_guard guard(st.lock);
            // The thread takes the lock no more once it is not reading.
            if ((!st.read_again && st.urgent.empty()) || st.stop_reading) {
                st.reading = false;
                return;
            }
            st.read_again = false;
        }
    });
}

}  // namespace

void preload() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    if (!s.files.empty()) {
        start_reader(s);
    }
}

std::uint64_t kept_version() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    return s.kept_version;
}

bool is_kept(std::uint64_t hash) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    return kept_for_now(s, hash);
}

void finish_preload() {
    State& s = state();
    JoiningThread reader;
    {
        const std::lock_guard guard(s.lock);
        reader = std::move(s.reader);
    }
    reader.join();
}

void set_play_list(const fs::path& file) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    s.play_list = file;
    s.play_pictures.clear();
    std::ifstream in(file);
    for (std::string line; std::getline(in, line);) {
        if (const auto hash = parse_hash(line)) {
            s.play_pictures.insert(*hash);
        }
    }
    s.play_list_known = !s.play_pictures.empty();
}

bool has_pictures() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    return !s.files.empty();
}

void read_level_pictures(std::span<const TextureId> textures) {
    State& s = state();

    struct Wanted {
        std::uint64_t hash;
        fs::path file;
        int width;
        int height;
        std::optional<Picture> picture;
    };

    std::vector<Wanted> wanted;
    Io io;
    unsigned generation = 0;
    {
        const std::lock_guard guard(s.lock);
        io = s.io;
        generation = s.generation;
        for (const TextureId& t : textures) {
            const auto files = s.files.find(t.hash);
            // An animation comes first (replace()); a picture kept or read ahead is there already.
            if (files == s.files.end() || files->second.empty() || s.animated.contains(t.hash)
                || kept_for_now(s, t.hash) || s.level_ahead.contains(t.hash)) {
                continue;
            }
            wanted.push_back({t.hash, files->second.front(), t.width, t.height, std::nullopt});
        }
    }
    std::sort(wanted.begin(), wanted.end(), [](const Wanted& a, const Wanted& b) {
        return a.hash < b.hash;
    });
    wanted.erase(
        std::unique(
            wanted.begin(),
            wanted.end(),
            [](const Wanted& a, const Wanted& b) { return a.hash == b.hash; }
        ),
        wanted.end()
    );
    if (wanted.empty()) {
        return;
    }
    // A few threads: decoding is the cost, the disk is rarely the limit.
    constexpr unsigned kMostThreads = 8;
    const unsigned count = std::clamp(
        std::thread::hardware_concurrency(),
        1u,
        static_cast<unsigned>(std::min<std::size_t>(kMostThreads, wanted.size()))
    );
    std::atomic<std::size_t> next{0};
    {
        std::vector<JoiningThread> workers;
        for (unsigned i = 0; i < count; ++i) {
            workers.emplace_back([&] {
                for (std::size_t at = next++; at < wanted.size(); at = next++) {
                    Wanted& w = wanted[at];
                    w.picture = load_still(io, w.file, w.width, w.height);
                }
            });
        }
    }
    const std::lock_guard guard(s.lock);
    // Another pack was configured meanwhile.
    if (s.generation != generation) {
        return;
    }
    // One that cannot be read is left to replace(), which says why and tries the next picture.
    for (Wanted& w : wanted) {
        if (w.picture && !w.picture->image.pixels.empty()) {
            s.level_ahead.insert_or_assign(w.hash, std::move(*w.picture));
        }
    }
}

void drop_level_pictures() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    s.level_ahead.clear();
}

void set_block_format(BlockFormat format) {
    g_block_format.store(format);
}

void set_shown(bool shown) {
    if (g_shown.exchange(shown) != shown) {
        g_shown_version.fetch_add(1);
    }
}

bool shown() {
    return g_shown.load();
}

std::uint64_t shown_version() {
    return g_shown_version.load();
}

bool dumping() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    return !s.dump.empty();
}

void dump_animated(
    std::span<const std::uint64_t> order, std::span<const Rgba8Image* const> images
) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    // Nothing to write without a dump, a writer or a whole cycle, or written already.
    if (s.dump.empty() || s.io.save == nullptr || order.empty() || images.size() != order.size()
        || !s.grouped.insert(order.front()).second) {
        return;
    }
    std::error_code e;
    const fs::path folder = s.dump / dump_folder({.cycle = order.front()});
    fs::create_directories(folder, e);
    std::ofstream list(folder / "order.txt");
    list << "# The frames of one texture the game cycles, in the order it shows them.\n";
    std::set<std::uint64_t> written;
    for (std::size_t i = 0; i < order.size(); ++i) {
        list << file_name(order[i]) << '\n';
        // A frame shown twice in a cycle (a pingpong's middle ones) is written once; one no longer
        // in memory is still listed, its picture in the dump's other folders.
        if (images[i] != nullptr && written.insert(order[i]).second
            && !s.io.save(folder / file_name(order[i]), *images[i])) {
            log::warn("texture {} could not be dumped", file_name(order[i]));
        }
    }
    log::info(
        "textures: {} cycles through {} frames; written to {}",
        file_name(order.front()),
        written.size(),
        folder.string()
    );
}

void dump_picture(const Rgba8Image& image, const Origin& origin) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    if (!s.dump.empty() && s.io.save != nullptr && !image.pixels.empty()) {
        dump(s, image_hash(image), image, origin);
    }
}

void undump(std::span<const std::uint64_t> hashes) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    for (const std::uint64_t hash : hashes) {
        const auto found = s.dumped.find(hash);
        if (s.written.erase(hash) == 0 || found == s.dumped.end()) {
            continue;
        }
        std::error_code e;
        fs::remove(found->second, e);
        s.dumped.erase(found);
    }
}

LiveWatch::Seen LiveWatch::see(std::uint64_t place, std::uint64_t hash, std::uint64_t tick) {
    Place& at = m_places[place];
    // A pause: what came before is forgotten (a tick counted back, as after a load, is one too).
    if (at.ticks != 0 && (tick < at.last_tick || tick - at.last_tick > kMaxGap)) {
        at = Place{};
    }
    Seen seen;
    if (at.live) {
        at.last_tick = tick;
        seen.live = true;
        return seen;
    }
    // One shown before in this run: a cycle's frame again, made again after it was dropped.
    if (!at.hashes.insert(hash).second) {
        return seen;
    }
    if (at.ticks == 0 || tick != at.last_tick) {
        ++at.ticks;
    }
    at.last_tick = tick;
    if (at.ticks >= kLiveTicks) {
        at.live = true;
        seen.live = true;
        seen.undump.assign(at.hashes.begin(), at.hashes.end());
        at.hashes.clear();
    }
    return seen;
}

std::optional<std::vector<std::uint64_t>> CycleWatch::see(
    std::uint64_t slot, std::uint64_t hash, std::uint64_t frame
) {
    Slot& at = m_slots[slot];
    // The same texture again is no change.
    if (!at.hashes.empty() && at.hashes.back() == hash) {
        at.last_frame = frame;
        return std::nullopt;
    }
    // Another texture in the same tick: two things drawn in the slot, never an animation's frame.
    if (!at.ticks.empty() && at.ticks.back() == frame) {
        at.hashes.back() = hash;
        return std::nullopt;
    }
    // A long pause between changes is not a cycle's pace: what came before is forgotten.
    if (!at.ticks.empty() && frame - at.ticks.back() > kMaxFrameGap) {
        at.hashes.clear();
        at.ticks.clear();
    }
    at.hashes.push_back(hash);
    at.ticks.push_back(frame);
    at.last_frame = frame;
    constexpr std::size_t kKept = kMaxCycle * 2;
    // Two whole cycles of the longest are all a test needs.
    if (at.hashes.size() > kKept) {
        at.hashes.erase(at.hashes.begin());
        at.ticks.erase(at.ticks.begin());
    }
    // Reported once: the dump writes each cycle once.
    if (at.reported) {
        return std::nullopt;
    }
    const std::vector<std::uint64_t>& h = at.hashes;
    for (std::size_t length = 2; length * 2 <= h.size(); ++length) {
        const auto tail = h.end() - static_cast<std::ptrdiff_t>(length);
        const auto before = tail - static_cast<std::ptrdiff_t>(length);
        // The last `length` changes repeat the `length` before them.
        if (!std::equal(tail, h.end(), before)) {
            continue;
        }
        const std::size_t first = at.ticks.size() - length * 2;
        bool paced = true;
        for (std::size_t i = first + 1; i < at.ticks.size(); ++i) {
            // A frame shown under kMinFrameGap ticks: two things taking turns in the slot.
            if (at.ticks[i] - at.ticks[i - 1] < kMinFrameGap) {
                paced = false;
            }
        }
        // Not at an animation's pace.
        if (!paced) {
            continue;
        }
        std::vector<std::uint64_t> cycle(before, tail);
        // One texture alternating with itself is no cycle.
        if (std::all_of(cycle.begin(), cycle.end(), [&](std::uint64_t f) {
                return f == cycle[0];
            })) {
            continue;
        }
        // The cycle starts at its lowest hash, so that the folder's name is the same whichever
        // frame the game was on when the watch began.
        std::rotate(cycle.begin(), std::min_element(cycle.begin(), cycle.end()), cycle.end());
        at.reported = true;
        return cycle;
    }
    return std::nullopt;
}

unsigned replaced_count() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    return s.replaced;
}

}  // namespace openrac::renderer::texture_pack
