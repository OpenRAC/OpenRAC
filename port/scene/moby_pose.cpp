// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/moby_pose.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace openrac::scene {

namespace rac1 = assets::rac1;

namespace {

// More joints than the scratchpad holds (0x6f) is not a class.
constexpr std::size_t kMaxJoints = 0x70;
// The most nodes a moby's joint-modifier or pose-layer list is walked for.
constexpr std::size_t kMaxListSteps = 64;

// set_extra_joint_modifiers: the moby and its modifiers.
std::mutex g_extra_lock;
// Each moby's own joint modifiers of the port's (set_extra_joint_modifiers), by its address.
std::unordered_map<std::uint32_t, std::vector<rac1::JointModifier>> g_extra_mods;

std::uint32_t ram_address(std::uint32_t address, std::size_t size) {
    constexpr std::uint32_t uncached = 0x20000000u;
    constexpr std::uint32_t uncached_accel = 0x30000000u;
    if (address < size) {
        return address;
    }
    if (address >= uncached && address - uncached < size) {
        return address - uncached;
    }
    if (address >= uncached_accel && address - uncached_accel < size) {
        return address - uncached_accel;
    }
    return address & 0x01FFFFFFu;
}

// The port's own game memory (set_port_memory): where it starts and its bytes.
std::uint32_t g_port_base = 0;
std::span<const std::uint8_t> g_port;

// The class region past RAM (set_moby_class_region): imported moby classes, read as RAM.
std::span<const std::uint8_t> g_region;
std::uint32_t g_region_at = 0;

// Where the `bytes` bytes at game address `address` are: the memory that holds them (game RAM or
// the port's own) and their offset in it; none when they are in neither. A moby's joint
// modifiers and pose layers live in its pvars, and a mod's copy of an enemy whose pvars are larger
// than a pool slot's keeps them in the port's memory (port/game/rac2/host/enemy_copies.c): read
// as RAM, its modifiers were the executable's code, and its joints went to infinity.
std::optional<std::pair<std::span<const std::uint8_t>, std::uint32_t>> locate(
    std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t bytes
) {
    if (!g_region.empty() && address >= g_region_at && address - g_region_at < g_region.size()) {
        const std::uint32_t offset = address - g_region_at;
        if (bytes > g_region.size() - offset) {
            return std::nullopt;
        }
        return std::pair{g_region, offset};
    }
    if (!g_port.empty() && address >= g_port_base && address - g_port_base < g_port.size()) {
        const std::uint32_t offset = address - g_port_base;
        if (bytes > g_port.size() - offset) {
            return std::nullopt;
        }
        return std::pair{g_port, offset};
    }
    address = ram_address(address, ram.size());
    if (address > ram.size() || bytes > ram.size() - address) {
        return std::nullopt;
    }
    return std::pair{ram, address};
}

template <typename T>
T at(std::span<const std::uint8_t> ram, std::uint32_t address) {
    T value{};
    if (const auto where = locate(ram, address, sizeof(T))) {
        std::memcpy(&value, where->first.data() + where->second, sizeof(T));
    }
    return value;
}

// Whether the bytes at `address` can be read: no null pointer, nor one that is RAM's first byte.
bool inside(std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t bytes) {
    const auto where = locate(ram, address, bytes);
    return address != 0 && where && (where->first.data() != ram.data() || where->second != 0);
}

// A key where the moby points; none when there is no pointer or what is there is not a
// frame (a blend's snapshot slot the port never filled is all zero: no quaternions).
std::optional<rac1::MobyFrame> key_at(
    std::span<const std::uint8_t> ram, std::uint32_t address, std::size_t joints
) {
    if (!inside(ram, address, 0x10)) {
        return std::nullopt;
    }
    const auto where = locate(ram, address, 0x10);
    try {
        rac1::MobyFrame f = rac1::parse_frame(assets::ByteView(where->first), where->second);
        if (f.header.quat_bytes == 0 || f.header.quat_bytes > 8 * kMaxJoints || joints == 0) {
            return std::nullopt;
        }
        return f;
    } catch (const assets::AssetError&) {
        return std::nullopt;
    }
}

// Where a moby's animation fields start (set_moby_anim_fields).
std::uint32_t g_anim_fields = 0x50;
// Whether the pose layers keep records (set_moby_layer_records).
bool g_layer_records = false;

}  // namespace

namespace {

std::vector<rac1::JointMatrix> evaluate_moby(
    std::span<const std::uint8_t> ram, std::uint32_t moby, bool bind, const AnimKeys* keys
);

}  // namespace

