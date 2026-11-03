// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements frame_capture.h, collecting existing graph declarations without changing their
// execution.
#include "renderer/vulkan/frame_capture.h"

#include <exception>

#include "common/env.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace detail {
namespace {
namespace graph = renderer::graph;

// Counts native commands without issuing a query or touching command buffers.
std::uint64_t command_count() {
    return commands_recorded(g->submission);
}

// Stops collection at the first diagnostic failure without affecting renderer recording.
void fail_capture(Error failure) {
    g->frame_capture.failure = std::move(failure);
    g->frame_capture.frame.reset();
    g->frame_capture.finished = true;
}
}  // namespace

graph::DeclarationRetention frame_capture_retention() {
    return g && g->frame_capture.frame ? graph::DeclarationRetention::Retain
                                       : graph::DeclarationRetention::Discard;
}

std::optional<unsigned> capture_graph_pass(
    const graph::Plan& plan, unsigned pass, bool entry, CaptureDomain domain
) {
    // The default renderer pays no allocation or declaration-copy cost for frame capture.
    if (!g || !g->frame_capture.frame) {
        return {};
    }

    auto& capture = g->frame_capture;

    // Diagnostic failures remain separate from the command recording they describe.
    try {
        const bool opaque = entry && capture.observed_commands != command_count();
        const auto event = capture.frame->record(
            plan, pass, opaque ? graph::FrameEntry::Opaque : graph::FrameEntry::Declared
        );

        // Invalid collected contracts must not interrupt actual GPU command recording.
        if (!event) {
            fail_capture(event.error());
            return {};
        }

        capture.has_passes = capture.has_passes
                             || (domain == CaptureDomain::Renderer
                                 && plan.passes[pass].kind == graph::PassKind::Work);
        return *event;
    } catch (const std::exception& failure) {
        fail_capture(Error{failure.what()});
    } catch (...) {
        fail_capture(Error{"unknown frame graph capture failure"});
    }

    return {};
}

void complete_graph_pass(const graph::Plan& plan, unsigned pass, std::optional<unsigned> event) {
    // A failed or disabled capture never changes native command ownership or error handling.
    if (!g || !g->frame_capture.frame || !event) {
        return;
    }

    auto& capture = g->frame_capture;

    // Nested graph declarations complete before their enclosing callback's final accesses.
    try {
        const auto result = capture.frame->complete(plan, pass, *event);

        // Reject incomplete diagnostics independently of the renderer result.
        if (!result) {
            fail_capture(result.error());
            return;
        }

        capture.observed_commands = command_count();
    } catch (const std::exception& failure) {
        fail_capture(Error{failure.what()});
    } catch (...) {
        fail_capture(Error{"unknown frame graph completion failure"});
    }
}

void abort_frame_capture(std::string_view reason) {
    // Rendering errors remain the executor's responsibility; only an active diagnostic is
    // invalidated.
    if (g && g->frame_capture.frame) {
        fail_capture(Error{std::string(reason)});
    }
}

void configure_frame_capture() {
    // The environment request captures the first nonempty presented frame and logs it once.
    if (env::flag(env::Var::RenderGraphDump)) {
        const auto result = capture_frame_graph();

        // Reject incomplete diagnostics independently of the renderer result.
        if (!result) {
            log::warn("frame graph capture: {}", result.error().message);
            return;
        }

        g->frame_capture.log_once = true;
    }
}

void finish_frame_capture() {
    auto& capture = g->frame_capture;

    // Disabled diagnostics have no per-frame planning or string formatting cost.
    if (!capture.frame && !capture.log_once) {
        return;
    }

    // Dump generation performs no synchronization and must not break presentation on failure.
    try {
        // Empty startup frames leave the environment request armed for actual declared renderer work.
        if (capture.log_once && !capture.has_passes && !capture.failure) {
            capture.frame = std::make_unique<graph::FrameContext>();
            capture.observed_commands = command_count();
            return;
        }

        // A stopped capture already carries its first diagnostic error.
        if (capture.frame) {
            const auto plan = capture.frame->compile();

            // Cross-subgraph validation failures cannot produce a successful frame dump.
            if (!plan) {
                fail_capture(plan.error());
            } else {
                capture.description =
                    "Declared frame graph (diagnostic only; opaque boundaries retained)\n"
                    + plan->describe();
                capture.finished = true;
                capture.frame.reset();
            }
        }

        // Environment capture produces one report, without logging every displayed frame.
        if (capture.log_once) {
            // Report the diagnostic failure in place of a misleading partial plan.
            if (capture.failure) {
                log::warn("frame graph capture: {}", capture.failure->message);
            } else {
                log::info("{}", capture.description);
            }

            capture.log_once = false;
        }
    } catch (const std::exception& failure) {
        fail_capture(Error{failure.what()});
        capture.log_once = false;
    } catch (...) {
        fail_capture(Error{"unknown frame graph compilation failure"});
        capture.log_once = false;
    }
}
}  // namespace detail

Result<void> capture_frame_graph() {
    // Capture belongs to the current context and its exclusive graphics owner.
    if (!detail::g || !detail::g->open) {
        return unexpected(Error{"frame graph capture requires an active Vulkan context"});
    }

    auto& capture = detail::g->frame_capture;

    // Replacing an active request would lose its incomplete recording interval.
    if (capture.frame) {
        return unexpected(Error{"frame graph capture is already active"});
    }

    capture = {};
    capture.frame = std::make_unique<renderer::graph::FrameContext>();
    capture.observed_commands = detail::command_count();
    return {};
}

Result<std::string> frame_graph_description() {
    // Only completed captures describe the whole requested recording interval.
    if (!detail::g || !detail::g->frame_capture.finished) {
        return unexpected(Error{"no completed frame graph capture"});
    }

    const auto& capture = detail::g->frame_capture;

    // Failed captures expose their reason instead of stale output from an earlier frame.
    if (capture.failure) {
        return unexpected(*capture.failure);
    }

    return capture.description;
}
}  // namespace openrac::vkgl
