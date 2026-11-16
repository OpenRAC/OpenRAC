// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Far mobys the game leaves asleep, animated in the picture only (Active distance: Match).
//
// The moby loop runs a moby only when it is active (Ratchet & Clank's func_0020E6B8, Going
// Commando's func_002A08A0, BuildActiveMobyChain): drawn last frame (+0x31), or its update range
// (+0x30) is 255, or the eye is within that range; and an active moby wakes its whole group (+0x21).
// The port never widens this: +0x31 is the console's (render_native.c, render.c). So past the
// console's distances a moby the window draws stands still in the pose it spawned with.
//
// Here the port poses such a moby itself, on a copy: it plays the sequence the moby holds at its
// own speed, from the frame it holds, and writes nothing to the game's memory. The game never sees
// it. A moby is taken as asleep only when the game's test says so and its animation fields did not
// change since the last tick; any change means the game ran it, and its own pose is drawn again.
// The clocks step only on ticks where some awake moby animated (the world ran).
//
// Only a loop plays. The game's advance always wraps a sequence round to its first frame; a moby
// that plays one once (coming out of the ground, opening) stops it in its own update, which does
// not run while it sleeps. The data has no loop flag, so a sequence counts as a loop only when its
// last frame flows back into its first: that step turns and moves no joint more than one and a
// half times the sequence's largest step between its own frames. Any other stays in the game's pose.
//
// Both games keep a moby's animation fields the same way, from `fields` (0x50 in Ratchet &
// Clank, 0x40 in Going Commando): +0 and +1 the frames of keys A and B, +2 and +3 their sequences,
// +4 the blend t, +8 the speed, +0xC the frame rate, +0x18 and +0x1C the keys. A class keeps its
// sequence count at +0x0C and its sequences at +0x48; a sequence its frame count at +0x10, its
// rate at +0x18 (0: each frame's own, the float the frame starts with) and its frames at +0x1C
// (MobyAnimAdvance: rac1 func_L00_002514B8, rac2 func_002A05A8).

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "scene/moby_pose.h"

namespace openrac::frontend {

class SleepingMobys {
public:
    // How far apart two frames of a class pose it (scene::key_gap; a test passes its own).
    using KeyGapFn = std::optional<scene::KeyGap> (*)(
        std::span<const std::uint8_t>, std::uint32_t, std::uint32_t, std::uint32_t
    );

    explicit SleepingMobys(KeyGapFn gap = scene::key_gap) : gap_(gap) {}

    // Forgets every clock and every sequence (a new level).
    void reset() {
        clocks_.clear();
        loops_.clear();
    }

    // One tick: `mobys` the live mobys' addresses, `eye` the camera's eye (game units), `fields`
    // where the animation fields start, `world_ran` whether the world ran this tick.
    void tick(
        std::span<const std::uint8_t> ram,
        const std::vector<std::uint32_t>& mobys,
        const std::array<float, 3>& eye,
        std::uint32_t fields,
        bool world_ran
    ) {
        // The groups an active moby wakes.
        std::unordered_set<int> awake_groups;
        for (const std::uint32_t m : mobys) {
            if (inside(ram, m, 0x100) && runs_itself(ram.data() + m, eye)) {
                const auto group = static_cast<std::int8_t>(ram[m + 0x21]);
                if (group >= 0) {
                    awake_groups.insert(group);
                }
            }
        }
        std::unordered_map<std::uint32_t, Clock> next;
        next.reserve(mobys.size());
        bool world_moved = false;
        for (const std::uint32_t m : mobys) {
            if (!inside(ram, m, 0x100) || !inside(ram, m + fields, 0x20)) {
                continue;
            }
            const std::uint8_t* b = ram.data() + m;
            Clock clock;
            std::memcpy(clock.seen.data(), b + fields, clock.seen.size());
            clock.cls = read<std::uint32_t>(ram, m + 0x24);
            clock.frame = clock.seen[0];
            clock.t = read<float>(ram, m + fields + 4);
            clock.rate = read<float>(ram, m + fields + 0xC);
            const auto group = static_cast<std::int8_t>(b[0x21]);
            const std::uint16_t mode = read<std::uint16_t>(ram, m + 0x34);
            // Mode 2: not run by the moby loop (the hero, his items, figures); 0x40: animation
            // not advanced. Their poses are someone else's.
            const bool asleep = (mode & 0x42) == 0 && !runs_itself(b, eye)
                                && !(group >= 0 && awake_groups.contains(group));
            const auto last = clocks_.find(m);
            if (last != clocks_.end() && last->second.cls == clock.cls
                && last->second.seen == clock.seen) {
                clock.frame = last->second.frame;
                clock.t = last->second.t;
                clock.rate = last->second.rate;
                clock.playable = last->second.playable;
                clock.asleep = asleep && clock.playable && loops(ram, clock);
            } else if (last != clocks_.end() && !asleep) {
                world_moved = true;
            }
            next.emplace(m, clock);
        }
        if (world_ran && world_moved) {
            for (auto& entry : next) {
                if (entry.second.asleep) {
                    step(ram, entry.second);
                }
            }
        }
        clocks_ = std::move(next);
    }

