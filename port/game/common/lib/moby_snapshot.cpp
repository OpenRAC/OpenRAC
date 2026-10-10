// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The blend snapshot of Ratchet & Clank's mobys (openrac_game_moby_snapshot, game_host.h): a
// moby's current pose re-encoded as a keyframe in a blend slot, as the hand-written VU0 routine
// the game calls on a sequence change leaves it (assets/geometry/moby_animation.h,
// encode_snapshot). The moby's key A then points at it while it blends to its next sequence; the
// renderer's pose evaluator, like the game's, reads it there.

#include <cstring>
#include <optional>

#include "assets/geometry/moby_animation.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

namespace rac1 = openrac::assets::rac1;

namespace {

constexpr std::uint32_t kMaxJoints = 0x70;  // the scratchpad's records

template <typename T>
T at(gaddr address) {
    T value{};
    std::memcpy(&value, G(address & 0x01FFFFFFu), sizeof(T));
    return value;
}

std::optional<rac1::MobyFrame> key_at(gaddr address) {
    address &= 0x01FFFFFFu;
    if (address == 0 || address + 0x10 > GUEST_RAM_SIZE) {
        return std::nullopt;
    }
    try {
        const openrac::assets::ByteView ram(static_cast<const std::uint8_t*>(G(0)), GUEST_RAM_SIZE);
        rac1::MobyFrame f = rac1::parse_frame(ram, address);
        if (f.header.quat_bytes == 0 || f.header.quat_bytes > 8 * kMaxJoints) {
            return std::nullopt;
        }
        return f;
    } catch (const openrac::assets::AssetError&) {
        return std::nullopt;
    }
}

}  // namespace

extern "C" int openrac_game_moby_snapshot(gaddr moby, gaddr dst) {
    const auto cls = at<std::uint32_t>(moby + 0x24);
    if (cls == 0) {
        return 0;
    }
    const std::size_t joints = at<std::uint8_t>(cls + 0x08);
    const auto common_trans = at<std::uint32_t>(cls + 0x18);
    if (joints == 0 || joints > kMaxJoints || common_trans == 0) {
        return 0;
    }
    rac1::MobyAnimClass anim;
    anim.joint_count = joints;
    anim.rest.resize(joints);
    anim.parent_word.resize(joints);
    for (std::size_t j = 0; j < joints; ++j) {
        const gaddr rec = common_trans + static_cast<gaddr>(0x10 * j);
        anim.rest[j] = {at<float>(rec), at<float>(rec + 4), at<float>(rec + 8)};
        anim.parent_word[j] = at<std::uint32_t>(rec + 0xC);
    }
    const auto frame_a = at<std::uint8_t>(moby + 0x50);
    const auto frame_b = at<std::uint8_t>(moby + 0x51);
    const auto seq_a = at<std::uint8_t>(moby + 0x52);
    const auto seq_b = at<std::uint8_t>(moby + 0x53);
    const float t = at<float>(moby + 0x54);
    std::optional<rac1::MobyFrame> a = key_at(at<std::uint32_t>(moby + 0x68));
    std::optional<rac1::MobyFrame> b = key_at(at<std::uint32_t>(moby + 0x6C));
    const bool plain = seq_a == seq_b && frame_b == frame_a + 1;
    const std::vector<std::uint8_t> frame =
        rac1::encode_snapshot(anim, a ? &*a : nullptr, b ? &*b : nullptr, b ? t : 0.0f, plain);
    if (frame.empty() || frame.size() > 0x800) {
        return 0;
    }
    std::memcpy(G(dst & 0x01FFFFFFu), frame.data(), frame.size());
    return 1;
}
