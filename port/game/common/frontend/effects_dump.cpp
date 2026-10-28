// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "effects_dump.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <limits>
#include <mutex>

namespace openrac::frontend::effects_dump {

namespace {

std::atomic<bool> g_armed{false};
std::atomic<unsigned> g_frames{0};
std::mutex g_mutex;
std::string g_latest;

constexpr unsigned kEvery = 30;      // frames between snapshots
constexpr float kNear = 4.0f;        // items closer than this to Ratchet list all their vertices
constexpr std::uint32_t kFarVertices = 4, kNearVertices = 96;

float distance(const renderer::Vec3& a, const std::optional<renderer::Vec3>& b) {
    if (!b) {
        return -1.0f;
    }
    const float dx = a[0] - (*b)[0], dy = a[1] - (*b)[1], dz = a[2] - (*b)[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::string number(float value) {
    return std::isfinite(value) ? std::format("{:.4g}", value) : std::string("null");
}

std::string vec(const renderer::Vec3& v) {
    return std::format("[{},{},{}]", number(v[0]), number(v[1]), number(v[2]));
}

std::string snapshot(const renderer::FrameInput& input, const std::optional<renderer::Vec3>& hero) {
    std::string out = "{";
    out += std::format("\"hero\":{},\"camera\":{},", hero ? vec(*hero) : std::string("null"),
                       vec(input.camera.position));
    out += "\"strips\":[";
    const auto vertices = input.strip_vertices;
    for (std::size_t i = 0; i < input.strips.size(); ++i) {
        const renderer::EffectStrip& strip = input.strips[i];
        renderer::Vec3 low{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max()};
        renderer::Vec3 high{-low[0], -low[1], -low[2]};
        float nearest = std::numeric_limits<float>::max();
        const std::uint32_t end = std::min<std::uint64_t>(
            static_cast<std::uint64_t>(strip.first) + strip.count, vertices.size());
        for (std::uint32_t v = strip.first; v < end; ++v) {
            const renderer::Vec3 p{vertices[v].x, vertices[v].y, vertices[v].z};
            for (int k = 0; k < 3; ++k) {
                low[k] = std::min(low[k], p[k]);
                high[k] = std::max(high[k], p[k]);
            }
            if (hero) {
                nearest = std::min(nearest, distance(p, hero));
            }
        }
        const renderer::Vec3 centre{(low[0] + high[0]) * 0.5f, (low[1] + high[1]) * 0.5f,
                                    (low[2] + high[2]) * 0.5f};
        out += std::format(
            "{}{{\"i\":{},\"surface\":{},\"count\":{},\"tex0\":\"{:x}\",\"tex1\":\"{:x}\",\"alpha\":\"{:x}\","
            "\"clamp\":\"{:x}\",\"uploads\":{},\"uv_motion\":{},\"min\":{},\"max\":{},\"centre_to_hero\":{},"
            "\"nearest_to_hero\":{},\"v\":[",
            i ? "," : "", i, strip.surface, strip.count, strip.tex0, strip.tex1, strip.alpha, strip.clamp,
            strip.upload_count, strip.uv_motion.enabled ? 1 : 0, vec(low), vec(high),
            number(distance(centre, hero)), hero ? number(nearest) : std::string("null"));
        const std::uint32_t listed = hero && nearest < kNear ? kNearVertices : kFarVertices;
        for (std::uint32_t v = strip.first; v < end && v - strip.first < listed; ++v) {
            const renderer::EffectVertex& e = vertices[v];
            out += std::format("{}[{},{},{},{},{},\"{:08x}\"]", v == strip.first ? "" : ",", number(e.x),
                               number(e.y), number(e.z), number(e.s), number(e.t), e.rgba);
        }
        out += "]}";
    }
    out += "],\"quads\":[";
    for (std::size_t i = 0; i < input.effects.size(); ++i) {
        const renderer::EffectQuad& q = input.effects[i];
        renderer::Vec3 centre{};
        for (const auto& c : q.corner) {
            const float w = c[3] != 0.0f ? c[3] : 1.0f;
            for (int k = 0; k < 3; ++k) {
                centre[k] += c[k] / w * 0.25f;
            }
        }
        out += std::format(
            "{}{{\"i\":{},\"tex0\":\"{:x}\",\"tex1\":\"{:x}\",\"alpha\":\"{:x}\",\"clamp\":\"{:x}\",\"tbp\":{},"
            "\"uploads\":{},\"centre\":{},\"centre_to_hero\":{},\"corners\":[",
            i ? "," : "", i, q.tex0, q.tex1, q.alpha, q.clamp, q.tbp, q.uploads, vec(centre),
            number(distance(centre, hero)));
        for (int k = 0; k < 4; ++k) {
            out += std::format("{}[{},{},{},{},{},{},\"{:08x}\"]", k ? "," : "", number(q.corner[k][0]),
                               number(q.corner[k][1]), number(q.corner[k][2]), number(q.corner[k][3]),
                               number(q.st[k][0]), number(q.st[k][1]), q.rgba[k]);
        }
        out += "]}";
    }
    out += "]}";
    return out;
}

}  // namespace

void arm(bool on) {
    g_armed = on;
    g_frames = 0;
    if (on) {
        const std::lock_guard lock(g_mutex);
        g_latest.clear();
    }
}

void offer(const renderer::FrameInput& input, const std::optional<renderer::Vec3>& hero) {
    if (!g_armed.load(std::memory_order_relaxed) || g_frames.fetch_add(1) % kEvery != 0) {
        return;
    }
    std::string taken = snapshot(input, hero);
    const std::lock_guard lock(g_mutex);
    g_latest = std::move(taken);
}

std::string latest() {
    const std::lock_guard lock(g_mutex);
    return g_latest;
}

}  // namespace openrac::frontend::effects_dump
