// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/texture_pack.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <mutex>
#include <set>
#include <string_view>
#include <unordered_map>

#include "common/json.h"
#include "common/log.h"

namespace openrac::renderer::texture_pack {
namespace {

namespace fs = std::filesystem;

struct State {
    std::mutex lock;  // levels load on another thread than the 2D layer converts on
    Io io;
    std::unordered_map<std::uint64_t, fs::path> files;  // the pack's pictures by hash
    // The pack's animations by hash: a folder of frames or a GIF, and those read so far.
    std::unordered_map<std::uint64_t, fs::path> animated;
    std::unordered_map<std::uint64_t, std::shared_ptr<const Animation>> animations;
    std::size_t animation_bytes = 0;
    fs::path dump;
    LevelNamer level_name = nullptr;
    std::unordered_map<std::uint64_t, fs::path> dumped;  // where each dumped texture lies, by hash
    std::set<std::uint64_t> grouped;  // first hashes of the cycles written under Animated/
    unsigned replaced = 0;
};

State& state() {
    static State s;
    return s;
}

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

// Writes or moves the texture's dump file; the lock is held.
void dump(State& s, std::uint64_t hash, const Rgba8Image& image, const Origin& origin) {
    std::error_code e;
    const fs::path folder = dump_folder(origin);
    const auto found = s.dumped.find(hash);
    if (found == s.dumped.end()) {
        fs::create_directories(s.dump / folder, e);
        const fs::path file = s.dump / folder / file_name(hash);
        if (!s.io.save(file, image)) {
            log::warn("texture {} could not be dumped", file_name(hash));
        }
        s.dumped.emplace(hash, file);
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
    s.dumped.clear();
    s.grouped.clear();
    s.replaced = 0;
    s.dump = dump;
    std::error_code e;
    if (!pack.empty() && fs::is_directory(pack, e)) {
        s.files = files_under(pack);
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
    const std::lock_guard guard(s.lock);
    if ((s.files.empty() && s.animated.empty() && s.dump.empty()) || image.pixels.empty()) {
        return false;
    }
    const std::uint64_t hash = image_hash(image);
    // The caller keeps the hash, to recognise the texture again (CycleWatch).
    if (found_out != nullptr) {
        *found_out = Replacement{hash, nullptr};
    }
    if (!s.dump.empty() && s.io.save != nullptr) {
        dump(s, hash, image, origin);
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
    if (found == s.files.end() || s.io.load == nullptr) {
        return false;
    }
    auto picture = s.io.load(found->second);
    if (!picture || picture->pixels.empty()) {
        log::warn("texture pack: {} cannot be read; the original is kept", found->second.string());
        s.files.erase(found);
        return false;
    }
    image = std::move(*picture);
    ++s.replaced;
    return true;
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
