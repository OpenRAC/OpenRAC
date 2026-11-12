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

#include "renderer/graph/command_buffer.h"
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

void test_imported_subresource_states() {
    constexpr graph::ImageRange base{graph::Aspect::Colour, 0, 1};
    constexpr graph::ImageRange written{graph::Aspect::Colour, 1, 1, 1};
    constexpr graph::ImageRange undefined{graph::Aspect::Colour, 2, 1, 1};
    graph::ImageDesc desc{
        {1, 0}, 3, 2, graph::Aspect::Colour, {}, false,
        {{base, {kSample, true}}, {written, {kWrite, true}}}
    };
    graph::PlanCache cache;
    const auto compile = [&](const graph::ImageDesc& entry, graph::ImageRange read) {
        graph::Graph builder;
        const auto handle = builder.import_image("mip array", entry).value();
        builder.add_pass({"sample", {{handle, read, kSample}}});
        return cache.compile(builder);
    };
    CHECK(compile(desc, base).value()->passes.front().barriers.empty());
    const auto plan = compile(desc, written).value();
    const auto& barrier = plan->passes.front().barriers.at(0);
    CHECK(std::get<graph::ImageRange>(barrier.range) == written && barrier.before == kWrite);
    CHECK(!compile(desc, undefined));
    auto discarded = desc;
    discarded.initial_ranges.back().contents.defined = false;
    CHECK(!compile(discarded, written));

    graph::Graph builder;
    const auto handle = builder.import_image("partial mip", desc).value();
    builder.add_pass({"patch", {{handle, undefined, kWrite, graph::Load::None,
                                graph::Store::Preserve, true}}});
    builder.add_pass({"sample incomplete mip", {{handle, undefined, kSample}}});
    CHECK(!builder.compile());
    desc.initial_ranges.push_back({written, {kSample, true}});
    CHECK(!builder.import_image("overlapping ranges", std::move(desc)));
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
    typed.add_pass({"wrong handle", {{{wrong->index, wrong->owner}, {}, kWrite}}});
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

    graph::PlanCache cache(1);
    CHECK(cache.compile(builder).value()->passes[1].separate);
    builder.clear();
    const auto reused = image(builder);
    builder.add_pass({"first", {{reused, {}, kWrite}}});
    builder.add_pass({"second", {{reused, {}, kWrite}}});
    CHECK(!cache.compile(builder).value()->passes[1].separate);
}

// Prints an entirely synthetic shadow-combination graph when explicitly requested by the developer.
void test_transfer_command_contract() {
    constexpr graph::State source{Stage::Transfer, Access::Read, Layout::TransferSource};
    constexpr graph::State destination{Stage::Transfer, Access::Write, Layout::TransferDestination};
    graph::Graph builder;
    const auto from = image(builder, 1), to = image(builder, 2);
    const graph::Pass pass{"blit", {{from, {}, source}, {to, {}, destination}}};
    graph::CommandBuffer commands;
    commands.blit(from, to, {{0, 0}, {8, 8}, {0, 8}, {8, 0}});
    CHECK(commands.validate(pass));

    auto missing_write = pass;
    missing_write.images.push_back({{2}, {}, destination});
    CHECK(!commands.validate(missing_write));
    auto wrong_read = pass;
    wrong_read.images[0].state = kSample;
    CHECK(!commands.validate(wrong_read));

    graph::Graph target;
    const auto target_to = image(target, 2), target_from = image(target, 1);
    const unsigned mapping[]{target_from.index, target_to.index};
    const auto remapped = commands.remap({mapping, builder.owner(), target.owner()});
    CHECK(remapped);
    const graph::Pass other{"remapped blit", {{target_from, {}, source}, {target_to, {}, destination}}};
    CHECK(remapped->validate(other));
    CHECK(!commands.remap({std::span{mapping}.first(1), builder.owner(), target.owner()}));
    CHECK(!commands.remap({mapping, target.owner(), builder.owner()}));
    auto foreign_read = pass;
    foreign_read.images.front().image = target_to;
    CHECK(!commands.validate(foreign_read));

    commands.blit(from, to, {{0, 0}, {8, 8}, {0, 0}, {8, 8}});
    CHECK(!commands.validate(pass));
}

