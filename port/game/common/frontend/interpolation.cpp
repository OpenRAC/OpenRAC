// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frames drawn between the game's ticks, a slice of frontend/state.h: the kept tick drawn with
// its camera, mobys, poses and shadow part way from the last tick's to its own (draw_kept); when a
// frame is due and whether one is drawn before the next tick (set_tick, interpolating,
// wants_between, between, frame_due); the swap and what it tells the pacing of the display
// (swapped); the paused game drawn again (hold); and the frame's cost switches (OPENRAC_FRAME_COST,
// OPENRAC_SLOW_FRAME). The pacing's pure arithmetic is frontend/pacing.h's; frame() in
// frontend.cpp reads each tick from the game and keeps it (Kept) for what is drawn here.
//
// Pacing and input stay on the window thread. The Vulkan worker records an owned world snapshot,
// then handles window copies and presentation after the window thread has composed the UI.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <SDL3/SDL.h>

#include "common/breadcrumbs.h"
#include "common/crash.h"
#include "common/env.h"
#include "common/log.h"
#include "common/precise_wait.h"
#include "common/sampler.h"
#include "common/slow_cpu.h"
#include "common/stats.h"
#include "debug_menu.h"
#include "debug_overlay.h"
#include "frame_pacing.h"
#include "frontend.h"
#include "game/common/frontend/camera_latch.h"
#include "game/common/frontend/effect_attachment.h"
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/input_latency.h"
#include "game/common/frontend/pacing.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "game/common/frontend/state.h"
#include "mod_menu.h"
#include "mod_overlay.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "platform/performance_hint.h"
#include "platform/pump.h"
#include "platform/window.h"
#include "renderer/direct/direct.h"
#include "renderer/effects/shadows.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer/ui/fps_counter.h"
#include "scene/level_renderer.h"
#include "scene/screenshot.h"
#include "settings.h"
#include "ui_colors.h"

namespace openrac::frontend {

namespace {

// OPENRAC_FRAME_COST=N: every N frames drawn (default 200), in the log, what a frame cost on
// average and where: `scene` (the tick's mobys and poses moved between ticks and sent to their
// buffers, on the CPU), `submit` (the renderers' OpenGL calls, on the CPU), `gpu` (the wait for the
// graphics card to finish them: this switch calls glFinish after each frame, so that the card's
// time is not counted into a later call; it lowers the frame rate a little) and `show` (the
// frame scaled to the window, and the swap). With the draw calls and triangles of the last frame.
// frame_cost is the one; only the window thread accumulates the completed job measurements.
struct FrameCost {
    bool on = false;
    int every = 200;
    int frames = 0;
    double scene = 0.0, submit = 0.0, gpu = 0.0, show = 0.0;
    std::chrono::steady_clock::time_point mark{};

    FrameCost() {
        if (env::value(env::Var::FrameCost)) {
            on = true;
            // Read as std::atoi read it, held to int's range, where atoi's answer was undefined.
            const std::int64_t asked = std::min<std::int64_t>(
                env::integer(env::Var::FrameCost, 0), std::numeric_limits<int>::max()
            );
            every = asked > 1 ? static_cast<int>(asked) : 200;
        }
    }

