// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Optional GPU timing segments follow command-buffer recording, not CPU scope lifetimes.
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/profiler.h"
#include "renderer/vulkan/vkgl_internal.h"

#include <tracy/TracyVulkan.hpp>

namespace openrac::vkgl::detail {
namespace {
struct GpuState;

// The CPU zone owns this handle; the state borrows it until end_zone unregisters it.
struct GpuZone {
    GpuState* owner = nullptr;
    std::string name;
    std::source_location location;
    unsigned stream = 0;
    std::optional<tracy::VkCtxScope> segment;
};

// Query contexts and borrowed handles belong to the graphics owner.
struct GpuState {
    // Uploads execute before main commands, even when their CPU scopes were nested the other way.
    std::array<TracyVkCtx, 2> contexts{};
    std::vector<GpuZone*> live;
    bool recording = false;
};

// Starts one segment using the scope's original source location and stable name.
void open_segment(GpuZone& zone) {
    const auto location = zone.location;
    zone.segment.emplace(
        zone.owner->contexts[zone.stream],
        location.line(),
        location.file_name(),
        std::strlen(location.file_name()),
        location.function_name(),
        std::strlen(location.function_name()),
        zone.name.data(),
        zone.name.size(),
        zone.stream == 0 ? frame().upload : frame().main,
        true
    );
}

// Scope timestamps stay in the primary buffer, after any deferred secondary work has executed.
void timestamp_boundary() {
    gpu_flush_draws();
    if (g->rendering && g->pass.deferred) {
        end_rendering();
    }
}

// Called only by GPU scopes on the graphics owner; deferred batches precede their timestamps.
void* begin_zone(std::string_view name, std::source_location location) {
    if (!g || !g->tracy_state) {
        return nullptr;
    }
    auto& state = *static_cast<GpuState*>(g->tracy_state);
    if (state.recording) {
        timestamp_boundary();
    }
    auto zone = std::make_unique<GpuZone>();
    zone->owner = &state;
    zone->name = name;
    zone->location = location;
    zone->stream = g->gpu.timing_upload ? 0u : 1u;
    if (state.recording) {
        open_segment(*zone);
    }
    state.live.push_back(zone.get());
    return zone.release();
}

// Submission may already have ended the segment; then this only releases the CPU zone's handle.
void end_zone(void* handle) {
    const std::unique_ptr<GpuZone> zone(static_cast<GpuZone*>(handle));
    if (zone->segment) {
        timestamp_boundary();
        zone->segment.reset();
    }
    if (zone->owner) {
        std::erase(zone->owner->live, zone.get());
    }
}

constexpr profiling::GpuHooks kGpuHooks{begin_zone, end_zone};
}  // namespace

// Creates separate upload and graphics timelines for the same hardware queue.
void tracy_gpu_open() {
    if (!g->timestamp_bits) {
        return;
    }
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g->family;
    VkCommandPool pool = VK_NULL_HANDLE;
    if (!check(
            vkCreateCommandPool(g->device, &pool_info, nullptr, &pool), "Tracy calibration pool"
        )) {
        return;
    }
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VkCommandBuffer command = VK_NULL_HANDLE;
    if (!check(vkAllocateCommandBuffers(g->device, &alloc, &command), "Tracy calibration buffer")) {
        vkDestroyCommandPool(g->device, pool, nullptr);
        return;
    }
    auto state = std::make_unique<GpuState>();
    for (auto& context : state->contexts) {
        // Initial clock calibration waits only at startup, never during frame collection.
        context =
            tracy::CreateVkContext(g->physical, g->device, g->queue, command, nullptr, nullptr);
    }
    vkDestroyCommandPool(g->device, pool, nullptr);
    state->contexts[0]->Name("Vulkan uploads", 14);
    state->contexts[1]->Name("Vulkan graphics", 15);
    g->tracy_state = state.release();
    profiling::set_gpu_hooks(&kGpuHooks);
}

// Collects ready timestamps and resumes live scopes on the newly begun buffers.
void tracy_gpu_collect() {
    if (!g->tracy_state) {
        return;
    }
    auto& state = *static_cast<GpuState*>(g->tracy_state);
    for (const auto context : state.contexts) {
        // Query resets must execute before uploads as well as graphics.
        context->Collect(frame().upload);
    }
    state.recording = true;
    for (auto* zone : state.live) {
        open_segment(*zone);
    }
}

// Finishes all current segments before either command buffer leaves the recording state.
void tracy_gpu_end_commands() {
    if (!g->tracy_state) {
        return;
    }
    auto& state = *static_cast<GpuState*>(g->tracy_state);
    if (!state.recording) {
        return;
    }
    if (!state.live.empty()) {
        timestamp_boundary();
    }
    // Close children before parents, while both command buffers still accept timestamps.
    for (auto at = state.live.rbegin(); at != state.live.rend(); ++at) {
        (*at)->segment.reset();
    }
    state.recording = false;
}

// Detaches surviving CPU handles and releases the query pools after the device is idle.
void tracy_gpu_close() {
    profiling::set_gpu_hooks(nullptr);
    if (!g->tracy_state) {
        return;
    }
    tracy_gpu_end_commands();
    const std::unique_ptr<GpuState> state(static_cast<GpuState*>(g->tracy_state));
    for (auto* zone : state->live) {
        zone->owner = nullptr;
    }
    for (const auto context : state->contexts) {
        tracy::DestroyVkContext(context);
    }
    g->tracy_state = nullptr;
}
}  // namespace openrac::vkgl::detail
