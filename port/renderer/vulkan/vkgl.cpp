// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// renderer/gl.h and renderer/world/gl_world.h carried out with Vulkan (vkgl.h says how GL's model
// becomes Vulkan's). Two command buffers a frame: `upload`, where what changes a texture nothing
// this frame has used yet goes (so it needs no pause of the drawing), run first, then `main`,
// where everything else goes in GL's order. Every image keeps the layout it was last left in,
// and is moved to the one an operation needs just before it.
//
// This file holds the backend's state `g` and the functions vkgl.h declares (get_proc is in
// vkgl_gl.cpp, beside the table); the rest is in the vkgl_*.cpp files, by concern, which share
// vkgl_internal.h.

#include "renderer/vulkan/vkgl.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <volk.h>

#include <SDL3/SDL.h>

#include "common/env.h"
#include "common/log.h"
#include "common/precise_wait.h"
#include "common/stats.h"
#include "renderer/gpu_draw.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {

namespace detail {

using namespace openrac::gl;

Context* g = nullptr;

}  // namespace detail

using namespace detail;

// ---- The device, the screen and the frames ----

// Back in the foreground: the window SDL holds is a new one (Android).
bool SDLCALL on_event(void*, SDL_Event* event) {
    if (g != nullptr && event->type == SDL_EVENT_DID_ENTER_FOREGROUND) {
        g->surface_lost = true;
    }
    return true;
}

// Whether the vendor's GPUs draw in tiles (tile_based()): Arm, Qualcomm, Apple and Imagination.
// OPENRAC_TILE_BASED decides instead when it is set: 1 for yes, anything else for no.
bool guess_tile_based(std::uint32_t vendor) {
    if (const auto chosen = env::value(env::Var::TileBased)) {
        return *chosen == "1";
    }
    constexpr std::array<std::uint32_t, 4> kTiled = {0x13B5, 0x5143, 0x106B, 0x1010};
    return std::find(kTiled.begin(), kTiled.end(), vendor) != kTiled.end();
}

namespace {

// Startup reports recording/allocation failures through open()'s existing error return.
bool begin_initial_frame(const Config& config, std::string& error) {
    try {
        int width = config.width;
        int height = config.height;

        // A window's actual pixel dimensions can differ from its requested dimensions.
        if (config.window != nullptr) {
            SDL_GetWindowSizeInPixels(config.window, &width, &height);
        }

        tracy_gpu_open();
        begin_buffers();

        // The initial attachments and dummy descriptors must exist before recording any draw.
        if (!make_dummies() || !make_default_framebuffer(std::max(width, 1), std::max(height, 1))) {
            error = "could not make the default framebuffer";
            return false;
        }

        return true;
    } catch (const std::exception& failure) {
        error = failure.what();
        return false;
    }
}

}  // namespace

bool open(const Config& config, std::string& error) {
    if (g != nullptr) {
        error = "a Vulkan context is open already";
        return false;
    }
    const VkResult loaded =
        config.get_instance_proc_addr != nullptr
            ? (volkInitializeCustom(
                   reinterpret_cast<PFN_vkGetInstanceProcAddr>(config.get_instance_proc_addr)
               ),
               VK_SUCCESS)
            : volkInitialize();
    if (loaded != VK_SUCCESS) {
        error = "no Vulkan loader on this system";
        return false;
    }
    g = new Context();
    g->window = config.window;
    g->interval = config.vsync ? 1 : 0;
    const bool validation = (config.validation || env::flag(env::Var::VulkanValidation))
                            && layer_available("VK_LAYER_KHRONOS_validation");
    bool ok = make_instance(config, validation, error);
    if (ok && config.window != nullptr) {
        ok = make_surface(config.window, error);
    }
    ok = ok && pick_device(error) && make_device(error) && make_frames();
    if (ok) {
        ok = begin_initial_frame(config, error);
    }
    if (ok && config.window != nullptr && !make_swapchain()) {
        // Minimised at the start: the swapchain comes with the first frame shown.
        g->swapchain_stale = true;
    }
    if (!ok) {
        destroy_all();
        delete g;
        g = nullptr;
        return false;
    }
    describe_device();
    g->tile_based = guess_tile_based(g->properties.vendorID);
    g->split_pass_timing = env::flag(env::Var::GpuPassTiming);
    if (g->timestamp_bits) {
        stats::set_gpu_hooks({timing_begin, timing_end, nullptr});
    }
    // GL's defaults that are not zero.
    g->state.viewport[2] = g->state.scissor[2] = g->default_width;
    g->state.viewport[3] = g->state.scissor[3] = g->default_height;
    g->open = true;
#if defined(__ANDROID__)
    if (config.window != nullptr) {
        SDL_AddEventWatch(on_event, nullptr);
    }
#endif
    log::info(
        "vulkan: {} ({}), Vulkan {}{}",
        g->device_name,
        g->driver_version,
        g->api_version,
        validation ? ", with the validation layer" : ""
    );
    return true;
}

