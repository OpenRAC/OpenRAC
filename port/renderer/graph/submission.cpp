// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Orders command streams and retires resource owners after submission completion.
#include "renderer/graph/submission.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "renderer/graph/breadcrumbs.h"

namespace openrac::renderer::graph {

namespace {
bool matches_presentation(const PresentImage& image, bool has_signal) {
    return image.swapchain && image.ready && has_signal;
}

// Rejects invalid stream edges and external binary semaphores before scheduling work.
Result<void> validate_streams(
    std::span<const CommandStream> streams, const std::optional<PresentImage>& presentation
) {
    std::unordered_set<std::uint64_t> commands, waits, signals;
    for (const auto& stream : streams) {
        if (!stream.commands || !commands.insert(stream.commands).second) {
            return unexpected(Error{stream.name + ": missing or repeated command stream"});
        }
        std::unordered_set<unsigned> predecessors;
        for (const auto predecessor : stream.predecessors) {
            if (predecessor >= streams.size() || !predecessors.insert(predecessor).second) {
                return unexpected(Error{stream.name + ": invalid or repeated predecessor"});
            }
        }
        for (const auto& wait : stream.waits) {
            if (!wait.semaphore || (wait.stages != Stage::All && wait.stages != Stage::Transfer)
                || !waits.insert(wait.semaphore).second) {
                return unexpected(Error{stream.name + ": invalid or repeated semaphore wait"});
            }
        }
        for (const auto signal : stream.signals) {
            if (!signal || !signals.insert(signal).second) {
                return unexpected(Error{stream.name + ": invalid or repeated semaphore signal"});
            }
        }
    }
    // These binary semaphores cross the submission boundary. Internal queue
    // dependencies are edges, not a wait that can deadlock this same submission.
    for (const auto wait : waits) {
        if (signals.contains(wait)) {
            return unexpected(Error{"submission waits and signals the same external semaphore"});
        }
    }
    if (presentation
        && !matches_presentation(*presentation, signals.contains(presentation->ready))) {
        return unexpected(Error{"presentation has no swapchain or matching producer signal"});
    }
    return {};
}
}  // namespace

Result<FrameSubmissionPlan> FrameSubmissionPlan::compile(
    std::uint64_t upload,
    std::uint64_t draw,
    std::uint64_t present,
    std::uint64_t acquired,
    std::uint64_t signal,
    const std::optional<PresentImage>& presentation
) {
    BreadcrumbScope operation{"graph schedule"};
    if (!upload || !draw || upload == draw || present == upload || present == draw) {
        return unexpected(Error{"frame submission has a missing or repeated command stream"});
    }
    if (acquired && acquired == signal) {
        return unexpected(Error{"submission waits and signals the same external semaphore"});
    }
    if (presentation && !matches_presentation(*presentation, presentation->ready == signal)) {
        return unexpected(Error{"presentation has no swapchain or matching producer signal"});
    }
    FrameSubmissionPlan plan;
    plan.m_commands = {upload, draw, present};
    plan.m_acquired = {acquired, present ? Stage::Transfer : Stage::All};
    plan.m_signal = signal;
    operation.complete();
    return plan;
}

std::size_t FrameSubmissionPlan::batch_count() const {
    return m_commands.back() && m_acquired.semaphore ? 2 : 1;
}

SubmissionBatchView FrameSubmissionPlan::batch(std::size_t index) const {
    const std::span commands{m_commands};
    const bool split = batch_count() == 2;
    if (split && index == 0) {
        return {commands.first(2), {}, {}};
    }
    return {
        split ? commands.last(1) : commands.first(m_commands.back() ? 3 : 2),
        {&m_acquired, m_acquired.semaphore ? 1u : 0u},
        {&m_signal, m_signal ? 1u : 0u}
    };
}

unsigned SubmissionGraph::add_stream(CommandStream stream) {
    if (m_streams.size() >= std::numeric_limits<unsigned>::max()) {
        throw std::length_error("too many graph command streams");
    }
    const auto index = static_cast<unsigned>(m_streams.size());
    m_streams.push_back(std::move(stream));
    return index;
}

Result<SubmissionPlan> SubmissionGraph::compile() const {
    BreadcrumbScope operation{"graph schedule"};
    SubmissionPlan plan;
    const auto valid = validate_streams(m_streams, m_presentation);
    if (!valid) {
        return unexpected(valid.error());
    }
    plan.presentation = m_presentation;
    std::vector<bool> emitted(m_streams.size());
    while (plan.order.size() < m_streams.size()) {
        unsigned next = static_cast<unsigned>(m_streams.size());
        for (unsigned index = 0; index < m_streams.size(); ++index) {
            if (!emitted[index]
                && std::ranges::all_of(m_streams[index].predecessors, [&](unsigned parent) {
                       return emitted[parent];
                   })) {
                next = index;
                break;
            }
        }
        if (next == m_streams.size()) {
            return unexpected(Error{"cycle in graph command-stream dependencies"});
        }
        emitted[next] = true;
        plan.order.push_back(next);
        const auto& stream = m_streams[next];
        // Keep acquisition waits off earlier streams. A signal also closes a
        // batch so it cannot be postponed past later unrelated work.
        if (plan.batches.empty() || !stream.waits.empty() || !plan.batches.back().signals.empty()) {
            plan.batches.emplace_back();
        }
        auto& batch = plan.batches.back();
        batch.commands.push_back(stream.commands);
        if (!stream.waits.empty()) {
            batch.waits = stream.waits;
        }
        batch.signals = stream.signals;
    }
    operation.complete();
    return plan;
}

void SubmissionLifetime::retain(std::shared_ptr<void> resource) {
    const auto identity = resource.get();
    if (resource && m_identities.insert(identity).second) {
        try {
            m_resources.push_back(std::move(resource));
        } catch (...) {
            m_identities.erase(identity);
            throw;
        }
    }
}

void SubmissionLifetime::retire(std::function<void()> destroy) {
    if (!destroy) {
        throw std::invalid_argument("empty graph resource retirement");
    }
    m_retirements.push_back(std::move(destroy));
}

Result<void> SubmissionLifetime::complete(bool gpu_complete) {
    BreadcrumbScope operation{"graph retirement"};
    if (m_pending && !gpu_complete) {
        return unexpected(Error{"graph resources are still in use by a submission"});
    }
    m_pending = false;
    // Run callbacks after detaching the old recording's ownership. Destructors
    // may append retirements to the next recording without invalidating iteration.
    auto resources = std::move(m_resources);
    auto retirements = std::move(m_retirements);
    m_resources.clear();
    m_retirements.clear();
    m_identities.clear();
    for (auto& destroy : retirements) {
        destroy();
    }
    // Release callback captures before their resources, then record retirement completion.
    retirements.clear();
    resources.clear();
    // A callback may have started filling the next recording. Reclaim capacity only when idle.
    if (m_retirements.empty() && m_retirements.capacity() < retirements.capacity()) {
        m_retirements.swap(retirements);
    }
    if (m_resources.empty() && m_resources.capacity() < resources.capacity()) {
        m_resources.swap(resources);
    }
    operation.complete();
    return {};
}

}  // namespace openrac::renderer::graph