void test_pipeline_command_contract() {
    const graph::Dispatch dispatch{{2, 1, 1}};
    const graph::Draw draw{3, 1, 0, 0};
    graph::Pass compute{"compute", {}, {{{0}, {0, 4}, kBufferWrite}}};
    const graph::Pass raster{
        "raster", {{{0}, {}, {Stage::ColourOutput, Access::Write, Layout::ColourAttachment}}}
    };
    CHECK(graph::validate_pipeline_command(compute, dispatch));
    CHECK(!graph::validate_pipeline_command(compute, draw));
    CHECK(graph::validate_pipeline_command(raster, draw));
    CHECK(!graph::validate_pipeline_command(raster, dispatch));
    compute.kind = graph::PassKind::Export;
    CHECK(!graph::validate_pipeline_command(compute, dispatch));
    graph::Pass resource_free{"resource-free compute"};
    resource_free.execution = Stage::ComputeShader;
    CHECK(graph::validate_pipeline_command(resource_free, dispatch));
    CHECK(!graph::validate_pipeline_command(resource_free, draw));
}

void test_buffer_copy_contract() {
    graph::Graph builder;
    const auto from = builder.import_buffer("from", {{1, 0}, 64}).value();
    const auto to = builder.import_buffer("to", {{2, 0}, 64}).value();
    graph::CommandBuffer commands;
    commands.copy_buffer(from, {8, 16}, to, 32);
    const auto pass = commands.declarations("buffer upload");
    CHECK(commands.validate(pass));
    auto wrong_range = pass;
    wrong_range.buffers[1].range.offset = 0;
    CHECK(!commands.validate(wrong_range));
    auto unwritten = pass;
    unwritten.buffers.push_back({{2}, {0, 16}, {Stage::Transfer, Access::Write, Layout::Buffer}});
    CHECK(!commands.validate(unwritten));
    graph::Graph target;
    const auto target_from = target.import_buffer("from", {{1, 0}, 64}).value();
    const auto target_to = target.import_buffer("to", {{2, 0}, 64}).value();
    const unsigned mapping[]{target_from.index, target_to.index};
    const auto remapped = commands.remap({mapping, builder.owner(), target.owner()});
    CHECK(remapped);
    graph::CommandBuffer expected;
    expected.copy_buffer(target_from, {8, 16}, target_to, 32);
    CHECK(remapped->validate(expected.declarations("remapped upload")));
    CHECK(!commands.remap({std::span{mapping}.first(1), builder.owner(), target.owner()}));
    commands.copy_buffer(from, {32, 16}, to, 40);
    CHECK(!commands.validate(commands.declarations("overlapping destination")));
    graph::CommandBuffer empty;
    empty.copy_buffer({0}, {0, 0}, {1}, 0);
    CHECK(!empty.validate(empty.declarations("zero-byte copy")));
    graph::CommandBuffer alias;
    alias.copy_buffer({0}, {0, 16}, {0}, 8);
    CHECK(!alias.validate(alias.declarations("overlapping alias")));
    graph::CommandBuffer disjoint;
    disjoint.copy_buffer({0}, {0, 16}, {0}, 16);
    CHECK(disjoint.validate(disjoint.declarations("disjoint alias")));
}

void test_handle_ownership() {
    graph::Graph builder;
    const auto texture = image(builder, 1);
    const auto buffer = builder.import_buffer("buffer", {{2, 0}, 64}).value();
    const graph::Pass pass{"write", {{texture, {}, kWrite}}, {{buffer, {0, 64}, kBufferWrite}}};
    builder.add_pass(pass);
    graph::PlanCache cache;
    const auto plan = cache.compile(builder).value();

    graph::Graph foreign;
    const auto foreign_image = image(foreign, 1);
    const auto foreign_buffer = foreign.import_buffer("buffer", {{2, 0}, 64}).value();
    CHECK(foreign_image.index == texture.index && foreign_buffer.index == buffer.index);
    auto invalid = pass;
    invalid.buffers.front().buffer = foreign_buffer;
    foreign.add_pass(std::move(invalid));
    CHECK(!foreign.compile());
    CHECK(!cache.compile(foreign));

    graph::Graph moved(std::move(builder));
    CHECK(cache.compile(moved).value() == plan);
    const auto replacement = image(builder, 1);
    CHECK(replacement.index == texture.index && replacement != texture);

    moved.clear();
    const auto current_image = image(moved, 1);
    const auto current_buffer = moved.import_buffer("buffer", {{2, 0}, 64}).value();
    invalid = pass;
    invalid.images.front().image = current_image;
    moved.add_pass(std::move(invalid));
    CHECK(!moved.compile());
    CHECK(!cache.compile(moved));

    graph::Graph assigned;
    const auto displaced = image(assigned, 1);
    assigned = std::move(moved);
    CHECK(!assigned.validate_handles({"displaced", {{displaced, {}, kWrite}}}));
    CHECK(assigned.validate_handles(
        {"transferred", {{current_image, {}, kWrite}}, {{current_buffer, {0, 64}, kBufferWrite}}}
    ));
}

