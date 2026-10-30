// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Weighted launcher progress, per-level native transcripts and a recoverable complete-run summary.
#include <algorithm>
#include <format>
#include <sstream>
#include <stdexcept>

#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/coordinator_protocol.h"
#include "enhanced/tools/probes/pipeline_internal.h"

namespace openrac::lighting::pipeline {
Events::Events(const Options& options, const std::vector<Level>& levels, std::ostream& output)
    : m_options(options),
      m_levels(levels),
      m_output(output),
      m_log(options.output / "probe-bake.log", std::ios::app),
      m_fractions(levels.size(), 0) {
    // A failed transcript open must stop before any level is admitted.
    if (!m_log) {
        throw std::runtime_error("cannot write probe-bake.log");
    }

    // Initial summary rows distinguish unfinished levels from prior run failures.
    for (const auto& level : levels) {
        m_results.push_back(parsed(
            "{\"level\":" + coordinator_string(level.path.filename().string())
            + ",\"verified\":false}"
        ));
    }

    write("Native enhanced bake started");
    summary();
    progress();
}

void Events::write(const std::string& text) {
    m_output << text << '\n' << std::flush;
    m_log << text << '\n' << std::flush;

    // Log failures cannot silently produce a successful preparation report.
    if (!m_output || !m_log) {
        throw std::runtime_error("cannot write bake transcript");
    }
}

void Events::progress() {
    double total = 0;
    double completed = 0;

    // Export size weights keep aggregate progress comparable across large and small levels.
    for (std::size_t index = 0; index < m_levels.size(); ++index) {
        const double weight =
            static_cast<double>(std::max(std::uint64_t{1}, m_levels[index].bytes));
        total += weight;
        completed += weight * m_fractions[index];
    }

    const double percent = 100 * completed / total;

    // Half-percent updates keep progress output bounded during tracing.
    if (percent - m_shown >= .5 || m_finished == m_levels.size()) {
        write(std::format("Lighting {:.1f}%", percent));
        m_shown = percent;
    }
}

void Events::line(unsigned id, const std::string& text) {
    const std::lock_guard lock(m_mutex);
    write(text);

    // Only native probe counters update the current level fraction.
    if (text.starts_with("Probes ")) {
        std::istringstream stream(text.substr(7));
        double done = 0;
        double total = 0;
        char separator = 0;

        // Malformed counters must not create invalid progress percentages.
        if (stream >> done >> separator >> total && separator == '/' && total > 0) {
            m_fractions.at(id) = std::clamp(done / total, 0., .99);
            progress();
        }
    }
}

void Events::summary() {
    std::string records = "[";

    // Summary rows remain in stable numeric level order.
    for (const auto& record : m_results) {
        // JSON separators belong between result records only.
        if (records.size() > 1) {
            records += ',';
        }

        records += bake_json::json_text(record);
    }

    save(
        m_options.output / "bake-all-result.json",
        parsed(
            "{\"version\":1,\"levels_dir\":" + coordinator_string(m_options.levels.string())
            + ",\"levels\":" + records + "]}"
        )
    );
}

void Events::finish(unsigned id, const json::Value& result) {
    const std::lock_guard lock(m_mutex);
    m_results.at(id) = result;
    m_fractions.at(id) = 1;
    summary();
    ++m_finished;
    std::string status = "failed";

    // Verification and publication distinguish a retained cache from a newly completed bake.
    if (result["verified"].boolean()) {
        status = result["kept"].boolean() ? "up to date" : "baked";
    }

    write(
        std::format(
            "[{}/{}] {}: {}",
            m_finished,
            m_levels.size(),
            m_levels.at(id).path.filename().string(),
            status
        )
    );
    progress();
}

bool Events::passed() const {
    const std::lock_guard lock(m_mutex);
    return m_finished == m_results.size()
           && std::all_of(m_results.begin(), m_results.end(), [](const json::Value& value) {
                  return value["verified"].boolean();
              });
}

Output::Output(Events& events, unsigned id, const std::filesystem::path& directory)
    : m_events(events),
      m_id(id),
      m_log(directory / "batch.log", std::ios::app) {
    // A per-level log must exist before native tracing starts.
    if (!m_log) {
        throw std::runtime_error("cannot write native batch.log");
    }
}

void Output::finish() {
    // A final diagnostic without a newline still belongs in the transcript.
    if (!m_pending.empty()) {
        m_events.line(m_id, m_pending);
        m_pending.clear();
    }

    m_log.flush();

    // Delayed log errors must propagate before installation.
    if (!m_log) {
        throw std::runtime_error("cannot flush native batch.log");
    }
}

std::streamsize Output::xsputn(const char* bytes, std::streamsize count) {
    m_log.write(bytes, count);

    // An unwritten batch log must stop native output processing.
    if (!m_log) {
        throw std::runtime_error("cannot write native batch.log");
    }

    // Complete lines allow progress reporting without retaining the full native transcript.
    for (std::streamsize index = 0; index < count; ++index) {
        // Only complete diagnostic lines are forwarded between controllers.
        if (bytes[index] == '\n') {
            m_events.line(m_id, m_pending);
            m_pending.clear();
        } else {
            m_pending += bytes[index];
        }

        // A malformed native diagnostic cannot grow memory without a bound.
        if (m_pending.size() > 128ull * 1024) {
            throw std::runtime_error("native diagnostic line exceeds 128 KiB");
        }
    }

    return count;
}

Output::int_type Output::overflow(int_type value) {
    // Stream EOF is a flush boundary, not a diagnostic byte.
    if (traits_type::eq_int_type(value, traits_type::eof())) {
        return traits_type::not_eof(value);
    }

    const char byte = traits_type::to_char_type(value);
    xsputn(&byte, 1);
    return value;
}
}  // namespace openrac::lighting::pipeline
