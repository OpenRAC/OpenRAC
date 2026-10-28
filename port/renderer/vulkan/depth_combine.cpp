// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <string>

#include "renderer/vulkan/shader_compiler.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace detail {
namespace {
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

// Makes one context-owned compute pipeline; descriptor sets come from the fenced frame pools.
bool make_combine() {
    auto& combine = g->depth_combine;
    if (combine.pipeline) {
        return true;
    }
    NativeComputeShader shader;
    std::string error;
    if (!compile_native_compute(kCombine, {}, {}, shader, error)) {
        log::error("shadow composition shader: {}", error);
        return false;
    }
    ++g->submission.shader_compiles;
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
        vkCreateComputePipelines(
            g->device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &combine.pipeline
        ),
        "shadow composition pipeline"
    );
    vkDestroyShaderModule(g->device, module, nullptr);
    if (!made) {
        release_depth_combine();
    } else {
        count_pipeline();
    }
    return made;
}
}  // namespace

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
    if (!a || !b || !out || !a->image || !b->image || !out->image || cached == destination
        || dynamic == destination) {
        return false;
    }
    const auto& target = *out->image;
    if (target.format != VK_FORMAT_R32_SFLOAT || !(target.usage & VK_IMAGE_USAGE_STORAGE_BIT)
        || a->image->width != target.width || a->image->height != target.height || rect[0] < 0
        || rect[1] < 0 || rect[2] != target.width || rect[3] != target.height
        || rect[0] + rect[2] > b->image->width || rect[1] + rect[3] > b->image->height) {
        return false;
    }
    end_rendering();
    const auto command = frame().main;
    barrier(command, *a->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    barrier(command, *b->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    barrier(command, *out->image, VK_IMAGE_LAYOUT_GENERAL);
    const VkDescriptorSet set = allocate_set(g->depth_combine.descriptors);
    if (!set) {
        return false;
    }
    SamplerParams params;
    params.min = params.mag = gl::GL_NEAREST;
    const VkSampler sampler = sampler_for(params, false);
    const std::array<VkDescriptorImageInfo, 3> images{
        {{sampler, sample_view(*a->image, 0, 0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
         {sampler, sample_view(*b->image, 0, 0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
         {VK_NULL_HANDLE, sample_view(*out->image, 0, 0), VK_IMAGE_LAYOUT_GENERAL}}
    };
    std::array<VkWriteDescriptorSet, 3> writes{};
    for (unsigned i = 0; i < writes.size(); ++i) {
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType =
            i == 2 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(
        g->device, static_cast<unsigned>(writes.size()), writes.data(), 0, nullptr
    );
    g->submission.descriptor_writes += writes.size();
    offset[0] = (offset[0] % target.width + target.width) % target.width;
    offset[1] = (offset[1] % target.height + target.height) % target.height;
    std::array<int, 8> constants{
        offset[0], offset[1], rect[0], rect[1], target.width, target.height, 0, 0
    };
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, g->depth_combine.pipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, g->depth_combine.layout, 0, 1, &set, 0, nullptr
    );
    vkCmdPushConstants(
        command,
        g->depth_combine.layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(constants),
        constants.data()
    );
    vkCmdDispatch(command, (target.width + 7) / 8, (target.height + 7) / 8, 1);
    count_command(Command::BindPipeline);
    count_command(Command::BindDescriptors);
    count_command(Command::Other);
    count_command(Command::Dispatch);
    barrier(command, *out->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    out->image->touched = g->recording;
    return true;
}
}  // namespace openrac::vkgl
