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
// Here the port poses such a moby itself, on a copy: it loops the sequence the moby holds at its
// own speed, from the frame it holds, and writes nothing to the game's memory. The game never sees
// it. A moby is taken as asleep only when the game's test says so and its animation fields did not
// change since the last tick; any change means the game ran it, and its own pose is drawn again.
// The clocks step only on ticks where some awake moby animated (the world ran).
//
// Both games keep a moby's animation fields the same way, from `fields` (0x50 in Ratchet &
// Clank, 0x40 in Going Commando): +0 and +1 the frames of keys A and B, +2 and +3 their sequences,
// +4 the blend t, +8 the speed, +0xC the frame rate, +0x18 and +0x1C the keys. A class keeps its
// sequence count at +0x0C and its sequences at +0x48; a sequence its frame count at +0x10 and its
// frames at +0x1C (MobyAnimAdvance, rac1 func_L00_002514B8, rac2 func_002A7470).

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

namespace openrac::frontend {

// Two keys of a moby's class and the blend between them (scene::AnimKeys).
struct SleepingKeys {
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    float t = 0.0f;
    bool plain = true;
};

class SleepingMobys {
public:
    // Forgets every clock (a new level).
    void reset() { clocks_.clear(); }

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
                clock.asleep = asleep;
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

    // The keys to pose `moby` with: none for a moby that is not asleep or cannot loop.
    std::optional<SleepingKeys> keys(std::span<const std::uint8_t> ram, std::uint32_t moby) const {
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
        SleepingKeys k;
        k.a = read<std::uint32_t>(ram, frames->table + 4u * clock.frame);
        k.b = read<std::uint32_t>(ram, frames->table + 4u * next);
        k.t = clock.t;
        k.plain = next == clock.frame + 1u;
        // A tagged pointer is a packed frame only its own update unpacks (Going Commando's hero).
        if ((k.a >> 28) != 0 || (k.b >> 28) != 0 || k.a == 0 || k.b == 0) {
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
        std::uint32_t frame = 0;  // the port's own key A and blend
        float t = 0.0f;
        bool asleep = false;
    };

    struct Frames {
        std::uint32_t table = 0;  // the sequence's frame table
        std::uint32_t count = 0;
    };

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
        Frames f{entry + 0x1C, read<std::uint8_t>(ram, entry + 0x10)};
        if (f.count < 2 || !inside(ram, f.table, 4u * f.count)) {
            return std::nullopt;
        }
        return f;
    }

    // One tick of a sleeping moby's clock: t moves by its speed times its frame rate, as the
    // game's advance moves it, and wraps to the next frame (the last one to the first).
    static void step(std::span<const std::uint8_t> ram, Clock& clock) {
        float speed = 0.0f, rate = 0.0f;
        std::memcpy(&speed, clock.seen.data() + 8, 4);
        std::memcpy(&rate, clock.seen.data() + 0xC, 4);
        const float by = speed * rate;
        const auto frames = sequence(ram, clock);
        if (!frames || !(by > 0.0f && by <= 1.0f) || !std::isfinite(clock.t)) {
            clock.asleep = false;
            return;
        }
        clock.frame %= frames->count;
        clock.t = std::max(clock.t, 0.0f) + by;
        while (clock.t >= 1.0f) {
            clock.t -= 1.0f;
            clock.frame = (clock.frame + 1u) % frames->count;
        }
    }

    std::unordered_map<std::uint32_t, Clock> clocks_;
};

}  // namespace openrac::frontend