    // The seconds since the last call, added to `part`.
    void lap(double& part) {
        const auto now = std::chrono::steady_clock::now();
        part += std::chrono::duration<double>(now - mark).count();
        mark = now;
    }
} frame_cost;

// [debug tools] The picture being drawn is a paused game's (hold). Only the thread that opened the
// window touches it.
bool g_held = false;

// What between() slept for the low-latency frames before the frame it draws, for draw_kept to
// pass on as the frame begins (Window::latency_begin); 0 for the others.
double g_latency_slept = 0.0;

bool separate_vulkan_thread() {
    static const bool requested = env::flag(env::Var::RenderThread);
    return requested && !mods::vulkan_needs_game_thread();
}

bool threaded_rendering() {
    return gl::is_vulkan() && separate_vulkan_thread();
}

// Returns the frames a second the frames between ticks are held to (the player's cap, or the
// display's rate), 0 for no limit and with one frame a tick.
//
// The cap and refresh share one deadline schedule. A quick presentation return
// is not proof of a missed VSync: Vulkan queues presentation asynchronously.
double frame_cap() {
    if (!interpolating()) {
        return 0.0;
    }
    const settings::Settings& s = settings::get();
    const double requested =
        s.frame_rate == settings::FrameRate::Capped
            ? static_cast<double>(
                  std::clamp(s.frame_cap, settings::kFrameCapMin, settings::kFrameCapMax)
              )
            : 0.0;
    return frame_pacing::limit(
        requested,
        s.frame_rate == settings::FrameRate::Display,
        g->swap != platform::SwapInterval::Immediate,
        pacing.interval
    );
}

// Which edge of a frame the cap holds to its time (pacing.h): its showing, the evenest frame times,
// when nothing else paces the frames shown (no swap waits for a refresh, no render thread's jobs
// shown after the next tick starts, a swap shown at once rather than at the next refresh), the
// player has not asked for low-latency frames, which begin each frame as late as they can, and
// the cap draws more frames than the game ticks; else its start.
CapEdge cap_edge() {
    return g->swap == platform::SwapInterval::Immediate && g->window->swap_shows_at_once()
                   && !threaded_rendering() && !settings::get().low_latency_frames
                   && frame_cap() * pacing.period.count() > 1.001
               ? CapEdge::Present
               : CapEdge::Start;
}

// When the frame cap lets the next frame start: the clock's epoch when it holds none back.
std::chrono::steady_clock::time_point cap_start_time() {
    const double cap = frame_cap();
    const double step = cap > 0.0 ? 1.0 / cap : 0.0;
    return cap_start(cap_edge(), pacing.due, present_lead(pacing.cost, pacing.cost_spread, step));
}

// The refresh a frame started now is shown at: the first after the frame is drawn, and no sooner
// than its time when the cap holds it to its showing.
std::chrono::steady_clock::time_point next_display(std::chrono::steady_clock::time_point now) {
    const auto shown = shown_at(now, pacing.cost, pacing.interval, pacing.last_swap);
    if (cap_edge() == CapEdge::Present && pacing.due.time_since_epoch().count() != 0) {
        return std::max(shown, pacing.due);
    }
    return shown;
}

// How far behind the kept tick the scene runs: none with the player's low-latency frames, nor with
// the render thread, else half a tick. The render thread's jobs stop when the next tick is due
// (between(): none is queued once the simulation resumes), so with the lag their scene only got
// part of the way to the kept tick: it stood still for the first half tick, moved, then jumped
// at the next one (Vulkan through MoltenVK: at most 0.34 of the way at the loading screen's 20
// ticks a second, its approach to the planet visibly choppy; 0.47 at 50).
double lag_ticks() {
    return settings::get().low_latency_frames || threaded_rendering() ? kLowLatencyLagTicks
                                                                      : kLagTicks;
}

// How long after the next tick is due a frame held by its showing may still be shown (pacing.h,
// tick_slip): none for a frame held by its start.
std::chrono::steady_clock::duration slip() {
    const double cap = frame_cap();
    if (cap_edge() != CapEdge::Present || cap <= 0.0) {
        return {};
    }
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(
            tick_slip(1.0 / cap, pacing.period.count(), lag_ticks(), pacing.tick_work)
        )
    );
}

// How far the scene is from the last tick's to the kept one's at that refresh.
float alpha_at(std::chrono::steady_clock::time_point display) {
    return tick_alpha(display, pacing.tick, pacing.period, lag_ticks());
}

// Swaps the window's buffers for the frame begun at `started`, and keeps what the swap tells the
// pacing: what a frame costs, the display's refresh interval when it is to be read again, the
// last swap that waited for a refresh, and when the frame cap lets the next frame start.
void swapped(std::chrono::steady_clock::time_point started, std::int64_t frame) {
    using namespace std::chrono;
    const auto before = steady_clock::now();
    platform::frame_work_done();
    const double cost = duration<double>(before - started).count();
    pacing.cost_spread = 0.8 * pacing.cost_spread + 0.2 * std::abs(cost - pacing.cost);
    pacing.cost = 0.8 * pacing.cost + 0.2 * cost;
    // Held by its showing, the frame waits here for its time (never longer than the next tick may
    // wait for it, tick_slip): drawn early, it is shown on time all the same.
    const bool held_to_show = cap_edge() == CapEdge::Present;
    if (held_to_show && pacing.due > before) {
        const stats::Scope wait("frame cap wait");
        const auto next_tick =
            pacing.tick + duration_cast<steady_clock::duration>(pacing.period) + slip();
        precise_wait::until(std::min(pacing.due, next_tick));
        platform::work_waited(duration<double>(steady_clock::now() - before).count());
    }
    const auto swap_started = steady_clock::now();
    swap_rendered_frame(frame);
    // In the background (another window has the focus, or the game is minimised), Windows'
    // compositor stops pacing the swap: it returns at once, the frames run unbounded and take the
    // whole machine's GPU and CPU with them. There the frames are held to the game's tick by a real
    // sleep (never a spin), so the game keeps its speed and the rest of the system its frames.
    {
        static steady_clock::time_point last_background;
        // OPENRAC_FOCUSED_PACING: not for a hidden run measuring the frame rate a player gets.
        static const bool focused_pacing = env::flag(env::Var::FocusedPacing);
        if (!focused_pacing && !g->window->focused() && pacing.period.count() > 0.0) {
            const auto due = last_background + duration_cast<steady_clock::duration>(pacing.period);
            if (due > steady_clock::now()) {
                std::this_thread::sleep_until(due);
            }
            last_background = steady_clock::now();
        } else {
            last_background = {};
        }
    }
    profiling::frame();
    const auto after = steady_clock::now();
    renderer::ui::fps_counter::frame_drawn(
        after, duration<double>(after - swap_started).count(), settings::get().show_fps
    );
    if (pacing.interval_stale) {
        const float rate = g->window->refresh_rate();
        pacing.interval = rate > 20.0f ? 1.0 / rate : 0.0;
        pacing.interval_stale = false;
    }
    // What a frame may take, for the system's CPU clocks (performance_hint.h), as the diagnostics
    // recording sets its target: with frames between ticks a refresh, or the player's frame cap
    // when it is lower; with a frame a tick (or a refresh not known), the game's tick.
    const double cap = frame_cap();
    const double display = cap > 0.0 ? std::max(1.0 / cap, pacing.interval) : pacing.interval;
    platform::set_frame_target(interpolating() && display > 0.0 ? display : pacing.period.count());
    if (g->swap == platform::SwapInterval::Immediate || gl::is_vulkan()
        || duration<double>(after - before).count() < 0.0005) {
        // Vulkan's return is not a vblank timestamp, and an immediate GL return
        // supplies no display phase either. The shared deadline paces both;
        // never add a second refresh-length sleep after drawing the tick.
        pacing.last_swap = {};
    } else {
        pacing.last_swap = after;
    }
    // A frame rate of the player's own (and the display's, when no swap waits): the next frame is
    // due that long after this one was, so a frame held up by the game's tick is made up for by the
    // next ones; one more than a frame late starts the count again. It holds beside the swap's
    // wait: 60 frames a second on a 120 Hz display are every other refresh. Nothing sleeps here:
    // the game's tick may be due before the next frame is (frame_due).
    // Held by its showing, the next frame's time counts from this one's swap.
    if (cap > 0.0) {
        const auto step = duration_cast<steady_clock::duration>(duration<double>(1.0 / cap));
        const auto now = steady_clock::now();
        pacing.due =
            frame_pacing::next_due(pacing.due, held_to_show ? swap_started : started, now, step);
    } else {
        pacing.due = {};
    }
    // The next frame's work counts from here: the swap, and any wait in it, is the display's.
    platform::work_resumed();
}

// OPENRAC_SLOW_FRAME=MS: every frame drawn takes that much longer, as on a graphics card too slow
// for the settings, for checking that the game's time does not slow with it.
void slow_frame() {
    static const double ms = env::number(env::Var::SlowFrame, 0.0);
    if (ms > 0.0) {
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(ms));
    }
}

}  // namespace

// Whether the frame cap lets a frame start now (within a millisecond: a sleep wakes that coarsely).
bool frame_due(std::chrono::steady_clock::time_point now) {
    return frame_may_start(frame_cap(), cap_start_time(), now);
}

// ---- The kept tick ----

std::string enhanced_graphics_error() {
    const auto* lighting = g ? g->render.scene.level_lighting() : nullptr;
    return lighting ? std::string(lighting->load_error()) : std::string{};
}

