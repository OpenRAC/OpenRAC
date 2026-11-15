// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Lowers compiled submission batches and presentation requests to Vulkan.
#include "renderer/vulkan/commands/graph_queue.h"

#include <bit>

#include "renderer/graph/breadcrumbs.h"

namespace openrac::vkgl {
namespace {
namespace graph = renderer::graph;

struct NativeBatch {
    std::span<VkCommandBuffer> commands;
    std::span<VkSemaphore> waits;
    std::span<VkPipelineStageFlags> stages;
    std::span<VkSemaphore> signals;
};

bool lower_batch(graph::SubmissionBatchView batch, NativeBatch native, VkSubmitInfo& submit) {
    for (std::size_t i = 0; i < batch.commands.size(); ++i) {
        native.commands[i] = std::bit_cast<VkCommandBuffer>(batch.commands[i]);
    }
    for (std::size_t i = 0; i < batch.waits.size(); ++i) {
        const auto wait = batch.waits[i];
        if (wait.stages != graph::Stage::Transfer && wait.stages != graph::Stage::All) {
            return false;
        }
        native.waits[i] = std::bit_cast<VkSemaphore>(wait.semaphore);
        native.stages[i] = wait.stages == graph::Stage::Transfer
                               ? VK_PIPELINE_STAGE_TRANSFER_BIT
                               : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    for (std::size_t i = 0; i < batch.signals.size(); ++i) {
        native.signals[i] = std::bit_cast<VkSemaphore>(batch.signals[i]);
    }
    submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = static_cast<std::uint32_t>(batch.commands.size());
    submit.pCommandBuffers = native.commands.data();
    submit.waitSemaphoreCount = static_cast<std::uint32_t>(batch.waits.size());
    submit.pWaitSemaphores = native.waits.data();
    submit.pWaitDstStageMask = native.stages.data();
    submit.signalSemaphoreCount = static_cast<std::uint32_t>(batch.signals.size());
    submit.pSignalSemaphores = native.signals.data();
    return true;
}

VkResult submit_batches(VkQueue queue, std::span<const VkSubmitInfo> submits, VkFence completion) {
    graph::BreadcrumbScope operation{"graph queue submit"};
    const auto result = vkQueueSubmit(
        queue, static_cast<std::uint32_t>(submits.size()), submits.data(), completion
    );
    if (result == VK_SUCCESS) {
        operation.complete();
    }
    return result;
}
}  // namespace

VkResult submit_graph_queue(VkQueue queue, const graph::SubmissionPlan& plan, VkFence completion) {
    struct Batch {
        std::vector<VkCommandBuffer> commands;
        std::vector<VkSemaphore> waits, signals;
        std::vector<VkPipelineStageFlags> stages;
    };

    std::vector<Batch> storage(plan.batches.size());
    std::vector<VkSubmitInfo> submits(plan.batches.size());
    for (std::size_t index = 0; index < plan.batches.size(); ++index) {
        const auto& batch = plan.batches[index];
        auto& native = storage[index];
        native.commands.resize(batch.commands.size());
        native.waits.resize(batch.waits.size());
        native.stages.resize(batch.waits.size());
        native.signals.resize(batch.signals.size());
        if (!lower_batch(
                {batch.commands, batch.waits, batch.signals},
                {native.commands, native.waits, native.stages, native.signals},
                submits[index]
            )) {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }
    }
    return submit_batches(queue, submits, completion);
}

VkResult submit_graph_queue(
    VkQueue queue, const graph::FrameSubmissionPlan& plan, VkFence completion
) {
    std::array<VkCommandBuffer, 3> commands{};
    std::array<VkSemaphore, 1> wait{}, signal{};
    std::array<VkPipelineStageFlags, 1> stage{};
    std::array<VkSubmitInfo, 2> submits{};
    std::size_t first_command = 0;
    for (std::size_t index = 0; index < plan.batch_count(); ++index) {
        const auto batch = plan.batch(index);
        if (!lower_batch(
                batch,
                {std::span(commands).subspan(first_command), wait, stage, signal},
                submits[index]
            )) {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }
        first_command += batch.commands.size();
    }
    return submit_batches(queue, std::span(submits).first(plan.batch_count()), completion);
}

VkResult present_graph_queue(
    VkQueue queue, const graph::PresentImage& image, const void* extensions
) {
    graph::BreadcrumbScope operation{"graph queue present"};
    const auto swapchain = std::bit_cast<VkSwapchainKHR>(image.swapchain);
    const auto ready = std::bit_cast<VkSemaphore>(image.ready);
    VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.pNext = extensions;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &ready;
    info.swapchainCount = 1;
    info.pSwapchains = &swapchain;
    info.pImageIndices = &image.image;
    const auto result = vkQueuePresentKHR(queue, &info);
    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        operation.complete();
    }
    return result;
}
}  // namespace openrac::vkgl
