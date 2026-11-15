// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Diagnostic composition of executed subgraphs, independent of GPU owners and command recording.
#pragma once

#include <optional>
#include <vector>

#include "renderer/graph/frame_graph.h"

namespace openrac::renderer::graph {
// Untracked commands require an explicit conservative boundary before a subgraph's imported uses.
enum class FrameEntry : std::uint8_t {
    Declared,
    Opaque
};

// One frame's owned declarations in actual recording order, including nested callback completion.
// The owner uses one graphics thread; no callbacks, GPU allocations or device state are retained.
// Compiled barriers describe declared ordering only and must not replace the executing subgraphs.
class FrameContext {
public:
    // Records one retained source pass; returns its event index or an invalid-contract diagnostic.
    // Opaque entry reconciles imports against untracked commands without attributing them to a
    // graph.
    [[nodiscard]] Result<unsigned> record(const Plan& source, unsigned pass, FrameEntry entry);
    // Closes a work callback; nested events require a conservative completion event for its
    // accesses.
    [[nodiscard]] Result<void> complete(const Plan& source, unsigned pass, unsigned event);
    // Compiles cross-subgraph dependencies and inclusive lifetimes, preserving actual event order.
    [[nodiscard]] Result<Plan> compile() const;

private:
    // Adds or closes a validated source event; public wrappers preserve the first failure.
    [[nodiscard]] Result<unsigned> record_event(
        const Plan& source, unsigned pass, FrameEntry entry
    );
    [[nodiscard]] Result<void> complete_event(const Plan& source, unsigned pass, unsigned event);

    // Canonicalises physical identities while checking allocation shape independently of entry
    // state.
    [[nodiscard]] Result<std::vector<unsigned>> import(const Plan& source);
    // Adds a conservative producer for imported contents touched by intervening untracked
    // recording.
    void opaque_boundary(const Plan& source, const std::vector<unsigned>& handles);
    // Retains display kinds separately so intermediate subgraph exports can precede later work.
    void append(Pass pass);

    std::optional<Error> m_failure;  // Failed imports/completions invalidate this entire capture.
    std::vector<bool>
        m_closed;   // Every declared callback must complete before a frame can compile.
    Graph m_graph;  // Frame-local access stream, used only for diagnostic compilation.
    std::vector<Resource> m_resources;  // Canonical descriptors supplied to m_graph.
    std::vector<PassKind>
        m_kinds;  // Original export labels, including interleaved subgraph boundaries.
};
}  // namespace openrac::renderer::graph