bool enhanced_graphics_available() {
    return g && g->render.enhanced_graphics_available();
}

namespace {

// The time spent writing shots, until take_shot_time asks (both on the window's thread).
std::chrono::steady_clock::duration shot_time{};

// Writes a requested capture only after the submitted frame is complete.
void capture_frame(const FramePresentation& output) {
    const bool first = output.first;
    const unsigned shown = output.shown;
    const int width = output.width;
    const int height = output.height;
    // OPENRAC_SHOT=FRAME:FILE.png writes that frame (for checking without looking at the screen);
    // OPENRAC_SHOT_BETWEEN also writes the frames drawn between ticks (FILE<frame>_b<n>.png).
    static const bool& shot_between =
        stats::probe("OPENRAC_SHOT_BETWEEN", "OPENRAC_SHOT also writes the frames between ticks");
    static int between_index = 0;
    between_index = first ? 0 : between_index + 1;
    // With one frame a tick (OPENRAC_UNCAPPED, OPENRAC_NO_BLEND, which scripted runs often set) no
    // frame is drawn between ticks, so there is none to write: said once, not left to guess.
    static bool told_none_between = false;
    if (shot_between && !told_none_between && one_frame_a_tick()) {
        told_none_between = true;
        log::warn(
            "OPENRAC_SHOT_BETWEEN: no frames between ticks to write while OPENRAC_UNCAPPED or "
            "OPENRAC_NO_BLEND is set (one frame a tick); run in real time with a frame rate above "
            "Original"
        );
    }
    // OPENRAC_SHOT_FROM=FRAME: no shots before that frame.
    static const std::uint64_t shot_from = env_frame(env::Var::ShotFrom).value_or(0);
    if ((first || shot_between) && shots().due(output.tick) && output.tick >= shot_from) {
        const auto writing = std::chrono::steady_clock::now();
        const std::string path = shots().prefix + std::to_string(output.tick)
                                 + (first ? std::string() : "_b" + std::to_string(between_index))
                                 + (output.enhanced_graphics ? "_enhanced" : "") + ".png";
        // As the screen shows it: the frame's alpha is what the GS blends wrote, not a picture's
        // coverage, and a viewer that shows it draws the sky full of the effects' blotches.
        std::vector<std::uint8_t> pixels;
        run_graphics(
            [&] { pixels = renderer::read_rgba(shown, width, height); }, output.source_frame
        );
        for (std::size_t i = 3; i < pixels.size(); i += 4) {
            pixels[i] = 0xFF;
        }
        scene::write_png_later(
            path,
            width,
            height,
            std::move(pixels),
            std::format("frame {} written to {}", output.tick, path)
        );
        shot_time += std::chrono::steady_clock::now() - writing;
    }
}

// Reports the diagnostic's blocking measurements after presentation.
void report_frame_cost(const FramePresentation& output) {
    const int width = output.width;
    const int height = output.height;
    if (frame_cost.on) {
        frame_cost.lap(frame_cost.show);
        if (++frame_cost.frames >= frame_cost.every) {
            const double n = frame_cost.frames * 0.001;
            const renderer::FrameStats stats =
                g->render.renderer ? g->render.renderer->last_stats() : renderer::FrameStats{};
            log::info(
                "cost tick {}: {:.3f} ms a frame over {} frames (scene {:.3f}, submit {:.3f}, "
                "gpu {:.3f}, show {:.3f}); {} draw calls, {} triangles, frame {}x{} x{}",
                g->kept->frame,
                (frame_cost.scene + frame_cost.submit + frame_cost.gpu + frame_cost.show) / n,
                frame_cost.frames,
                frame_cost.scene / n,
                frame_cost.submit / n,
                frame_cost.gpu / n,
                frame_cost.show / n,
                stats.draw_calls,
                stats.triangles,
                width,
                height,
                g->render.frame.samples()
            );
            frame_cost.frames = 0;
            frame_cost.scene = frame_cost.submit = frame_cost.gpu = frame_cost.show = 0.0;
        }
    }
}

// Why a move of `step` in a tick is drawn part way or where the tick left it (geometry.h kept_pace):
// "near" (within `limit`), "pace" (farther, but at the pace of the tick before), "off-pace" (farther,
// and `miss` units off that pace) or "no-pace" (farther, with no tick before to keep pace with).
const char* motion_reason(
    const renderer::Vec3& step, float limit_squared, const renderer::Vec3* pace, float& miss
) {
    miss = 0.0f;
    if (renderer::dot(step, step) <= limit_squared) {
        return "near";
    }
    if (pace == nullptr) {
        return "no-pace";
    }
    const renderer::Vec3 off{step[0] - (*pace)[0], step[1] - (*pace)[1], step[2] - (*pace)[2]};
    miss = std::sqrt(renderer::dot(off, off));
    return miss * miss <= limit_squared ? "pace" : "off-pace";
}

// OPENRAC_MOTION_LOG (log_motion, note_between).
bool motion_log_on() {
    static const bool& on = stats::probe(
        "OPENRAC_MOTION_LOG",
        "each frame drawn: tick, alpha, frame time, and whether the eye and the fastest moby were "
        "drawn part way or snapped (and why), in the log"
    );
    return on;
}

// Why between() last drew no frame (OPENRAC_MOTION_LOG, note_between).
std::string g_between_stop;

// OPENRAC_MOTION_LOG: each frame drawn, the tick it is drawn from, how far between ticks, the time
// since the frame before and what it cost, then the eye and the fastest moby of the tick (the ship
// in a flight): each drawn part way ("blend") or where the tick left it ("snap") and why
// (motion_reason; "cut" a turn too far, "new" no last place), its move in the tick and where it is
// drawn. Frames whose drawn places repeat or jump show stepping; long frame times, real drops.
void log_motion(const FramePresentation& output) {
    if (!motion_log_on() || !g || !g->kept) {
        return;
    }
    const Kept& k = *g->kept;
    static std::chrono::steady_clock::time_point last{};
    const double since =
        last.time_since_epoch().count() == 0
            ? 0.0
            : std::chrono::duration<double, std::milli>(output.started - last).count();
    last = output.started;
    const float t = output.alpha;

    // The eye.
    std::string eye = "eye none";
    if (k.has_state) {
        const auto& p1 = k.state.camera_position;
        if (!k.has_prev_state || t >= 1.0f) {
            eye = std::format(
                "eye snap {} at ({:.2f} {:.2f} {:.2f})",
                k.has_prev_state ? "tick" : "no-prev",
                p1[0],
                p1[1],
                p1[2]
            );
        } else {
            const auto& p0 = k.prev_state.camera_position;
            const renderer::Vec3 step{p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
            const renderer::Vec3* pace = camera_pace(k);
            float miss = 0.0f;
            const char* why = motion_reason(step, 64.0f, pace, miss);
            const float turn = renderer::dot(k.prev_state.forward, k.state.forward);
            const bool cut = turn < 0.7f;
            const bool blended =
                !cut && (std::string_view(why) == "near" || std::string_view(why) == "pace");
            const host::GameState drawn = blend_camera(k.prev_state, k.state, t, pace);
            eye = std::format(
                "eye {} {} step {:.2f} miss {:.2f} at ({:.2f} {:.2f} {:.2f})",
                blended ? "blend" : "snap",
                cut ? "cut" : why,
                std::sqrt(renderer::dot(step, step)),
                miss,
                drawn.camera_position[0],
                drawn.camera_position[1],
                drawn.camera_position[2]
            );
        }
    }

    // The moby that moved farthest into this tick.
    std::string moby = "moby none";
    float farthest = -1.0f;
    std::size_t which = 0;
    bool known = false;
    for (std::size_t i = 0; i < k.live.size() && i < k.addresses.size(); ++i) {
        const auto before = k.prev_matrices.find(k.addresses[i]);
        if (before == k.prev_matrices.end()) {
            continue;
        }
        const auto& m0 = before->second;
        const auto& m1 = k.live[i].matrix;
        const renderer::Vec3 step{m1[12] - m0[12], m1[13] - m0[13], m1[14] - m0[14]};
        const float d = renderer::dot(step, step);
        if (d > farthest) {
            farthest = d;
            which = i;
            known = true;
        }
    }
    // Every moby: how many were drawn where the tick left them (moved off their pace, or no pace).
    int snapped = 0, fresh = 0;
    for (std::size_t i = 0; i < k.live.size() && i < k.addresses.size(); ++i) {
        const auto before = k.prev_matrices.find(k.addresses[i]);
        if (before == k.prev_matrices.end()) {
            ++fresh;
            continue;
        }
        const auto& m0 = before->second;
        const auto& m1 = k.live[i].matrix;
        const renderer::Vec3 step{m1[12] - m0[12], m1[13] - m0[13], m1[14] - m0[14]};
        float miss = 0.0f;
        const std::string_view why = motion_reason(step, 16.0f, moby_pace(k, k.addresses[i]), miss);
        snapped += why != "near" && why != "pace" ? 1 : 0;
    }
    if (known) {
        const std::uint32_t address = k.addresses[which];
        const auto& m0 = k.prev_matrices.at(address);
        const auto& m1 = k.live[which].matrix;
        const renderer::Vec3 step{m1[12] - m0[12], m1[13] - m0[13], m1[14] - m0[14]};
        const renderer::Vec3* pace = moby_pace(k, address);
        float miss = 0.0f;
        const char* why = motion_reason(step, 16.0f, pace, miss);
        const bool blended =
            t < 1.0f && (std::string_view(why) == "near" || std::string_view(why) == "pace");
        const renderer::Mat4 drawn = t < 1.0f ? blend_matrix(m0, m1, t, pace) : m1;
        moby = std::format(
            "moby {:#x} of {} ({} snapped, {} new) {} {} step {:.2f} miss {:.2f} at ({:.2f} {:.2f} "
            "{:.2f})",
            address,
            k.live.size(),
            snapped,
            fresh,
            blended ? "blend" : "snap",
            t >= 1.0f ? "tick" : why,
            std::sqrt(farthest),
            miss,
            drawn[12],
            drawn[13],
            drawn[14]
        );
    } else if (!k.live.empty()) {
        moby = std::format("moby new ({} live, none with a last place)", k.live.size());
    }
    // The largest effect quad (the flight's planet, its world quad): its id, owner, whether the last
    // tick had it (drawn part way) and where its middle is drawn.
    std::string quad = "quad none";
    {
        float biggest = -1.0f;
        const renderer::EffectQuad* q = nullptr;
        for (const renderer::EffectQuad& e : k.effects) {
            const float dx = e.corner[0][0] - e.corner[3][0], dy = e.corner[0][1] - e.corner[3][1],
                        dz = e.corner[0][2] - e.corner[3][2];
            const float d = dx * dx + dy * dy + dz * dz;
            if (d > biggest) {
                biggest = d;
                q = &e;
            }
        }
        if (q != nullptr) {
            const auto before =
                q->id != 0 ? k.prev_effect_corners.find(q->id) : k.prev_effect_corners.end();
            const bool known = before != k.prev_effect_corners.end();
            float mid[3] = {0, 0, 0}, was[3] = {0, 0, 0};
            for (int c = 0; c < 4; ++c) {
                for (int i = 0; i < 3; ++i) {
                    mid[i] += q->corner[c][i] / 4.0f;
                    was[i] += known ? before->second[c][i] / 4.0f : q->corner[c][i] / 4.0f;
                }
            }
            const float tt = known && t < 1.0f ? t : 1.0f;
            int with_id = 0, with_prev = 0, owned = 0;
            for (const renderer::EffectQuad& e : k.effects) {
                with_id += e.id != 0 ? 1 : 0;
                with_prev += e.id != 0 && k.prev_effect_corners.contains(e.id) ? 1 : 0;
                owned += e.parent != 0 || e.attachment != 0 ? 1 : 0;
            }
            // A puff of a trail or a particle (an id of its own, no owner) that moved farthest in
            // the tick and is still drawn part way, drawn as set_effect_quads draws it (blend_effect_quad): where its middle is.
            std::string puff = "puff none";
            float farthest_puff = -1.0f;
            for (const renderer::EffectQuad& e : k.effects) {
                const auto last = e.id != 0 && e.parent == 0 && e.attachment == 0
                                      ? k.prev_effect_corners.find(e.id)
                                      : k.prev_effect_corners.end();
                if (last == k.prev_effect_corners.end()) {
                    continue;
                }
                float d = 0.0f;
                for (int i = 0; i < 3; ++i) {
                    const float m =
                        (e.corner[0][i] + e.corner[3][i] - last->second[0][i] - last->second[3][i])
                        * 0.5f;
                    d += m * m;
                }
                // A slot used again by a new puff is drawn whole (kMaximumEffectStep, 4 units).
                if (d > farthest_puff && d < 16.0f) {
                    farthest_puff = d;
                    renderer::EffectQuad drawn = e;
                    if (t < 1.0f) {
                        blend_effect_quad(drawn, last->second, t);
                    }
                    puff = std::format(
                        "puff {:#x} step {:.2f} at ({:.2f} {:.2f} {:.2f})",
                        e.id,
                        std::sqrt(d),
                        (drawn.corner[0][0] + drawn.corner[3][0]) * 0.5f,
                        (drawn.corner[0][1] + drawn.corner[3][1]) * 0.5f,
                        (drawn.corner[0][2] + drawn.corner[3][2]) * 0.5f
                    );
                }
            }
            quad = std::format(
                "{}; quads {} ({} with an id, {} of them in the last tick, {} owned); biggest id "
                "{:#x} parent {:#x} {} size {:.1f} step {:.2f} at ({:.2f} {:.2f} "
                "{:.2f})",
                puff,
                k.effects.size(),
                with_id,
                with_prev,
                owned,
                q->id,
                q->parent,
                known ? "blend" : (q->id == 0 ? "snap no-id" : "snap new"),
                std::sqrt(biggest),
                std::sqrt(
                    (mid[0] - was[0]) * (mid[0] - was[0]) + (mid[1] - was[1]) * (mid[1] - was[1])
                    + (mid[2] - was[2]) * (mid[2] - was[2])
                ),
                was[0] + (mid[0] - was[0]) * tt,
                was[1] + (mid[1] - was[1]) * tt,
                was[2] + (mid[2] - was[2]) * tt
            );
        }
    }
    // The sky's shells: the most any turned in the tick (degrees, its X axis), and whether this
    // frame turns them part way (blend_turning) or shows the tick's.
    std::string sky = "sky none";
    if (k.sky_shell_mask != 0) {
        float most = -1.0f;
        for (std::size_t i = 0; i < k.sky_shells.size(); ++i) {
            if ((k.sky_shell_mask & k.prev_sky_shell_mask & (1u << i)) == 0) {
                continue;
            }
            const auto& a = k.prev_sky_shells[i];
            const auto& b = k.sky_shells[i];
            const float la = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
            const float lb = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
            if (la > 0.0f && lb > 0.0f) {
                const float c = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (la * lb);
                most = std::max(most, std::acos(std::clamp(c, -1.0f, 1.0f)) * 57.29578f);
            }
        }
        sky = most < 0.0f ? std::format("sky {:#x} snap no-prev", k.sky_shell_mask)
                          : std::format(
                                "sky {:#x} {} turn {:.3f} deg",
                                k.sky_shell_mask,
                                t < 1.0f && most < 45.57f ? "blend" : "snap",
                                most
                            );
    }
    log::info(
        "motion tick {} alpha {:.3f} {} dt {:.2f} ms cost {:.2f} ms period {:.2f} ms level {} | {} "
        "| "
        "{} | {} | {}",
        k.frame,
        t,
        output.first ? "tick" : "between",
        since,
        pacing.cost * 1000.0,
        pacing.period.count() * 1000.0,
        k.level,
        eye,
        moby,
        sky,
        quad
    );
}

// Logs the submitted interpolation fraction using window-thread pacing state.
void log_pacing(const FramePresentation& output) {
    // OPENRAC_PACING: each frame shown, when (ms), how far between ticks, the refresh interval.
    static const bool& pacing_log =
        stats::probe("OPENRAC_PACING", "each frame shown and when, in the log");
    if (pacing_log) {
        static const auto t0 = output.started;
        log::info(
            "pace {} {:.2f} ms alpha {:.3f} {} refresh {:.2f} ms cost {:.2f} ms",
            g->kept->frame,
            std::chrono::duration<double, std::milli>(output.started - t0).count(),
            output.alpha,
            output.first ? "tick" : "between",
            pacing.interval * 1000.0,
            pacing.cost * 1000.0
        );
    }
}

RenderWorker& graphics_worker() {
    if (!g->render_worker) {
        g->render_worker = std::make_unique<RenderWorker>();
        log::info(
            "Vulkan rendering and presentation on a dedicated thread; one snapshot in flight"
        );
    }
    return *g->render_worker;
}

void wait_for_graphics(RenderWorker& worker) {
    while (!worker.ready_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(50))) {
        platform::pump_events();
    }
}

// Pins the immutable captured scene and transfers graphics ownership to its persistent worker.
void queue_frame(const std::shared_ptr<FramePresentation>& output, bool views_begun) {
    auto& worker = graphics_worker();
    const auto snapshot_started = std::chrono::steady_clock::now();
    std::function<void()> job = [owner = &g->render,
                                 scene = std::shared_ptr<const Kept>(g->kept),
                                 graphics = settings::get(),
                                 output,
                                 views_begun] {
        // The render thread's share of the frame, for the system's CPU clocks.
        const auto job_started = std::chrono::steady_clock::now();
        platform::add_this_thread();
        renderer::g_dump_draws = output->dump_draws;
        render_scene(*owner, *scene, graphics, *output, views_begun);
        sampler::note_thread("render");
        {
            const stats::Scope slow("simulated slow cpu");
            slow_cpu::pad();
        }
        platform::render_work_done(
            std::chrono::duration<double>(std::chrono::steady_clock::now() - job_started).count()
        );
    };
    stats::gauge(
        "render snapshot ms",
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - snapshot_started
        )
            .count()
    );
    g->pending_frame = output;
    std::string error;
    if (!worker.submit(std::move(job), error, output->source_frame)) {
        g->pending_frame.reset();
        g->render_failed = true;
        log::error("render thread: {}", error);
    }
}

