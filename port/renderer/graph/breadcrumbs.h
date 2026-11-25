// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded crash history for graph API operations, without routine logging or heap allocation.
#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "common/breadcrumbs.h"

namespace openrac::renderer::graph {

// Copies labels so temporary pass names remain valid until completion or exception unwinding.
// Normal events use the captured frame context; failures also survive outside a frame.
class BreadcrumbScope {
public:
    explicit BreadcrumbScope(std::string_view operation, std::string_view name = {}) noexcept {
        append(m_label, m_size, operation);
        if (!name.empty()) {
            append(m_label, m_size, ": ");
            append(m_label, m_size, name);
        }
        write("begin", false);
    }

    ~BreadcrumbScope() {
        if (!m_complete) {
            write("failed", true);
        }
    }

    BreadcrumbScope(const BreadcrumbScope&) = delete;
    BreadcrumbScope& operator=(const BreadcrumbScope&) = delete;

    // Mark completion only after the operation's Result or native return code reports success.
    void complete() noexcept {
        if (!m_complete) {
            write("complete", false);
            m_complete = true;
        }
    }

private:
    using Message = std::array<char, crash::Breadcrumbs::kMessageSize>;

    // Reserve the terminating NUL even when a label fills the entire crash-history message.
    static void append(Message& message, std::size_t& used, std::string_view text) noexcept {
        const auto count = std::min(text.size(), message.size() - 1 - used);
        if (count != 0) {
            std::copy_n(text.begin(), count, message.begin() + used);
            used += count;
        }
    }

    // Failure events retain the caller's trail even without a captured rendering frame.
    void write(std::string_view phase, bool failed) const noexcept {
        Message message{};
        std::size_t used = 0;
        append(message, used, phase);
        append(message, used, " ");
        append(message, used, {m_label.data(), m_size});
        const std::string_view event{message.data(), used};
        if (failed) {
            crash::failure_breadcrumb(event);
        } else {
            crash::breadcrumb(event);
        }
    }

    Message m_label{};  // Owned operation and pass label, truncated to the ring's message limit.
    std::size_t m_size = 0;   // Valid label bytes, excluding the terminating NUL.
    bool m_complete = false;  // An incomplete operation records failure on scope exit.
};

}  // namespace openrac::renderer::graph