void test_cached_validation() {
    graph::PlanCache cache;
    const auto compile = [](graph::PlanCache& target, bool defined, std::uint64_t size,
                            graph::BufferRange range,
                            graph::State initial = {Stage::None, Access::None, Layout::Buffer}) {
        graph::Graph builder;
        const auto buffer = builder.import_buffer("input", {{1, 0}, size, initial, defined});
        builder.add_pass({"read", {}, {{buffer.value(), range, kBufferRead}}});
        return target.compile(builder);
    };
    const auto first = compile(cache, true, 64, {0, 32});
    const auto& original = first.value();
    const auto hit = compile(cache, true, 64, {0, 32});
    CHECK(hit && *hit == original);
    CHECK(!compile(cache, false, 64, {0, 32}));
    CHECK(!compile(cache, true, 16, {0, 32}));
    CHECK(!compile(cache, true, 64, {40, 32}));
    const auto written = compile(cache, true, 64, {0, 32}, kBufferWrite);
    CHECK(written && *written != original);
    CHECK(written.value()->passes[0].barriers.at(0).before == kBufferWrite);
    for (unsigned index = 1; index <= 32; ++index) {
        CHECK(compile(cache, true, 64, {index, 32}));
    }
    const auto recompiled = compile(cache, true, 64, {0, 32});
    CHECK(recompiled && *recompiled != original);
    CHECK(original->describe() == recompiled.value()->describe());

    graph::PlanCache single_entry(1);
    auto retained = compile(single_entry, true, 64, {0, 32}).value();
    CHECK(!compile(single_entry, false, 64, {0, 32}));
    CHECK(compile(single_entry, true, 64, {0, 32}).value() == retained);

    const std::weak_ptr<const graph::Plan> published = retained;
    retained.reset();
    graph::Graph replacement;
    const auto buffer = replacement.import_buffer(
        "other input", {{2, 0}, 64, {Stage::None, Access::None, Layout::Buffer}, true}
    ).value();
    replacement.add_pass({"read", {}, {{buffer, {0, 64}, kBufferRead}}});
    const auto borrowed = single_entry.compile_borrowed(replacement);
    CHECK(borrowed && (*borrowed)->resources.front().name == "other input");
    CHECK(published.expired());
}

void test_reused_builder() {
    graph::Graph builder;
    const auto original = builder.import_image("previous image label", {{1, 0}}).value();
    auto write = builder.make_pass("previous pass label");
    write.images.push_back({original, {}, kWrite});
    auto export_pass = builder.make_pass("previous consumers");
    graph::shader_read_export(export_pass, write, Stage::FragmentShader);
    builder.add_pass(std::move(write));
    builder.add_pass(std::move(export_pass));
    const auto old_plan = builder.compile().value();
    graph::Plan reused;
    CHECK(builder.compile_into(reused, graph::DeclarationRetention::Retain));
    CHECK(reused.declarations.size() == 2);
    CHECK(reused.passes.back().barriers.size() == 1);

    builder.clear();
    CHECK(builder.resources().empty() && builder.passes().empty());
    const auto next = builder.import_buffer("current buffer label", {{1, 0}, 64}).value();
    auto pass = builder.make_pass("current pass label");
    CHECK(pass.images.empty() && pass.buffers.empty());
    pass.buffers.push_back({next, {0, 64}, kBufferWrite});
    builder.add_pass(std::move(pass));
    const auto second = builder.make_pass("current second pass");
    CHECK(second.kind == graph::PassKind::Work);
    CHECK(second.images.empty() && second.buffers.empty());

    const auto plan = builder.compile().value();
    CHECK(builder.compile_into(reused, graph::DeclarationRetention::Discard));
    CHECK(reused.declarations.empty() && reused.dependencies.empty());
    CHECK(reused.passes.size() == 1 && reused.passes.front().barriers.empty());
    CHECK(plan.resources.size() == 1 && plan.resources.front().name == "current buffer label");
    CHECK(plan.passes.size() == 1 && plan.passes.front().name == "current pass label");
    CHECK(old_plan.resources.front().name == "previous image label");
}