// Builds the debug UI on the window thread after the world snapshot is complete.
void draw_overlay(const FramePresentation& output) {
    const int window_width = output.window_width;
    const int window_height = output.window_height;
    const Box box = output.box;
    // [debug tools] The overlay, over the frame in the window.
    debug_overlay::draw(
        {g->window->sdl(),
         window_width,
         window_height,
         box.x,
         box.y,
         box.width,
         box.height,
         g->render.renderer.get(),
         &output.camera,
         g->render.frame.width(),
         g->render.frame.height(),
         g->render.last_world.width(),
         g->render.last_world.height(),
         g->swap != platform::SwapInterval::Immediate,
         interpolating(),
         pacing.interval,
         pacing.cost,
         pacing.period.count(),
         &g->picker,
         &g->render.level,
         output.picker_live,
         output.picker_palette,
         output.picker_level,
         output.tick,
         output.picker_layers,
         output.freecam,
         output.camera_level,
         output.enhanced_graphics}
    );
}

// Orders the window-thread UI and render-thread presentation for a completed world snapshot.
void present_frame(FramePresentation& output) {
    const crash::BreadcrumbScope breadcrumbs(output.source_frame);
    // A between-tick job keeps draw logging suppressed through its later presentation too.
    struct RestoreDrawDump {
        bool previous = renderer::g_dump_draws;

        ~RestoreDrawDump() { renderer::g_dump_draws = previous; }
    } restore_dump;

    renderer::g_dump_draws = output.dump_draws;
    // What is shown: the frame with one sample a pixel (a multisampled one is resolved here, once),
    // with the port's own overlays over it.
    const unsigned shown = output.shown;
    const int width = output.width;
    const int height = output.height;
    const int window_width = output.window_width;
    const int window_height = output.window_height;
    const float aspect = output.aspect;
    const auto resumed = std::chrono::steady_clock::now();
    const auto started = output.started + (resumed - output.finished);
    frame_cost.scene += output.scene_seconds;
    frame_cost.submit += output.record_seconds;
    frame_cost.gpu += output.gpu_seconds;
    frame_cost.mark = resumed;
    // The window thread's own parts of a frame, measured apart (performance captures).
    {
        const stats::Scope menus("menus");
        mod_overlay::draw(
            output.camera,
            output.has_camera,
            output.world_under_menu,
            output.overlay_phase,
            shown,
            width,
            height,
            aspect
        );
        debug_menu::draw(shown, width, height);
        run_graphics(
            [&] {
                mod_menu::draw(shown, width, height);
                if (settings::get().show_fps != settings::FrameCounter::Off) {
                    const ui_colors::Colors& colors = ui_colors::launcher();
                    renderer::ui::fps_counter::draw(
                        shown, width, height, colors.amber, colors.ink_deep
                    );
                }
            },
            output.source_frame
        );
    }
    {
        const stats::Scope capture("capture");
        capture_frame(output);
        slow_frame();
    }
    crash::breadcrumb(crash::BreadcrumbThread::Main, "presenting frame", output.source_frame);
    run_graphics(
        [=] { present(shown, width, height, window_width, window_height, false, aspect); },
        output.source_frame
    );
    {
        const stats::Scope touch("touch pad");
        draw_touch_pad(window_width, window_height);
    }
    draw_overlay(output);
    const stats::Scope swap_scope("swap");
    swapped(started, output.source_frame);
    input_latency::shown(output.tick);  // [debug tools] OPENRAC_INPUT_LATENCY
    // A mod's views were submitted with the frame: the mod may hand them on (nothing without one).
    finish_views();
    report_frame_cost(output);
}

}  // namespace

