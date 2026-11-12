// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture packs (docs/port/TEXTURE_PACKS.md): a folder of PNGs, each named by the hash of the
// texture it replaces (its size and RGBA texels as the port converted them), at any size. The
// level's textures (scene/level.cpp) and the 2D layer's uploads (texture_pool.cpp) both ask here.
// With a dump folder, every texture is written there once, by the same name, for pack makers,
// sorted into folders a player can find their way in: Interface/ (the 2D layer: HUD, menus, font),
// Planets/<planet>/ (a level's own textures) and Shared/ (those met on more than one planet).
//
// A replacement can be animated: a folder named by the hash holding frames not named by a hash
// (0001.png, 0002.png...) and an optional animation.json, or a <hash>.gif. Its frames are picked
// by the game's tick (frame_at), so they pause with the game and look the same at any frame rate.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "renderer/textures/texture.h"

namespace openrac::renderer::texture_pack {

// How an animated replacement goes on after its last frame.
enum class AnimationMode : std::uint8_t {
    Loop,      // back to the first frame (the default)
    PingPong,  // backwards to the first, then forwards again
    Once,      // stays on the last frame
};

// An animated replacement: its frames, all of one size, and how long each is shown.
struct Animation {
    std::vector<Rgba8Image> frames;
    std::vector<std::uint32_t> durations_us;  // per frame, in microseconds, never 0
    AnimationMode mode = AnimationMode::Loop;

    // The texels the frames hold, in bytes.
    std::size_t bytes() const;
};

// The game's ticks a second, the clock animations run by (program.h's tick_rate: every version).
inline constexpr std::uint32_t kTicksPerSecond = 60;

// The frames a second of a folder without animation.json, or one that names none.
inline constexpr double kDefaultFps = 10.0;

// The texels all loaded animations may hold together: one past it is refused (its first frame is
// used as a still picture), so a pack cannot fill the memory.
inline constexpr std::size_t kMaxAnimationBytes = std::size_t{256} << 20;

// The frame `animation` shows `ticks` game ticks after it started.
std::size_t frame_at(const Animation& animation, std::uint64_t ticks);

// Reading and writing pictures, from the program (the renderer itself links no image library).
struct Io {
    std::optional<Rgba8Image> (*load)(const std::filesystem::path& file) = nullptr;
    bool (*save)(const std::filesystem::path& file, const Rgba8Image& image) = nullptr;
    // A GIF's frames and their delays (mode Loop); null: GIFs are not read.
    std::optional<Animation> (*load_gif)(const std::filesystem::path& file) = nullptr;
};

// The animation in folder `folder`: its frames in order (every PNG named by digits but not by a
// hash), at the fps and mode of its animation.json. Nothing, with `why` set, when it has no frame,
// a frame cannot be read or the frames differ in size.
std::optional<Animation> load_folder(
    const std::filesystem::path& folder, const Io& io, std::string& why
);

// Where a texture was met, which picks its dump folder.
struct Origin {
    bool interface = false;   // the 2D layer's uploads (texture_pool.cpp)
    int level = -1;           // a level's own texture (scene/level.cpp): the game's level number
    std::uint64_t cycle = 0;  // a frame of a texture the game cycles: the cycle's first hash
};

// A level's name for its dump folder (the game's planet table); nullptr or an empty name gives
// "Level <number>".
using LevelNamer = std::string (*)(int level);

// The pack's folder (empty or missing: none) and the dump folder (empty: no dump). The pack's
// file names, and those already in the dump folder, are read once here.
void configure(
    const std::filesystem::path& pack,
    const std::filesystem::path& dump,
    Io io,
    LevelNamer level_name = nullptr
);

// The dump's folder, relative to the dump folder, for a texture met there: "Interface",
// "Planets/<name>" or, with no level, "Shared"; a frame of a cycle, "Animated/<first hash>". Characters no file system takes become spaces.
std::filesystem::path dump_folder(const Origin& origin);

// The 64-bit hash a texture is known by (FNV-1a over its width, height and texels).
std::uint64_t image_hash(const Rgba8Image& image);

// Its file name: 16 lowercase hex digits and ".png".
std::string file_name(std::uint64_t hash);

// What replace() found for a texture.
struct Replacement {
    std::uint64_t hash = 0;                      // the original's hash; 0 with no pack and no dump
    std::shared_ptr<const Animation> animation;  // its frames, when the replacement is animated
};

// `image` replaced by the pack's picture of it, when the pack has one (true); dumped first when
// dumping, into the folder of `origin`. A level's texture already dumped under another planet
// moves to Shared/. A pack file that cannot be read leaves the image as it was. An animated
// replacement gives its first frame here and all of them in `found` (a caller without `found`
// draws the first frame as a still picture).
bool replace(Rgba8Image& image, const Origin& origin = {}, Replacement* found = nullptr);

// Whether textures are being dumped (a dump folder was configured).
bool dumping();

// Writes the frames of a texture the game cycles in dump_folder({.cycle = first hash}): each as
// <hash>.png, the name a pack uses, and order.txt listing one cycle in the order the game shows it
// (a frame shown twice in it is listed twice). `images` matches `order`; a null one is listed but
// not written. Once per first hash.
void dump_animated(std::span<const std::uint64_t> order, std::span<const Rgba8Image* const> images);

// Finds the textures the game cycles in one place: per slot (where a texture is drawn), the
// hashes it showed, change by change. A slot whose latest changes are one sequence of at least two
// different textures twice over, each shown for kMinFrameGap to kMaxFrameGap ticks, cycles. (Two
// textures that take turns every tick are two things drawn in one slot, not an animation.)
class CycleWatch {
public:
    // The shortest and longest a cycle's frame is shown, in ticks, and the most frames a cycle
    // has.
    static constexpr std::uint64_t kMinFrameGap = 2;
    static constexpr std::uint64_t kMaxFrameGap = 120;
    static constexpr std::size_t kMaxCycle = 32;

    // `hash` drawn in `slot` on tick `frame`. Returns one cycle, in the order shown and starting at
    // its lowest hash, the first time the slot is found cycling, else nothing.
    std::optional<std::vector<std::uint64_t>> see(
        std::uint64_t slot, std::uint64_t hash, std::uint64_t frame
    );

private:
    // One slot's latest changes.
    struct Slot {
        std::vector<std::uint64_t> hashes;  // one per change, the oldest first
        std::vector<std::uint64_t> ticks;   // the tick of each change
        std::uint64_t last_frame = 0;       // when it was last drawn
        bool reported = false;
    };

    std::unordered_map<std::uint64_t, Slot> m_slots;
};

// How many textures the pack replaced so far.
unsigned replaced_count();

}  // namespace openrac::renderer::texture_pack
