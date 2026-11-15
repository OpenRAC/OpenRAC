// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks submission ordering, recording scopes and completion-gated retirement.
#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include "renderer/graph/recording_scope.h"
#include "renderer/graph/submission.h"
#include "tests/check.h"

namespace graph = openrac::renderer::graph;
static_assert(!std::is_copy_constructible_v<graph::SubmissionLifetime>);
static_assert(!std::is_move_constructible_v<graph::SubmissionLifetime>);

namespace {

// Compiled work scopes nest and unwind; exports never authorize native work.
void test_recording_scope() {
    CHECK(!graph::recording_active());
    bool rejected = false;
    try {
        graph::require_recording();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    CHECK(rejected);
    graph::Graph scope_graph;
    scope_graph.add_pass({"compiled work"});
    const auto scope_plan = scope_graph.compile();
    CHECK(scope_plan);
    const auto& work = scope_plan->passes[0];
    try {
        const graph::RecordingScope outer(work);
        CHECK(graph::recording_active());
        {
            const graph::RecordingScope inner(work);
            graph::require_recording();
        }
        CHECK(graph::recording_active());
        throw std::runtime_error("recording failed");
    } catch (const std::runtime_error&) {
    }
    CHECK(!graph::recording_active());
    rejected = false;
    try {
        const graph::PlannedPass export_pass{"export", graph::PassKind::Export, {}};
        const graph::RecordingScope invalid(export_pass);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    CHECK(rejected && !graph::recording_active());
}

// Acquisition waits stay after unrelated uploads and presentation uses its producer signal.
void test_submission_batches() {
    graph::SubmissionGraph scheduler;
    scheduler.add_stream({"upload", 1});
    scheduler.add_stream({"draw", 2, {0}});
    scheduler.add_stream({"present", 3, {1}, {{10, graph::Stage::Transfer}}, {11}});
    scheduler.present({20, 2, 11});
    const auto plan = scheduler.compile();
    CHECK(plan);
    if (plan) {
        CHECK((plan->order == std::vector<unsigned>{0, 1, 2}));
        CHECK(plan->batches.size() == 2);
        CHECK((plan->batches[0].commands == std::vector<std::uint64_t>{1, 2}));
        CHECK(plan->batches[0].waits.empty());
        CHECK(plan->batches[1].waits[0].semaphore == 10);
        CHECK(plan->batches[1].waits[0].stages == graph::Stage::Transfer);
        CHECK(plan->batches[1].signals[0] == 11);
        CHECK(
            plan->presentation && plan->presentation->swapchain == 20
            && plan->presentation->image == 2 && plan->presentation->ready == 11
        );
    }
    scheduler.present({20, 2, 12});
    CHECK(!scheduler.compile());
}

// Invalid dependencies and semaphore reuse fail before queue submission.
void test_submission_validation() {
    graph::SubmissionGraph reordered;
    reordered.add_stream({"consumer", 2, {1}});
    reordered.add_stream({"producer", 1});
    CHECK((reordered.compile()->order == std::vector<unsigned>{1, 0}));
    reordered.add_stream({"same buffer twice", 1});
    CHECK(!reordered.compile());
    graph::SubmissionGraph cycle;
    cycle.add_stream({"a", 1, {1}});
    cycle.add_stream({"b", 2, {0}});
    CHECK(!cycle.compile());
    graph::SubmissionGraph invalid;
    invalid.add_stream({"invalid dependency", 1, {4}});
    CHECK(!invalid.compile());
    graph::SubmissionGraph duplicate_wait;
    duplicate_wait.add_stream({"a", 1, {}, {{10, graph::Stage::All}}});
    duplicate_wait.add_stream({"b", 2, {}, {{10, graph::Stage::Transfer}}});
    CHECK(!duplicate_wait.compile());
    graph::SubmissionGraph deadlock;
    deadlock.add_stream({"same semaphore", 1, {}, {{10, graph::Stage::All}}, {10}});
    CHECK(!deadlock.compile());
    graph::SubmissionGraph signalled;
    signalled.add_stream({"signal", 1, {}, {}, {10}});
    signalled.add_stream({"after signal", 2});
    CHECK(signalled.compile()->batches.size() == 2);
}

void test_frame_submission() {
    constexpr graph::PresentImage image{20, 2, 11};
    for (const std::uint64_t present : {0, 3}) {
        for (const std::uint64_t acquired : {0, 10}) {
            graph::SubmissionGraph scheduler;
            graph::CommandStream upload{"upload", 1};
            graph::CommandStream draw{"draw", 2, {0}};
            graph::CommandStream screen{"present", 3, {1}};
            if (acquired) {
                auto& first_consumer = present ? screen : upload;
                first_consumer.waits.push_back(
                    {acquired, present ? graph::Stage::Transfer : graph::Stage::All}
                );
            }
            (present ? screen : draw).signals.push_back(image.ready);
            scheduler.add_stream(std::move(upload));
            scheduler.add_stream(std::move(draw));
            if (present) {
                scheduler.add_stream(std::move(screen));
            }
            scheduler.present(image);
            const auto expected = scheduler.compile().value();
            const auto actual =
                graph::FrameSubmissionPlan::compile(1, 2, present, acquired, image.ready, image)
                    .value();
            CHECK(actual.batch_count() == expected.batches.size());
            for (std::size_t i = 0; i < actual.batch_count(); ++i) {
                const auto batch = actual.batch(i);
                const auto& reference = expected.batches.at(i);
                CHECK(std::ranges::equal(batch.commands, reference.commands));
                CHECK(std::ranges::equal(batch.signals, reference.signals));
                CHECK(std::ranges::equal(batch.waits, reference.waits, [](auto left, auto right) {
                    return left.semaphore == right.semaphore && left.stages == right.stages;
                }));
            }
        }
    }
    const auto readback = graph::FrameSubmissionPlan::compile(1, 2, 0, 0, 0, {}).value();
    CHECK(readback.batch_count() == 1);
    CHECK(readback.batch(0).commands.size() == 2);
    CHECK(readback.batch(0).waits.empty() && readback.batch(0).signals.empty());
    CHECK(!graph::FrameSubmissionPlan::compile(0, 2, 0, 0, 0, {}));
    CHECK(!graph::FrameSubmissionPlan::compile(1, 2, 1, 0, 0, {}));
    CHECK(!graph::FrameSubmissionPlan::compile(1, 2, 0, 10, 10, {}));
    CHECK(!graph::FrameSubmissionPlan::compile(1, 2, 3, 10, 12, image));
}

// Owners and retirement callbacks survive until completion, including reentrant retirement.
void test_submission_lifetime() {
    graph::SubmissionLifetime lifetime;
    unsigned destroyed = 0;

    struct Resource {
        unsigned& destroyed;

        explicit Resource(unsigned& count) : destroyed(count) {}

        ~Resource() { ++destroyed; }
    };

    auto resource = std::make_shared<Resource>(destroyed);
    lifetime.retain(resource);
    lifetime.retain(resource);
    CHECK(lifetime.retained() == 1);
    resource.reset();
    lifetime.submitted();
    CHECK(!lifetime.complete(false));
    CHECK(destroyed == 0 && lifetime.pending());
    unsigned retirements = 0;
    lifetime.retire([&] { ++retirements; });
    CHECK(lifetime.complete(true));
    CHECK(destroyed == 1 && retirements == 1 && !lifetime.pending());
    CHECK(lifetime.complete(true));
    CHECK(destroyed == 1 && retirements == 1);
    lifetime.retire([&] { lifetime.retire([&] { ++retirements; }); });
    CHECK(lifetime.complete(true));
    CHECK(retirements == 1);
    CHECK(lifetime.complete(true));
    CHECK(retirements == 2);
}

}  // namespace

int main() {
    try {
        test_recording_scope();
        test_submission_batches();
        test_submission_validation();
        test_frame_submission();
        test_submission_lifetime();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "graph submission test: %s\n", error.what());
        return 1;
    }
}