void run_graphics(
    std::function<void()> job, std::int64_t frame, std::source_location submitted_at
) {
    if (gl::is_vulkan()) {
        run_vulkan_graphics(std::move(job), frame, submitted_at);
    } else {
        job();
    }
}

void run_vulkan_graphics(
    std::function<void()> job, std::int64_t frame, std::source_location submitted_at
) {
    if (!separate_vulkan_thread()) {
        job();
        return;
    }
    auto& worker = graphics_worker();
    std::string error;
    if (worker.submit(std::move(job), error, frame, submitted_at)) {
        wait_for_graphics(worker);
        if (worker.finish(error)) {
            return;
        }
    }
    // Movie and image callbacks can catch this before finish_render sees it.
    g->render_failed = true;
    throw std::runtime_error(error);
}

void swap_rendered_frame(std::int64_t frame) {
    int width = 0, height = 0;
    g->window->drawable_size(width, height);
#if defined(OPENRAC_IOS)
    if (width > 0 && height > 0 && threaded_rendering()) {
        g->window->restore_vulkan_surface();
    }
#endif
    run_graphics([=] { g->window->swap(width, height); }, frame);
}

#if defined(OPENRAC_IOS)
void drain_graphics_for_background() try {
    if (!g || g->render_failed || gl::glFinish == nullptr) {
        return;
    }
    if (!gl::is_vulkan() || !separate_vulkan_thread()) {
        gl::glFinish();
        return;
    }

    // Keep this synchronous inside the UIKit notification. The worker finishes any outstanding
    // job first. Surface recovery on iOS runs on the game thread, so this wait needs no SDL pump.
    auto& worker = graphics_worker();
    std::string error;
    if (!worker.submit([] { gl::glFinish(); }, error) || !worker.finish(error)) {
        g->render_failed = true;
        log::error("cannot drain graphics for iOS backgrounding: {}", error);
    }
} catch (const std::exception& failure) {
    g->render_failed = true;
    log::error("cannot drain graphics for iOS backgrounding: {}", failure.what());
} catch (...) {
    g->render_failed = true;
    log::error("cannot drain graphics for iOS backgrounding: unknown exception");
}
#endif