    // The keys to pose `moby` with: none for a moby that is not asleep or does not loop.
    std::optional<scene::AnimKeys> keys(
        std::span<const std::uint8_t> ram, std::uint32_t moby
    ) const {
        const auto it = clocks_.find(moby);
        if (it == clocks_.end() || !it->second.asleep) {
            return std::nullopt;
        }
        const Clock& clock = it->second;
        const auto frames = sequence(ram, clock);
        if (!frames || clock.frame >= frames->count) {
            return std::nullopt;
        }
        const std::uint32_t next = (clock.frame + 1u) % frames->count;
        scene::AnimKeys k;
        k.a = frame_at(ram, *frames, clock.frame);
        k.b = frame_at(ram, *frames, next);
        k.t = clock.t;
        k.plain = next == clock.frame + 1u;
        if (k.a == 0 || k.b == 0) {
            return std::nullopt;
        }
        return k;
    }

    // Whether the moby loop runs `moby` this tick by its own test, before groups: drawn last frame
    // (+0x31), its range (+0x30) 255, or the eye within that range of its position (+0x10).
    static bool runs_itself(const std::uint8_t* moby, const std::array<float, 3>& eye) {
        const std::uint8_t range = moby[0x30];
        if (moby[0x31] != 0 || range == 0xFF) {
            return true;
        }
        float p[3];
        std::memcpy(p, moby + 0x10, sizeof p);
        const float dx = p[0] - eye[0], dy = p[1] - eye[1], dz = p[2] - eye[2];
        const float r = static_cast<float>(range);
        return !(r * r - dx * dx - dy * dy - dz * dz < 0.0f);
    }

private:
    struct Clock {
        std::array<std::uint8_t, 0x20> seen{};  // the animation fields as last read
        std::uint32_t cls = 0;
        std::uint32_t frame = 0;  // the port's own key A, blend and frame rate
        float t = 0.0f;
        float rate = 0.0f;
        bool asleep = false;
        bool playable = true;  // false once its clock could not step (kept until the game moves it)
    };

    struct Frames {
        std::uint32_t table = 0;  // the sequence's frame table
        std::uint32_t count = 0;
        float rate = 0.0f;  // the sequence's frame rate; 0: each frame's own
    };

    // A loop's step back to its first frame against its largest own step (see the top), and what
    // any step may add to that: a slight turn (radians) or move (packed model units).
    static constexpr float kLoopStep = 1.5f;
    static constexpr float kLoopTurn = 0.05f;
    static constexpr float kLoopMove = 16.0f;

    static bool inside(
        std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t bytes
    ) {
        return address != 0 && address + bytes <= ram.size();
    }

    template <typename T>
    static T read(std::span<const std::uint8_t> ram, std::uint32_t address) {
        T value{};
        address &= 0x1FFFFFFFu;
        if (address + sizeof(T) <= ram.size()) {
            std::memcpy(&value, ram.data() + address, sizeof(T));
        }
        return value;
    }

    // The sequence the moby holds, when it is one sequence (no blend between two) of two or more
    // frames.
    static std::optional<Frames> sequence(std::span<const std::uint8_t> ram, const Clock& clock) {
        const std::uint8_t seq = clock.seen[2];
        if (seq != clock.seen[3] || clock.cls == 0) {
            return std::nullopt;
        }
        const std::uint32_t cls = clock.cls & 0x1FFFFFFFu;
        if (!inside(ram, cls, 0x4C) || seq >= read<std::uint8_t>(ram, cls + 0x0C)) {
            return std::nullopt;
        }
        const std::uint32_t entry = read<std::uint32_t>(ram, cls + 0x48 + 4u * seq) & 0x1FFFFFFFu;
        if (!inside(ram, entry, 0x20)) {
            return std::nullopt;
        }
        Frames f{
            entry + 0x1C, read<std::uint8_t>(ram, entry + 0x10), read<float>(ram, entry + 0x18)
        };
        if (f.count < 2 || !inside(ram, f.table, 4u * f.count)) {
            return std::nullopt;
        }
        return f;
    }

