// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU-only graph contracts. Expected dependencies and ranges follow the declared accesses,
// not a GPU capture, retail data, timing, or the order of worker threads.
#include <algorithm>
#include <cstdio>
#include <exception>
#include <limits>
#include <string_view>

#include "renderer/graph/frame_graph.h"
#include "tests/check.h"

namespace {
namespace graph = openrac::renderer::graph;
using graph::Access;
using graph::Layout;
using graph::Stage;
// Shader states used by synthetic producers, consumers and exports.
constexpr graph::State kWrite{Stage::ComputeShader, Access::Write, Layout::General};
constexpr graph::State kRead{Stage::ComputeShader, Access::Read, Layout::General};
constexpr graph::State kSample{Stage::FragmentShader, Access::Read, Layout::Sampled};
constexpr graph::State kBufferWrite{Stage::ComputeShader, Access::Write, Layout::Buffer};
constexpr graph::State kBufferRead{Stage::ComputeShader, Access::Read, Layout::Buffer};

// Creates a small uninitialised image unless the test supplies explicit imported contents.
graph::ImageHandle image(graph::Graph& builder, std::uint64_t key = 1, bool defined = false) {
    const auto imported = builder.import_image(
        "image",
        {{key, 0}, 3, 2, graph::Aspect::Colour, defined ? kSample : graph::State{}, defined}
    );
    CHECK(imported.has_value());
    return imported.value();
}

// Finds an allocation-specific edge without depending on the diagnostic edge order.
bool edge(const graph::Plan& plan, unsigned from, unsigned to, unsigned resource) {
    return std::any_of(
        plan.dependencies.begin(),
        plan.dependencies.end(),
        [from, to, resource](const graph::Dependency& candidate) {
            return candidate.source == from && candidate.destination == to
                   && candidate.resource == resource;
        }
    );
}

// Rejects both image and buffer reads unless their ranges have imported or graph-written contents.
void test_unwritten_reads() {
    graph::Graph builder;
    const auto texture = image(builder);
    builder.add_pass({"unwritten", {{texture, {}, kRead}}});
    const auto failed = builder.compile();
    CHECK(!failed);
    CHECK(failed.error().message.find("unwritten: read of unwritten") != std::string::npos);
    graph::Graph buffers;
    const auto buffer = buffers.import_buffer("buffer", {{2, 0}, 32});
    CHECK(buffer.has_value());
    buffers.add_pass({"buffer read", {}, {{*buffer, {0, 16}, kBufferRead}}});
    CHECK(!buffers.compile());
}

// Default-constructed handles must never alias the first valid allocation of either resource type.
void test_unbound_handles() {
    graph::Graph images;
    (void)image(images);
    images.add_pass({"unbound image", {{graph::ImageHandle{}, {}, kWrite}}});
    CHECK(!images.compile());
    graph::Graph buffers;
    CHECK(buffers.import_buffer("valid", {{2, 0}, 16}));
    buffers.add_pass({"unbound buffer", {}, {{graph::BufferHandle{}, {0, 16}, kBufferWrite}}});
    CHECK(!buffers.compile());
}

// Shader exports retain exact ranges, remove producer attachment operations and leave the source unchanged.
void test_shader_read_export() {
    graph::Graph builder;
    const auto texture = image(builder);
    const auto buffer = builder.import_buffer("partial output", {{2, 0}, 32});
    CHECK(buffer);
    const graph::ImageRange whole{graph::Aspect::Colour, 0, 3, 0, 2};
    const graph::State attachment{Stage::ColourOutput, Access::Write, Layout::ColourAttachment};
    const graph::Pass producer{
        "produce",
        {{texture, whole, attachment, graph::Load::Clear}},
        {{*buffer, {4, 8}, kBufferWrite}}
    };
    const auto stages = Stage::FragmentShader | Stage::ComputeShader;
    const auto exported = graph::shader_read_export("shader consumers", producer, stages);
    CHECK(exported.name == "shader consumers" && exported.kind == graph::PassKind::Export);
    CHECK(exported.images[0].range == whole);
    CHECK(exported.images[0].state == (graph::State{stages, Access::Read, Layout::Sampled}));
    CHECK(exported.images[0].load == graph::Load::None);
    CHECK(exported.images[0].store == graph::Store::Preserve);
    CHECK(exported.buffers[0].range == (graph::BufferRange{4, 8}));
    CHECK(exported.buffers[0].state == (graph::State{stages, Access::Read, Layout::Buffer}));
    CHECK(producer.kind == graph::PassKind::Work);
    CHECK(producer.images[0].load == graph::Load::Clear);
    builder.add_pass(producer);
    builder.add_pass(exported);
    const auto plan = builder.compile();
    CHECK(plan);
    CHECK(plan->dependencies.size() == 2);
}

// A mip/layer write defines only that subresource, not another mip or layer.
void test_partial_image_contents() {
    graph::Graph builder;
    const auto texture = image(builder);
    builder.add_pass({"write mip", {{texture, {}, kWrite}}});
    builder.add_pass({"read mip", {{texture, {}, kRead}}});
    auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(edge(*plan, 0, 1, texture.index));
    CHECK(std::get<graph::ImageRange>(plan->passes[1].barriers[0].range) == graph::ImageRange{});
    builder.add_pass({"other mip", {{texture, {graph::Aspect::Colour, 1, 1, 0, 1}, kRead}}});
    CHECK(!builder.compile());
    graph::Graph layers;
    const auto array = image(layers);
    layers.add_pass({"first layer", {{array, {}, kWrite}}});
    layers.add_pass({"second layer", {{array, {graph::Aspect::Colour, 0, 1, 1, 1}, kRead}}});
    CHECK(!layers.compile());
}

// Partial buffer writes define no bytes outside their union, and overlap barriers retain exact bounds.
void test_buffer_intervals() {
    graph::Graph builder;
    const auto buffer = builder.import_buffer("buffer", {{3, 0}, 64});
    CHECK(buffer.has_value());
    builder.add_pass({"first", {}, {{*buffer, {0, 16}, kBufferWrite}}});
    builder.add_pass({"second", {}, {{*buffer, {16, 16}, kBufferWrite}}});
    builder.add_pass({"joined read", {}, {{*buffer, {8, 16}, kBufferRead}}});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(plan->passes[2].barriers.size() == 2);
    CHECK((
        std::get<graph::BufferRange>(plan->passes[2].barriers[0].range) == graph::BufferRange{8, 8}
    ));
    CHECK((
        std::get<graph::BufferRange>(plan->passes[2].barriers[1].range) == graph::BufferRange{16, 8}
    ));
    CHECK(edge(*plan, 0, 2, buffer->index));
    CHECK(edge(*plan, 1, 2, buffer->index));
    builder.add_pass({"unwritten tail", {}, {{*buffer, {24, 16}, kBufferRead}}});
    CHECK(!builder.compile());
}

// GENERAL-to-GENERAL writes and reads need barriers even though no image layout changes.
void test_read_write_hazards() {
    graph::Graph builder;
    const auto texture = image(builder);
    builder.add_pass({"write", {{texture, {}, kWrite}}});
    builder.add_pass({"read", {{texture, {}, kRead}}});
    builder.add_pass({"overwrite", {{texture, {}, kWrite}}});
    builder.add_pass({"overwrite again", {{texture, {}, kWrite}}});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    for (unsigned pass = 1; pass < 4; ++pass) {
        CHECK(plan->passes[pass].barriers.size() == 1);
        CHECK(plan->passes[pass].barriers[0].before.layout == Layout::General);
        CHECK(plan->passes[pass].barriers[0].after.layout == Layout::General);
    }
    CHECK(edge(*plan, 0, 1, texture.index));
    CHECK(edge(*plan, 1, 2, texture.index));
    CHECK(edge(*plan, 2, 3, texture.index));
}

// Read-only consumers can share a layout, but a subsequent writer waits for every reader stage.
void test_all_readers_before_writer() {
    graph::Graph builder;
    const auto texture = image(builder, 1, true);
    builder.add_pass(
        {"compute", {{texture, {}, {Stage::ComputeShader, Access::Read, Layout::General}}}}
    );
    builder.add_pass(
        {"fragment", {{texture, {}, {Stage::FragmentShader, Access::Read, Layout::General}}}}
    );
    builder.add_pass({"writer", {{texture, {}, kWrite}}});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(plan->passes[1].barriers.empty());
    CHECK(
        plan->passes[2].barriers[0].before.stages == (Stage::ComputeShader | Stage::FragmentShader)
    );
    CHECK(edge(*plan, 0, 2, texture.index));
    CHECK(edge(*plan, 1, 2, texture.index));
}

// A write-to-read barrier makes its producer visible to compute and fragment consumers of one layout.
void test_all_readers_after_writer() {
    graph::Graph builder;
    const auto buffer = builder.import_buffer("shared AO", {{1, 0}, 64});
    CHECK(buffer.has_value());
    builder.add_pass({"horizons", {}, {{*buffer, {0, 64}, kBufferWrite}}});
    builder.add_pass({"compute reader", {}, {{*buffer, {0, 64}, kBufferRead}}});
    builder.add_pass(
        {"fragment reader",
         {},
         {{*buffer, {0, 64}, {Stage::FragmentShader, Access::Read, Layout::Buffer}}}}
    );
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(plan->passes[1].barriers[0].before == kBufferWrite);
    CHECK(
        plan->passes[1].barriers[0].after.stages == (Stage::ComputeShader | Stage::FragmentShader)
    );
    CHECK(edge(*plan, 0, 1, buffer->index));
    CHECK(edge(*plan, 0, 2, buffer->index));
}

void test_writer_scope_excludes_disjoint_reader() {
    graph::Graph builder;
    const auto buffer = builder.import_buffer("buffer", {{1, 0}, 32});
    CHECK(buffer);
    const graph::State fragment{Stage::FragmentShader, Access::Read, Layout::Buffer};
    builder.add_pass({"initialise", {}, {{*buffer, {0, 32}, kBufferWrite}}});
    builder.add_pass({"read prefix", {}, {{*buffer, {0, 16}, kBufferRead}}});
    builder.add_pass({"overwrite prefix", {}, {{*buffer, {0, 16}, kBufferWrite}}});
    builder.add_pass({"read suffix", {}, {{*buffer, {16, 16}, fragment}}});
    const auto plan = builder.compile();
    CHECK(plan);
    CHECK(
        plan->passes[1].barriers[0].after.stages == (Stage::ComputeShader | Stage::FragmentShader)
    );
    CHECK(plan->passes[2].barriers[0].before == kBufferRead);
    CHECK(plan->passes[3].barriers[0].before == kBufferWrite);
}

// Allocation aliases canonicalise, while recycled generations remain independent resources.
void test_alias_identity() {
    graph::Graph builder;
    const graph::ImageDesc desc{{7, 1}, 1, 1, graph::Aspect::Colour, kSample, true};
    const auto a = builder.import_image("first view", desc);
    const auto b = builder.import_image("second view", desc);
    CHECK(a->index == b->index);
    auto next = desc;
    next.key.generation++;
    const auto c = builder.import_image("new allocation", next);
    CHECK(c->index != a->index);
    auto bad = desc;
    bad.mips = 2;
    CHECK(!builder.import_image("inconsistent view", bad));
    builder.add_pass({"conflict", {{*a, {}, kWrite}, {*b, {}, kRead}}});
    CHECK(!builder.compile());
}

// Duplicate reads are legal, while conflicting writers must use one explicit read/write declaration.
void test_pass_feedback() {
    graph::Graph builder;
    const auto texture = image(builder, 1, true);
    builder.add_pass({"two reads", {{texture, {}, kRead}, {texture, {}, kRead}}});
    CHECK(builder.compile().has_value());
    builder.add_pass(
        {"in place", {{texture, {}, {Stage::ComputeShader, Access::ReadWrite, Layout::General}}}}
    );
    CHECK(builder.compile().has_value());
    builder.add_pass({"two writes", {{texture, {}, kWrite}, {texture, {}, kWrite}}});
    CHECK(!builder.compile());
}

// A shorter compatible read must not hide a conflict with an earlier enclosing range.
void test_nested_range_conflict() {
    graph::Graph builder;
    const auto buffer = builder.import_buffer("buffer", {{1, 0}, 64});
    CHECK(buffer);
    builder.add_pass(
        {"nested read and conflicting write",
         {},
         {{*buffer, {32, 8}, kBufferWrite},
          {*buffer, {8, 8}, kBufferRead},
          {*buffer, {0, 64}, kBufferRead}}}
    );
    const auto plan = builder.compile();
    CHECK(!plan);
    CHECK(plan.error().message.find("overlapping writes") != std::string::npos);
}

// Overflowing ranges and handles of the wrong resource type fail before interval arithmetic.
void test_invalid_ranges() {
    graph::Graph builder;
    const auto texture = image(builder);
    builder.add_pass(
        {"overflow mip",
         {{texture,
           {graph::Aspect::Colour, 1, std::numeric_limits<unsigned>::max(), 0, 1},
           kWrite}}}
    );
    CHECK(!builder.compile());
    graph::Graph buffers;
    const auto buffer = buffers.import_buffer("buffer", {{2, 0}, 64});
    buffers.add_pass(
        {"overflow bytes",
         {},
         {{*buffer, {8, std::numeric_limits<std::uint64_t>::max()}, kBufferWrite}}}
    );
    CHECK(!buffers.compile());
    graph::Graph typed;
    const auto wrong = typed.import_buffer("not an image", {{3, 0}, 64});
    typed.add_pass({"wrong handle", {{{wrong->index}, {}, kWrite}}});
    CHECK(!typed.compile());
    CHECK(!typed.import_image("empty", {{4, 0}, 0}));
}

// Clear permits attachment read/write on fresh data; discard invalidates later consumers.
void test_attachment_contents() {
    graph::Graph builder;
    const auto texture = image(builder);
    const graph::State blend{Stage::ColourOutput, Access::ReadWrite, Layout::ColourAttachment};
    builder.add_pass({"clear", {{texture, {}, blend, graph::Load::Clear}}});
    builder.add_pass({"load", {{texture, {}, blend, graph::Load::Preserve}}});
    CHECK(builder.compile().has_value());
    builder.add_pass(
        {"discard", {{texture, {}, blend, graph::Load::Preserve, graph::Store::Discard}}}
    );
    builder.add_pass({"read discarded", {{texture, {}, kSample}}});
    CHECK(!builder.compile());
    graph::Graph fresh;
    const auto uninitialised = image(fresh);
    fresh.add_pass({"invalid load", {{uninitialised, {}, blend, graph::Load::Preserve}}});
    CHECK(!fresh.compile());
}

// Depth writes never initialise stencil, even when both aspects share an allocation.
void test_depth_stencil_aspects() {
    graph::Graph builder;
    const auto texture = builder.import_image(
        "depth stencil", {{1, 0}, 1, 1, graph::Aspect::Depth | graph::Aspect::Stencil}
    );
    CHECK(texture.has_value());
    builder.add_pass(
        {"depth",
         {{*texture,
           {graph::Aspect::Depth},
           {Stage::DepthTests, Access::Write, Layout::DepthAttachment},
           graph::Load::Clear}}}
    );
    builder.add_pass(
        {"stencil",
         {{*texture,
           {graph::Aspect::Stencil},
           {Stage::DepthTests, Access::Read, Layout::DepthReadOnly}}}}
    );
    CHECK(!builder.compile());
}

// Incompatible layout/stage pairs and attachments of the wrong aspect are rejected.
void test_incompatible_access() {
    graph::Graph sampled;
    const auto texture = image(sampled, 1, true);
    sampled.add_pass(
        {"sampled write", {{texture, {}, {Stage::ComputeShader, Access::Write, Layout::Sampled}}}}
    );
    CHECK(!sampled.compile());
    graph::Graph stages;
    const auto storage = image(stages);
    stages.add_pass(
        {"transfer as compute",
         {{storage, {}, {Stage::ComputeShader, Access::Write, Layout::TransferDestination}}}}
    );
    CHECK(!stages.compile());
    graph::Graph aspects;
    const auto colour = image(aspects);
    aspects.add_pass(
        {"colour as depth",
         {{colour,
           {},
           {Stage::DepthTests, Access::Write, Layout::DepthAttachment},
           graph::Load::Clear}}}
    );
    CHECK(!aspects.compile());
    graph::Graph buffers;
    const auto buffer = buffers.import_buffer("buffer", {{5, 0}, 16});
    buffers.add_pass(
        {"attachment buffer",
         {},
         {{*buffer, {0, 16}, {Stage::ColourOutput, Access::Write, Layout::Buffer}}}}
    );
    CHECK(!buffers.compile());
}

// A read-only layout transition still orders preceding readers before the transition.
void test_read_layout_change() {
    graph::Graph builder;
    const auto texture = image(builder, 1, true);
    builder.add_pass({"sampled", {{texture, {}, kSample}}});
    builder.add_pass({"general read", {{texture, {}, kRead}}});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(edge(*plan, 0, 1, texture.index));
    CHECK(plan->passes[1].barriers[0].before.layout == Layout::Sampled);
    CHECK(plan->passes[1].barriers[0].after.layout == Layout::General);
}

// Imports reject contradictory prior states before any backend can translate their barriers.
void test_invalid_imports() {
    graph::Graph builder;
    graph::ImageDesc description{{1, 0}};
    description.initial = {Stage::DepthTests, Access::Write, Layout::DepthAttachment};
    CHECK(!builder.import_image("colour in depth layout", description));
    description.initial = {Stage::ComputeShader, Access::Write, Layout::Sampled};
    CHECK(!builder.import_image("write sampled image", description));
    description.initial = {Stage::None, Access::Read, Layout::General};
    CHECK(!builder.import_image("access without execution scope", description));
    description.initial = {};
    description.defined = true;
    CHECK(!builder.import_image("contents in undefined layout", description));
    CHECK(!builder.import_buffer("image layout on buffer", {{2, 0}, 16, kSample, true}));

    graph::ImageDesc depth{{3, 0}, 1, 1, graph::Aspect::Depth};
    depth.initial = {Stage::ColourOutput, Access::Read, Layout::DepthAttachment};
    CHECK(!builder.import_image("depth read without depth tests", depth));
    depth.initial = {Stage::All, Access::Read, Layout::Present};
    CHECK(!builder.import_image("present depth image", depth));
    depth.initial = {Stage::ColourOutput, Access::Write, Layout::DepthAttachment};
    CHECK(builder.import_image("depth resolve output", depth));
}

// Unused imports remain unused, and a compiled plan owns its labels independently of the builder.
void test_unused_and_owned_plan() {
    graph::Graph builder;
    const auto unused = image(builder);
    auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(!plan->resources[unused.index].first && !plan->resources[unused.index].last);
    builder = graph::Graph{};
    CHECK(plan->resources[unused.index].name == "image");
    CHECK(plan->passes.empty());
}

// Exports establish future sampling access and resource lifetimes without adding GPU work.
void test_export_and_dump() {
    graph::Graph builder;
    const auto texture = image(builder);
    builder.add_pass({"combine", {{texture, {}, kWrite}}});
    builder.add_pass({"lighting", {{texture, {}, kSample}}, {}, graph::PassKind::Export});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(plan->resources[texture.index].first == 0);
    CHECK(plan->resources[texture.index].last == 1);
    CHECK(plan->passes[1].kind == graph::PassKind::Export);
    CHECK(plan->passes[1].barriers[0].before == kWrite);
    CHECK(plan->passes[1].barriers[0].after == kSample);
    const auto text = plan->describe();
    CHECK(text == builder.compile()->describe());
    CHECK(text.find("pass 0 work combine") != std::string::npos);
    CHECK(text.find("dependency 0 -> 1 via image") != std::string::npos);
    builder.add_pass({"late work", {{texture, {}, kRead}}});
    CHECK(!builder.compile());
}

// Independent work retains insertion order and creates no false resource dependencies.
void test_fixed_order() {
    graph::Graph builder;
    const auto a = image(builder, 1), b = image(builder, 2);
    builder.add_pass({"first", {{a, {}, kWrite}}});
    builder.add_pass({"second", {{b, {}, kWrite}}});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(plan->passes[0].name == "first" && plan->passes[1].name == "second");
    CHECK(plan->dependencies.empty());
}

// A pass declared separate stays so in the plan, for the executor to start a render pass of its own.
void test_separate_pass() {
    graph::Graph builder;
    const auto target = image(builder);
    graph::Pass second{"second", {{target, {}, kWrite}}};
    second.separate = true;
    builder.add_pass({"first", {{target, {}, kWrite}}});
    builder.add_pass(std::move(second));
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    CHECK(!plan->passes[0].separate && plan->passes[1].separate);
}

// Prints an entirely synthetic shadow-combination graph when explicitly requested by the developer.
void dump_shadow_example() {
    graph::Graph builder;
    const auto cached =
        builder.import_image("static shadow", {{1, 0}, 1, 1, graph::Aspect::Colour, kSample, true});
    const auto moving = builder.import_image(
        "dynamic shadow", {{2, 0}, 1, 1, graph::Aspect::Colour, kSample, true}
    );
    const auto combined = builder.import_image("combined shadow", {{3, 0}});
    builder.add_pass(
        {"shadow depth combine",
         {{*cached, {}, kSample}, {*moving, {}, kSample}, {*combined, {}, kWrite}}}
    );
    builder.add_pass({"lighting", {{*combined, {}, kSample}}, {}, graph::PassKind::Export});
    const auto plan = builder.compile();
    CHECK(plan.has_value());
    if (plan) {
        std::printf("%s", plan->describe().c_str());
    }
}
}  // namespace

// Runs the CPU contracts and reports unexpected exceptions as test failures.
int main(int argc, char** argv) {
    // Unexpected allocation or contract errors must fail the test rather than escape main.
    try {
        test_unwritten_reads();
        test_unbound_handles();
        test_shader_read_export();
        test_partial_image_contents();
        test_buffer_intervals();
        test_read_write_hazards();
        test_all_readers_before_writer();
        test_all_readers_after_writer();
        test_writer_scope_excludes_disjoint_reader();
        test_alias_identity();
        test_pass_feedback();
        test_nested_range_conflict();
        test_invalid_ranges();
        test_attachment_contents();
        test_depth_stencil_aspects();
        test_incompatible_access();
        test_read_layout_change();
        test_invalid_imports();
        test_unused_and_owned_plan();
        test_export_and_dump();
        test_fixed_order();
        test_separate_pass();
        if (argc == 2 && std::string_view(argv[1]) == "--dump") {
            dump_shadow_example();
        }
        return openrac::test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "frame graph test exception: %s\n", failure.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "frame graph test: unknown exception\n");
        return 1;
    }
}
