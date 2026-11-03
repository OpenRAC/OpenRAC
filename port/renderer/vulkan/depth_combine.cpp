// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native shadow composition with stable frame descriptors and shared graph dependencies.
#include <array>
#include <optional>
#include <string>

#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/prepared_dispatch.h"
#include "renderer/vulkan/shader_compiler.h"
#include "renderer/vulkan/surface_batch.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace detail {
namespace {
// ---- Pipeline setup ----

// The existing kernel takes the nearer reversed-Z depth from cached and dynamic maps.
constexpr auto kCombine = R"(#version 450
layout(local_size_x=8, local_size_y=8) in;
layout(binding=0) uniform sampler2D static_depth;
layout(binding=1) uniform sampler2D dynamic_depth;
layout(binding=2, r32f) writeonly uniform image2D combined_depth;
layout(push_constant) uniform Parameters { ivec4 offset_rect; ivec4 extent; } params;
void main() {
    ivec2 q = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(q, params.extent.xy))) return;
    ivec2 cached = (q + params.offset_rect.xy) % params.extent.xy;
    float a = texelFetch(static_depth, cached, 0).r;
    float b = texelFetch(dynamic_depth, q + params.offset_rect.zw, 0).r;
    imageStore(combined_depth, q, vec4(max(a,b)));
}
)";

// Stable native bindings and push constants; image owners retain all descriptor views until execution.
struct ShadowComposition {
    std::array<ImagePtr, 3> images;  // Pins static, dynamic and combined allocations.
    PreparedDispatch dispatch;       // Frame-owned descriptors and context-owned pipeline.
    std::array<int, 8> constants;    // Toroidal offset, atlas origin and output dimensions.
};