void set_moby_anim_fields(std::uint32_t first) {
    g_anim_fields = first;
}

void set_moby_layer_records(bool records) {
    g_layer_records = records;
}

void set_moby_class_region(std::span<const std::uint8_t> bytes, std::uint32_t address) {
    g_region = bytes;
    g_region_at = address;
}

void set_port_memory(std::uint32_t base, std::span<const std::uint8_t> memory) {
    g_port_base = base;
    g_port = memory;
}

std::vector<rac1::JointMatrix> moby_palette(std::span<const std::uint8_t> ram, std::uint32_t moby) {
    return evaluate_moby(ram, moby, true, nullptr);
}

std::vector<rac1::JointMatrix> moby_palette(
    std::span<const std::uint8_t> ram, std::uint32_t moby, const AnimKeys& keys
) {
    return evaluate_moby(ram, moby, true, &keys);
}

std::vector<rac1::JointMatrix> moby_pose_matrices(std::span<const std::uint8_t> ram, std::uint32_t moby) {
    return evaluate_moby(ram, moby, false, nullptr);
}

std::vector<rac1::JointMatrix> moby_pose_matrices(
    std::span<const std::uint8_t> ram, std::uint32_t moby, const AnimKeys& keys
) {
    return evaluate_moby(ram, moby, false, &keys);
}

std::optional<KeyGap> key_gap(
    std::span<const std::uint8_t> ram, std::uint32_t cls, std::uint32_t a, std::uint32_t b
) {
    if (!inside(ram, cls, 0x48)) {
        return std::nullopt;
    }
    const std::size_t joints = at<std::uint8_t>(ram, cls + 0x08);
    if (joints == 0 || joints > kMaxJoints) {
        return std::nullopt;
    }
    const auto ka = key_at(ram, a, joints);
    const auto kb = key_at(ram, b, joints);
    if (!ka || !kb) {
        return std::nullopt;
    }
    KeyGap gap;
    for (std::size_t j = 0; j < joints; ++j) {
        const auto qa = ka->quat_at(j);
        const auto qb = kb->quat_at(j);
        double dot = 0.0, na = 0.0, nb = 0.0;
        for (std::size_t k = 0; k < 4; ++k) {
            dot += double(qa[k]) * qb[k];
            na += double(qa[k]) * qa[k];
            nb += double(qb[k]) * qb[k];
        }
        if (na == 0.0 || nb == 0.0) {
            if (na != nb) {
                gap.turn = std::numeric_limits<float>::infinity();
            }
            continue;
        }
        // q and -q are one turn.
        const double cos_half = std::min(1.0, std::abs(dot) / std::sqrt(na * nb));
        gap.turn = std::max(gap.turn, static_cast<float>(2.0 * std::acos(cos_half)));
    }
    // A joint's translation in a frame: its record's, else its rest translation (common_trans),
    // as the evaluator sets it.
    const auto common_trans = at<std::uint32_t>(ram, cls + 0x18);
    if (!inside(ram, common_trans, 0x10 * joints)) {
        return std::nullopt;
    }
    std::vector<std::array<float, 3>> ta(joints), tb(joints);
    std::vector<bool> moved(joints, false);
    for (std::size_t j = 0; j < joints; ++j) {
        for (std::size_t k = 0; k < 3; ++k) {
            ta[j][k] = tb[j][k] =
                at<float>(ram, common_trans + static_cast<std::uint32_t>(0x10 * j + 4 * k));
        }
    }
    const auto apply = [&](const rac1::MobyFrame& f, std::vector<std::array<float, 3>>& out) {
        for (const rac1::MobyTransRecord& r : f.trans) {
            const auto j = static_cast<std::int8_t>(r.joint);
            if (j >= 0 && static_cast<std::size_t>(j) < joints) {
                for (std::size_t k = 0; k < 3; ++k) {
                    out[j][k] = static_cast<float>(r.trans[k]);
                }
                moved[j] = true;
            }
        }
    };
    apply(*ka, ta);
    apply(*kb, tb);
    for (std::size_t j = 0; j < joints; ++j) {
        for (std::size_t k = 0; moved[j] && k < 3; ++k) {
            gap.move = std::max(gap.move, std::fabs(ta[j][k] - tb[j][k]));
        }
    }
    return gap;
}