void close() {
    if (g == nullptr) {
        return;
    }
#if defined(__ANDROID__)
    SDL_RemoveEventWatch(on_event, nullptr);
#endif
    destroy_all();
    delete g;
    g = nullptr;
}

bool active() {
    return g != nullptr && g->open;
}

bool gpu_driven_active() {
    if (!active() || !g->features.drawIndirectFirstInstance) {
        return false;
    }
    static const bool& off = stats::probe(
        "OPENRAC_GPU_DRIVEN_OFF", "Reference: CPU instance visibility and direct mesh draws"
    );
    return !off && gpu_make_engine();
}

// Returns the identity a reusable visibility batch must belong to, opening a recording if needed.
std::uint64_t gpu_instance_recording() {
    // CPU rendering has no reusable GPU instance batches.
    if (!gpu_driven_active()) {
        return 0;
    }

    prepare_frame();
    return g->recording;
}

bool indexed_viewports_supported(unsigned count) {
    return active() && g->viewport_index_supported && count > 0
           && count <= g->properties.limits.maxViewports && material_enabled();
}

bool gpu_materials_active() {
    return active() && material_enabled();
}

void indexed_viewports(std::span<const std::array<int, 4>> rectangles) {
    if (!rectangles.empty()
        && !indexed_viewports_supported(static_cast<unsigned>(rectangles.size()))) {
        throw std::runtime_error("indexed viewports unavailable");
    }
    gpu_flush_draws();
    g->indexed_viewports.assign(rectangles.begin(), rectangles.end());
    g->bound_pipeline = VK_NULL_HANDLE;
}

renderer::GpuInstanceBatch gpu_instances(
    unsigned source,
    std::span<const renderer::GpuCullBounds> bounds,
    const renderer::GpuCullView& view
) {
    if (view.plane_count > 6) {
        throw std::runtime_error("invalid GPU culling plane count");
    }
    return gpu_make_batch(source, bounds, view);
}

bool draw_gpu_instances(renderer::GpuInstanceBatch batch, unsigned first, unsigned count) {
    return active() && gpu_submit_draw(batch, first, count);
}

bool draw_gpu_instance_ranges(
    renderer::GpuInstanceBatch batch, std::span<const renderer::GpuIndexRange> ranges
) {
    return active() && gpu_submit_ranges(batch, ranges);
}

bool draw_indexed_indirect(unsigned arguments, unsigned index_count) {
    // Commands must reside in GPU storage, and the pipeline setup needs a representable index count.
    if (!active() || index_count == 0
        || index_count > static_cast<unsigned>(std::numeric_limits<gl::GLsizei>::max())) {
        return false;
    }

    const auto* buffer = g->buffers.find(arguments);

    // The command contains five uint32 words, written by compute before this call.
    if (!buffer || !buffer->storage || buffer->size < sizeof(VkDrawIndexedIndirectCommand)) {
        return false;
    }

    draw(
        gl::GL_TRIANGLES,
        static_cast<gl::GLsizei>(index_count),
        1,
        true,
        gl::GL_UNSIGNED_INT,
        0,
        0,
        nullptr,
        0,
        arguments
    );
    return true;
}

bool gpu_register_instances(unsigned source, std::span<const renderer::GpuCullBounds> bounds) {
    return active() && gpu_register(source, bounds);
}

