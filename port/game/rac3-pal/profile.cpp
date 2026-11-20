// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Up Your Arsenal's profile for PAL (game/common/profile.h): nothing of its memory is read yet, as
// for a game with no profile, but the frame it shows is the display list it sends through VIF1
// DMA, without Ratchet & Clank's two alternating list buffers. It defines game_profile() for
// openrac-rac3-pal.

#include "game/common/profile.h"

#include <string_view>

namespace openrac::game::rac3 {
namespace {

// The game's profile: the base's answers, and the display list it sends.
class Profile final : public host::GameProfile {
public:
    bool sends_display_list() const override { return true; }

    // The buffer the frame is drawn in: block 0x1E00 (FBP 0xF0, the front end's register packet at
    // 0x00142120), past the 512-line display buffer, as PAL Going Commando's. Not where the
    // display's size puts it (0x1000): taken for an off-screen target, the frame was drawn as a
    // square and copied into the picture's middle, and what spans the whole window in a picture
    // wider than 4:3 (the clears, the menu's darkening, the background's sides) stopped at its
    // edges.
    std::uint32_t draw_buffer_block() const override { return 0x1E00; }
};

// The one instance, a constant for the program's life.
const Profile kProfile;

}  // namespace
}  // namespace openrac::game::rac3

namespace openrac::host {

const GameProfile& game_profile(std::string_view /*game*/) {
    return game::rac3::kProfile;
}

}  // namespace openrac::host
