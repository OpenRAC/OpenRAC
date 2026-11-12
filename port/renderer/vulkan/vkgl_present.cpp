// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The screen (vkgl_internal.h): the default framebuffer, the swapchain and its present mode, moving
// on to the next frame, a new surface when the old one went, copying a finished frame to the
// window, and the render counters published as the frame's gauges.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <volk.h>

#include <SDL3/SDL.h>

#include "common/breadcrumbs.h"
#include "common/log.h"
#include "common/profiler.h"
#include "common/stats.h"
#include "renderer/render_opt.h"
#include "renderer/vulkan/graph_presentation.h"
#include "renderer/vulkan/swapchain_images.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {

namespace render_opt = renderer::render_opt;

using namespace openrac::gl;

namespace {

// ---- The render counters ----

// How a render counter's gauge reads it: the part the frame added, or the total so far (the
// cumulative counts, and the bytes held now for the mirror of GL's buffers).
enum class Span : std::uint8_t {
    Frame,
    Total,
};

// One render counter published as a gauge (docs/port/RENDER_PERF.md, "The counters").
struct Gauge {
    std::string_view name;                      // The gauge's name, vk_ and the counter's.
    std::uint64_t SubmissionCounters::* value;  // The counter it reads.
    Span span;                                  // Whether it shows the frame's part or the total.
};

// The render counters other than the commands, in the order of SubmissionCounters.
// clang-format off: one gauge per row.
constexpr auto kGauges = std::to_array<Gauge>({
    {"vk_pipelines_created", &SubmissionCounters::pipelines_created, Span::Total},
    {"vk_pipelines_linked", &SubmissionCounters::pipelines_linked, Span::Total},
    {"vk_pipelines_created_after_warmup", &SubmissionCounters::pipelines_created_after_warmup,
     Span::Total},
    {"vk_descriptor_sets_allocated", &SubmissionCounters::descriptor_sets_allocated, Span::Frame},
    {"vk_descriptor_writes", &SubmissionCounters::descriptor_writes, Span::Frame},
    {"vk_render_passes", &SubmissionCounters::render_passes, Span::Frame},
    {"vk_attachment_load_bytes", &SubmissionCounters::attachment_load_bytes, Span::Frame},
    {"vk_attachment_store_bytes", &SubmissionCounters::attachment_store_bytes, Span::Frame},
    {"vk_barriers", &SubmissionCounters::barriers, Span::Frame},
    {"vk_barriers_all_commands", &SubmissionCounters::barriers_all_commands, Span::Frame},
    {"vk_submits", &SubmissionCounters::submits, Span::Frame},
    {"vk_frames_presented", &SubmissionCounters::frames_presented, Span::Total},
    {"vk_fullscreen_copies", &SubmissionCounters::fullscreen_copies, Span::Frame},
    {"vk_ring_bytes", &SubmissionCounters::ring_bytes, Span::Frame},
    {"vk_texture_creations", &SubmissionCounters::texture_creations, Span::Frame},
    {"vk_texture_upload_bytes", &SubmissionCounters::texture_upload_bytes, Span::Frame},
    {"vk_buffer_static_bytes_host_visible", &SubmissionCounters::buffer_static_bytes_host_visible,
     Span::Total},
    {"vk_cpu_mirror_bytes", &SubmissionCounters::cpu_mirror_bytes, Span::Total},
    {"vk_gpu_payload_bytes", &SubmissionCounters::gpu_payload_bytes, Span::Frame},
    {"vk_shader_fp16_ops", &SubmissionCounters::shader_fp16_ops, Span::Total},
    {"vk_shader_fp32_ops", &SubmissionCounters::shader_fp32_ops, Span::Total},
    {"vk_shader_fp16_modules", &SubmissionCounters::shader_fp16_modules, Span::Total},
    {"vk_shader_compiles", &SubmissionCounters::shader_compiles, Span::Total},
    {"vk_program_reuses", &SubmissionCounters::program_reuses, Span::Total},
});
// clang-format on

// The gauge of each kind of command, in the order of Command.
constexpr std::array<std::string_view, kCommandKinds> kCommandGauges = {
    "vk_cmds_bind_pipeline",
    "vk_cmds_bind_descriptors",
    "vk_cmds_bind_buffers",
    "vk_cmds_dynamic_state",
    "vk_cmds_draw",
    "vk_cmds_barrier",
    "vk_cmds_copy",
    "vk_cmds_dispatch",
    "vk_cmds_render_pass",
    "vk_cmds_query",
    "vk_cmds_other",
};

// Returns the value of `gauge` from the counters `now`, given those of the frame before.
double gauge_value(
    const Gauge& gauge, const SubmissionCounters& now, const SubmissionCounters& before
) {
    const std::uint64_t total = now.*gauge.value;
    return static_cast<double>(gauge.span == Span::Total ? total : total - before.*gauge.value);
}

// ---- The device, the screen and the frames ----

ImagePtr default_image(const Format& f, int width, int height) {
    ImagePtr image = make_image(f, width, height, 1, VK_SAMPLE_COUNT_1_BIT);
    return image;
}

void swapchain_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to) {
    // The screen's image is only ever written by transfers (a clear, a blit) and then presented.
    // Out of UNDEFINED it waits for nothing but the acquire, which the submit waits for at the
    // transfer stage; into PRESENT_SRC nothing of the queue reads it after (the R-08 table).
    const bool tight = render_opt::enabled(render_opt::Opt::VkTightSync);
    VkPipelineStageFlags source = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destination = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    // The narrow form: transfer to transfer, or transfer to the end of the queue's work.
    if (tight) {
        const bool fresh = from == VK_IMAGE_LAYOUT_UNDEFINED;
        const bool shown = to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        source = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination = shown ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT;
        b.srcAccessMask = fresh ? VkAccessFlags{0} : VkAccessFlags{VK_ACCESS_TRANSFER_WRITE_BIT};
        b.dstAccessMask = shown ? VkAccessFlags{0} : VkAccessFlags{VK_ACCESS_TRANSFER_WRITE_BIT};
    }

    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, source, destination, 0, 0, nullptr, 0, nullptr, 1, &b);
    count_barrier(source, destination, "present");
}

