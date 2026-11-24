// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// How fast the mouse moves now, from its last motion: so that a frame between ticks can show the
// mouse's turn as it will be when the frame is shown, not when it was drawn
// (frontend/camera_latch.cpp). Frames are drawn at uneven times before they are shown (a tick's
// frame at once, the others later), so the motion read at drawing alone made the turn uneven on
// screen: a fast turn looked doubled at 90 Hz.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace openrac::platform {

class MouseRate {
public:
    // How far back the rate is measured. A mouse reports at least every 8 ms (125 Hz), so three
    // reports at the slowest; a mouse that stopped is still within 25 ms.
    static constexpr std::uint64_t kWindowNs = 25'000'000;

    // Motion (dx, dy) that came at `ns` (SDL's clock).
    void add(std::uint64_t ns, float dx, float dy) {
        m_samples[m_next] = {ns, dx, dy};
        m_next = (m_next + 1) % m_samples.size();
    }

    // The motion a second over the last kWindowNs before `now_ns`.
    void rate(std::uint64_t now_ns, float& vx, float& vy) const {
        float x = 0.0f;
        float y = 0.0f;
        for (const Sample& s : m_samples) {
            if (s.ns != 0 && s.ns <= now_ns && now_ns - s.ns < kWindowNs) {
                x += s.dx;
                y += s.dy;
            }
        }
        constexpr float kWindowSeconds = static_cast<float>(kWindowNs) * 1e-9f;
        vx = x / kWindowSeconds;
        vy = y / kWindowSeconds;
    }

private:
    struct Sample {
        std::uint64_t ns = 0;
        float dx = 0.0f;
        float dy = 0.0f;
    };

    // Enough for an 8000 Hz mouse over the window.
    std::array<Sample, 256> m_samples{};
    std::size_t m_next = 0;
};

}  // namespace openrac::platform
