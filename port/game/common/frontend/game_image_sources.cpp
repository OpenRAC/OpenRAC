// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the game handed over at its addresses (game_image_sources.h).
#include "game/common/frontend/game_image_sources.h"

#include <algorithm>
#include <iterator>

#include "openrac/guest.h"

namespace openrac::frontend {
namespace {

// A RAM address with its uncached views folded onto the bytes they show.
std::uint32_t ram_address(std::uint32_t address) {
    return openrac_guest_ram_address_for_size(address, 0x10000000u);
}

}  // namespace

void GameImageSources::keep(
    std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t size
) {
    const std::uint32_t start = ram_address(address);
    if (size == 0 || size > kMaxBytes || start >= ram.size() || ram.size() - start < size) {
        return;
    }
    const std::uint64_t end = std::uint64_t{start} + size;
    // Those it overlaps: from the one before `start`, if it reaches past it.
    auto at = m_kept.lower_bound(start);
    if (at != m_kept.begin()) {
        const auto before = std::prev(at);
        if (std::uint64_t{before->first} + before->second.bytes.size() > start) {
            at = before;
        }
    }
    while (at != m_kept.end() && at->first < end) {
        m_bytes -= at->second.bytes.size();
        at = m_kept.erase(at);
    }
    while (m_bytes + size > kMaxBytes && !m_kept.empty()) {
        const auto oldest =
            std::min_element(m_kept.begin(), m_kept.end(), [](const auto& a, const auto& b) {
                return a.second.order < b.second.order;
            });
        m_bytes -= oldest->second.bytes.size();
        m_kept.erase(oldest);
    }
    ++m_generation;
    const auto bytes = ram.subspan(start, size);
    m_kept[start] = Kept{std::vector<std::uint8_t>(bytes.begin(), bytes.end()), m_generation};
    m_bytes += size;
}

std::span<const std::uint8_t> GameImageSources::find(
    std::uint32_t address, std::size_t size
) const {
    if (m_kept.empty() || size == 0) {
        return {};
    }
    const auto found = m_kept.find(ram_address(address));
    if (found == m_kept.end() || found->second.bytes.size() < size) {
        return {};
    }
    return std::span<const std::uint8_t>(found->second.bytes).first(size);
}

void GameImageSources::clear() {
    m_kept.clear();
    m_bytes = 0;
    ++m_generation;
}

GameImageSources& game_image_sources() {
    static GameImageSources sources;
    return sources;
}

}  // namespace openrac::frontend
