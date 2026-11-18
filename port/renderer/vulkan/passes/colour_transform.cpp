// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements native_compute.h's per-texel colour transform with graph or reference barriers.
#include <array>

#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/graph_pipeline.h"
#include "renderer/vulkan/passes/native_compute.h"
#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace detail {
// Releases pipeline objects after their context has retired recorded work.
void release_colour_transform() {
    auto& transform = g->colour_transform;
    vkDestroyPipeline(g->device, transform.pipeline, nullptr);
    vkDestroyPipelineLayout(g->device, transform.layout, nullptr);
    vkDestroyDescriptorSetLayout(g->device, transform.descriptors, nullptr);
    transform = {};
}

namespace {
// Creates the sampler/storage bindings and one-float push constant contract before recording.
bool make_transform_layout() {
    auto& transform = g->colour_transform;
    const std::array<VkDescriptorSetLayoutBinding, 2> bindings{
        {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
         {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}}
    };
    VkDescriptorSetLayoutCreateInfo descriptors{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    };
    descriptors.bindingCount = static_cast<unsigned>(bindings.size());
    descriptors.pBindings = bindings.data();
    // The sampler and storage view share one descriptor set.
    if (!check(
            vkCreateDescriptorSetLayout(g->device, &descriptors, nullptr, &transform.descriptors),
            "colour transform descriptors"
        )) {
        return false;
    }
    const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &transform.descriptors;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    // Native descriptors and exposure must agree with the compiled shader layout.
    if (!check(
            vkCreatePipelineLayout(g->device, &layout, nullptr, &transform.layout),
            "colour transform layout"
        )) {
        return false;
    }
    return true;
}

// Creates a cached compute pipeline and releases its temporary shader module on either outcome.
bool make_transform_pipeline(const NativeComputeShader& shader) {
    auto& transform = g->colour_transform;
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo code{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    code.codeSize = shader.code.size() * sizeof(std::uint32_t);
    code.pCode = shader.code.data();
    // Failed module creation leaves no pipeline to record.
    if (!check(
            vkCreateShaderModule(g->device, &code, nullptr, &module), "colour transform module"
        )) {
        return false;
    }
    VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipeline.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipeline.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module = module;
    pipeline.stage.pName = "main";
    pipeline.layout = transform.layout;
    const bool made = check(
        timed_pipeline_creation([&] {
            return vkCreateComputePipelines(
                g->device, g->pipeline_cache, 1, &pipeline, nullptr, &transform.pipeline
            );
        }),
        "colour transform pipeline"
    );
    vkDestroyShaderModule(g->device, module, nullptr);
    // The temporary module is already released even when pipeline creation fails.
    if (!made) {
        return false;
    }
    return true;
}

// Creates the immutable per-context pipeline before any source image can be modified.
bool make_transform(std::string_view source) {
    auto& transform = g->colour_transform;
    // A context retains a single shader variant; a different source uses the raster fallback.
    if (transform.pipeline) {
        return transform.source == source;
    }
    // Repeated preparation failures are not retried until the context releases its resources.
    if (transform.failed) {
        return false;
    }
    NativeComputeShader shader;
    std::string error;
    // No commands or target changes happen until compilation succeeds.
    if (!compile_native_compute(source, {}, {}, shader, error)) {
        log::warn("Colour transform unavailable: {}", error);
        transform.failed = true;
        return false;
    }
    ++g->submission.shader_compiles;
    // One owner releases every partially constructed pipeline object on preparation failure.
    if (!make_transform_layout() || !make_transform_pipeline(shader)) {
        release_colour_transform();
        g->colour_transform.failed = true;
        return false;
    }
    transform.source = source;
    count_pipeline();
    return true;
}

// Records the unchanged per-texel kernel; its caller owns image transitions and submission.
void record_transform(VkCommandBuffer command, Image& image, VkDescriptorSet set, float parameter) {
    SamplerParams parameters;
    parameters.min = parameters.mag = gl::GL_NEAREST;
    const auto view = sample_view(image, 0, 0);
    const std::array<VkDescriptorImageInfo, 2> descriptors{
        {{sampler_for(parameters, false), view, VK_IMAGE_LAYOUT_GENERAL},
         {VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL}}
    };
    std::array<VkWriteDescriptorSet, 2> writes{};
    // Both bindings refer to the same image; each invocation owns its sampled/stored texel.
    for (unsigned i = 0; i < writes.size(); ++i) {
        auto& write = writes[i];
        write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.dstBinding = i;
        write.descriptorCount = 1;
        // Binding zero samples; binding one stores.
        write.descriptorType =
            i == 0 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &descriptors[i];
    }
    vkUpdateDescriptorSets(
        g->device, static_cast<unsigned>(writes.size()), writes.data(), 0, nullptr
    );
    g->submission.descriptor_writes += writes.size();
    const auto& transform = g->colour_transform;
    NativeCommandEncoder{command}.bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, transform.pipeline);
    NativeCommandEncoder{command}.bind_descriptor_sets(
        VK_PIPELINE_BIND_POINT_COMPUTE, transform.layout, 0, 1, &set, 0, nullptr
    );
    NativeCommandEncoder{command}.push_constants(
        transform.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parameter), &parameter
    );
    // The unchanged kernel covers one texel per invocation in 8x8 workgroups.
    constexpr int kWorkgroupWidth = 8;
    record_pipeline_command(
        command,
        renderer::graph::Dispatch{
            {static_cast<unsigned>((image.width + kWorkgroupWidth - 1) / kWorkgroupWidth),
             static_cast<unsigned>((image.height + kWorkgroupWidth - 1) / kWorkgroupWidth),
             1}
        }
    );
    count_command(Command::BindPipeline);
    count_command(Command::BindDescriptors);
    count_command(Command::Other);
    count_command(Command::Dispatch);
}