void test_borrowed_import_name() {
    graph::Graph builder;
    CHECK(builder.import_image("depth", {{1, 0}}));
    for (unsigned identity = 2; identity <= 32; ++identity) {
        CHECK(builder.import_image(builder.resources().front().name, {{identity, 0}}));
        CHECK(builder.resources().back().name == "depth");
    }
}

void test_cull_unused_passes() {
    graph::Graph builder;
    const auto scratch = builder.import_buffer("scratch", {{1, 0}, 64}).value();
    const auto output = builder.import_image("output", {{2, 0}}).value();
    const auto unused = builder.import_image("unused", {{3, 0}}).value();
    const auto optional = [&](graph::Pass pass) {
        pass.cull_if_unused = true;
        builder.add_pass(std::move(pass));
    };
    optional({"overwritten", {}, {{scratch, {0, 32}, kBufferWrite}}});
    optional({"unused bytes", {}, {{scratch, {32, 32}, kBufferWrite}}});
    optional({"producer", {}, {{scratch, {0, 32}, kBufferWrite}}});
    optional({"unused reader", {{unused, {}, kWrite}}, {{scratch, {0, 32}, kBufferRead}}});
    optional({"build output", {{output, {}, kWrite}}, {{scratch, {0, 32}, kBufferRead}}});
    builder.add_pass({"history update"});
    builder.add_pass({"present output", {{output, {}, kSample}}, {}, graph::PassKind::Export});

    const auto plan = builder.compile().value();
    CHECK(plan.culled_passes == 3);
    CHECK(plan.passes[0].culled && plan.passes[1].culled && plan.passes[3].culled);
    CHECK(!plan.passes[2].culled && !plan.passes[4].culled && !plan.passes[5].culled);
    CHECK(!plan.passes[6].culled && plan.passes[0].barriers.empty());
    CHECK(plan.resources[scratch.index].first == 2 && plan.resources[scratch.index].last == 4);
    CHECK(!plan.resources[unused.index].first);
    CHECK(plan.passes[2].barriers.empty() || plan.passes[2].barriers.front().from_import);
    CHECK(std::ranges::none_of(plan.dependencies, [&](const auto& edge) {
        return plan.passes[edge.source].culled || plan.passes[edge.destination].culled;
    }));
    CHECK(plan.describe().find("pass 0 culled overwritten") != std::string::npos);
}

void test_cull_partial_writes_and_validation() {
    graph::Graph builder;
    const auto image = builder.import_image("image", {{1, 0}}).value();
    graph::Pass producer{"producer", {{image, {}, kWrite}}};
    producer.cull_if_unused = true;
    builder.add_pass(std::move(producer));
    builder.add_pass(
        {"patch", {{image, {}, kWrite, graph::Load::None, graph::Store::Preserve, true}}}
    );
    builder.add_pass({"read patched image", {{image, {}, kSample}}});
    CHECK(builder.compile().value().culled_passes == 0);

    const auto undefined = builder.import_image("unwritten", {{2, 0}}).value();
    graph::Pass dead_read{"invalid optional pass", {{undefined, {}, kSample}}};
    dead_read.cull_if_unused = true;
    builder.add_pass(std::move(dead_read));
    CHECK(!builder.compile());
}

void test_cached_cull_policy() {
    graph::PlanCache cache;
    const auto compile = [&](bool cull_if_unused) {
        graph::Graph builder;
        graph::Pass pass{"optional work"};
        pass.cull_if_unused = cull_if_unused;
        builder.add_pass(std::move(pass));
        return cache.compile(builder).value();
    };
    const auto kept = compile(false);
    const auto culled = compile(true);
    CHECK(kept->culled_passes == 0 && culled->culled_passes == 1);
    CHECK(compile(true) == culled);
}

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
        test_imported_subresource_states();
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
        test_transfer_command_contract();
        test_pipeline_command_contract();
        test_buffer_copy_contract();
        test_cached_validation();
        test_handle_ownership();
        test_reused_builder();
        test_borrowed_import_name();
        test_cull_unused_passes();
        test_cull_partial_writes_and_validation();
        test_cached_cull_policy();
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