namespace {

std::vector<rac1::JointMatrix> evaluate_moby(
    std::span<const std::uint8_t> ram, std::uint32_t moby, bool bind, const AnimKeys* keys
) {
    const auto cls = at<std::uint32_t>(ram, moby + 0x24);
    if (!inside(ram, cls, 0x48)) {
        return {};
    }
    const std::size_t joints = at<std::uint8_t>(ram, cls + 0x08);
    const auto skeleton = at<std::uint32_t>(ram, cls + 0x14);
    const auto common_trans = at<std::uint32_t>(ram, cls + 0x18);
    if (joints == 0 || joints > kMaxJoints || !inside(ram, skeleton, 0x40 * joints)
        || !inside(ram, common_trans, 0x10 * joints)) {
        return {};
    }

    // The keys and the blend (moby +0x50..+0x6c, as Ratchet & Clank keeps them).
    const std::uint32_t fields = moby + g_anim_fields;
    const auto frame_a = at<std::uint8_t>(ram, fields);
    const auto frame_b = at<std::uint8_t>(ram, fields + 1);
    const auto seq_a = at<std::uint8_t>(ram, fields + 2);
    const auto seq_b = at<std::uint8_t>(ram, fields + 3);
    float t = at<float>(ram, fields + 4);
    std::optional<rac1::MobyFrame> a =
        key_at(ram, keys ? keys->a : at<std::uint32_t>(ram, fields + 0x18), joints);
    std::optional<rac1::MobyFrame> b =
        key_at(ram, keys ? keys->b : at<std::uint32_t>(ram, fields + 0x1C), joints);
    bool plain = keys ? keys->plain : seq_a == seq_b && frame_b == frame_a + 1;
    if (keys) {
        t = keys->t;
    }
    if (!a && b) {
        // Key A missing (a snapshot the port has not written): the target key alone.
        a = std::move(b);
        b.reset();
        t = 0.0f;
    } else if (a && !b) {
        t = 0.0f;
    }
    if (!a) {
        return {};
    }
    if (!(t > 0.0f && t <= 1.0f)) {
        t = 0.0f;
    }

    // The class as MobyAnimEval reads it: the inverse bind matrices, the rest translations and
    // the parent words (0x70000000 + 0x40 * parent, the scratchpad record of the parent).
    rac1::MobyAnimClass anim;
    anim.joint_count = joints;
    anim.skeleton.resize(joints);
    anim.rest.resize(joints);
    anim.parent_word.resize(joints);
    for (std::size_t j = 0; j < joints; ++j) {
        for (std::size_t r = 0; r < 4; ++r) {
            for (std::size_t c = 0; c < 4; ++c) {
                anim.skeleton[j][r][c] = bind ? at<float>(
                    ram, skeleton + static_cast<std::uint32_t>(0x40 * j + 0x10 * r + 4 * c)
                ) : rac1::kIdentityJoint[r][c];
            }
        }
        const auto rec = common_trans + static_cast<std::uint32_t>(0x10 * j);
        anim.rest[j] = {at<float>(ram, rec), at<float>(ram, rec + 4), at<float>(ram, rec + 8)};
        anim.parent_word[j] = at<std::uint32_t>(ram, rec + 0xC);
    }
    // The joint-modifier list (moby +0x64, each node's next at +8): what spins a vendor's
    // hologram and its logo's text, turns an NPC's head. Its order is the game's, head first.
    // The walk is bounded by its steps as well: a list that loops back (a node linked twice) is
    // drawn with what it gave, never walked for ever.
    std::vector<rac1::JointModifier> mods;
    std::size_t steps = 0;
    for (std::uint32_t node = at<std::uint32_t>(ram, fields + 0x14);
         inside(ram, node, 0x40) && mods.size() < 32 && steps++ < kMaxListSteps;
         node = at<std::uint32_t>(ram, node + 8)) {
        const std::uint32_t record = at<std::uint32_t>(ram, node + 4);
        if (record < 0x70000000u || ((record - 0x70000000u) & 0x3F) != 0) {
            continue;
        }
        rac1::JointModifier m;
        m.joint = static_cast<std::uint8_t>((record - 0x70000000u) / 0x40);
        m.mode = at<std::uint8_t>(ram, node + 3);
        m.weight = at<float>(ram, node + 0xC);
        for (std::size_t k = 0; k < 4; ++k) {
            m.quat[k] = at<float>(ram, node + 0x10 + static_cast<std::uint32_t>(4 * k));
        }
        for (std::size_t k = 0; k < 3; ++k) {
            m.scale[k] = at<float>(ram, node + 0x20 + static_cast<std::uint32_t>(4 * k));
            m.trans[k] = at<float>(ram, node + 0x30 + static_cast<std::uint32_t>(4 * k));
        }
        mods.push_back(m);
    }
    // The pose layers (moby +0x60, each node's next at +0x1C, PAL func_L00_00250060 links them in
    // front): Ratchet's weapon arm, its keys at +0x38 / +0x3C ready in his joint layout. Without them
    // his arms kept the unarmed idle and a gun hung from his lowered hand.
    std::vector<rac1::MobyFrame> layer_keys;
    layer_keys.reserve(16);
    std::vector<rac1::PoseLayer> layers;
    steps = 0;
    for (std::uint32_t node = at<std::uint32_t>(ram, fields + 0x10);
         inside(ram, node, 0x40) && layers.size() < 6 && steps++ < kMaxListSteps;
         node = at<std::uint32_t>(ram, node + 0x1C)) {
        const auto count = at<std::uint16_t>(ram, node + 2);
        const auto list = at<std::uint32_t>(ram, node + 0x0C);
        if (count == 0 || count > kMaxJoints || !inside(ram, list, count)) {
            continue;
        }
        rac1::PoseLayer l;
        const auto joints_at = locate(ram, list, count);
        l.joints.assign(
            joints_at->first.begin() + joints_at->second,
            joints_at->first.begin() + joints_at->second + count
        );
        l.weight = at<float>(ram, node + 0x08);
        if (g_layer_records) {
            const auto buffer = at<std::uint32_t>(ram, node + 0x10);
            if (!inside(ram, buffer, 0x30u * count)) {
                continue;
            }
            l.records.resize(count);
            for (std::uint32_t i = 0; i < count; ++i) {
                rac1::PoseRecord& r = l.records[i];
                const std::uint32_t record = buffer + 0x30 * i;
                for (std::uint32_t k = 0; k < 4; ++k) {
                    r.quat[k] = at<std::uint32_t>(ram, record + 4 * k);
                    r.scale[k] = at<std::uint32_t>(ram, record + 0x10 + 4 * k);
                    r.trans[k] = at<std::uint32_t>(ram, record + 0x20 + 4 * k);
                }
                r.joint = l.joints[i];
            }
            layers.push_back(std::move(l));
            continue;
        }
        l.t = at<float>(ram, node + 0x24);
        if (!(l.t > 0.0f && l.t <= 1.0f)) {
            l.t = 0.0f;
        }
        l.plain = at<std::uint8_t>(ram, node + 0x22) == at<std::uint8_t>(ram, node + 0x23)
                  && at<std::uint8_t>(ram, node + 0x21) == at<std::uint8_t>(ram, node + 0x20) + 1;
        auto ka = key_at(ram, at<std::uint32_t>(ram, node + 0x38), joints);
        auto kb = l.t != 0.0f ? key_at(ram, at<std::uint32_t>(ram, node + 0x3C), joints) : std::nullopt;
        if (!ka || (l.t != 0.0f && !kb)) {
            continue;
        }
        layer_keys.push_back(std::move(*ka));
        l.a = &layer_keys.back();
        if (kb) {
            layer_keys.push_back(std::move(*kb));
            l.b = &layer_keys.back();
        }
        layers.push_back(std::move(l));
    }
    {
        const std::lock_guard<std::mutex> lock(g_extra_lock);
        const auto extra = g_extra_mods.find(moby);
        if (extra != g_extra_mods.end()) {
            mods.insert(mods.end(), extra->second.begin(), extra->second.end());
        }
    }
    std::vector<rac1::JointMatrix> pose =
        rac1::evaluate_keys(anim, &*a, b ? &*b : nullptr, t, plain, mods, layers);
    // A joint the console would have sent to infinity or NaN (whatever its data held): the moby is
    // drawn as stored rather than as triangles across the screen.
    for (const rac1::JointMatrix& joint : pose) {
        for (const auto& row : joint) {
            for (const float x : row) {
                if (!std::isfinite(x)) {
                    return {};
                }
            }
        }
    }
    return pose;
}

}  // namespace

void set_extra_joint_modifiers(std::uint32_t moby, std::vector<rac1::JointModifier> mods) {
    const std::lock_guard<std::mutex> lock(g_extra_lock);
    if (moby == 0) {
        g_extra_mods.clear();
    } else if (mods.empty()) {
        g_extra_mods.erase(moby);
    } else {
        g_extra_mods[moby] = std::move(mods);
    }
}

std::vector<rac1::JointModifier> extra_joint_modifiers(std::uint32_t moby) {
    const std::lock_guard<std::mutex> lock(g_extra_lock);
    const auto extra = g_extra_mods.find(moby);
    if (moby == 0 || extra == g_extra_mods.end()) {
        return {};
    }
    return extra->second;
}

}  // namespace openrac::scene
