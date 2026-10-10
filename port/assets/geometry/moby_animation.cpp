// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "assets/geometry/moby_animation.h"

#include <cstring>

namespace openrac::assets::rac1 {

namespace {

using ps2::V4;

constexpr u32 kOne = ps2::kOne;
constexpr u32 kNegOne = ps2::kNegOne;
// The scratchpad the common_trans parent words point into (0x70000000 +
// 0x40 * parent), and its joint records (at most 0x70 joints).
constexpr u32 kScratchpad = 0x7000'0000;
constexpr std::size_t kRecords = 0x70;
constexpr V4 kUnitScale = {kOne, kOne, kOne, 0};
constexpr std::array<V4, 4> kIdentity4 = {
    V4{kOne, 0, 0, 0},
    V4{0, kOne, 0, 0},
    V4{0, 0, kOne, 0},
    V4{0, 0, 0, kOne},
};

std::optional<std::size_t> scratchpad_record(u32 word) {
    if (word < kScratchpad || (word - kScratchpad) % 0x40 != 0) {
        return std::nullopt;
    }
    const std::size_t i = (word - kScratchpad) / 0x40;
    return i < kRecords ? std::optional<std::size_t>(i) : std::nullopt;
}

MobyFrame read_frame(ByteView b, std::size_t at) {
    MobyFrame f;
    f.header = b.read<MobyFrameHeader>(at, "moby frame header");
    const std::size_t qb = f.header.quat_bytes;
    const std::size_t to = f.header.trans_offset;
    const std::size_t tc = f.header.trans_count;
    if (qb % 8 != 0 || to < qb || (to - qb) % 8 != 0) {
        fail(
            "moby frame at {:#x}: quat bytes {:#x} and translation offset {:#x} disagree",
            at,
            qb,
            to
        );
    }
    if ((to - qb) / 8 != f.header.scale_count) {
        fail("moby frame at {:#x}: scale count mismatch", at);
    }
    if (f.header.qwc != (to + 8 * tc + 15) >> 4) {
        fail("moby frame at {:#x}: qwc mismatch", at);
    }
    const ByteView payload = b.sub(at + 0x10, std::size_t{f.header.qwc} * 16, "moby frame payload");
    f.quats = payload.read_array<std::array<s16, 4>>(0, qb / 8, "moby frame quaternions");
    f.scales = payload.read_array<MobyScaleRecord>(qb, (to - qb) / 8, "moby frame scale records");
    f.trans = payload.read_array<MobyTransRecord>(to, tc, "moby frame translation records");
    f.payload = payload.to_vector();
    return f;
}

V4 lanes_lerp(const V4& a, const V4& b, u32 u, u32 t, std::size_t n, const V4& keep) {
    V4 r = keep;
    for (std::size_t k = 0; k < n; ++k) {
        r[k] = ps2::add(ps2::mul(a[k], u), ps2::mul(b[k], t));
    }
    return r;
}

// vmulax ACC, m0, v.x; vmadday ACC, m1, v.y; vmaddz r, m2, v.z (all lanes).
V4 mat3(const V4& m0, const V4& m1, const V4& m2, const V4& v) {
    V4 r{};
    for (std::size_t k = 0; k < 4; ++k) {
        r[k] =
            ps2::add(ps2::add(ps2::mul(m0[k], v[0]), ps2::mul(m1[k], v[1])), ps2::mul(m2[k], v[2]));
    }
    return r;
}

// ... then vmaddw r, m3, vf0.w.
V4 mat4_point(const std::array<V4, 4>& m, const V4& v) {
    V4 r{};
    for (std::size_t k = 0; k < 4; ++k) {
        r[k] = ps2::add(
            ps2::add(
                ps2::add(ps2::mul(m[0][k], v[0]), ps2::mul(m[1][k], v[1])), ps2::mul(m[2][k], v[2])
            ),
            ps2::mul(m[3][k], kOne)
        );
    }
    return r;
}

V4 quat_bits(const std::array<s16, 4>& q) {
    // s16 / 32768: exact.
    return {
        ps2::bits(static_cast<f32>(q[0]) / 32768.0f),
        ps2::bits(static_cast<f32>(q[1]) / 32768.0f),
        ps2::bits(static_cast<f32>(q[2]) / 32768.0f),
        ps2::bits(static_cast<f32>(q[3]) / 32768.0f),
    };
}

V4 with_w(const std::array<f32, 3>& v, u32 w) {
    return {ps2::bits(v[0]), ps2::bits(v[1]), ps2::bits(v[2]), w};
}

std::array<f32, 3> scale_value(const MobyScaleRecord& r) {
    // u16 / 4096: exact.
    return {r.scale[0] / 4096.0f, r.scale[1] / 4096.0f, r.scale[2] / 4096.0f};
}

std::array<f32, 3> trans_value(const MobyTransRecord& r) {
    return {
        static_cast<f32>(r.trans[0]), static_cast<f32>(r.trans[1]), static_cast<f32>(r.trans[2])
    };
}

}  // namespace

MobyFrame parse_frame(ByteView base, std::size_t offset) {
    return read_frame(base, offset);
}

std::array<s16, 4> MobyFrame::quat_at(std::size_t j) const {
    std::array<s16, 4> q{};
    for (std::size_t k = 0; k < 4; ++k) {
        const std::size_t o = 8 * j + 2 * k;
        if (o + 2 <= payload.size()) {
            std::memcpy(&q[k], payload.data() + o, 2);
        }
    }
    return q;
}

MobySequence parse_sequence(ByteView base, std::size_t offset) {
    MobySequence s;
    s.header = base.read<MobySequenceHeader>(offset, "moby sequence header");
    const std::size_t fc = s.header.frame_count;
    const auto pointers = base.read_array<u32>(offset + 0x1c, fc, "moby frame pointers");
    s.triggers = base.read_array<u32>(
        offset + 0x1c + 4 * fc, s.header.trigger_count, "moby sequence triggers"
    );
    s.frames.reserve(fc);
    for (const u32 p : pointers) {
        if (p >> 28 != 0) {
            fail("moby sequence: frame pointer {:#x} has a non-zero top nibble", p);
        }
        s.frames.push_back(read_frame(base, p));
    }
    return s;
}

std::vector<std::optional<MobySequence>> parse_sequences(
    ByteView class_blob, const MobyClass& moby
) {
    std::vector<std::optional<MobySequence>> out;
    out.reserve(moby.sequence_pointers.size());
    for (const s32 p : moby.sequence_pointers) {
        if (p <= 0) {
            out.emplace_back();
        } else {
            out.emplace_back(parse_sequence(class_blob, static_cast<std::size_t>(p)));
        }
    }
    return out;
}

std::vector<std::pair<u8, std::array<u32, 20>>> gait_records(
    ByteView class_blob, const MobyClass& moby
) {
    std::vector<std::pair<u8, std::array<u32, 20>>> out;
    for (std::size_t i = 0; i < moby.sequence_pointers.size(); ++i) {
        const s32 p = moby.sequence_pointers[i];
        if (p <= 0) {
            continue;
        }
        try {
            const auto h = class_blob.read<MobySequenceHeader>(
                static_cast<std::size_t>(p), "moby sequence header"
            );
            if (h.trigger_data == 0) {
                continue;
            }
            out.emplace_back(
                static_cast<u8>(i),
                class_blob.read<std::array<u32, 20>>(h.trigger_data, "gait record")
            );
        } catch (const AssetError&) {
            // Not every pointer leads to a gait record; skip what does not read.
        }
    }
    return out;
}

MobyAnimClass::MobyAnimClass(const MobyClass& moby, std::vector<std::optional<MobySequence>> seqs)
    : joint_count(moby.header.joint_count),
      sequences(std::move(seqs)) {
    const MobySkeleton& sk = moby.skeleton;
    for (std::size_t j = 0; j < joint_count; ++j) {
        skeleton.push_back(j < sk.matrices.size() ? sk.matrices[j] : kIdentityJoint);
        if (j < sk.trans.size()) {
            rest.push_back(sk.trans[j].vector);
            parent_word.push_back(sk.trans[j].parent_offset | u32{sk.trans[j].seventy} << 16);
        } else {
            rest.push_back({});
            parent_word.push_back(0);
        }
    }
}

const MobySequence* MobyAnimClass::sequence(u8 seq) const {
    if (seq >= sequences.size() || !sequences[seq]) {
        return nullptr;
    }
    return &*sequences[seq];
}

const MobyFrame* MobyAnimClass::frame(u8 seq, u8 index) const {
    const MobySequence* s = sequence(seq);
    return s != nullptr && index < s->frames.size() ? &s->frames[index] : nullptr;
}

std::optional<std::size_t> MobyAnimClass::parent(std::size_t joint) const {
    return joint < parent_word.size() ? scratchpad_record(parent_word[joint]) : std::nullopt;
}

AnimState AnimState::spawn(const MobyAnimClass& anim) {
    AnimState s;
    if (anim.sequences.size() == 1 && anim.sequences[0]
        && anim.sequences[0]->header.frame_count <= 1) {
        s.speed = 0;
        s.skip_advance = (anim.sequences[0]->header.loop_sound & 0x80) != 0;
    }
    return s;
}

bool advance(AnimState& s, const MobyAnimClass& anim) {
    const u32 sp = ps2::bits(s.speed);
    u32 r = ps2::bits(s.rate);
    s.flags = 0;
    if (r == 0) {
        return false;
    }
    // Same sequence: adda.s ACC = 0 + t; madd.s t' = ACC + speed * rate.
    // A transition: add.s t' = t + rate.
    u32 t = s.seq_a == s.seq_b ? ps2::add(ps2::add(0, ps2::bits(s.t)), ps2::mul(sp, r))
                               : ps2::add(ps2::bits(s.t), r);
    if (sp == 0) {
        return false;
    }
    const auto ti = static_cast<s32>(t);
    // The snap window, a signed compare of the bits: negatives never snap.
    const bool snap = ti >= 0x3f7f'0000 && ti <= 0x3f80'8000;
    const bool stepping = snap || ti > static_cast<s32>(kOne) || ti < 0;
    if (!stepping) {
        s.t = ps2::to_float(t);
        return true;
    }
    const MobySequence* seq = anim.sequence(s.seq_b);
    if (seq == nullptr) {
        return false;
    }
    const s32 fc = seq->header.frame_count;
    const u32 rate_override = ps2::bits(seq->header.rate_override);
    u8 flags = 0;
    auto rate_of = [&](s32 frame) -> std::optional<u32> {
        if (rate_override != 0) {
            return rate_override;
        }
        if (frame < 0 || static_cast<std::size_t>(frame) >= seq->frames.size()) {
            return std::nullopt;
        }
        return ps2::bits(seq->frames[static_cast<std::size_t>(frame)].header.rate);
    };
    if (snap || ti > static_cast<s32>(kOne)) {
        if (snap) {
            t = kOne;
        }
        // Forward, while t > 1.0 (no snap inside the loop).
        while (true) {
            t = ps2::sub(t, kOne);
            const u8 new_a = s.frame_b;
            s32 new_b = s.frame_b + 1;
            if (s.seq_a != s.seq_b) {
                s.trigger_count = seq->header.trigger_count;
                s.seq_a = s.seq_b;
            }
            t = ps2::div(t, r);
            flags |= 1;
            if (fc - new_b <= 0) {
                new_b = 0;
                flags |= 2;
            }
            // The new key A's own rate (the old B frame), unless overridden.
            const auto next = rate_of(new_a);
            if (!next) {
                return false;
            }
            r = *next;
            t = ps2::mul(t, r);
            s.frame_a = new_a;
            s.frame_b = static_cast<u8>(new_b);
            s.rate = ps2::to_float(r);
            s.t = ps2::to_float(t);
            if (static_cast<s32>(t) <= static_cast<s32>(kOne)) {
                break;
            }
        }
    } else {
        // Backward, while t < 0 (the sign bit: -0.0 counts).
        while (true) {
            t = ps2::div(t, r);
            const u8 new_b = s.frame_a;
            s32 new_a = s.frame_a - 1;
            flags |= 1;
            if (new_a < 0) {
                new_a = fc - 1;
                flags |= 2;
            }
            const auto next = rate_of(new_a);
            if (!next) {
                return false;
            }
            r = *next;
            t = ps2::mul(t, r);
            s.frame_a = static_cast<u8>(new_a);
            s.frame_b = new_b;
            t = ps2::add(t, kOne);
            s.rate = ps2::to_float(r);
            s.t = ps2::to_float(t);
            if ((t & ps2::kSign) == 0) {
                break;
            }
        }
    }
    s.flags = flags;
    return true;
}

bool hard_cut(AnimState& s, const MobyAnimClass& anim, u8 seq, s32 frame) {
    const MobySequence* q = anim.sequence(seq);
    if (q == nullptr) {
        return false;
    }
    const s32 fc = q->header.frame_count;
    const auto a = static_cast<u8>(frame < fc ? frame : fc - 1);
    auto b = static_cast<u8>(a + 1);
    if (fc - 1 < b) {
        b = static_cast<u8>(fc - 1);
    }
    if (fc <= b) {
        b = 0;
    }
    if (a >= q->frames.size()) {
        return false;
    }
    s.seq_a = seq;
    s.frame_a = a;
    s.seq_b = seq;
    s.frame_b = b;
    // Key A's own rate (the override is not consulted here).
    s.rate = q->frames[a].header.rate;
    s.flags = static_cast<u8>(s.flags & ~2);
    s.trigger_count = q->header.trigger_count;
    return true;
}

bool consecutive(const AnimState& s) {
    return s.seq_a == s.seq_b && s.frame_b == s.frame_a + 1;
}

std::vector<u8> post_scale_list(std::span<const u8> a, std::span<const u8> b) {
    // Each node's link: none, the raw record bits (never followed), or a joint.
    enum class Kind : u8 {
        Zero,
        Raw,
        To
    };

    struct Link {
        Kind kind = Kind::Zero;
        u8 joint = 0;
    };

    Link head;
    std::array<Link, 256> link{};
    std::optional<u8> last;
    auto set = [&](Link v) {
        if (last) {
            link[*last] = v;
        } else {
            head = v;
        }
    };
    for (const u8 j : a) {
        set({Kind::To, j});
        last = j;
        // The record store overwrites the node's own link word with the
        // record's bits.
        link[j] = {Kind::Raw, 0};
    }
    std::array<bool, 256> b_written{};
    for (const u8 j : b) {
        const bool was = b_written[j];
        b_written[j] = true;
        if (!was) {
            set({Kind::To, j});
            last = j;
        }
    }
    set({Kind::Zero, 0});
    std::vector<u8> out;
    Link cur = head;
    while (cur.kind == Kind::To && out.size() <= 512) {
        out.push_back(cur.joint);
        cur = link[cur.joint];
    }
    return out;
}

std::array<V4, 3> quat_rows(const V4& q) {
    using ps2::add;
    using ps2::mul;
    using ps2::sub;
    const u32 x = q[0];
    const u32 y = q[1];
    const u32 z = q[2];
    const u32 w = q[3];
    // Every product is (q_a + q_a) * q_b.
    const u32 x2 = add(x, x);
    const u32 y2 = add(y, y);
    const u32 z2 = add(z, z);
    const u32 xw2 = mul(x2, w);
    const u32 yw2 = mul(y2, w);
    const u32 zw2 = mul(z2, w);
    const u32 xx2 = mul(x2, x);
    const u32 yx2 = mul(y2, x);
    const u32 zx2 = mul(z2, x);
    const u32 yy2 = mul(y2, y);
    const u32 zy2 = mul(z2, y);
    const u32 zz2 = mul(z2, z);
    return {
        V4{sub(sub(kOne, yy2), zz2), sub(yx2, zw2), add(zx2, yw2), 0},
        V4{add(add(0, zw2), yx2), sub(sub(kOne, xx2), zz2), sub(zy2, xw2), 0},
        V4{add(sub(0, yw2), zx2), add(add(0, xw2), zy2), sub(sub(kOne, xx2), yy2), 0},
    };
}

V4 nlerp_flip(const V4& a, const V4& b, u32 u, u32 t) {
    using ps2::add;
    using ps2::mul;
    V4 p{};
    V4 nb{};
    for (std::size_t k = 0; k < 4; ++k) {
        p[k] = mul(a[k], b[k]);
        nb[k] = mul(b[k], kNegOne);
    }
    const u32 d = add(add(add(p[1], p[0]), mul(kOne, p[2])), mul(kOne, p[3]));
    const V4 q =
        (d & ps2::kSign) == 0 ? lanes_lerp(a, b, u, t, 4, a) : lanes_lerp(a, nb, u, t, 4, a);
    V4 sq{};
    for (std::size_t k = 0; k < 4; ++k) {
        sq[k] = mul(q[k], q[k]);
    }
    const u32 n = add(add(add(sq[0], sq[1]), mul(kOne, sq[2])), mul(kOne, sq[3]));
    const u32 qq = ps2::rsqrt(kOne, n);
    return {mul(q[0], qq), mul(q[1], qq), mul(q[2], qq), mul(q[3], qq)};
}

std::vector<JointMatrix> evaluate(const MobyAnimClass& anim, const AnimState& s) {
    const std::size_t jc = anim.joint_count;
    if (jc == 0) {
        return {kIdentityJoint};
    }
    const MobyFrame* fa = anim.frame(s.seq_a, s.frame_a);
    if (fa == nullptr) {
        return std::vector<JointMatrix>(jc, kIdentityJoint);
    }
    const MobyFrame* fb = nullptr;
    if (ps2::bits(s.t) != 0) {
        fb = anim.frame(s.seq_b, s.frame_b);
        if (fb == nullptr) {
            return std::vector<JointMatrix>(jc, kIdentityJoint);
        }
    }
    return evaluate_keys(anim, fa, fb, s.t, consecutive(s));
}

std::vector<JointMatrix> evaluate_keys(
    const MobyAnimClass& anim, const MobyFrame* fa, const MobyFrame* fb, f32 t_value, bool plain
) {
    const std::size_t jc = anim.joint_count;
    if (jc == 0) {
        return {kIdentityJoint};
    }
    const u32 t = ps2::bits(t_value);
    if (fa == nullptr || (t != 0 && fb == nullptr)) {
        return std::vector<JointMatrix>(jc, kIdentityJoint);
    }
    if (t == 0) {
        fb = nullptr;
    }
    const u32 u = ps2::sub(kOne, t);
    const std::size_t n = std::min(jc, kRecords);

    // The scratchpad joint records: [0] quaternion (or inherited scale A
    // before the quaternions), [1] inherited scale (B slot; w != 0 present),
    // [2] translation A (w = parent word), [3] translation B.
    std::vector<std::array<V4, 4>> rec(kRecords);
    std::array<V4, 256> post_a;
    std::array<V4, 256> post_b;
    post_a.fill(kUnitScale);
    post_b.fill(kUnitScale);
    for (std::size_t j = 0; j < n; ++j) {
        const V4 rest = with_w(anim.rest[j], anim.parent_word[j]);
        rec[j] = {kUnitScale, kUnitScale, rest, V4{rest[0], rest[1], rest[2], 0}};
    }
    auto valid = [&](std::size_t j) {
        return j < n;
    };
    std::vector<u8> post_list;

    if (fb == nullptr) {
        // One key (t = 0): no lerp, quaternions as stored.
        for (const MobyScaleRecord& sr : fa->scales) {
            const std::size_t j = sr.joint;
            if (sr.inherited()) {
                if (valid(j)) {
                    rec[j][1] = with_w(scale_value(sr), (u32{sr.joint} | u32{sr.flags} << 8) << 3);
                }
            } else {
                post_a[j] = with_w(scale_value(sr), 0);
                post_list.push_back(sr.joint);
            }
        }
        for (const MobyTransRecord& tr : fa->trans) {
            const auto j = static_cast<s8>(tr.joint);
            if (j >= 0 && valid(static_cast<std::size_t>(j))) {
                V4& slot = rec[static_cast<std::size_t>(j)][2];
                slot = with_w(trans_value(tr), slot[3]);
            }
        }
        for (std::size_t j = 0; j < n; ++j) {
            rec[j][0] = quat_bits(fa->quat_at(j));
        }
    } else {
        // Two keys: A's channels, then B's.
        std::array<bool, 256> inherited{};
        std::array<bool, 256> translated{};
        std::vector<u8> post_a_order;
        std::vector<u8> post_b_order;
        for (const MobyScaleRecord& sr : fa->scales) {
            const std::size_t j = sr.joint;
            if (sr.inherited()) {
                if (valid(j)) {
                    rec[j][0] = with_w(scale_value(sr), 0);
                    inherited[j] = true;
                }
            } else {
                post_a[j] = with_w(scale_value(sr), 0);
                post_a_order.push_back(sr.joint);
            }
        }
        for (const MobyTransRecord& tr : fa->trans) {
            const auto j = static_cast<s8>(tr.joint);
            if (j >= 0 && valid(static_cast<std::size_t>(j))) {
                V4& slot = rec[static_cast<std::size_t>(j)][2];
                slot = with_w(trans_value(tr), slot[3]);
                translated[static_cast<std::size_t>(j)] = true;
            }
        }
        for (const MobyScaleRecord& sr : fb->scales) {
            const std::size_t j = sr.joint;
            if (sr.inherited()) {
                if (valid(j)) {
                    rec[j][1] = with_w(scale_value(sr), 0);
                    inherited[j] = true;
                }
            } else {
                post_b[j] = with_w(scale_value(sr), 0);
                post_b_order.push_back(sr.joint);
            }
        }
        for (const MobyTransRecord& tr : fb->trans) {
            const auto j = static_cast<s8>(tr.joint);
            if (j >= 0 && valid(static_cast<std::size_t>(j))) {
                V4& slot = rec[static_cast<std::size_t>(j)][3];
                slot = with_w(trans_value(tr), slot[3]);
                translated[static_cast<std::size_t>(j)] = true;
            }
        }
        post_list = post_scale_list(post_a_order, post_b_order);

        // Inherited scale into [1] with w = 1; translation into [2], parent kept.
        for (std::size_t j = 0; j < n; ++j) {
            if (inherited[j]) {
                rec[j][1] = lanes_lerp(rec[j][0], rec[j][1], u, t, 3, V4{0, 0, 0, kOne});
            }
            if (translated[j]) {
                rec[j][2] = lanes_lerp(rec[j][2], rec[j][3], u, t, 3, rec[j][2]);
            }
        }
        for (std::size_t j = 0; j < n; ++j) {
            const V4 qa = quat_bits(fa->quat_at(j));
            const V4 qb = quat_bits(fb->quat_at(j));
            rec[j][0] = plain ? lanes_lerp(qa, qb, u, t, 4, qa) : nlerp_flip(qa, qb, u, t);
        }
    }

    // Local matrices and the parent chain, joints in index order.
    for (std::size_t j = 0; j < n; ++j) {
        const V4 q = rec[j][0];
        const V4 sc = rec[j][1];
        const V4 tr = rec[j][2];
        std::array<V4, 3> rows = quat_rows(q);
        if (sc[3] != 0) {
            for (std::size_t i = 0; i < 3; ++i) {
                for (std::size_t c = 0; c < 3; ++c) {
                    rows[i][c] = ps2::mul(rows[i][c], sc[i]);
                }
            }
        }
        std::array<V4, 4> pp = kIdentity4;
        if (tr[3] != 0) {
            if (const auto p = scratchpad_record(tr[3])) {
                pp = rec[*p];
            }
        }
        const V4 r3 = {ps2::add(0, tr[0]), ps2::add(0, tr[1]), ps2::add(0, tr[2]), kOne};
        rec[j] = {
            mat3(pp[0], pp[1], pp[2], rows[0]),
            mat3(pp[0], pp[1], pp[2], rows[1]),
            mat3(pp[0], pp[1], pp[2], rows[2]),
            mat4_point(pp, r3),
        };
    }

    // Post-scale, after the whole chain: children do not see it.
    for (const u8 jj : post_list) {
        const std::size_t j = jj;
        if (!valid(j)) {
            continue;
        }
        const V4 sv = t == 0 ? post_a[j] : lanes_lerp(post_a[j], post_b[j], u, t, 3, post_a[j]);
        // Row i (of the three axes) by scale i.
        for (std::size_t i = 0; i < 3; ++i) {
            for (std::size_t c = 0; c < 3; ++c) {
                rec[j][i][c] = ps2::mul(rec[j][i][c], sv[i]);
            }
        }
    }

    // The palette: F.r_i = P0 S_i.x + P1 S_i.y + P2 S_i.z; F.r3 adds P3.
    std::vector<JointMatrix> out;
    out.reserve(n);
    for (std::size_t j = 0; j < n; ++j) {
        const std::array<V4, 4>& p = rec[j];
        std::array<V4, 4> sk{};
        for (std::size_t i = 0; i < 4; ++i) {
            sk[i] = ps2::bits(anim.skeleton[j][i]);
        }
        const std::array<V4, 4> f = {
            mat3(p[0], p[1], p[2], sk[0]),
            mat3(p[0], p[1], p[2], sk[1]),
            mat3(p[0], p[1], p[2], sk[2]),
            mat4_point(p, sk[3]),
        };
        JointMatrix m{};
        for (std::size_t i = 0; i < 4; ++i) {
            m[i] = ps2::to_floats(f[i]);
        }
        out.push_back(m);
    }
    return out;
}

}  // namespace openrac::assets::rac1
