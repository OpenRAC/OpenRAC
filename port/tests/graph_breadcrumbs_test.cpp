// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks graph crash history against explicit operation order and the bounded ring's message limit.
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "renderer/graph/breadcrumbs.h"
#include "renderer/graph/frame_graph.h"
#include "renderer/graph/submission.h"
#include "tests/check.h"

namespace {
namespace graph = openrac::renderer::graph;
namespace crash = openrac::crash;

// Filters independent test frames without relying on the clock or clearing process-lifetime history.
std::vector<std::string> messages(std::int64_t frame) {
    crash::Breadcrumbs::Snapshot snapshot{};
    const auto count = crash::breadcrumbs().snapshot(snapshot);
    std::vector<std::string> result;
    for (std::size_t index = 0; index < count; ++index) {
        if (snapshot[index].value != frame) {
            continue;
        }
        CHECK(snapshot[index].thread == crash::BreadcrumbThread::Render);
        result.emplace_back(snapshot[index].message);
    }
    return result;
}

// Real planner, scheduler and retirement API calls preserve their order and captured render frame.
void test_successful_operations() {
    constexpr std::int64_t kFrame = 1;  // Distinguishes this test's events from other ring entries.
    const crash::BreadcrumbScope context{crash::BreadcrumbThread::Render, kFrame};
    graph::Graph planner;
    planner.add_pass({"work"});
    CHECK(planner.compile());
    graph::SubmissionGraph scheduler;
    scheduler.add_stream({"draw", 1});
    CHECK(scheduler.compile());
    graph::SubmissionLifetime lifetime;
    lifetime.submitted();
    CHECK(lifetime.complete(true));
    const std::vector<std::string> expected{
        "begin graph compile",
        "complete graph compile",
        "begin graph schedule",
        "complete graph schedule",
        "begin graph retirement",
        "complete graph retirement"
    };
    CHECK(messages(kFrame) == expected);
}

// Rejected plans and incomplete submissions record failure instead of a misleading completion.
void test_rejected_operations() {
    constexpr std::int64_t kFrame = 2;  // Independent captured frame for validation failures.
    const crash::BreadcrumbScope context{crash::BreadcrumbThread::Render, kFrame};
    graph::Graph invalid;
    invalid.add_pass({""});
    CHECK(!invalid.compile());
    graph::SubmissionGraph cycle;
    cycle.add_stream({"self-dependent", 1, {0}});
    CHECK(!cycle.compile());
    graph::SubmissionLifetime pending;
    pending.submitted();
    CHECK(!pending.complete(false));
    CHECK(pending.complete(true));
    const std::vector<std::string> expected{
        "begin graph compile",
        "failed graph compile",
        "begin graph schedule",
        "failed graph schedule",
        "begin graph retirement",
        "failed graph retirement",
        "begin graph retirement",
        "complete graph retirement"
    };
    CHECK(messages(kFrame) == expected);
}

// Temporary labels survive exception unwinding and are capped before reaching crash-report storage.
void test_owned_exception_label() {
    constexpr std::int64_t kFrame = 3;  // Independent frame for the temporary-name lifetime check.
    const crash::BreadcrumbScope context{crash::BreadcrumbThread::Render, kFrame};
    bool interrupted = false;
    try {
        const graph::BreadcrumbScope operation{
            "graph callback", std::string(crash::Breadcrumbs::kMessageSize * 2, 'x')
        };
        throw std::runtime_error("interrupted callback");
    } catch (const std::runtime_error&) {
        interrupted = true;
    }
    CHECK(interrupted);
    std::string expected = "failed graph callback: ";
    expected.append(crash::Breadcrumbs::kMessageSize, 'x');
    expected.resize(crash::Breadcrumbs::kMessageSize - 1);
    const auto history = messages(kFrame);
    CHECK(history.size() == 2);
    if (history.size() == 2) {
        CHECK(history.back() == expected);
    }
}

// Successful standalone planning is silent; a failure remains useful without any frame context.
void test_standalone_failures() {
    crash::Breadcrumbs::Snapshot before{}, after{};
    const auto previous = crash::breadcrumbs().snapshot(before);
    graph::Graph valid;
    valid.add_pass({"standalone"});
    CHECK(valid.compile());
    CHECK(crash::breadcrumbs().snapshot(after) == previous);
    graph::Graph invalid;
    invalid.add_pass({""});
    CHECK(!invalid.compile());
    const auto count = crash::breadcrumbs().snapshot(after);
    CHECK(count == previous + 1);
    if (count != previous + 1) {
        return;
    }
    CHECK(after[count - 1].value == -1);
    CHECK(std::string_view(after[count - 1].message) == "failed graph compile");
}
}  // namespace

int main() {
    try {
        test_successful_operations();
        test_rejected_operations();
        test_owned_exception_label();
        test_standalone_failures();
        return openrac::test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "graph breadcrumbs: %s\n", failure.what());
    } catch (...) {
        std::fprintf(stderr, "graph breadcrumbs: unexpected exception\n");
    }
    return 1;
}
