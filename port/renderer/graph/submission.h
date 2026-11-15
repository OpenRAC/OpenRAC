// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Single-queue submission plans and completion-gated resource ownership.
#pragma once

#include <array>
#include <functional>
#include <memory>
#include <memory_resource>
#include <unordered_set>

#include "renderer/graph/frame_graph.h"

namespace openrac::renderer::graph {

// Opaque backend tokens: the scheduler never dereferences a device object.
struct SemaphoreWait {
    std::uint64_t semaphore = 0;
    Stage stages = Stage::All;  // Acquired-image waits currently support All or Transfer.
};

struct CommandStream {
    std::string name;
    std::uint64_t commands = 0;
    std::vector<unsigned> predecessors{};
    std::vector<SemaphoreWait> waits{};
    std::vector<std::uint64_t> signals{};
};

struct SubmissionBatch {
    std::vector<std::uint64_t> commands;
    std::vector<SemaphoreWait> waits;
    std::vector<std::uint64_t> signals;
};

struct PresentImage {
    std::uint64_t swapchain = 0;
    unsigned image = 0;
    std::uint64_t ready = 0;
};

struct SubmissionBatchView {
    std::span<const std::uint64_t> commands;
    std::span<const SemaphoreWait> waits;
    std::span<const std::uint64_t> signals;
};

// The frame's fixed stream order is uploads, rendering, then an optional acquired-image copy.
class FrameSubmissionPlan {
public:
    [[nodiscard]] static Result<FrameSubmissionPlan> compile(
        std::uint64_t upload,
        std::uint64_t draw,
        std::uint64_t present,
        std::uint64_t acquired,
        std::uint64_t signal,
        const std::optional<PresentImage>& presentation
    );

    [[nodiscard]] std::size_t batch_count() const;
    [[nodiscard]] SubmissionBatchView batch(std::size_t index) const;

private:
    FrameSubmissionPlan() = default;
    std::array<std::uint64_t, 3> m_commands{};
    SemaphoreWait m_acquired;
    std::uint64_t m_signal = 0;
};

struct SubmissionPlan {
    std::vector<SubmissionBatch> batches;
    std::vector<unsigned> order;
    std::optional<PresentImage> presentation;
};

// A single queue's command-stream DAG. External acquisition waits belong to the
// first stream that touches the acquired image, not unrelated earlier work.
class SubmissionGraph {
public:
    unsigned add_stream(CommandStream stream);

    void present(PresentImage image) { m_presentation = image; }

    [[nodiscard]] Result<SubmissionPlan> compile() const;

private:
    std::vector<CommandStream> m_streams;
    std::optional<PresentImage> m_presentation;
};

// One frame slot's resource ownership. Completion must be established by the
// executor's fence/idle boundary before retirement; a failed submit is not completion.
class SubmissionLifetime {
public:
    SubmissionLifetime() = default;
    SubmissionLifetime(const SubmissionLifetime&) = delete;
    SubmissionLifetime& operator=(const SubmissionLifetime&) = delete;
    SubmissionLifetime(SubmissionLifetime&&) = delete;
    SubmissionLifetime& operator=(SubmissionLifetime&&) = delete;
    void retain(std::shared_ptr<void> resource);
    void retire(std::function<void()> destroy);

    void submitted() { m_pending = true; }

    [[nodiscard]] Result<void> complete(bool gpu_complete);

    [[nodiscard]] std::size_t retained() const { return m_resources.size(); }

    [[nodiscard]] bool pending() const { return m_pending; }

private:
    bool m_pending = false;
    std::pmr::unsynchronized_pool_resource m_identity_pool;
    std::pmr::unordered_set<void*> m_identities{&m_identity_pool};
    std::vector<std::shared_ptr<void>> m_resources;
    std::vector<std::function<void()>> m_retirements;
};

}  // namespace openrac::renderer::graph
