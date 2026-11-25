// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native command lowering for graph executors and backend instrumentation.
#pragma once

#include <cstdint>
#include <volk.h>

namespace openrac::vkgl {

// Vulkan lowering for one already-open native command buffer. This is the boundary
// for native backend commands; graph clients record renderer::graph::CommandBuffer.
class NativeCommandEncoder {
public:
    explicit NativeCommandEncoder(VkCommandBuffer buffer) : m_buffer(buffer) {}

    // Tracks potentially image-writing work on the recording thread, including deferred clears.
    static void note_image_work();
    [[nodiscard]] static std::uint64_t image_work_count();

    void begin_debug_utils_label(const VkDebugUtilsLabelEXT* p_label_info);
    [[nodiscard]] static bool supports_begin_debug_utils_label();
    void begin_rendering(const VkRenderingInfo* p_rendering_info);
    void bind_descriptor_sets(
        VkPipelineBindPoint pipeline_bind_point,
        VkPipelineLayout layout,
        std::uint32_t first_set,
        std::uint32_t descriptor_set_count,
        const VkDescriptorSet* p_descriptor_sets,
        std::uint32_t dynamic_offset_count,
        const std::uint32_t* p_dynamic_offsets
    );
    void bind_index_buffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type);
    void bind_pipeline(VkPipelineBindPoint pipeline_bind_point, VkPipeline pipeline);
    void bind_vertex_buffers(
        std::uint32_t first_binding,
        std::uint32_t binding_count,
        const VkBuffer* p_buffers,
        const VkDeviceSize* p_offsets
    );
    void blit_image(
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        std::uint32_t region_count,
        const VkImageBlit* p_regions,
        VkFilter filter
    );
    void build_acceleration_structures(
        std::uint32_t count,
        const VkAccelerationStructureBuildGeometryInfoKHR* infos,
        const VkAccelerationStructureBuildRangeInfoKHR* const* ranges
    );
    void clear_attachments(
        std::uint32_t attachment_count,
        const VkClearAttachment* p_attachments,
        std::uint32_t rect_count,
        const VkClearRect* p_rects
    );
    void clear_color_image(
        VkImage image,
        VkImageLayout image_layout,
        const VkClearColorValue* p_color,
        std::uint32_t range_count,
        const VkImageSubresourceRange* p_ranges
    );
    void clear_depth_stencil_image(
        VkImage image,
        VkImageLayout image_layout,
        const VkClearDepthStencilValue* p_depth_stencil,
        std::uint32_t range_count,
        const VkImageSubresourceRange* p_ranges
    );
    void copy_buffer(
        VkBuffer src_buffer,
        VkBuffer dst_buffer,
        std::uint32_t region_count,
        const VkBufferCopy* p_regions
    );
    void copy_buffer_to_image(
        VkBuffer src_buffer,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        std::uint32_t region_count,
        const VkBufferImageCopy* p_regions
    );
    void copy_image(
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        std::uint32_t region_count,
        const VkImageCopy* p_regions
    );
    void copy_image_to_buffer(
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkBuffer dst_buffer,
        std::uint32_t region_count,
        const VkBufferImageCopy* p_regions
    );
    void dispatch(
        std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z
    );
    void draw(
        std::uint32_t vertex_count,
        std::uint32_t instance_count,
        std::uint32_t first_vertex,
        std::uint32_t first_instance
    );
    void draw_indexed(
        std::uint32_t index_count,
        std::uint32_t instance_count,
        std::uint32_t first_index,
        std::int32_t vertex_offset,
        std::uint32_t first_instance
    );
    void draw_indexed_indirect(
        VkBuffer buffer, VkDeviceSize offset, std::uint32_t draw_count, std::uint32_t stride
    );
    void end_debug_utils_label();
    [[nodiscard]] static bool supports_end_debug_utils_label();
    void end_rendering();
    void execute_commands(
        std::uint32_t command_buffer_count, const VkCommandBuffer* p_command_buffers
    );
    void pipeline_barrier(
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask,
        VkDependencyFlags dependency_flags,
        std::uint32_t memory_barrier_count,
        const VkMemoryBarrier* p_memory_barriers,
        std::uint32_t buffer_memory_barrier_count,
        const VkBufferMemoryBarrier* p_buffer_memory_barriers,
        std::uint32_t image_memory_barrier_count,
        const VkImageMemoryBarrier* p_image_memory_barriers
    );
    void pipeline_barrier2(const VkDependencyInfo& dependency);
    void push_constants(
        VkPipelineLayout layout,
        VkShaderStageFlags stage_flags,
        std::uint32_t offset,
        std::uint32_t size,
        const void* p_values
    );
    void reset_query_pool(
        VkQueryPool query_pool, std::uint32_t first_query, std::uint32_t query_count
    );
    void resolve_image(
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        std::uint32_t region_count,
        const VkImageResolve* p_regions
    );
    void set_blend_constants(const float blend_constants[4]);
    void set_checkpoint(const void* p_checkpoint_marker);
    [[nodiscard]] static bool supports_set_checkpoint();
    void set_cull_mode(VkCullModeFlags cull_mode);
    void set_depth_compare_op(VkCompareOp depth_compare_op);
    void set_depth_test_enable(VkBool32 depth_test_enable);
    void set_depth_write_enable(VkBool32 depth_write_enable);
    void set_front_face(VkFrontFace front_face);
    void set_scissor(
        std::uint32_t first_scissor, std::uint32_t scissor_count, const VkRect2D* p_scissors
    );
    void set_stencil_compare_mask(VkStencilFaceFlags face_mask, std::uint32_t compare_mask);
    void set_stencil_op(
        VkStencilFaceFlags face_mask,
        VkStencilOp fail_op,
        VkStencilOp pass_op,
        VkStencilOp depth_fail_op,
        VkCompareOp compare_op
    );
    void set_stencil_reference(VkStencilFaceFlags face_mask, std::uint32_t reference);
    void set_stencil_test_enable(VkBool32 stencil_test_enable);
    void set_stencil_write_mask(VkStencilFaceFlags face_mask, std::uint32_t write_mask);
    void set_viewport(
        std::uint32_t first_viewport, std::uint32_t viewport_count, const VkViewport* p_viewports
    );
    void write_buffer_marker(
        VkPipelineStageFlagBits pipeline_stage,
        VkBuffer dst_buffer,
        VkDeviceSize dst_offset,
        std::uint32_t marker
    );
    [[nodiscard]] static bool supports_write_buffer_marker();
    void write_timestamp(
        VkPipelineStageFlagBits pipeline_stage, VkQueryPool query_pool, std::uint32_t query
    );

private:
    VkCommandBuffer m_buffer;
};

}  // namespace openrac::vkgl
