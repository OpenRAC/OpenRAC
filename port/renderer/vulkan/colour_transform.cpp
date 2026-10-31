// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>

#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/shader_compiler.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace detail {
void release_colour_transform() {
    auto& transform = g->colour_transform;
    vkDestroyPipeline(g->device, transform.pipeline, nullptr);
    vkDestroyPipelineLayout(g->device, transform.layout, nullptr);
    vkDestroyDescriptorSetLayout(g->device, transform.descriptors, nullptr);
    transform = {};
}

namespace {
bool make_transform(std::string_view source) {
    auto& transform = g->colour_transform;
    if (transform.pipeline) {
        return transform.source == source;
    }
    if (transform.failed) {
        return false;
    }
    const auto failed = [] {
        release_colour_transform();
        g->colour_transform.failed = true;
        return false;
    };
    NativeComputeShader shader;
    std::string error;
    if (!compile_native_compute(source, {}, {}, shader, error)) {
        log::warn("Colour transform unavailable: {}", error);
        return failed();
    }
    ++g->submission.shader_compiles;
    const std::array<VkDescriptorSetLayoutBinding, 2> bindings{
        {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
         {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}}
    };
    VkDescriptorSetLayoutCreateInfo descriptors{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    };
    descriptors.bindingCount = static_cast<unsigned>(bindings.size());
    descriptors.pBindings = bindings.data();
    if (!check(
            vkCreateDescriptorSetLayout(g->device, &descriptors, nullptr, &transform.descriptors),
            "colour transform descriptors"
        )) {
        return failed();
    }
    const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &transform.descriptors;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    if (!check(
            vkCreatePipelineLayout(g->device, &layout, nullptr, &transform.layout),
            "colour transform layout"
        )) {
        return failed();
    }
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo code{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    code.codeSize = shader.code.size() * sizeof(std::uint32_t);
    code.pCode = shader.code.data();
    if (!check(
            vkCreateShaderModule(g->device, &code, nullptr, &module), "colour transform module"
        )) {
        return failed();
    }
    VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipeline.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipeline.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module = module;
    pipeline.stage.pName = "main";
    pipeline.layout = transform.layout;
    const bool made = check(
        vkCreateComputePipelines(
            g->device, g->pipeline_cache, 1, &pipeline, nullptr, &transform.pipeline
        ),
        "colour transform pipeline"
    );
    vkDestroyShaderModule(g->device, module, nullptr);
    if (!made) {
        return failed();
    }
    transform.source = source;
    count_pipeline();
    return true;
}
}  // namespace
}  // namespace detail

bool transform_colour(unsigned texture, std::string_view source, float parameter) {
    using namespace detail;
    if (!active()) {
        return false;
    }
    auto* found = g->textures.find(texture);
    if (!found || !found->image || found->image->format != VK_FORMAT_R8G8B8A8_UNORM
        || found->image->samples != VK_SAMPLE_COUNT_1_BIT
        || !(found->image->usage & VK_IMAGE_USAGE_STORAGE_BIT) || !make_transform(source)) {
        return false;
    }
    // Allocate before recording work: failure leaves the original colour untouched for fallback.
    const VkDescriptorSet set = allocate_set(g->colour_transform.descriptors);
    if (!set) {
        return false;
    }
    auto& image = *found->image;
    end_rendering(PassEnd::Compute);
    const auto command = frame().main;
    barrier(command, image, VK_IMAGE_LAYOUT_GENERAL);
    SamplerParams parameters;
    parameters.min = parameters.mag = gl::GL_NEAREST;
    const auto view = sample_view(image, 0, 0);
    const std::array<VkDescriptorImageInfo, 2> descriptors{
        {{sampler_for(parameters, false), view, VK_IMAGE_LAYOUT_GENERAL},
         {VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL}}
    };
    std::array<VkWriteDescriptorSet, 2> writes{};
    for (unsigned i = 0; i < writes.size(); ++i) {
        auto& write = writes[i];
        write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.dstBinding = i;
        write.descriptorCount = 1;
        write.descriptorType =
            i == 0 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &descriptors[i];
    }
    vkUpdateDescriptorSets(
        g->device, static_cast<unsigned>(writes.size()), writes.data(), 0, nullptr
    );
    g->submission.descriptor_writes += writes.size();
    const auto& transform = g->colour_transform;
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, transform.pipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, transform.layout, 0, 1, &set, 0, nullptr
    );
    vkCmdPushConstants(
        command, transform.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parameter), &parameter
    );
    vkCmdDispatch(command, (image.width + 7) / 8, (image.height + 7) / 8, 1);
    count_command(Command::BindPipeline);
    count_command(Command::BindDescriptors);
    count_command(Command::Other);
    count_command(Command::Dispatch);
    // Each invocation owns one texel. No neighbourhood reads or inter-invocation dependency.
    barrier(command, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    image.discarded = 0;
    image.touched = g->recording;
    return true;
}
}  // namespace openrac::vkgl