bool render_pending() {
    return g && static_cast<bool>(g->pending_frame);
}

bool finish_render() try {
    // [debug tools] A simulated slow processor (common/slow_cpu.h): the window thread's work since
    // it last waited (the game's tick, the next frame's preparation, the frame it hands over),
    // padded before it waits for the render thread, as a slower processor would still be busy
    // while the render thread draws. Measured only when no render job holds the profiler's
    // section stack.
    {
        std::optional<stats::Scope> slow;
        if (!render_pending()) {
            slow.emplace("simulated slow cpu");
        }
        slow_cpu::pad();
    }
    // Synchronous frames and initialisation have no handoff to drain.
    if (!render_pending()) {
        return !g || !g->render_failed;
    }
    const auto started = std::chrono::steady_clock::now();
    wait_for_graphics(*g->render_worker);
    std::string error;
    const bool complete = g->render_worker->finish(error);
    auto output = std::move(g->pending_frame);
    // A worker failure has already relinquished all graphics resources.
    if (!complete) {
        log::error("render thread: {}", error);
        g->render_failed = true;
        return false;
    }
    const std::chrono::duration<double> waited = std::chrono::steady_clock::now() - started;
    stats::gauge("render thread wait ms", waited.count() * 1000.0);
    platform::work_waited(waited.count());  // the render thread's work, reported as its own
    present_frame(*output);
    return true;
} catch (const std::exception& failure) {
    g->render_failed = true;
    log::error("cannot finish render: {}", failure.what());
    crash::report(failure.what(), crash::Kind::Exception);
    return false;
} catch (...) {
    g->render_failed = true;
    log::error("cannot finish render: unknown exception");
    crash::report("cannot finish render: unknown exception", crash::Kind::Exception);
    return false;
}

