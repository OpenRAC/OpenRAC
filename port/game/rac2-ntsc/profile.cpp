// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's profile for NTSC-U (game/common/profile.h): the camera and the live mobys for
// the debug overlay and the scripted walk, read from the words of host/rac2_addresses.h through the
// loaded program's copies, and the frame it shows, the display list it sends through VIF1 DMA,
// without Ratchet & Clank's two alternating list buffers. It defines game_profile() for
// openrac-rac2-ntsc.

#include "game/common/profile.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>

#include "game/rac2-ntsc/host/rac2_addresses.h"
#include "openrac/guest.h"

namespace openrac::game::rac2 {
namespace {

constexpr std::uint32_t kMobySize = 0x100;

template <typename T>
T at(std::span<const std::uint8_t> ram, std::uint32_t address) {
    T value{};
    address = openrac_guest_ram_address_for_size(address, static_cast<std::uint32_t>(ram.size()));

    if (address + sizeof(T) <= ram.size()) {
        std::memcpy(&value, ram.data() + address, sizeof(T));
    }

    return value;
}

renderer::Vec3 vec3(std::span<const std::uint8_t> ram, std::uint32_t address) {
    return {at<float>(ram, address), at<float>(ram, address + 4), at<float>(ram, address + 8)};
}

// The moby's place: Ratchet & Clank's fields (game/rac1/game_state.cpp), but the class at +0xAA.
std::optional<host::LiveMoby> live_moby(std::span<const std::uint8_t> ram, std::uint32_t m) {
    if (m == 0 || m + kMobySize > ram.size()) {
        return std::nullopt;
    }

    const renderer::Vec3 position = vec3(ram, m + 0x10);

    if (!std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2])) {
        return std::nullopt;
    }

    host::LiveMoby live;
    live.address = m;
    live.class_id = at<std::int16_t>(ram, m + 0xAA);
    live.occlusion = at<std::uint16_t>(ram, m + 0x36);
    live.matrix = renderer::translation(position);
    return live;
}

// The game's profile: the camera and the mobys, and the display list it sends.
class Profile final : public host::GameProfile {
public:
    bool sends_display_list() const override { return true; }

    host::GameState read_state(std::span<const std::uint8_t> ram, int level) const override {
        host::GameState s;

        if (level < 0) {
            return s;
        }

        s.camera_position = vec3(ram, OPENRAC_DATA(RAC2_g_cameraPos));
        const std::uint32_t rows = OPENRAC_DATA(RAC2_g_cameraMatrix);
        s.forward = vec3(ram, rows);
        s.left = vec3(ram, rows + 0x10);
        s.up = vec3(ram, rows + 0x20);
        const auto first = openrac_guest_ram_address_for_size(
            at<std::uint32_t>(ram, OPENRAC_DATA(RAC2_g_mobyTableBase)),
            static_cast<std::uint32_t>(ram.size())
        );
        const auto end = openrac_guest_ram_address_for_size(
            at<std::uint32_t>(ram, OPENRAC_DATA(RAC2_g_mobyTableEnd)),
            static_cast<std::uint32_t>(ram.size())
        );

        if (first == 0 || end <= first || end > ram.size() || (end - first) % kMobySize != 0) {
            return s;
        }

        for (std::uint32_t m = first; m < end; m += kMobySize) {
            // A state with its sign bit set is no moby any more, as in Ratchet & Clank.
            if (at<std::uint8_t>(ram, m + 0x20) < 0x80) {
                if (const auto live = live_moby(ram, m)) {
                    s.mobys.push_back(*live);
                }
            }
        }

        return s;
    }

    std::optional<host::LiveMoby> read_moby(
        std::span<const std::uint8_t> ram, std::uint32_t moby
    ) const override {
        return live_moby(ram, moby);
    }
};

// The one instance, a constant for the program's life.
const Profile kProfile;

}  // namespace
}  // namespace openrac::game::rac2

namespace openrac::host {

const GameProfile& game_profile(std::string_view /*game*/) {
    return game::rac2::kProfile;
}

}  // namespace openrac::host
