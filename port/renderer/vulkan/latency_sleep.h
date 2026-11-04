// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The low-latency frames' sleep (vkgl::latency_sleep): how long a frame waits before it begins, so
// that the processor finishes it just as the graphics card is free for it. A frame of
// low-latency frames is queued only once the one before it is done on the card; begun at once, its
// processor work is done early and waits there for the card, holding a picture made from an older
// moment. The sleep moves that wait to before the frame begins: the frame then starts (and reads
// the time it is drawn for, and the input) later by as much, and is queued as soon as it can be.
// A frame then costs the longer of the processor's and the card's time, as two frames in flight
// do, and is shown about one processor frame and one card frame after it began.
//
// Arithmetic on what it is given only, so that tests/latency_sleep_test.cpp can pin it.

#pragma once

#include <algorithm>

namespace openrac::vkgl {

class LatencySleep {
public:
    // What the frame begins earlier than the prediction says: the times move from frame to frame,
    // and a frame begun too late costs a frame rate, one begun too early only a little latency.
    static constexpr double kMargin = 0.0005;
    // No sleep is ever longer, whatever the measures say (a thirtieth of a second).
    static constexpr double kMaxDelay = 1.0 / 30.0;
    // The weight of a new measure in the smoothed ones.
    static constexpr double kWeight = 0.1;

    // A frame's time on the graphics card, in seconds (its timestamps).
    void gpu_frame(double seconds) {
        if (seconds > 0.0) {
            m_gpu = m_gpu < 0.0 ? seconds : m_gpu + kWeight * (seconds - m_gpu);
        }
    }

    // A frame's processor work from its beginning to its queueing, without the sleep or any wait
    // for the card (`work`), what it slept before it began (`slept`), and the wait for the frame
    // before it to finish that was left when it was queued (`wait`): seconds.
    void cpu_frame(double work, double slept, double wait) {
        if (work <= 0.0) {
            return;
        }
        m_cpu = m_cpu < 0.0 ? work : m_cpu + kWeight * (work - m_cpu);
        if (!known()) {
            return;
        }
        // What the measures foretell: the card's frame less the processor's, the margin early.
        const double predicted = m_gpu - m_cpu - kMargin;
        // What this frame showed: had it slept the wait it was left with as well, less the margin,
        // it would have been queued just as the card was free. A frame that had no wait left came
        // after the card was free: the sleep shrinks by the margin a frame until one waits again.
        const double shown = slept + wait - kMargin;
        m_delay = std::clamp(std::min(predicted, shown), 0.0, kMaxDelay);
    }

    // Both the card's and the processor's frame have been measured: before, frames are not slept
    // and wait for the frame before them as they began (vkgl's prepare_frame).
    bool known() const { return m_gpu >= 0.0 && m_cpu >= 0.0; }

    // How long the next frame sleeps before it begins, in seconds: 0 until the times are known.
    double delay() const { return known() ? m_delay : 0.0; }

private:
    double m_gpu = -1.0;
    double m_cpu = -1.0;
    double m_delay = 0.0;
};

}  // namespace openrac::vkgl