// Creates the sampled/static/output descriptor layout and the unchanged push-constant range.
bool make_combine_layout() {
    auto& combine = g->depth_combine;
    const std::array<VkDescriptorSetLayoutBinding, 3> bindings{
        {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
         {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
         {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}}
    };
    VkDescriptorSetLayoutCreateInfo sets{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    sets.bindingCount = static_cast<unsigned>(bindings.size());
    sets.pBindings = bindings.data();
    if (!check(
            vkCreateDescriptorSetLayout(g->device, &sets, nullptr, &combine.descriptors),
            "shadow composition descriptors"
        )) {
        return false;
    }
    const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, 32};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &combine.descriptors;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    if (!check(
            vkCreatePipelineLayout(g->device, &layout, nullptr, &combine.layout),
            "shadow composition layout"
        )) {
        release_depth_combine();
        return false;
    }
    return true;
}

// Makes one context-owned compute pipeline; descriptor sets come from the fenced frame pools.
bool make_combine() {
    auto& combine = g->depth_combine;
    // Pipeline and layout remain context-owned across all prepared frame packets.
    if (combine.pipeline) {
        return true;
    }
    NativeComputeShader shader;
    std::string error;
    // A failed shader leaves all shadow casters on the established raster fallback.
    if (!compile_native_compute(kCombine, {}, {}, shader, error)) {
        log::error("shadow composition shader: {}", error);
        return false;
    }
    ++g->submission.shader_compiles;
    // Pipeline creation needs both descriptor bindings and its push-constant range.
    if (!make_combine_layout()) {
        return false;
    }
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo code{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    code.codeSize = shader.code.size() * sizeof(std::uint32_t);
    code.pCode = shader.code.data();
    if (!check(
            vkCreateShaderModule(g->device, &code, nullptr, &module), "shadow composition module"
        )) {
        release_depth_combine();
        return false;
    }
    VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipeline.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipeline.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module = module;
    pipeline.stage.pName = "main";
    pipeline.layout = combine.layout;
    const bool made = check(
        timed_pipeline_creation([&] {
            return vkCreateComputePipelines(
                g->device, g->pipeline_cache, 1, &pipeline, nullptr, &combine.pipeline
            );
        }),
        "shadow composition pipeline"
    );
    vkDestroyShaderModule(g->device, module, nullptr);
    // A failed native pipeline cannot retain incomplete context-owned objects.
    if (!made) {
        release_depth_combine();
    } else {
        count_pipeline();
    }
    return made;
}

// ---- Prepared composition ----

// Allocates frame descriptors without sampling or transitioning any pending shadow output.
std::optional<ShadowComposition> prepare_combine(
    const std::array<ImagePtr, 3>& images, std::array<int, 2> offset, std::array<int, 4> rect
) {
    const VkDescriptorSet set = allocate_set(g->depth_combine.descriptors);
    // A missing set cannot be retained as an executable composition packet.
    if (!set) {
        return std::nullopt;
    }
    SamplerParams params;
    params.min = params.mag = gl::GL_NEAREST;
    const VkSampler sampler = sampler_for(params, false);
    const std::array<VkDescriptorImageInfo, 3> views{
        {{sampler, sample_view(*images[0], 0, 0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
         {sampler, sample_view(*images[1], 0, 0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
         {VK_NULL_HANDLE, sample_view(*images[2], 0, 0), VK_IMAGE_LAYOUT_GENERAL}}
    };
    std::array<VkWriteDescriptorSet, 3> writes{};
    // Two sampled depths feed one complete storage output.
    for (unsigned i = 0; i < writes.size(); ++i) {
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType =
            i == 2 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &views[i];
    }
    vkUpdateDescriptorSets(
        g->device, static_cast<unsigned>(writes.size()), writes.data(), 0, nullptr
    );
    g->submission.descriptor_writes += writes.size();
    const int width = images[2]->width;
    const int height = images[2]->height;
    // Positive modulo retains identical wrapped sampling for negative scroll origins.
    offset[0] = (offset[0] % width + width) % width;
    offset[1] = (offset[1] % height + height) % height;
    return ShadowComposition{
        images,
        {g->depth_combine.pipeline,
         g->depth_combine.layout,
         set,
         0,
         false,
         {static_cast<unsigned>((width + 7) / 8), static_cast<unsigned>((height + 7) / 8), 1}},
        {offset[0], offset[1], rect[0], rect[1], width, height, 0, 0}
    };
}

// Records the original composition kernel using only immutable descriptors and push constants.
void record_combine(const ShadowComposition& prepared) {
    vkCmdPushConstants(
        frame().main,
        prepared.dispatch.layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(prepared.constants),
        prepared.constants.data()
    );
    count_command(Command::Other);
    record_prepared_dispatch(prepared.dispatch);
    prepared.images[2]->discarded = 0;
    prepared.images[2]->touched = g->recording;
}

// Joins cached and dynamic depth producers to the actual later lighting sampler dependencies.
bool record_combine_graph(std::array<unsigned, 3> textures, const ShadowComposition& prepared) {
    namespace graph = renderer::graph;
    GraphExecution execution;
    std::array<graph::ImageHandle, 3> handles;
    constexpr std::array<const char*, 3> kNames{
        "static shadow", "dynamic shadow", "combined shadow"
    };
    // Fresh outputs discard old contents; both input maps retain their earlier producers.
    for (unsigned index = 0; index < textures.size(); ++index) {
        const auto handle = execution.import_image(
            kNames[index],
            textures[index],
            index == 2 ? InitialContents::Discard : InitialContents::Preserve
        );
        // Import failure leaves the caller's all-caster fallback available.
        if (!handle) {
            log::warn("shadow graph: {}", handle.error().message);
            return false;
        }
        handles[index] = *handle;
    }
    constexpr graph::State kRead{
        graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Sampled
    };
    constexpr graph::State kWrite{
        graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::General
    };
    constexpr auto kShaders =
        graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
    const auto aspect = [](const ImagePtr& image) {
        return image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? graph::Aspect::Depth
                                                         : graph::Aspect::Colour;
    };
    execution.add_pass(
        {"shadow depth combine",
         {{handles[0], {aspect(prepared.images[0])}, kRead},
          {handles[1], {aspect(prepared.images[1])}, kRead},
          {handles[2], {graph::Aspect::Colour}, kWrite}}}
    );
    execution.add_pass(
        {"shadow lighting reads",
         {{handles[2], {}, {kShaders, graph::Access::Read, graph::Layout::Sampled}}},
         {},
         graph::PassKind::Export}
    );
    // The scope retains descriptors, constants and image owners through compilation and execution.
    const auto result = execution.run_prepared([prepared](unsigned) { record_combine(prepared); });
    // Report a rejected graph before any caller advertises its combined output.
    if (!result) {
        log::warn("shadow graph: {}", result.error().message);
    }
    return bool(result);
}
}  // namespace

// ---- Context lifetime ----

void release_depth_combine() {
    auto& combine = g->depth_combine;
    vkDestroyPipeline(g->device, combine.pipeline, nullptr);
    vkDestroyPipelineLayout(g->device, combine.layout, nullptr);
    vkDestroyDescriptorSetLayout(g->device, combine.descriptors, nullptr);
    combine = {};
}
}  // namespace detail

bool combine_shadow_depth(
    unsigned cached,
    unsigned dynamic,
    unsigned destination,
    std::array<int, 2> offset,
    std::array<int, 4> rect
) {
    using namespace detail;
    if (!active() || !make_combine()) {
        return false;
    }
    auto* a = g->textures.find(cached);
    auto* b = g->textures.find(dynamic);
    auto* out = g->textures.find(destination);
    if (!a || !b || !out || !a->image || !b->image || !out->image
        || a->image->image == out->image->image || b->image->image == out->image->image) {
        return false;
    }
    const auto& target = *out->image;
    if (target.width <= 0 || target.height <= 0 || target.format != VK_FORMAT_R32_SFLOAT
        || !(target.usage & VK_IMAGE_USAGE_STORAGE_BIT) || a->image->width != target.width
        || a->image->height != target.height || rect[0] < 0 || rect[1] < 0
        || rect[2] != target.width || rect[3] != target.height || rect[2] > b->image->width
        || rect[3] > b->image->height || rect[0] > b->image->width - rect[2]
        || rect[1] > b->image->height - rect[3]) {
        return false;
    }
    const std::array<ImagePtr, 3> images{a->image, b->image, out->image};
    const auto prepared = prepare_combine(images, offset, rect);
    // Descriptor allocation failure retains the existing all-caster fallback.
    if (!prepared) {
        return false;
    }
    // Combination imports single-level images into the shadow graph.
    if (a->image->levels != 1 || b->image->levels != 1 || target.levels != 1) {
        return false;
    }
    return record_combine_graph({cached, dynamic, destination}, *prepared);
}
}  // namespace openrac::vkgl
