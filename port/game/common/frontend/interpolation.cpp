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
// Pacing, input and presentation stay on the window thread. Vulkan world recording consumes an
// owned snapshot on RenderWorker; finish_render returns exclusive graphics ownership before UI work.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <SDL3/SDL.h>

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
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/pacing.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "game/common/frontend/state.h"
#include "mod_menu.h"
#include "mod_overlay.h"
#include "openrac/game_host.h"
#include "platform/performance_hint.h"
#include "platform/pump.h"
#include "platform/window.h"
#include "renderer/direct.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "renderer/shadows.h"
#include "renderer/texture_pool.h"
#include "renderer/ui/fps_counter.h"
#include "scene/level_renderer.h"
#include "scene/screenshot.h"
#include "settings.h"

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

// Vulkan jobs own their snapshots; mod views share the window thread and graphics queue.
bool threaded_rendering() {
    static const bool requested = env::flag(env::Var::RenderThread);
    return gl::is_vulkan() && requested && !views_wanted();
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

// How far behind the kept tick the scene runs: none with the player's low-latency frames (not
// for the render thread's jobs, which may be shown after the next tick starts), else half a tick.
double lag_ticks() {
    return settings::get().low_latency_frames && !threaded_rendering() ? kLowLatencyLagTicks
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
void swapped(std::chrono::steady_clock::time_point started) {
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
    g->window->swap();
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
    const auto* lighting = g ? g->scene.level_lighting() : nullptr;
    return lighting ? std::string(lighting->load_error()) : std::string{};
}

bool enhanced_graphics_available() {
    return g && gl::enhanced_rendering_available() && g->scene.probe_data_available();
}

namespace {

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
    // OPENRAC_SHOT_FROM=FRAME: no shots before that frame.
    static const std::uint64_t shot_from = env_frame(env::Var::ShotFrom).value_or(0);
    if ((first || shot_between) && shots().due(output.tick) && output.tick >= shot_from) {
        const std::string path = shots().prefix + std::to_string(output.tick)
                                 + (first ? std::string() : "_b" + std::to_string(between_index))
                                 + ".png";
        // As the screen shows it: the frame's alpha is what the GS blends wrote, not a picture's
        // coverage, and a viewer that shows it draws the sky full of the effects' blotches.
        std::vector<std::uint8_t> pixels = renderer::read_rgba(shown, width, height);
        for (std::size_t i = 3; i < pixels.size(); i += 4) {
            pixels[i] = 0xFF;
        }
        scene::write_png(path, width, height, pixels);
        log::info("frame {} written to {}", output.tick, path);
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
                g->renderer ? g->renderer->last_stats() : renderer::FrameStats{};
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
                g->frame.samples()
            );
            frame_cost.frames = 0;
            frame_cost.scene = frame_cost.submit = frame_cost.gpu = frame_cost.show = 0.0;
        }
    }
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

// Pins the immutable captured scene and transfers graphics ownership to its persistent worker.
void queue_frame(const std::shared_ptr<FramePresentation>& output) {
    // The worker is persistent; every job owns poses, packets, textures and graphics settings.
    if (!g->render_worker) {
        // These renderer capability caches must be populated before the game and worker can query them.
        [[maybe_unused]] const int samples = renderer::FrameBuffer::max_samples();
        [[maybe_unused]] const float anisotropy = renderer::TexturePool::max_anisotropy();
        g->render_worker = std::make_unique<RenderWorker>();
        log::info("Vulkan world rendering on a dedicated thread; one snapshot in flight");
    }
    const auto snapshot_started = std::chrono::steady_clock::now();
    auto snapshot =
        std::make_shared<RenderSnapshot>(RenderSnapshot{g->kept, settings::get(), output});
    stats::gauge(
        "render snapshot ms",
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - snapshot_started
        )
            .count()
    );
    g->pending_frame = output;
    std::string error;
    State& owner = *g;
    const bool queued = g->render_worker->submit(
        [&owner, snapshot = std::move(snapshot)] {
            // The render thread's share of the frame, for the system's CPU clocks
            // (performance_hint.h).
            const auto job_started = std::chrono::steady_clock::now();
            platform::add_this_thread();
            renderer::g_dump_draws = snapshot->presentation->dump_draws;
            render_scene(owner, *snapshot->scene, snapshot->graphics, *snapshot->presentation);
            sampler::note_thread("render");  // [debug tools] sampled while a capture runs
            // [debug tools] The render thread's share of a simulated slow processor.
            {
                const stats::Scope slow("simulated slow cpu");
                slow_cpu::pad();
            }
            platform::render_work_done(
                std::chrono::duration<double>(std::chrono::steady_clock::now() - job_started)
                    .count()
            );
        },
        error
    );
    // Job rejection means a previous failure, never an unbounded queue.
    if (!queued) {
        g->pending_frame.reset();
        g->render_failed = true;
        log::error("render thread: {}", error);
    }
}