void draw_kept(bool first, bool held) try {
    sampler::note_thread("window");  // [debug tools] sampled while a capture runs
    // One in-flight snapshot bounds both latency and ownership of the current scene (and the
    // simulated slow processor's share of the window thread is spent there).
    if (!finish_render()) {
        return;
    }
    // The frame's beginning, for the low-latency frames' sleep: a tick's frame, drawn as soon as
    // the tick's time and input are read, is not slept (between() sleeps the others). With the
    // render thread too: the window thread holds the graphics here (finish_render), and the job it
    // hands over is queued by its present_frame, after the next finish_render.
    if (settings::get().low_latency_frames) {
        g->window->latency_begin(std::exchange(g_latency_slept, 0.0));
    }
    auto output = std::make_shared<FramePresentation>();
    output->source_frame = static_cast<std::int64_t>(g->kept->frame);
    output->started = std::chrono::steady_clock::now();
    output->first = first;
    // The phase of the tick the frame is drawn from, not the game's now (a queued frame of play is
    // shown after the game opened a menu); none for a picture left standing over a load.
    output->overlay_phase = held ? 0u : g->kept->overlay_phase;
    output->camera_debug = debug_overlay::camera_debug_frame();
    output->dump_draws = renderer::g_dump_draws;
    const auto display = next_display(output->started);
    output->alpha = interpolating() ? alpha_at(display) : 1.0f;
    // Not while the debug overlay holds the game: no tick comes to correct the guess.
    const CameraTurn turn = g_held ? CameraTurn{} : latch_camera(*g->kept, output->alpha, display);
    output->latch_yaw = turn.yaw;
    output->latch_pivot = turn.pivot;
    output->latch_pitch = turn.pitch;
    g->window->drawable_size(output->window_width, output->window_height);
    output->aspect = picture_aspect(output->window_width, output->window_height);
    output->box = frame_box(output->window_width, output->window_height, output->aspect);
    debug_overlay::begin(first, g_held, output->alpha, g->kept->frame);
    pacing.last_alpha = output->alpha;
    frame_cost.mark = output->started;
    output->measure_cost = frame_cost.on;
    log_pacing(*output);
    log_motion(*output);
    // OpenGL contexts retain the original window-thread path.
    if (!threaded_rendering()) {
        crash::breadcrumb(
            crash::BreadcrumbThread::Main, "synchronous render started", output->source_frame
        );
        render_scene(g->render, *g->kept, settings::get(), *output);
        crash::breadcrumb(
            crash::BreadcrumbThread::Main, "synchronous render finished", output->source_frame
        );
        present_frame(*output);
        return;
    }
    const bool mod_views = views_wanted();
    if (mod_views) {
        begin_views();
    }
    queue_frame(output, mod_views);
    if (mod_views) {
        // Complete the view callbacks before the next game tick can run mod code.
        [[maybe_unused]] const bool presented = finish_render();
    }

} catch (const std::exception& failure) {
    g->pending_frame.reset();
    g->render_failed = true;
    log::error("cannot prepare render snapshot: {}", failure.what());
    crash::report(failure.what(), crash::Kind::Exception);
} catch (...) {
    g->pending_frame.reset();
    g->render_failed = true;
    log::error("cannot prepare render snapshot: unknown exception");
    crash::report("cannot prepare render snapshot: unknown exception", crash::Kind::Exception);
}

// ---- Between ticks ----

void set_tick(
    std::chrono::steady_clock::time_point tick,
    std::chrono::duration<double> period,
    std::chrono::duration<double> late,
    std::chrono::duration<double> work
) {
    pacing.tick = tick;
    pacing.period = period;
    pacing.late = late;
    pacing.tick_work = std::max(work.count(), 0.9 * pacing.tick_work);
}

bool interpolating() {
    return !one_frame_a_tick() && settings::get().frame_rate != settings::FrameRate::Original && g
           && g->window;
}

std::chrono::steady_clock::duration take_shot_time() {
    return std::exchange(shot_time, std::chrono::steady_clock::duration{});
}

bool wants_between() {
    // A skipped capture keeps the last complete picture for pause, but cannot interpolate a new tick.
    // Without a camera, only for a game whose menus are 2D alone (profile.h): there is nothing to
    // move between the ticks, and the frames are the tick's picture shown at the player's rate.
    return interpolating() && g->kept->frame + 1 == g->index && g->kept->has_state
           && (g->kept->has_camera || g->render.profile->frames_between_without_camera());
}

