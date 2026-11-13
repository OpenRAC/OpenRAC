// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Records bounded GPU progress markers for device-loss diagnostics.
#include <algorithm>
#include <format>

#include "common/breadcrumbs.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {

void setup_gpu_breadcrumbs() {
    if (!g->allow_gpu_breadcrumbs) {
        return;
    }
    if (!NativeCommandEncoder::supports_write_buffer_marker()
        && !(NativeCommandEncoder::supports_set_checkpoint() && vkGetQueueCheckpointDataNV)) {
        diagnostic("GPU breadcrumbs unavailable on this Vulkan device");
        return;
    }

    auto state = std::make_unique<GpuBreadcrumbs>();
    state->nvidia = NativeCommandEncoder::supports_set_checkpoint() && vkGetQueueCheckpointDataNV;
    if (!state->nvidia) {
        VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer.size = state->markers.size() * sizeof(std::uint32_t);
        buffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VmaAllocationCreateInfo allocation{};
        allocation.usage = VMA_MEMORY_USAGE_AUTO;
        allocation.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocation.requiredFlags =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VmaAllocationInfo memory{};
        const auto result = vmaCreateBuffer(
            g->allocator, &buffer, &allocation, &state->buffer, &state->allocation, &memory
        );
        if (result != VK_SUCCESS) {
            diagnostic(
                std::format(
                    "GPU breadcrumbs unavailable: marker buffer allocation ({})", int(result)
                )
            );
            return;
        }
        auto* words = static_cast<std::uint32_t*>(memory.pMappedData);
        std::fill_n(words, state->markers.size(), 0u);
        state->mapped = words;
    }
    diagnostic(
        state->nvidia ? "GPU breadcrumbs enabled (NVIDIA checkpoints)"
                      : "GPU breadcrumbs enabled (AMD buffer markers)"
    );
    g->gpu_breadcrumbs = std::move(state);
}

void record_gpu_breadcrumb(VkCommandBuffer command, std::string_view pass, bool end) {
    auto& state = *g->gpu_breadcrumbs;
    const std::scoped_lock lock(state.mutex);
    if (++state.next == 0) {
        ++state.next;
    }
    const auto slot = state.next % state.markers.size();
    auto& marker = state.markers[slot];
    marker.id = state.next;
    marker.recording = g->recording;
    // Inspect the slot directly: frame() can start a recording and re-enter this function.
    const auto& current = g->frames[static_cast<std::size_t>(g->frame)];
    if (command == current.upload) {
        marker.buffer = "upload";
    } else if (command == current.main) {
        marker.buffer = "main";
    } else if (command == current.present) {
        marker.buffer = "present";
    } else {
        marker.buffer = "other";
    }
    marker.end = end;
    const auto length = pass.copy(marker.pass.data(), marker.pass.size() - 1);
    marker.pass[length] = '\0';
    if (state.nvidia) {
        // The driver uses this integer token as opaque data and never dereferences it.
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        NativeCommandEncoder{command}.set_checkpoint(
            reinterpret_cast<const void*>(std::uintptr_t(marker.id))
        );
    } else {
        NativeCommandEncoder{command}.write_buffer_marker(
            end ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            state.buffer,
            slot * sizeof(std::uint32_t),
            marker.id
        );
    }
}

void report_gpu_breadcrumbs() {
    diagnostic(
        std::format(
            "GPU progress: current recording {}, last frame recording accepted by vkQueueSubmit {} "
            "(acceptance does not imply completion)",
            g->recording,
            g->last_submitted_recording
        )
    );
    if (!g->gpu_breadcrumbs) {
        diagnostic("GPU progress markers unavailable or disabled");
        return;
    }
    crash::failure_breadcrumb("Vulkan query GPU breadcrumbs");
    auto& state = *g->gpu_breadcrumbs;
    const std::scoped_lock lock(state.mutex);
    const auto report = [](const GpuBreadcrumbs::Marker& marker, VkPipelineStageFlags stage) {
        diagnostic(
            std::format(
                "GPU breadcrumb: recording {}, {} buffer, CPU marker ID {}, {} {} (stage {:#x})",
                marker.recording,
                marker.buffer,
                marker.id,
                marker.end ? "end" : "begin",
                marker.pass.data(),
                stage
            )
        );
    };
    bool found = false;
    if (state.nvidia) {
        std::array<VkCheckpointDataNV, 64> checkpoints{};
        for (auto& checkpoint : checkpoints) {
            checkpoint.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
        }
        std::uint32_t count = checkpoints.size();
        vkGetQueueCheckpointDataNV(g->queue, &count, checkpoints.data());
        for (std::size_t i = 0; i < std::min<std::size_t>(count, checkpoints.size()); ++i) {
            const auto& checkpoint = checkpoints[i];
            const auto id = reinterpret_cast<std::uintptr_t>(checkpoint.pCheckpointMarker);
            const auto& marker = state.markers[id % state.markers.size()];
            if (id && id == marker.id) {
                report(marker, checkpoint.stage);
                found = true;
            }
        }
    } else {
        diagnostic("GPU breadcrumbs: newest CPU marker ID first; GPU execution order may differ");
        // Keep both kinds of boundary: top of pipe can be ahead of completed work.
        // A short history also retains the previous recording when a submit fails outright.
        std::array<unsigned, 2> remaining{8, 8};
        auto id = state.next;
        for (std::size_t i = 0; i < state.markers.size(); ++i, --id) {
            if (id == 0) {
                --id;
            }
            const auto slot = id % state.markers.size();
            const auto& marker = state.markers[slot];
            // After device loss the mapped contents are best effort. Reject stale slots.
            if (marker.id == id && state.mapped[slot] == id && remaining[marker.end] != 0) {
                report(
                    marker,
                    marker.end ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
                               : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                );
                found = true;
                --remaining[marker.end];
            }
            if (remaining[0] == 0 && remaining[1] == 0) {
                break;
            }
        }
    }
    if (!found) {
        diagnostic("GPU breadcrumbs: no recent executed markers recovered");
    }
}

}  // namespace openrac::vkgl::detail