void gpu_release_instances() {
    // A context already closed let its buffers go with it (gpu_release).
    if (!active()) {
        return;
    }

    gpu_release_resident();
}

GpuDrawCounters gpu_completed_draws() {
    return active() && stats::enabled() ? g->gpu.completed : GpuDrawCounters{};
}

GpuMaterialCounters gpu_material_counters() {
    return active() ? GpuMaterialCounters{g->bindless_supported, g->material_draws,
        g->material_records, g->material_textures, g->measures.native_draws}
        : GpuMaterialCounters{};
}

// The frame's measures, as gauges of the frame the debug tools are measuring.
void report_measures() {
    const auto& current = g->submission;
    const auto& previous = g->reported_submission;
    stats::gauge(
        "vk_buffer_zero_fill_bytes",
        double(current.buffer_zero_fill_bytes - previous.buffer_zero_fill_bytes)
    );
    stats::gauge(
        "vk_buffer_mirror_restore_bytes",
        double(current.buffer_mirror_restore_bytes - previous.buffer_mirror_restore_bytes)
    );
    stats::gauge("vk_uniform_uploads", double(current.uniform_uploads - previous.uniform_uploads));
    stats::gauge(
        "vk_uniform_upload_kb",
        double(current.uniform_upload_bytes - previous.uniform_upload_bytes) / 1024.0
    );
    stats::gauge(
        "vk_uniform_writes_skipped",
        double(current.uniform_writes_skipped - previous.uniform_writes_skipped)
    );
    stats::gauge("vk_descriptor_sets", double(current.descriptor_sets - previous.descriptor_sets));
    stats::gauge(
        "vk_descriptor_reuses", double(current.descriptor_reuses - previous.descriptor_reuses)
    );
    g->reported_submission = current;
    Measures& m = g->measures;
    stats::gauge("vk_draws", static_cast<double>(m.draws));
    stats::gauge("vk_draw_api_calls", double(m.native_draws));
    stats::gauge("vk_material_draws", double(g->material_draws));
    stats::gauge("vk_material_records", double(g->material_records));
    stats::gauge("vk_material_textures", double(g->material_textures));
    g->material_draws = g->material_records = g->material_textures = 0;
    stats::gauge("vk_gpu_instance_batches", double(g->gpu.batches));
    stats::gauge("vk_gpu_instance_candidates", double(g->gpu.candidates));
    stats::gauge("vk_gpu_indirect_commands", double(g->gpu.indirect));
    stats::gauge("vk_gpu_indirect_calls", double(g->gpu.indirect_calls));
    stats::gauge("vk_gpu_indirect_merged", double(g->gpu.merged));
    const char* break_names[] =
        {"pipeline",
         "descriptors",
         "uniforms",
         "vertices",
         "indices",
         "materials",
         "viewport",
         "dynamic",
         "arguments",
         "material_gap"};
    for (std::size_t i = 0; i < g->gpu.batch_breaks.size(); ++i) {
        stats::gauge(
            std::string("vk_gpu_batch_break_") + break_names[i], double(g->gpu.batch_breaks[i])
        );
    }
    g->gpu.batch_breaks.fill(0);
    stats::gauge("vk_gpu_completed_recording", double(g->gpu.completed.recording));
    stats::gauge("vk_gpu_completed_draws", double(g->gpu.completed.draws));
    stats::gauge("vk_gpu_completed_triangles", double(g->gpu.completed.triangles));
    stats::gauge("vk_gpu_completed_visible_instances", double(g->gpu.completed.visible_instances));
    stats::gauge("vk_gpu_counter_readback_bytes", double(g->gpu.completed.readback_bytes));
    g->gpu.batches = g->gpu.candidates = g->gpu.indirect = g->gpu.indirect_calls = g->gpu.merged =
        0;
    stats::gauge("vk_pipelines_made", static_cast<double>(m.pipelines));
    stats::gauge("vk_ring_kb", static_cast<double>(m.ring_bytes) / 1024.0);
    stats::gauge("vk_flushes", static_cast<double>(m.flushes));
    stats::gauge("vk_draw_ms", m.draw_s * 1000.0);
    stats::gauge(
        "vk_draw_us_each", m.draws != 0 ? m.draw_s * 1e6 / static_cast<double>(m.draws) : 0.0
    );
    stats::gauge("vk_present_ms", m.present_s * 1000.0);
    stats::gauge("vk_wait_ms", m.wait_s * 1000.0);
    m = Measures{};
}

