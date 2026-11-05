// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements graph_presentation.h with external swapchain images and no new allocation, submit or
// wait.
#include "renderer/vulkan/graph_presentation.h"

#include <bit>
#include <exception>

#include "common/breadcrumbs.h"
#include "renderer/vulkan/graph_barriers.h"

namespace openrac::vkgl::detail {
namespace {
namespace graph = renderer::graph;

// The acquired swapchain contents are discarded; only the presentation copy defines the next screen
// image.
Result<graph::Plan> presentation_plan(
    const Image& source, const Image& destination, bool screen, const PresentationEncode& encode
) {
    graph::Graph builder;
    const auto state = imported_state(source.layout);

    // Unsupported entry layouts cannot supply a complete frame to presentation.
    if (!state) {
        return unexpected(state.error());
    }

    const auto from = builder.import_image(
        "presentation source",
        {{source.serial, 0},
         1,
         1,
         graph::Aspect::Colour,
         *state,
         source.layout != VK_IMAGE_LAYOUT_UNDEFINED
             && !(source.discarded & VK_IMAGE_ASPECT_COLOR_BIT)}
    );
    // Swapchain handles have an independent generation namespace from renderer-owned image serials.
    const auto to = builder.import_image(
        "presentation destination",
        {{std::bit_cast<std::uint64_t>(destination.image), 2},
         1,
         1,
         graph::Aspect::Colour,
         {},
         false}
    );

    // Reject incompatible image identities before recording acquired-image barriers.
    if (!from || !to) {
        return unexpected(!from ? from.error() : to.error());
    }

    auto copy_source = *from;
    if (encode.record) {
        auto encoded = *to;
        if (encode.staging) {
            const auto staging_state = imported_state(encode.staging->layout);
            if (!staging_state) {
                return unexpected(staging_state.error());
            }
            const auto staging = builder.import_image(
                "HDR staging",
                {{encode.staging->serial, 0}, 1, 1, graph::Aspect::Colour, *staging_state, false}
            );
            if (!staging) {
                return unexpected(staging.error());
            }
            encoded = *staging;
            copy_source = encoded;
        }
        builder.add_pass(
            {"HDR encode",
             {{*from,
               {},
               {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Sampled}},
              {encoded,
               {},
               {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::General}}}}
        );
    }
    if (!encode.record || encode.staging) {
        builder.add_pass(
            {"presentation copy",
             {{copy_source,
               {},
               {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}},
              {*to,
               {},
               {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}}}
        );
    }
    builder.add_pass(
        {"presentation consumer",
         {{*to,
           {},
           {screen ? graph::Stage::All : graph::Stage::Transfer,
            graph::Access::Read,
            screen ? graph::Layout::Present : graph::Layout::TransferSource}}},
         {},
         graph::PassKind::Export}
    );
    return builder.compile(frame_capture_retention());
}
}  // namespace

Result<void> record_presentation(
    VkCommandBuffer command,
    const ImagePtr& source,
    Image& destination,
    bool screen,
    const std::function<void()>& copy,
    const PresentationEncode& encode
) {
    // Presentation cannot borrow missing images or change the allocation while a callback records.
    if (!source || !destination.image || !command || ((!encode.record || encode.staging) && !copy)
        || source->image == destination.image
        || (encode.staging
            && (!encode.record || !encode.staging->image || encode.staging->image == source->image
                || encode.staging->image == destination.image))) {
        return unexpected(Error{"invalid presentation graph bindings"});
    }

    const auto plan = presentation_plan(*source, destination, screen, encode);

    // Compilation must succeed before the presentation callback or either image changes state.
    if (!plan) {
        return unexpected(plan.error());
    }

    // Owners and swapchain fences outlive this synchronous callback, even if its recording fails.
    source->touched = g->recording;
    destination.touched = g->recording;
    if (encode.staging) {
        encode.staging->touched = g->recording;
    }

    // Native recording errors remain inside the presentation graph boundary.
    try {
        // The acquired image is fully copied before its external consumer can see it.
        for (unsigned index = 0; index < plan->passes.size(); ++index) {
            const auto& pass = plan->passes[index];
            const auto event =
                capture_graph_pass(*plan, index, index == 0, CaptureDomain::Presentation);
            // Apply each compiled transition on the already chosen main or presentation command
            // buffer.
            for (const auto& transition : pass.barriers) {
                const auto& description =
                    std::get<graph::ImageDesc>(plan->resources[transition.resource].description);
                auto& image = description.key.generation == 2              ? destination
                              : description.key.identity == source->serial ? *source
                                                                           : *encode.staging;
                // The first screen transition joins the semaphore wait on its submission.
                const VkPipelineStageFlags wait_stage = encode.record
                                                            ? VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
                                                            : VK_PIPELINE_STAGE_TRANSFER_BIT;
                const VkPipelineStageFlags acquire =
                    screen && &image == &destination
                            && transition.before.layout == graph::Layout::Undefined
                        ? wait_stage
                        : 0;
                if (screen && &image == &destination) {
                    const TracyPresentZone timing(
                        command,
                        acquire ? "Swapchain acquire transition" : "Swapchain present transition"
                    );
                    record_graph_image_barrier(command, transition, image, acquire);
                } else {
                    record_graph_image_barrier(command, transition, image);
                }
            }

            // Export phases change visibility only and must not repeat the native screen copy.
            if (pass.kind == graph::PassKind::Work) {
                crash::breadcrumb(pass.name);
                if (g->gpu_breadcrumbs) {
                    record_gpu_breadcrumb(command, pass.name, false);
                }
                if (encode.record && index == 0) {
                    const TracyPresentZone timing(command, "HDR encode");
                    encode.record();
                } else if (screen) {
                    const TracyPresentZone timing(command, "Swapchain copy");
                    copy();
                } else {
                    copy();
                }
                if (g->gpu_breadcrumbs) {
                    record_gpu_breadcrumb(command, pass.name, true);
                }
            }
            complete_graph_pass(*plan, index, event);
        }

        return {};
    } catch (const std::exception& failure) {
        abort_frame_capture(failure.what());
        return unexpected(Error{failure.what()});
    } catch (...) {
        abort_frame_capture("presentation callback threw an unknown exception");
        return unexpected(Error{"presentation callback threw an unknown exception"});
    }
}
}  // namespace openrac::vkgl::detail