    // Frame `i` of a sequence; 0 for a tagged pointer, a packed frame only its own update unpacks
    // (Going Commando's hero).
    static std::uint32_t frame_at(
        std::span<const std::uint8_t> ram, const Frames& frames, std::uint32_t i
    ) {
        const auto frame = read<std::uint32_t>(ram, frames.table + 4u * i);
        return (frame >> 28) == 0 ? frame : 0;
    }

    // Whether the sequence the moby holds is a loop (see the top): three frames or more (two
    // would only swing between them), every one readable, and the last flowing back into the
    // first. Kept per class and sequence until reset().
    bool loops(std::span<const std::uint8_t> ram, const Clock& clock) {
        const auto frames = sequence(ram, clock);
        if (!frames) {
            return false;
        }
        const std::uint64_t key = (std::uint64_t{clock.cls} << 8) | clock.seen[2];
        if (const auto found = loops_.find(key); found != loops_.end()) {
            return found->second;
        }
        const auto gap = [&](std::uint32_t from, std::uint32_t to) -> std::optional<scene::KeyGap> {
            const std::uint32_t a = frame_at(ram, *frames, from);
            const std::uint32_t b = frame_at(ram, *frames, to);
            if (a == 0 || b == 0) {
                return std::nullopt;
            }
            return gap_(ram, clock.cls, a, b);
        };
        bool loop = frames->count >= 3;
        scene::KeyGap largest;
        for (std::uint32_t i = 0; loop && i + 1 < frames->count; ++i) {
            const auto own = gap(i, i + 1);
            loop = own.has_value();
            if (own) {
                largest.turn = std::max(largest.turn, own->turn);
                largest.move = std::max(largest.move, own->move);
            }
        }
        if (loop) {
            const auto wrap = gap(frames->count - 1, 0);
            loop = wrap && wrap->turn <= kLoopStep * largest.turn + kLoopTurn
                   && wrap->move <= kLoopStep * largest.move + kLoopMove;
        }
        loops_.emplace(key, loop);
        return loop;
    }

    // One tick of a sleeping moby's clock, as the game's advance moves one sequence forward: t
    // moves by speed times rate and from 1 - 1/256 to 1 + 1/256 is 1; past 1 the next frame
    // begins (the last one's next is the first) and t is carried over into its rate, the
    // sequence's or else the frame's own. A clock that would not move forward stops.
    static void step(std::span<const std::uint8_t> ram, Clock& clock) {
        float speed = 0.0f;
        std::memcpy(&speed, clock.seen.data() + 8, 4);
        const auto frames = sequence(ram, clock);
        const float by = speed * clock.rate;
        if (!frames || !(speed > 0.0f) || !(clock.rate > 0.0f)
            || !(by <= static_cast<float>(frames->count)) || !std::isfinite(clock.t)
            || clock.t < 0.0f) {
            clock.asleep = clock.playable = false;
            return;
        }
        clock.frame %= frames->count;
        float t = clock.t + by;
        if (t >= 0.99609375f && t <= 1.00390625f) {  // 0x3F7F0000 and 0x3F808000
            t = 1.0f;
        } else if (!(t > 1.0f)) {
            clock.t = t;
            return;
        }
        // One frame on a pass. A rate of 1/256 or more ends them in time; a smaller one stops.
        for (std::uint32_t pass = 0; pass <= 256u * frames->count; ++pass) {
            clock.frame = (clock.frame + 1u) % frames->count;
            float rate = frames->rate;
            if (rate == 0.0f) {
                const std::uint32_t frame = frame_at(ram, *frames, clock.frame);
                rate = frame != 0 ? read<float>(ram, frame) : 0.0f;
            }
            if (!(rate > 0.0f) || !std::isfinite(rate)) {
                break;
            }
            t = (t - 1.0f) / clock.rate * rate;
            clock.rate = rate;
            if (!(t > 1.0f)) {
                clock.t = t;
                return;
            }
        }
        clock.asleep = clock.playable = false;
    }

    KeyGapFn gap_;
    std::unordered_map<std::uint32_t, Clock> clocks_;
    std::unordered_map<std::uint64_t, bool> loops_;
};

}  // namespace openrac::frontend