// Orders the preserved in-place image before dispatch and exports it for later shader consumers.
Result<void> graph_transform(unsigned texture, VkDescriptorSet set, float parameter) {
    namespace graph = renderer::graph;
    GraphExecution execution;
    const auto image = execution.import_image("colour transform", texture);

    // An in-place transform must never sample undefined source colour.
    if (!image) {
        return unexpected(image.error());
    }

    graph::Pass pass{
        "colour transform",
        {{*image,
          {},
          {graph::Stage::ComputeShader, graph::Access::ReadWrite, graph::Layout::General}}}
    };
    execution.add_pass(pass);
    execution.add_pass(
        graph::shader_read_export(
            "colour transform consumers",
            pass,
            graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader
        )
    );
    // GraphExecution pins the allocation until this render-thread callback returns.
    const auto owner = g->textures.find(texture)->image;
    return execution.run_prepared([owner, set, parameter](unsigned) {
        record_transform(frame().main, *owner, set, parameter);
    });
}
}  // namespace
}  // namespace detail

bool transform_colour(unsigned texture, std::string_view source, float parameter) {
    using namespace detail;
    // Compatibility rendering has no native compute allocation to borrow.
    if (!active()) {
        return false;
    }

    auto* found = g->textures.find(texture);
    // The kernel's single-level RGBA8 storage contract must hold before resource preparation.
    if (!found || !found->image || found->image->format != VK_FORMAT_R8G8B8A8_UNORM
        || found->image->samples != VK_SAMPLE_COUNT_1_BIT || found->image->levels != 1
        || !(found->image->usage & VK_IMAGE_USAGE_STORAGE_BIT) || !make_transform(source)) {
        return false;
    }

    // Allocate before recording work: failure leaves the original colour untouched for fallback.
    const VkDescriptorSet set = allocate_set(g->colour_transform.descriptors);
    if (!set) {
        return false;
    }

    const auto result = graph_transform(texture, set, parameter);
    return bool(result);
}
}  // namespace openrac::vkgl