int between(std::chrono::steady_clock::time_point next_tick) {
    if (!g) {
        g_between_stop = "no frontend";
        return 0;
    }
    const bool asynchronous = threaded_rendering();
    // Present completed work while waiting for the tick; unfinished jobs may cross its boundary.
    if (render_pending()) {
        if (!g->render_worker->ready_until(next_tick)) {
            g_between_stop = "render pending past the tick";
            return 0;
        }
        if (!finish_render()) {
            return -1;
        }
    }
    auto now = std::chrono::steady_clock::now();
    // The frame cap: a frame not due before the next tick is left to after it; one due sooner is
    // waited for here, to its time.
    if (const auto start = cap_start_time(); now < start) {
        if (start >= next_tick) {
            g_between_stop = "frame cap past the tick";
            return 0;
        }
        precise_wait::until(start);
        platform::work_resumed();
        now = std::chrono::steady_clock::now();
    }
    // Low-latency frames: the frame begins as late as lets it be queued just as the graphics card
    // is free for it, so that what it draws for and the input it reads are as fresh as they can
    // be, and never after the next tick is due (Window::latency_sleep). With the render thread as
    // well: the job before it is finished (above), and the window thread reads the time and the
    // input for the next one below. The sleep is a wait, not work, for the system's CPU clocks.
    double slept = 0.0;
    if (settings::get().low_latency_frames) {
        slept = g->window->latency_sleep(next_tick);
        platform::work_waited(slept);
        now = std::chrono::steady_clock::now();
    }
    // Do not queue another interpolation job once simulation is due to resume.
    if (asynchronous && now >= next_tick) {
        g_between_stop = "render thread past the tick";
        return 0;
    }
    // A frame only for a refresh the kept tick is still shown at (the next tick's, with the lag;
    // without it, any before the next tick's own frame).
    const auto display = next_display(now);
    if (display >= between_limit(
            next_tick, pacing.period, lag_ticks(), pacing.cost, pacing.interval, pacing.last_swap
        )) {
        if (motion_log_on()) {
            const auto ms = [](std::chrono::steady_clock::duration d) {
                return std::chrono::duration<double, std::milli>(d).count();
            };
            g_between_stop = std::format(
                "display past the limit (from the tick: now {:.2f} ms, display {:.2f} ms, next "
                "tick "
                "{:.2f} ms, limit {:.2f} ms, last swap {:.2f} ms, interval {:.2f} ms)",
                ms(now - pacing.tick),
                ms(display - pacing.tick),
                ms(next_tick - pacing.tick),
                ms(between_limit(
                       next_tick,
                       pacing.period,
                       lag_ticks(),
                       pacing.cost,
                       pacing.interval,
                       pacing.last_swap
                   )
                   - pacing.tick),
                ms(pacing.last_swap - pacing.tick),
                pacing.interval * 1000.0
            );
        }
        return 0;
    }
    // Synchronous rendering must finish before the tick (without a swap that waits, or with the
    // low-latency frames, whose next tick starts on time); a frame held by its showing must be
    // shown before it, or no later than it may wait for one (tick_slip). Owned Vulkan jobs may
    // overlap its simulation.
    const auto ready = cap_edge() == CapEdge::Present
                           ? display
                           : now
                                 + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                     std::chrono::duration<double>(pacing.cost)
                                 );
    if (!asynchronous
        && (g->swap == platform::SwapInterval::Immediate || lag_ticks() == kLowLatencyLagTicks)
        && ready >= next_tick + slip()) {
        g_between_stop = "would be late";
        return 0;
    }
    // The scene has arrived at the kept tick's: another frame would be the same picture again.
    if (!pacing.first_pending && pacing.last_alpha >= 1.0f && alpha_at(display) >= 1.0f) {
        g_between_stop = "already at the tick";
        return 0;
    }
    {
        const stats::Scope events("events");
        SDL_Event event;
        while (platform::poll_event(event)) {
            if (event.type == SDL_EVENT_QUIT) {
                return -1;
            }
            const bool consumed = window_key(event) || debug_overlay::event(event)
                                  || debug_menu::event(event, openrac_game_loaded_overlay());
            debug_overlay::camera_debug_event(event);
            if (consumed) {
                continue;
            }
            g->input.handle_event(event);
        }
        apply_mouse_capture();
    }
    const bool dump = renderer::g_dump_draws;
    renderer::g_dump_draws = false;
    const bool first = pacing.first_pending;
    pacing.first_pending = false;
    {
        const profiling::Zone trace("Queue interpolated frame");
        g_latency_slept = slept;
        draw_kept(first);
    }
    renderer::g_dump_draws = dump;
    // The caller can wait up to the tick deadline on its next iteration, then resume simulation.
    return g->render_failed ? -1 : 1;
}

// OPENRAC_MOTION_LOG: each tick, the game's work for it (`work`, seconds: from the end of the last
// wait to its vertical blank) and the frames drawn between it and the next; none, why: not wanted
// (wants_between's conditions) or where between() stopped (g_between_stop).
void note_between(int frames, double work, double entered_late, double set_late) {
    if (!motion_log_on() || !g || !g->kept) {
        return;
    }
    const bool wanted = wants_between();
    log::info(
        "motion tick {} work {:.2f} ms, late {:.2f} ms at the blank, {:.2f} ms at the tick's time, "
        "{} "
        "frames between{}{} (period {:.2f} ms, cost {:.2f} ms)",
        g->kept->frame,
        work * 1000.0,
        entered_late * 1000.0,
        set_late * 1000.0,
        frames,
        frames != 0 ? "" : ", stopped: ",
        frames != 0 ? ""
                    : (wanted ? std::string_view(g_between_stop) : std::string_view("not wanted")),
        pacing.period.count() * 1000.0,
        pacing.cost * 1000.0
    );
}

// [debug tools] While the overlay holds the game (pause): the last tick drawn again, the window's
// events read, until it lets a tick through. False once the window was closed.
bool hold() {
    // Pausing requires a complete presented frame before the debug UI reads it.
    if (debug_overlay::holding() && !finish_render()) {
        return false;
    }
    while (g && g->window && debug_overlay::holding()) {
        SDL_Event event;
        while (platform::poll_event(event)) {
            if (event.type == SDL_EVENT_QUIT) {
                return false;
            }
            const bool consumed = window_key(event) || debug_overlay::event(event);
            debug_overlay::camera_debug_event(event);
            if (consumed) {
                continue;
            }
            g->input.handle_event(event);
        }
        apply_mouse_capture();  // the pointer as the menu, now open or shut, wants it
        if (debug_overlay::take_settings_changed()) {
            apply_settings();  // the drawer is open over the paused game
        }
        const auto started = std::chrono::steady_clock::now();
        const bool dump = renderer::g_dump_draws;
        renderer::g_dump_draws = false;
        g_held = true;
        draw_kept(false);
        const bool displayed = finish_render();
        if (!displayed) {
            return false;
        }
        g_held = false;
        renderer::g_dump_draws = dump;
        // No faster than the display shows them (a 120 Hz screen's drawer moves at 120), and no
        // faster than 60 a second where its rate is not known.
        const double interval = pacing.interval > 0.0 ? pacing.interval : 1.0 / 60.0;
        std::this_thread::sleep_until(
            started
            + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(interval * 0.9)
            )
        );
    }
    return true;
}

}  // namespace openrac::frontend