// The surface and swapchain made again: back from the background on Android, the window is a new
// one and the old surface draws nowhere (the screen stayed black while the game went on).
bool renew_surface() {
    crash::breadcrumb("Vulkan renew surface");
    require_success(vkDeviceWaitIdle(g->device), "wait before surface renewal");
    release_hdr_views();
    if (g->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(g->device, g->swapchain, nullptr);
        g->swapchain = VK_NULL_HANDLE;
    }
    if (g->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(g->instance, g->surface, nullptr);
        g->surface = VK_NULL_HANDLE;
    }
    std::string error;
    if (!make_surface(g->window, error)) {
        diagnostic(std::format("Vulkan surface renewal deferred: {}", error));
        return false;  // no window yet: tried again with the next frame
    }
    g->surface_lost = false;
    g->swapchain_stale = true;
    log::info("vulkan: the window's surface made again");
    return true;
}

}  // namespace

// ---- The device, the screen and the frames ----

void run_deferred(Frame& f) {
    std::vector<std::function<void()>> list = std::move(f.deferred);
    f.deferred.clear();
    for (auto& fn : list) {
        fn();
    }
}

bool make_default_framebuffer(int width, int height) {
    // In half floats while HDR is shown (vkgl.h, hdr_frame), so the frame's light above white
    // reaches the encode pass; 8-bit otherwise, as always.
    g->default_colour = default_image(
        g->hdr.shown != hdr::Output::Sdr
            ? Format{VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 8}
            : Format{VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 4},
        width,
        height
    );
    g->default_depth = default_image(
        {g->depth_format,
         VK_IMAGE_ASPECT_DEPTH_BIT
             | (has_stencil(g->depth_format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u),
         4},
        width,
        height
    );
    g->default_width = width;
    g->default_height = height;
    return g->default_colour && g->default_depth;
}

VkPresentModeKHR present_mode() {
    std::uint32_t n = 0;
    if (!check(
            vkGetPhysicalDeviceSurfacePresentModesKHR(g->physical, g->surface, &n, nullptr),
            "count present modes"
        )
        || n == 0) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    std::vector<VkPresentModeKHR> modes(n);
    if (!check(
            vkGetPhysicalDeviceSurfacePresentModesKHR(g->physical, g->surface, &n, modes.data()),
            "read present modes"
        )) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    modes.resize(n);
    auto has = [&](VkPresentModeKHR m) {
        return std::find(modes.begin(), modes.end(), m) != modes.end();
    };
    if (g->interval == 0) {
        if (has(VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        // Uncapped without tearing: the latest frame at each refresh, as mailbox, where offered.
        const bool latest = render_opt::enabled(render_opt::Opt::PresentTiming)
                            && g->latest_ready_enabled
                            && has(VK_PRESENT_MODE_FIFO_LATEST_READY_KHR);
        if (latest) {
            return VK_PRESENT_MODE_FIFO_LATEST_READY_KHR;
        }

        if (has(VK_PRESENT_MODE_MAILBOX_KHR)) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    if (g->interval < 0 && has(VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

bool make_swapchain() {
    if (g->surface == VK_NULL_HANDLE) {
        return true;
    }
    crash::breadcrumb("Vulkan recreate swapchain");
    const auto idle = vkDeviceWaitIdle(g->device);
    if (g->open) {
        require_success(idle, "wait before swapchain recreation");
    } else if (!check(idle, "wait before initial swapchain")) {
        return false;
    }
    VkSurfaceCapabilitiesKHR caps{};
    const auto capabilities =
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g->physical, g->surface, &caps);
    if (capabilities == VK_ERROR_SURFACE_LOST_KHR) {
        g->surface_lost = true;
    }
    if (!check(capabilities, "query surface capabilities")) {
        return false;
    }
    std::uint32_t n = 0;
    if (!check(
            vkGetPhysicalDeviceSurfaceFormatsKHR(g->physical, g->surface, &n, nullptr),
            "count surface formats"
        )
        || n == 0) {
        return false;
    }
    std::vector<VkSurfaceFormatKHR> formats(n);
    if (!check(
            vkGetPhysicalDeviceSurfaceFormatsKHR(g->physical, g->surface, &n, formats.data()),
            "read surface formats"
        )) {
        return false;
    }
    formats.resize(n);
    if (formats.empty()) {
        return false;
    }
    // A linear format: the picture is in the display's own (gamma) space already, as on the GS;
    // or, asked for and offered, an HDR one the encode pass writes (surface_format.h).
    const VkSurfaceFormatKHR format = choose_hdr_format(formats, caps);
    const bool hdr = g->hdr.shown != hdr::Output::Sdr;
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(g->window, &w, &h);
    VkExtent2D extent{
        static_cast<std::uint32_t>(std::max(w, 1)), static_cast<std::uint32_t>(std::max(h, 1))
    };
#if !defined(__ANDROID__)
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        extent = caps.currentExtent;
    }
#endif
    extent.width = std::clamp(
        extent.width, caps.minImageExtent.width, std::max(caps.maxImageExtent.width, 1u)
    );
    extent.height = std::clamp(
        extent.height, caps.minImageExtent.height, std::max(caps.maxImageExtent.height, 1u)
    );
    if (extent.width == 0 || extent.height == 0) {
        return false;  // minimised: made again when it is shown
    }
    std::uint32_t count =
        swapchain_images(caps.minImageCount, g->low_latency, kCompositorHoldsImage);
    if (caps.maxImageCount != 0) {
        count = std::min(count, caps.maxImageCount);
    }
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = g->surface;
    info.minImageCount = count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | (hdr && g->hdr.direct ? VK_IMAGE_USAGE_STORAGE_BIT : 0u);
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // The system turns the picture to the screen's way up (no pre-rotation of our own).
    info.preTransform = (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0
                            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                            : caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (const VkCompositeAlphaFlagBitsKHR a :
         {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
          VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
          VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
          VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR}) {
        if ((caps.supportedCompositeAlpha & a) != 0) {
            info.compositeAlpha = a;
            break;
        }
    }
    info.presentMode = present_mode();
    g->shows_at_once = info.presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR;
    info.clipped = VK_TRUE;
    info.oldSwapchain = g->swapchain;

    // VkSurfaceFullScreenExclusiveInfoEXT (vulkan_win32.h, which needs windows.h), the same
    // layout: borderless full screen stays a window the desktop composes, as DXGI's flip model.
    struct {
        VkStructureType sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        void* pNext = nullptr;
        std::int32_t fullScreenExclusive = 2;  // VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT
    } no_exclusive;

    if (g->no_exclusive_full_screen) {
        info.pNext = &no_exclusive;
    }
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    const auto created = vkCreateSwapchainKHR(g->device, &info, nullptr, &swapchain);
    // Passing oldSwapchain retires it even if creation fails.
    release_hdr_views();
    if (g->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(g->device, g->swapchain, nullptr);
    }
    g->swapchain = VK_NULL_HANDLE;
    g->swapchain_images.clear();
    for (VkSemaphore s : g->rendered) {
        vkDestroySemaphore(g->device, s, nullptr);
    }
    g->rendered.clear();
    if (created == VK_ERROR_SURFACE_LOST_KHR) {
        g->surface_lost = true;
    }
    if (created == VK_ERROR_DEVICE_LOST && g->open) {
        require_success(created, "create swapchain");
    }
    if (!check(created, "create swapchain")) {
        return false;
    }
    g->swapchain = swapchain;
    g->swapchain_format = format.format;
    g->swapchain_extent = extent;
    // Present ids count per swapchain: the new one's start again.
    g->present_serial = 0;
    g->present_seen = PresentTiming{};
    const auto abandon = [&] {
        for (VkSemaphore semaphore : g->rendered) {
            vkDestroySemaphore(g->device, semaphore, nullptr);
        }
        g->rendered.clear();
        g->swapchain_images.clear();
        vkDestroySwapchainKHR(g->device, swapchain, nullptr);
        g->swapchain = VK_NULL_HANDLE;
        return false;
    };
    if (!check(vkGetSwapchainImagesKHR(g->device, swapchain, &n, nullptr), "count swapchain images")
        || n == 0) {
        return abandon();
    }
    g->swapchain_images.resize(n);
    if (!check(
            vkGetSwapchainImagesKHR(g->device, swapchain, &n, g->swapchain_images.data()),
            "read swapchain images"
        )
        || n == 0) {
        return abandon();
    }
    g->swapchain_images.resize(n);
    g->rendered.assign(n, VK_NULL_HANDLE);
    for (VkSemaphore& s : g->rendered) {
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        if (!check(vkCreateSemaphore(g->device, &si, nullptr, &s), "create present semaphore")) {
            return abandon();
        }
    }
    hdr_swapchain_made(format);
    g->swapchain_stale = false;
    diagnostic(
        std::format(
            "Vulkan swapchain: {}x{}, {} images, format {}, present mode {}, HDR {}",
            extent.width,
            extent.height,
            n,
            int(format.format),
            int(info.presentMode),
            hdr
        )
    );
    return true;
}

void publish_counters() {
    ++g->submission.frames_presented;
    const SubmissionCounters& now = g->submission;

    // Nobody is measuring: the next frame that is measured shows only its own part.
    if (!stats::enabled()) {
        g->published = now;
        g->where.clear();
        return;
    }

    const SubmissionCounters& before = g->published;

    // The counts and bytes, each as its row of the table says.
    for (const Gauge& gauge : kGauges) {
        stats::gauge(gauge.name, gauge_value(gauge, now, before));
    }

    // Where the frame's passes were split and the whole picture copied (count_where).
    for (auto& [event, count] : g->where) {
        stats::gauge(event, count);
        count = 0.0;
    }

    stats::gauge(
        "vk_cmds_recorded", static_cast<double>(commands_recorded(now) - commands_recorded(before))
    );

    // The frame's commands of each kind.
    for (std::size_t kind = 0; kind < kCommandKinds; ++kind) {
        stats::gauge(
            kCommandGauges[kind], static_cast<double>(now.commands[kind] - before.commands[kind])
        );
    }

    stats::gauge("shader_compile_s", now.shader_compile_s);
    stats::gauge("pipeline_compile_s", g->pipeline_compile_s.load(std::memory_order_relaxed));
    stats::gauge(
        "pipeline_creation_calls",
        static_cast<double>(g->pipeline_creation_calls.load(std::memory_order_relaxed))
    );
    stats::gauge("vk_gpu_timestamps", g->timestamp_bits != 0 ? 1.0 : 0.0);
    // The presentation's timing (R-09): whether presents can be waited on, whether one has been
    // seen finished, and how long the frame waited for a free swapchain image.
    stats::gauge("present_wait_supported", g->present_wait_enabled ? 1.0 : 0.0);
    stats::gauge("present_time_known", g->present_seen.known ? 1.0 : 0.0);
    stats::gauge("acquire_wait_s", g->acquire_wait_s);
    g->published = now;
}

// Selects the next slot without blocking simulation or CPU scene preparation after presentation.
void advance() {
    g->frame = (g->frame + 1) % kFrames;
    ++g->frame_count;
    g->frame_pending = true;
}

namespace {

// The slot of the frame queued before this one.
Frame& previous_frame() {
    return g->frames[static_cast<std::size_t>((g->frame + kFrames - 1) % kFrames)];
}

// Low-latency frames: waits until the frame before this one is done on the GPU (one in flight),
// counted with the frame's GPU waits; the seconds waited, 0 when it was not queued.
double wait_for_previous_frame() {
    Frame& last = previous_frame();
    if (!last.submitted) {
        return 0.0;
    }
    const stats::Scope wait("vulkan frame wait", profiling::ZoneColour::Idle);
    const Clock::time_point start = Clock::now();
    if (!check(
            vkWaitForFences(g->device, 1, &last.fence, VK_TRUE, UINT64_MAX), "previous frame fence"
        )) {
        throw std::runtime_error("cannot wait for the previous Vulkan frame");
    }
    const double waited = seconds_since(start);
    g->measures.wait_s += waited;
    return waited;
}

}  // namespace

// Pools and upload memory remain untouched until this slot's previous submission has completed.
void prepare_frame() {
    if (!g || !g->frame_pending) {
        return;
    }
    const profiling::Zone trace("Prepare Vulkan frame slot");
    Frame& f = g->frames[static_cast<std::size_t>(g->frame)];
    // Low-latency frames: the frame before this one is done on the GPU first (one in flight). A
    // frame whose beginning the frontend noted (latency_sleep), once the times are known, is
    // recorded meanwhile and waits for it only as it is queued (queue_after_previous): the GPU
    // still has one frame at a time, and the CPU's recording no longer waits behind it.
    g->latency.deferred = g->low_latency && g->latency.marked && g->latency.sleep.known();
    if (g->low_latency && !g->latency.deferred) {
        wait_for_previous_frame();
    }
    if (f.submitted) {
        const stats::Scope wait("vulkan frame wait", profiling::ZoneColour::Idle);
        const Clock::time_point start = Clock::now();
        require_success(
            vkWaitForFences(g->device, 1, &f.fence, VK_TRUE, UINT64_MAX), "frame-slot fence"
        );
        g->measures.wait_s += seconds_since(start);
        timing_results(f);
        f.submitted = false;
    }
    // Helpers below also access frame(); exclusive graphics ownership prevents concurrent reuse.
    g->frame_pending = false;
    run_deferred(f);
    f.timings.clear();
    f.timestamp_used = 0;
    f.active_timing = -1;
    require_success(vkResetCommandPool(g->device, f.pool, 0), "reset frame pool");
    for (VkDescriptorPool pool : f.pools) {
        require_success(vkResetDescriptorPool(g->device, pool, 0), "reset frame descriptors");
    }
    f.pool_at = 0;
    reset_upload_ring(f);
    begin_buffers();

    // What the pre-warm workers made since, a whole pipeline in place of a linked one retired
    // with this frame; and, every two minutes that made new pipelines, the caches saved on a
    // thread of their own, so a run killed or crashed keeps them.
    collect_prewarm();
    if (Clock::now() - g->saved_at >= std::chrono::minutes(2)) {
        save_pipeline_caches_soon(false);
    }

    // A copy that waited for a screen there was not (no window, or none shown): made now, so the
    // default framebuffer holds what glBlitFramebuffer put there, as before.
    make_present_copy();
}

void queue_after_previous() {
    auto& l = g->latency;
    if (!l.marked) {
        return;
    }
    // The frame's own work: from its beginning to here, without the waits for the GPU in it.
    double wait = g->measures.wait_s - l.waited;
    const double work = seconds_since(l.began) - wait;
    // Its wait for the frame before it, left by prepare_frame: what is left of it is what the next
    // frame's sleep could have taken as well.
    if (l.deferred && previous_frame().submitted) {
        wait = wait_for_previous_frame();
    }
    l.sleep.cpu_frame(work, l.slept, wait);
    l.marked = false;
    l.deferred = false;
}

namespace {

// True when present-timing is on: present ids given and their waits polled.
bool timing_on() {
    return render_opt::enabled(render_opt::Opt::PresentTiming);
}

// Waits at most `timeout_ns` for the last numbered present to finish, and when it has, keeps the
// moment it was seen (g->present_seen). False when there is nothing to wait for or it is not done.
bool note_finished_presents(std::uint64_t timeout_ns) {
    const std::uint64_t id = g->present_serial;

    // No numbered present waits, or the last one was seen already.
    if (!timing_on() || !g->present_wait_enabled || g->swapchain == VK_NULL_HANDLE || id == 0
        || g->present_seen.id == id) {
        return g->present_seen.known && g->present_seen.id == id && id != 0;
    }

    const VkResult r = vkWaitForPresentKHR(g->device, g->swapchain, id, timeout_ns);

    // Not yet shown (a timeout), or the swapchain went: nothing more is known.
    if (r != VK_SUCCESS) {
        return false;
    }

    g->present_seen.known = true;
    g->present_seen.id = id;
    g->present_seen.visible = Clock::now();
    return true;
}

// True when the screen's format holds what the default framebuffer's does, eight unsigned bits a
// channel without sRGB encoding, so that a blit into either rounds the same.
bool same_numbers_as_default(VkFormat screen) {
    return screen == VK_FORMAT_B8G8R8A8_UNORM || screen == VK_FORMAT_R8G8B8A8_UNORM;
}

// True when the copy that waits (PendingPresent) can go straight into a screen of the swapchain's
// size and format: the two-step path's second copy is then 1:1 and NEAREST, so one blit is exact.
bool composable() {
    const Image& colour = *g->default_colour;
    const VkExtent2D screen = g->swapchain_extent;
    return g->present_copy.pending && same_numbers_as_default(g->swapchain_format)
           && colour.width == static_cast<int>(screen.width)
           && colour.height == static_cast<int>(screen.height);
}

// Records the copy that waits straight into `screen` (`extent`, in TRANSFER_DST_OPTIMAL): the
// clear colour over all of it, then the picture's blit with its destination turned upside down, as
// the default framebuffer's copy to the screen turns it (vkgl.h, "Rows").
void compose_into(VkCommandBuffer cmd, VkImage screen, VkExtent2D extent) {
    PendingPresent waiting = std::move(g->present_copy);
    g->present_copy = PendingPresent{};
    const VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(
        cmd, screen, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &waiting.clear, 1, &range
    );
    count_command(Command::Copy);
    barrier(cmd, *waiting.source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // The clear's writes done before the blit's, which overlap them.
    swapchain_barrier(
        cmd, screen, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    VkImageBlit blit = waiting.region;
    const auto height = static_cast<std::int32_t>(extent.height);
    blit.dstOffsets[0].y = height - blit.dstOffsets[0].y;
    blit.dstOffsets[1].y = height - blit.dstOffsets[1].y;
    vkCmdBlitImage(
        cmd,
        waiting.source->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        screen,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        waiting.filter
    );
    count_copy(
        static_cast<std::uint32_t>(std::abs(blit.dstOffsets[1].x - blit.dstOffsets[0].x)),
        static_cast<std::uint32_t>(std::abs(blit.dstOffsets[1].y - blit.dstOffsets[0].y))
    );
}

// Records the default framebuffer's copy into `screen` (`extent`, in TRANSFER_DST_OPTIMAL),
// turned the right way up: row 0 of the picture is the bottom of the screen, as in GL.
void copy_default_into(VkCommandBuffer cmd, VkImage screen, VkExtent2D extent) {
    Image& colour = *g->default_colour;
    barrier(cmd, colour, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.srcOffsets[1] = {colour.width, colour.height, 1};
    blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.dstOffsets[0] = {0, static_cast<std::int32_t>(extent.height), 0};
    blit.dstOffsets[1] = {static_cast<std::int32_t>(extent.width), 0, 1};
    const bool same = colour.width == static_cast<int>(extent.width)
                      && colour.height == static_cast<int>(extent.height);
    vkCmdBlitImage(
        cmd,
        colour.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        screen,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        same ? VK_FILTER_NEAREST : VK_FILTER_LINEAR
    );
    count_copy(extent.width, extent.height);
}

}  // namespace

bool present_to_screen() {
    const stats::Scope pass("vulkan present");
    if (g->surface_lost && !renew_surface()) {
        return false;
    }
    if (g->swapchain_stale || g->swapchain == VK_NULL_HANDLE) {
        if (!make_swapchain()) {
            return false;
        }
    }
    Frame& f = frame();
    std::uint32_t index = 0;
    note_finished_presents(0);
    // The wait for a free image of the swapchain, measured: the game thread blocks here.
    const Clock::time_point acquiring = Clock::now();
    std::optional<stats::Scope> step(std::in_place, "vulkan acquire");
    VkResult r;
    {
        const profiling::Zone acquire_trace("Acquire swapchain image");
        crash::breadcrumb("Vulkan acquire swapchain image");
        r = vkAcquireNextImageKHR(
            g->device, g->swapchain, UINT64_MAX, f.acquired, VK_NULL_HANDLE, &index
        );
    }
    g->acquire_wait_s = seconds_since(acquiring);
    step.reset();
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        diagnostic("Vulkan acquire: swapchain out of date");
        if (!make_swapchain()) {
            return false;
        }
        r = vkAcquireNextImageKHR(
            g->device, g->swapchain, UINT64_MAX, f.acquired, VK_NULL_HANDLE, &index
        );
    }
    if (r == VK_ERROR_SURFACE_LOST_KHR) {
        g->surface_lost = true;
        crash::breadcrumb("Vulkan acquire: surface lost");
        diagnostic("Vulkan acquire: VK_ERROR_SURFACE_LOST_KHR");
        return false;
    }
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        g->swapchain_stale = true;
        return false;
    }
    if (r != VK_SUBOPTIMAL_KHR) {
        require_success(r, "acquire swapchain image");
    }
    end_rendering(PassEnd::Present);
    const VkImage screen = g->swapchain_images[index];
    const bool composed = composable();

    // A copy that cannot go straight to the screen goes into the window first, in `main`.
    if (!composed) {
        make_present_copy();
    }

    // The copy to the screen in a buffer of its own, so that only it waits for the screen's
    // image (vk-tight-sync); or at the end of `main`, as before.
    // An HDR screen is written by a compute pass, which the present's own buffer (waiting at the
    // transfer stage) does not cover: HDR frames record it at the end of `main`.
    const bool hdr = g->hdr.shown != hdr::Output::Sdr;
    const VkCommandBuffer own =
        render_opt::enabled(render_opt::Opt::VkTightSync) && !hdr ? present_cmd() : VK_NULL_HANDLE;
    VkCommandBuffer cmd = own != VK_NULL_HANDLE ? own : f.main;
    // HDR setup can fall back to the transfer graph before recording an encode.
    if (!hdr || !hdr_encode_into(cmd, index)) {
        // Native transfer callbacks retain composition's internal clear-to-blit ordering.
        const auto copy_screen = [&] {
            if (composed) {
                compose_into(cmd, screen, g->swapchain_extent);
            } else {
                copy_default_into(cmd, screen, g->swapchain_extent);
            }
        };
        Image borrowed;
        borrowed.image = screen;
        const auto source = composed ? g->present_copy.source : g->default_colour;
        const auto recorded = record_presentation(cmd, source, borrowed, true, copy_screen);
        if (!recorded) {
            throw std::runtime_error(recorded.error().message);
        }
    }
    // The frame's GPU-driven work recorded and everything handed to the queue; then the present,
    // which a driver may hold until the display or the GPU has room for another picture.
    step.emplace("vulkan submit");
    submit(f.acquired, g->rendered[index], false, own);
    step.emplace("vulkan queue present");
    VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &g->rendered[index];
    info.swapchainCount = 1;
    info.pSwapchains = &g->swapchain;
    info.pImageIndices = &index;
    VkPresentIdKHR id{VK_STRUCTURE_TYPE_PRESENT_ID_KHR};
    const std::uint64_t serial = g->present_serial + 1;

    // Each present numbered, so that its wait can say when it reached the screen.
    if (timing_on() && g->present_id_enabled) {
        id.swapchainCount = 1;
        id.pPresentIds = &serial;
        info.pNext = &id;
        g->present_serial = serial;
    }

    {
        const profiling::Zone present_trace("Queue present");
        crash::breadcrumb("Vulkan queue present");
        r = vkQueuePresentKHR(g->queue, &info);
    }
    if (r == VK_ERROR_SURFACE_LOST_KHR) {
        g->surface_lost = true;
        crash::breadcrumb("Vulkan present: surface lost");
        diagnostic("Vulkan present: VK_ERROR_SURFACE_LOST_KHR");
        return true;  // Rendering was already submitted.
    }
#if defined(__ANDROID__)
    // Android says "suboptimal" for every frame of a turned screen that the system turns for us.
    const bool stale = r == VK_ERROR_OUT_OF_DATE_KHR;
#else
    const bool stale = r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR;
#endif
    if (stale) {
        g->swapchain_stale = true;
        diagnostic(std::format("Vulkan present: swapchain needs recreation ({})", int(r)));
    } else if (r != VK_SUBOPTIMAL_KHR) {
        require_success(r, "queue present");
    }
    return true;
}

}  // namespace openrac::vkgl::detail

namespace openrac::vkgl {

namespace {

// Reads `image` back (four bytes a texel, rows as stored) into `out`, waiting for the GPU.
bool read_back(detail::Image& image, std::vector<std::uint8_t>& out) {
    using namespace detail;
    const auto bytes = static_cast<VkDeviceSize>(image.width) * image.height * 4;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = bytes;
    info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;
    alloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo got{};

    // No buffer to read into.
    if (!check(
            vmaCreateBuffer(g->allocator, &info, &alloc, &buffer, &allocation, &got), "a read-back"
        )) {
        return false;
    }

    VkCommandBuffer cmd = cmd_for(&image, true);
    barrier(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent =
        {static_cast<std::uint32_t>(image.width), static_cast<std::uint32_t>(image.height), 1};
    vkCmdCopyImageToBuffer(
        cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &copy
    );
    count_command(Command::Copy);
    flush_and_wait();
    vmaInvalidateAllocation(g->allocator, allocation, 0, VK_WHOLE_SIZE);
    const auto* from = static_cast<const std::uint8_t*>(got.pMappedData);
    out.assign(from, from + bytes);
    vmaDestroyBuffer(g->allocator, buffer, allocation);
    return true;
}

}  // namespace

bool test_present_paths(std::vector<std::uint8_t>& single, std::vector<std::uint8_t>& two_step) {
    using namespace detail;

    // Only a copy that waits can be drawn both ways.
    if (g == nullptr || !g->present_copy.pending) {
        return false;
    }

    end_rendering();
    const Format screen_format{VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 4};
    const int width = g->default_width;
    const int height = g->default_height;
    const ImagePtr composed = make_image(screen_format, width, height, 1, VK_SAMPLE_COUNT_1_BIT);
    const ImagePtr copied = make_image(screen_format, width, height, 1, VK_SAMPLE_COUNT_1_BIT);

    // No stand-in images: nothing to compare.
    if (!composed || !copied) {
        return false;
    }

    const VkExtent2D extent{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    const PendingPresent waiting = g->present_copy;
    VkCommandBuffer cmd = cmd_for(composed.get(), true);
    const auto recorded = record_presentation(cmd, waiting.source, *composed, false, [&] {
        compose_into(cmd, composed->image, extent);
    });
    // Temporary images may already be referenced by recorded barriers on failure.
    if (!recorded) {
        retire(composed);
        retire(copied);
        return false;
    }
    // The same copy again, the two-step way: into the default framebuffer, then into the image.
    g->present_copy = waiting;
    make_present_copy();
    cmd = cmd_for(copied.get(), true);
    const auto copied_result = record_presentation(cmd, g->default_colour, *copied, false, [&] {
        copy_default_into(cmd, copied->image, extent);
    });
    // Temporary images may already be referenced by recorded barriers on failure.
    if (!copied_result) {
        retire(composed);
        retire(copied);
        return false;
    }
    const bool read = read_back(*composed, single) && read_back(*copied, two_step);
    retire(composed);
    retire(copied);
    return read;
}

}  // namespace openrac::vkgl

namespace openrac::vkgl {

PresentTiming present_timing() {
    using namespace detail;

    // No context: nothing presented.
    if (g == nullptr) {
        return {};
    }

    note_finished_presents(0);
    PresentTiming timing = g->present_seen;
    timing.supported = timing_on() && g->present_wait_enabled && g->swapchain != VK_NULL_HANDLE;
    return timing;
}

bool wait_for_present(std::chrono::nanoseconds timeout) {
    using namespace detail;
    const auto ns = static_cast<std::uint64_t>(std::max<std::int64_t>(timeout.count(), 0));
    return g != nullptr && note_finished_presents(ns);
}

}  // namespace openrac::vkgl
