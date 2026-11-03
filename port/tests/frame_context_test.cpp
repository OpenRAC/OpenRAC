// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Frame-wide diagnostic contracts using synthetic allocations and declared accesses, without a GPU.
#include <algorithm>
#include <exception>

#include "renderer/graph/frame_context.h"
#include "tests/check.h"

namespace {
namespace graph = openrac::renderer::graph;
constexpr graph::State kWrite{
    graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::General
};
constexpr graph::State kDiscard{
    graph::Stage::ColourOutput, graph::Access::Write, graph::Layout::ColourAttachment
};
constexpr graph::State kRead{
    graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled
};

// Creates independent source handles for the same physical image, optionally discarded by its
// writer.
graph::Plan image_plan(bool writer, bool discard = false, std::uint64_t generation = 0) {
    graph::Graph builder;
    const auto image = builder.import_image(
        "shared colour",
        {{1, generation}, 1, 1, graph::Aspect::Colour, writer ? graph::State{} : kRead, !writer}
    );
    CHECK(image);
    // A discarded attachment invalidates contents without changing the storage writer case.
    auto state = kRead;
    // Discard invalidation requires an attachment write in the synthetic contract.
    if (writer) {
        state = discard ? kDiscard : kWrite;
    }
    builder.add_pass(
        {writer ? "producer" : "consumer",
         {{*image,
           {},
           state,
           graph::Load::None,
           discard ? graph::Store::Discard : graph::Store::Preserve}}}
    );

    // A discarded writer cannot promise a valid read export.
    if (writer && !discard) {
        builder.add_pass({"producer export", {{*image, {}, kRead}}, {}, graph::PassKind::Export});
    }

    const auto plan = builder.compile(graph::DeclarationRetention::Retain);
    // Print the rejected synthetic contract before the assertion reports failure.
    if (!plan) {
        std::fprintf(stderr, "%s\n", plan.error().message.c_str());
    }
    CHECK(plan);
    return *plan;
}

// Appends a validated flat subgraph in its original execution order.
void record_all(
    graph::FrameContext& frame,
    const graph::Plan& plan,
    graph::FrameEntry entry = graph::FrameEntry::Declared
) {
    // Only the first pass reconciles a subgraph's external entry contract.
    for (unsigned index = 0; index < plan.passes.size(); ++index) {
        const auto recorded =
            frame.record(plan, index, index == 0 ? entry : graph::FrameEntry::Declared);
        CHECK(recorded);
        CHECK(frame.complete(plan, index, *recorded));
    }
}

// Joins aliases and tracks a producer through its export into another graph's consumer.
void test_cross_graph_dependencies() {
    graph::FrameContext frame;
    record_all(frame, image_plan(true));
    record_all(frame, image_plan(false));
    const auto plan = frame.compile();
    CHECK(plan);
    CHECK(plan->resources.size() == 1);
    CHECK(plan->resources[0].first == 0);
    CHECK(plan->resources[0].last == 2);
    CHECK(plan->passes[1].kind == graph::PassKind::Export);
    CHECK(
        std::any_of(
            plan->dependencies.begin(),
            plan->dependencies.end(),
            [](const graph::Dependency& candidate) {
                return candidate.source == 0 && candidate.destination == 2
                       && candidate.resource == 0;
            }
        )
    );
    CHECK(plan->describe() == frame.compile()->describe());
}

// A later local import cannot silently revive contents discarded by a preceding declared writer.
void test_discard_across_graphs() {
    graph::FrameContext frame;
    record_all(frame, image_plan(true, true));
    record_all(frame, image_plan(false));
    CHECK(!frame.compile());
}

// Explicit untracked work can supply new imported contents, retaining a visible conservative
// producer.
void test_opaque_boundary() {
    graph::FrameContext frame;
    record_all(frame, image_plan(true, true));
    record_all(frame, image_plan(false), graph::FrameEntry::Opaque);
    const auto plan = frame.compile();
    CHECK(plan);
    CHECK(plan->passes[1].name.find("opaque") != std::string::npos);
    CHECK(plan->resources[0].last == 2);
}

// Nested callbacks receive a completion event after their children, extending their allocations'
// lifetimes.
void test_nested_completion() {
    graph::FrameContext frame;
    const auto parent = image_plan(true);
    const auto child = image_plan(false);
    const auto event = frame.record(parent, 0, graph::FrameEntry::Declared);
    CHECK(event);
    record_all(frame, child);
    CHECK(frame.complete(parent, 0, *event));
    const auto exported = frame.record(parent, 1, graph::FrameEntry::Declared);
    CHECK(exported);
    CHECK(frame.complete(parent, 1, *exported));
    const auto plan = frame.compile();
    CHECK(plan);
    CHECK(plan->passes.size() == 4);
    CHECK(plan->passes[2].name == "producer (callback completion)");
    CHECK(plan->resources[0].last == 3);
}

// Recycled allocations require distinct generations, and normal plans retain no diagnostic copies.
void test_retention_and_generations() {
    graph::Graph empty;
    CHECK(empty.compile()->declarations.empty());
    graph::FrameContext frame;
    record_all(frame, image_plan(true));
    record_all(frame, image_plan(false, false, 1));
    CHECK(frame.compile()->resources.size() == 2);
    CHECK(!frame.record(*empty.compile(), 0, graph::FrameEntry::Declared));
}

// Separate buffer byte intervals retain their initialization and dependencies across subgraphs.
void test_partial_buffers() {
    graph::Graph writer;
    const auto buffer = writer.import_buffer(
        "shared buffer",
        {{2, 1}, 16, {graph::Stage::None, graph::Access::None, graph::Layout::Buffer}, false}
    );
    CHECK(buffer);
    writer.add_pass(
        {"prefix",
         {},
         {{*buffer,
           {0, 8},
           {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}}}
    );
    graph::Graph reader;
    const auto alias = reader.import_buffer(
        "same buffer",
        {{2, 1}, 16, {graph::Stage::All, graph::Access::ReadWrite, graph::Layout::Buffer}, true}
    );
    CHECK(alias);
    reader.add_pass(
        {"read prefix",
         {},
         {{*alias,
           {0, 8},
           {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer}}}}
    );
    graph::FrameContext frame;
    record_all(frame, *writer.compile(graph::DeclarationRetention::Retain));
    record_all(frame, *reader.compile(graph::DeclarationRetention::Retain));
    CHECK(frame.compile());
    reader.add_pass(
        {"read unwritten suffix",
         {},
         {{*alias,
           {8, 8},
           {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer}}}}
    );
    record_all(frame, *reader.compile(graph::DeclarationRetention::Retain));
    CHECK(!frame.compile());
}

// Incomplete callbacks, changed live allocation shapes and failed imports cannot publish a partial
// plan.
void test_invalid_capture() {
    const auto source = image_plan(true);
    graph::FrameContext frame;
    const auto event = frame.record(source, 0, graph::FrameEntry::Declared);
    CHECK(event);
    CHECK(!frame.compile());
    CHECK(frame.complete(source, 0, *event));
    auto changed = source;
    std::get<graph::ImageDesc>(changed.resources[0].description).mips = 2;
    CHECK(!frame.record(changed, 0, graph::FrameEntry::Declared));
    CHECK(!frame.compile());
    graph::FrameContext duplicate;
    const auto second = duplicate.record(source, 0, graph::FrameEntry::Declared);
    CHECK(second);
    CHECK(duplicate.complete(source, 0, *second));
    CHECK(!duplicate.complete(source, 0, *second));
    CHECK(!duplicate.compile());
}
}  // namespace

// Runs pure CPU contracts; unexpected exceptions report failure without escaping the test process.
int main() {
    // Standalone tests must report unexpected exceptions as failures.
    try {
        test_cross_graph_dependencies();
        test_discard_across_graphs();
        test_opaque_boundary();
        test_nested_completion();
        test_retention_and_generations();
        test_partial_buffers();
        test_invalid_capture();
        return openrac::test::result();
    } catch (...) {
        return 1;
    }
}
