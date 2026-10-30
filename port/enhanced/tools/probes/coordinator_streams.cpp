// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Coordinator stream adapters preserve existing inspection and progress lines without interleaving.
#include <stdexcept>

#include "enhanced/tools/probes/coordinator_internal.h"
#include "enhanced/tools/probes/coordinator_protocol.h"

namespace openrac::lighting {
void CoordinatorEvents::emit(
    std::string_view event, unsigned id, std::string_view text, int status
) {
    const std::lock_guard lock(m_mutex);
    m_output << "{\"event\":" << coordinator_string(event) << ",\"id\":" << id
             << ",\"text\":" << coordinator_string(text) << ",\"status\":" << status << "}\n"
             << std::flush;
}

std::streamsize CoordinatorOutput::xsputn(const char* bytes, std::streamsize count) {
    {
        const std::lock_guard lock(m_job.mutex);

        // Progress writes provide cancellation points without interrupting in-flight ray queries.
        if (m_job.closed) {
            throw std::runtime_error("level bake cancelled");
        }
    }
    // Inspection is bounded separately by the client; an unterminated line is never unbounded.
    constexpr std::size_t kMaximumLine = 1024ull * 1024u;

    for (std::streamsize index = 0; index < count; ++index) {
        if (bytes[index] == '\n') {
            m_events.emit("line", m_id, m_pending + '\n');
            m_pending.clear();
        } else {
            if (m_pending.size() == kMaximumLine) {
                throw std::runtime_error("coordinator diagnostic line exceeds 1 MiB");
            }

            m_pending += bytes[index];
        }
    }

    return count;
}

CoordinatorOutput::int_type CoordinatorOutput::overflow(int_type value) {
    if (traits_type::eq_int_type(value, traits_type::eof())) {
        return traits_type::not_eof(value);
    }

    const char byte = traits_type::to_char_type(value);
    xsputn(&byte, 1);
    return value;
}

void CoordinatorOutput::finish() {
    if (!m_pending.empty()) {
        m_events.emit("line", m_id, m_pending);
        m_pending.clear();
    }
}

CoordinatorInput::int_type CoordinatorInput::underflow() {
    std::unique_lock lock(m_job.mutex);
    m_job.changed.wait(lock, [this] { return !m_job.inputs.empty() || m_job.closed; });

    if (m_job.closed) {
        return traits_type::eof();
    }

    m_bytes = std::move(m_job.inputs.front());

    // Client writes may already include their line terminator.
    if (m_bytes.empty() || m_bytes.back() != '\n') {
        m_bytes += '\n';
    }

    m_job.inputs.pop_front();
    setg(m_bytes.data(), m_bytes.data(), m_bytes.data() + m_bytes.size());
    return traits_type::to_int_type(*gptr());
}
}  // namespace openrac::lighting
