// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned configuration, cache operations and line-based diagnostics for the automatic native pipeline.
#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <span>
#include <streambuf>
#include <string>
#include <vector>

#include "common/json.h"
#include "enhanced/tools/probes/activation.h"

namespace openrac::lighting {
class BakeThreadPool;

namespace pipeline {
// All paths are resolved before any destructive cache operation or controller admission.
struct Options {
    std::filesystem::path levels;  // Immutable export root.
    std::filesystem::path output;  // Cache root outside exports.
    std::filesystem::path profiles =
        "port/enhanced/lighting/profiles";  // Reviewed shared authoring.
    unsigned jobs = 0;         // Zero uses one admitted level per four tracing workers.
    unsigned threads = 0;      // Resolved background CPU budget.
    std::uint64_t memory = 0;  // Resolved resident scene admission budget in bytes.
    unsigned rays = 0;         // Zero retains the tracked profile's transport setting.
    int bounces = -1;          // Negative retains the profile; zero is a valid direct-only bake.
    bool update = false;       // Existing output roots require explicit cache mode.
    bool dry_run = false;      // Validate and list without creating or changing files.
};

// One sorted export and conservative admission reservation.
struct Level {
    std::filesystem::path path;  // Canonical source directory.
    std::uint64_t bytes = 0;     // Export bytes, used for progress and largest-first scheduling.
    std::uint64_t memory = 0;    // At least one GiB, or four times export bytes.
};

// OS-held cache lock; closing it releases ownership even after an interrupted bake.
class CacheLock {
public:
    // Acquires the same first-byte/whole-file lock as the Python reference flow.
    explicit CacheLock(const std::filesystem::path& directory);
    // Releases the descriptor and its process-held lock.
    ~CacheLock();
    CacheLock(const CacheLock&) = delete;
    CacheLock& operator=(const CacheLock&) = delete;
    CacheLock(CacheLock&&) = delete;
    CacheLock& operator=(CacheLock&&) = delete;

private:
    int m_file = -1;  // Owned platform file descriptor.
};

// Aggregates progress, transcript and atomic summary updates from native controllers.
class Events {
public:
    // Borrows configuration and levels until every controller has joined.
    Events(const Options& options, const std::vector<Level>& levels, std::ostream& output);
    // Records one complete controller diagnostic; Probes lines update weighted progress.
    void line(unsigned id, const std::string& text);
    // Publishes a verified or failed level result and releases its progress allocation.
    void finish(unsigned id, const json::Value& result);
    // Reports whether every admitted level completed successfully.
    bool passed() const;

private:
    // Writes console and transcript with m_mutex held.
    void write(const std::string& text);
    // Stages the current complete-run summary with m_mutex held.
    void summary();
    // Publishes one weighted progress line with m_mutex held.
    void progress();
    const Options& m_options;            // Borrowed immutable configuration.
    const std::vector<Level>& m_levels;  // Borrowed sorted export descriptors.
    std::ostream& m_output;              // Borrowed until controllers join.
    std::ofstream m_log;                 // Owned session transcript.
    mutable std::mutex m_mutex;          // Guards output, fractions and results.
    std::vector<double> m_fractions;     // Fraction per level, capped until verified publication.
    std::vector<json::Value> m_results;  // Current summary records in numeric level order.
    unsigned m_finished = 0;             // Completed or failed controllers; guarded by m_mutex.
    double m_shown = -1;                 // Last reported percentage; guarded by m_mutex.
};

// Converts formatted batch output to complete transcript/progress lines without retaining it all.
class Output : public std::streambuf {
public:
    // Owns one per-level log; the shared event sink outlives this controller.
    Output(Events& events, unsigned id, const std::filesystem::path& directory);
    // Publishes any final unterminated diagnostic before releasing its log.
    void finish();

private:
    std::streamsize xsputn(const char* bytes, std::streamsize count) override;
    int_type overflow(int_type value) override;
    Events& m_events;       // Borrowed synchronized sink.
    unsigned m_id;          // Stable sorted level index.
    std::ofstream m_log;    // Owned native batch log.
    std::string m_pending;  // Bounded current diagnostic line.
};

// Resolves CLI settings and rejects invalid or recursive output paths before writing.
Options arguments(std::span<const std::string> arguments);
// Discovers and validates all exported levels in numeric order before admission.
std::vector<Level> discover(const Options& options);
// Resolves and validates region defaults from the same tracked per-level profile.
json::Value region_settings(const std::filesystem::path& level);
// Resolves the profile and the two transport overrides accepted by automatic baking.
LevelBakeProfile profile(const std::filesystem::path& level, const Options& options);
// Resolves background defaults and lowers native worker priority without graphics initialization.
void background(Options& options);
// Validates retained sources and prepares explicit emission and native water fields.
void prepare_sources(const Options& options, const std::filesystem::path& level);
// Builds a semantic cache identity, independent of executable build or file timestamps.
json::Value stamp(const Options& options, const std::filesystem::path& level);
// Refreshes the resumable source identity after native preparation generates interior authoring.
void update_partial_sources(const std::filesystem::path& output, std::uint64_t source);
// Checks a matching complete stamp and every registered fixed asset before reusing a level.
bool current(const std::filesystem::path& output, const json::Value& identity);
// Restores a retained previous directory before checking an interrupted cache installation.
void recover(const std::filesystem::path& final);
// Restores interrupted publication and replaces a final directory with rollback on failure.
void publish(const std::filesystem::path& work, const std::filesystem::path& final);
// Removes only a verified immediate child of the output cache, never an arbitrary recursive path.
void remove_cache_child(const std::filesystem::path& child, const std::filesystem::path& root);
// Runs one admitted level through preparation, cache checks, native baking and publication.
void run_level(
    const Options& options, const Level& level, unsigned id, Events& events, BakeThreadPool& pool
);
// Wraps a parsed generated JSON value; malformed generated text throws at the controller boundary.
json::Value parsed(const std::string& text);
// Publishes JSON through the shared staged-file writer, propagating write failures.
void save(const std::filesystem::path& path, const json::Value& value);
}  // namespace pipeline
}  // namespace openrac::lighting