// Draws the debug UI after the render worker has released all referenced graphics state.
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
         g->renderer.get(),
         &output.camera,
         g->frame.width(),
         g->frame.height(),
         g->last_world.width(),
         g->last_world.height(),
         g->swap != platform::SwapInterval::Immediate,
         interpolating(),
         pacing.interval,
         pacing.cost,
         pacing.period.count(),
         &g->picker,
         &g->level,
         output.picker_live,
         output.picker_palette,
         output.picker_level,
         output.tick,
         output.picker_layers}
    );
}

// Composites debug UI and presents on the window thread, after exclusive graphics ownership returns.
void present_frame(FramePresentation& output) {
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
        mod_overlay::draw(output.camera, output.has_camera, shown, width, height, aspect);
        debug_menu::draw(shown, width, height);
        mod_menu::draw(shown, width, height);
        if (settings::get().show_fps != settings::FrameCounter::Off) {
            renderer::ui::fps_counter::draw(shown, width, height);
        }
    }
    {
        const stats::Scope capture("capture");
        capture_frame(output);
        slow_frame();
    }
    present(shown, width, height, window_width, window_height, false, aspect);
    {
        const stats::Scope touch("touch pad");
        draw_touch_pad(window_width, window_height);
    }
    draw_overlay(output);
    const stats::Scope swap_scope("swap");
    swapped(started);
    // A mod's views were submitted with the frame: the mod may hand them on (nothing without one).
    finish_views();
    report_frame_cost(output);
}

}  // namespace

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
    return false;
} catch (...) {
    g->render_failed = true;
    log::error("cannot finish render: unknown exception");
    return false;
}

void draw_kept(bool first) try {
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
    output->started = std::chrono::steady_clock::now();
    output->first = first;
    output->dump_draws = renderer::g_dump_draws;
    const auto display = next_display(output->started);
    output->alpha = interpolating() ? alpha_at(display) : 1.0f;
    // Not while the debug overlay holds the game: no tick comes to correct the guess.
    const CameraTurn turn = g_held ? CameraTurn{} : latch_camera(*g->kept, output->alpha, display);
    output->latch_yaw = turn.yaw;
    output->latch_pivot = turn.pivot;
    g->window->drawable_size(output->window_width, output->window_height);
    output->aspect = picture_aspect(output->window_width, output->window_height);
    output->box = frame_box(output->window_width, output->window_height, output->aspect);
    debug_overlay::begin(first, g_held, output->alpha, g->kept->frame);
    pacing.last_alpha = output->alpha;
    frame_cost.mark = output->started;
    output->measure_cost = frame_cost.on;
    log_pacing(*output);
    // OpenGL contexts retain the original window-thread path.
    if (!threaded_rendering()) {
        render_scene(*g, *g->kept, settings::get(), *output);
        present_frame(*output);
        return;
    }
    queue_frame(output);

} catch (const std::exception& failure) {
    g->pending_frame.reset();
    g->render_failed = true;
    log::error("cannot prepare render snapshot: {}", failure.what());
} catch (...) {
    g->pending_frame.reset();
    g->render_failed = true;
    log::error("cannot prepare render snapshot: unknown exception");
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

bool wants_between() {
    // A skipped capture keeps the last complete picture for pause, but cannot interpolate a new tick.
    return interpolating() && g->kept->frame + 1 == g->index && g->kept->has_state
           && g->kept->has_camera;
}

int between(std::chrono::steady_clock::time_point next_tick) {
    if (!g) {
        return 0;
    }
    const bool asynchronous = threaded_rendering();
    // Present completed work while waiting for the tick; unfinished jobs may cross its boundary.
    if (render_pending()) {
        if (!g->render_worker->ready_until(next_tick)) {
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
        return 0;
    }
    // A frame only for a refresh the kept tick is still shown at (the next tick's, with the lag;
    // without it, any before the next tick's own frame).
    const auto display = next_display(now);
    if (display >= between_limit(
            next_tick, pacing.period, lag_ticks(), pacing.cost, pacing.interval, pacing.last_swap
        )) {
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
        return 0;
    }
    // The scene has arrived at the kept tick's: another frame would be the same picture again.
    if (!pacing.first_pending && pacing.last_alpha >= 1.0f && alpha_at(display) >= 1.0f) {
        return 0;
    }
    {
        const stats::Scope events("events");
        SDL_Event event;
        while (platform::poll_event(event)) {
            if (event.type == SDL_EVENT_QUIT) {
                return -1;
            }
            if (window_key(event) || debug_overlay::event(event)
                || debug_menu::event(event, openrac_game_loaded_overlay())) {
                continue;
            }
            g->input.handle_event(event);
        }
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
            if (window_key(event) || debug_overlay::event(event)) {
                continue;
            }
            g->input.handle_event(event);
        }
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
