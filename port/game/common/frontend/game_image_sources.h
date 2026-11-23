// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bytes the game handed over at a game address and may not keep there (openrac_game_image_keep,
// docs/port/TEXTURES.md section 2): an image it sends once and draws from then on without sending it
// again (rac3's HUD bank, sent from a decompression buffer the game reuses), as it was when sent.
// The 2D layer's image cache reads these in place of what memory holds at that address now, so the
// image is what the game sent, as on the console. A copy of the game's own data, keyed by the
// game's address; nothing about where the console's graphics chip kept it.
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace openrac::frontend {

class GameImageSources {
public:
    // Keeps `size` bytes at game address `address` of `ram` (game memory, its addresses as
    // offsets; uncached views fold onto the same bytes). Earlier ones they overlap go. Nothing
    // when they are not all in `ram`.
    void keep(std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t size);

    // The kept bytes starting at `address`, when at least `size` were kept there; else none.
    std::span<const std::uint8_t> find(std::uint32_t address, std::size_t size) const;

    // Changes each time something is kept or let go: a cache of what addresses hold must look
    // again.
    std::uint64_t generation() const { return m_generation; }

    std::size_t bytes() const { return m_bytes; }

    void clear();

    // Past this, keeping more lets the earliest go first (a game keeps a few hundred KB).
    static constexpr std::size_t kMaxBytes = std::size_t{64} << 20;

private:
    struct Kept {
        std::vector<std::uint8_t> bytes;
        std::uint64_t order = 0;
    };

    std::map<std::uint32_t, Kept> m_kept;  // by RAM address
    std::size_t m_bytes = 0;
    std::uint64_t m_generation = 0;
};

// The process's kept sources: what openrac_game_image_keep fills and the 2D layer's image cache
// reads. Game thread only.
GameImageSources& game_image_sources();

}  // namespace openrac::frontend
