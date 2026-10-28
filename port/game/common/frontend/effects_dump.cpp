// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "effects_dump.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <limits>
#include <mutex>
#include <vector>

namespace openrac::frontend::effects_dump {

namespace {

std::atomic<bool> g_armed{false};
std::atomic<unsigned> g_frames{0};
std::mutex g_mutex;
std::vector<std::string> g_snapshots;
std::string g_note;  // the game's latest notes (under g_mutex)
std::string g_particles;  // the latest frame's particles near Ratchet (under g_mutex)

constexpr unsigned kEvery = 20;         // frames between snapshots
constexpr std::size_t kKept = 400;      // the last so many snapshots
constexpr float kNear = 6.0f;           // items closer than this to Ratchet are listed

float distance(const renderer::Vec3& a, const renderer::Vec3& b) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::string number(float value) {
    return std::isfinite(value) ? std::format("{:.4g}", value) : std::string("null");
}

std::string vec(const renderer::Vec3& v) {
    return std::format("[{},{},{}]", number(v[0]), number(v[1]), number(v[2]));
}

// One frame: its counts, and the strips and quads near Ratchet with their GS state and extent.
std::string snapshot(unsigned frame, const renderer::FrameInput& input, const renderer::Vec3& hero) {
    std::string out = std::format(
        "{{\"frame\":{},\"hero\":{},\"camera\":{},\"strip_count\":{},\"quad_count\":{},\"strips\":[",
        frame, vec(hero), vec(input.camera.position), input.strips.size(), input.effects.size());
    const auto vertices = input.strip_vertices;
    bool first = true;
    for (std::size_t i = 0; i < input.strips.size(); ++i) {
        const renderer::EffectStrip& strip = input.strips[i];
        constexpr float kBig = std::numeric_limits<float>::max();
        renderer::Vec3 low{kBig, kBig, kBig}, high{-kBig, -kBig, -kBig};
        float nearest = kBig;
        std::uint32_t rgba_low = 0xFFFFFFFFu, rgba_high = 0;
        const std::uint64_t end = std::min<std::uint64_t>(
            static_cast<std::uint64_t>(strip.first) + strip.count, vertices.size());
        for (std::uint64_t v = strip.first; v < end; ++v) {
            const renderer::Vec3 p{vertices[v].x, vertices[v].y, vertices[v].z};
            for (int k = 0; k < 3; ++k) {
                low[k] = std::min(low[k], p[k]);
                high[k] = std::max(high[k], p[k]);
            }
            nearest = std::min(nearest, distance(p, hero));
            // By the alpha byte, then the colour: the brightest and the darkest vertex.
            rgba_low = std::min(rgba_low, vertices[v].rgba);
            rgba_high = std::max(rgba_high, vertices[v].rgba);
        }
        if (nearest >= kNear) {
            continue;
        }
        out += std::format(
            "{}{{\"i\":{},\"surface\":{},\"count\":{},\"tex0\":\"{:x}\",\"alpha\":\"{:x}\",\"clamp\":\"{:x}\","
            "\"uv_motion\":{},\"min\":{},\"max\":{},\"nearest\":{},\"rgba\":[\"{:08x}\",\"{:08x}\"]}}",
            first ? "" : ",", i, strip.surface, strip.count, strip.tex0, strip.alpha, strip.clamp,
            strip.uv_motion.enabled ? 1 : 0, vec(low), vec(high), number(nearest), rgba_low, rgba_high);
        first = false;
    }
    out += "],\"quads\":[";
    first = true;
    for (std::size_t i = 0; i < input.effects.size(); ++i) {
        const renderer::EffectQuad& q = input.effects[i];
        float nearest = std::numeric_limits<float>::max();
        for (const auto& c : q.corner) {
            const float w = c[3] != 0.0f ? c[3] : 1.0f;
            nearest = std::min(nearest, distance({c[0] / w, c[1] / w, c[2] / w}, hero));
        }
        if (nearest >= kNear) {
            continue;
        }
        out += std::format(
            "{}{{\"i\":{},\"tex0\":\"{:x}\",\"alpha\":\"{:x}\",\"clamp\":\"{:x}\",\"tbp\":{},\"nearest\":{},"
            "\"corners\":[",
            first ? "" : ",", i, q.tex0, q.alpha, q.clamp, q.tbp, number(nearest));
        for (int k = 0; k < 4; ++k) {
            out += std::format("{}[{},{},{},{},{},{},\"{:08x}\"]", k ? "," : "", number(q.corner[k][0]),
                               number(q.corner[k][1]), number(q.corner[k][2]), number(q.corner[k][3]),
                               number(q.st[k][0]), number(q.st[k][1]), q.rgba[k]);
        }
        out += "]}";
        first = false;
    }
    out += "]";
    {
        const std::lock_guard lock(g_mutex);
        if (!g_note.empty()) {
            out += ",\"game\":" + g_note;
        }
        if (!g_particles.empty()) {
            out += ",\"particles\":" + g_particles;
        }
    }
    out += "}";
    return out;
}

}  // namespace

void arm(bool on) {
    g_armed = on;
    g_frames = 0;
    if (on) {
        const std::lock_guard lock(g_mutex);
        g_snapshots.clear();
        g_note.clear();
        g_particles.clear();
    }
}

bool armed() {
    return g_armed.load(std::memory_order_relaxed);
}

void note(std::string json) {
    const std::lock_guard lock(g_mutex);
    g_note = std::move(json);
}

void particles(std::string json) {
    const std::lock_guard lock(g_mutex);
    g_particles = std::move(json);
}

void offer(const renderer::FrameInput& input, const std::optional<renderer::Vec3>& hero) {
    if (!g_armed.load(std::memory_order_relaxed)) {
        return;
    }
    const unsigned frame = g_frames.fetch_add(1);
    if (frame % kEvery != 0 || !hero) {
        return;
    }
    std::string taken = snapshot(frame, input, *hero);
    const std::lock_guard lock(g_mutex);
    if (g_snapshots.size() >= kKept) {
        g_snapshots.erase(g_snapshots.begin());
    }
    g_snapshots.push_back(std::move(taken));
}

std::string latest() {
    const std::lock_guard lock(g_mutex);
    if (g_snapshots.empty()) {
        return {};
    }
    std::string out = "{\"every\":" + std::to_string(kEvery) + ",\"near\":" + number(kNear) + ",\"snapshots\":[\n";
    for (std::size_t i = 0; i < g_snapshots.size(); ++i) {
        out += g_snapshots[i];
        out += i + 1 < g_snapshots.size() ? ",\n" : "\n";
    }
    out += "]}\n";
    return out;
}

}  // namespace openrac::frontend::effects_dump