SubmissionCounters submission_counters() {
    if (!g) {
        return {};
    }
    auto counters = g->submission;
    counters.validation_errors = g->validation_errors.load();
    counters.pipeline_compile_s = g->pipeline_compile_s.load(std::memory_order_relaxed);
    counters.pipeline_creation_calls = g->pipeline_creation_calls.load(std::memory_order_relaxed);
    return counters;
}

void present() {
    if (!active()) {
        return;
    }
    const Clock::time_point start = Clock::now();
    queue_after_previous();
    bool shown = false;
    if (g->window != nullptr) {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(g->window, &w, &h);
        if (w > 0 && h > 0) {
            shown = present_to_screen();
        }
    }
    if (!shown) {
        submit(VK_NULL_HANDLE, VK_NULL_HANDLE, false);
    }
    g->measures.present_s += seconds_since(start);
    report_measures();
    publish_counters();
    advance();
    // A window of another size: the default framebuffer follows, as a GL window's does.
    if (g->window != nullptr) {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(g->window, &w, &h);
        if (w > 0 && h > 0 && (w != g->default_width || h != g->default_height)) {
            retire(std::move(g->default_colour));
            retire(std::move(g->default_depth));
            make_default_framebuffer(w, h);
            g->swapchain_stale = true;
        }
    }
}

int set_swap_interval(int interval) {
    if (!active()) {
        return interval;
    }
    g->interval = interval;
    g->swapchain_stale = true;
    if (g->surface == VK_NULL_HANDLE) {
        return interval;
    }
    const VkPresentModeKHR mode = present_mode();
    g->shows_at_once = mode == VK_PRESENT_MODE_IMMEDIATE_KHR;
    return mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR ? -1 : mode == VK_PRESENT_MODE_FIFO_KHR ? 1 : 0;
}

bool swap_shows_at_once() {
    return active() && g->shows_at_once;
}

void set_low_latency(bool on) {
    if (!active() || g->low_latency == on) {
        return;
    }
    g->low_latency = on;
    g->latency = {};  // the times are measured again from the first low-latency frame
    g->swapchain_stale = true;
}

namespace {

// A frame the low-latency frames' sleep is for: not one whose graphics have begun already, which
// waited for the frame before it then.
bool latency_frame() {
    return active() && g->low_latency && !g->latency.marked && g->frame_pending;
}

}  // namespace

double latency_sleep(std::chrono::steady_clock::time_point limit) {
    if (!latency_frame()) {
        return 0.0;
    }
    const double delay = g->latency.sleep.delay();
    const Clock::time_point start = Clock::now();
    const Clock::time_point until = std::min(
        limit,
        start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(delay))
    );
    if (delay <= 0.0 || until <= start) {
        return 0.0;
    }
    const stats::Scope sleep("latency sleep");
    precise_wait::until(until);
    return seconds_since(start);
}

void latency_begin(double slept) {
    if (!latency_frame()) {
        return;
    }
    g->latency.began = Clock::now();
    g->latency.marked = true;
    g->latency.slept = slept;
    g->latency.waited = g->measures.wait_s;
}

const std::string& device_name() {
    static const std::string none;
    return g != nullptr ? g->device_name : none;
}

const std::string& driver_version() {
    static const std::string none;
    return g != nullptr ? g->driver_version : none;
}

const std::string& api_version() {
    static const std::string none;
    return g != nullptr ? g->api_version : none;
}

bool software() {
    return g != nullptr && g->software;
}

bool tile_based() {
    return g != nullptr && g->tile_based;
}

std::uint32_t vendor_id() {
    return g != nullptr ? g->properties.vendorID : 0;
}

bool split_pass_timing() {
    return g != nullptr && g->split_pass_timing;
}

}  // namespace openrac::vkgl
