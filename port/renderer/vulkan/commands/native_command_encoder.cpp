// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native Vulkan command recording implementation used by graph executors.
#include "renderer/vulkan/commands/native_command_encoder.h"

#include "renderer/graph/recording_scope.h"

namespace openrac::vkgl {
namespace {
thread_local std::uint64_t image_work = 0;
}  // namespace

void NativeCommandEncoder::note_image_work() {
    renderer::graph::require_recording();
    ++image_work;
}

std::uint64_t NativeCommandEncoder::image_work_count() {
    return image_work;
}

void NativeCommandEncoder::build_acceleration_structures(
    std::uint32_t count,
    const VkAccelerationStructureBuildGeometryInfoKHR* infos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ranges
) {
    renderer::graph::require_recording();
    vkCmdBuildAccelerationStructuresKHR(m_buffer, count, infos, ranges);
}

void NativeCommandEncoder::begin_debug_utils_label(const VkDebugUtilsLabelEXT* p_label_info) {
    vkCmdBeginDebugUtilsLabelEXT(m_buffer, p_label_info);
}

bool NativeCommandEncoder::supports_begin_debug_utils_label() {
    return vkCmdBeginDebugUtilsLabelEXT != nullptr;
}

void NativeCommandEncoder::begin_rendering(const VkRenderingInfo* p_rendering_info) {
    vkCmdBeginRendering(m_buffer, p_rendering_info);
}

// NOLINTNEXTLINE(readability-function-size): Mirrors the native Vulkan command signature.
void NativeCommandEncoder::bind_descriptor_sets(
    VkPipelineBindPoint pipeline_bind_point,
    VkPipelineLayout layout,
    std::uint32_t first_set,
    std::uint32_t descriptor_set_count,
    const VkDescriptorSet* p_descriptor_sets,
    std::uint32_t dynamic_offset_count,
    const std::uint32_t* p_dynamic_offsets
) {
    vkCmdBindDescriptorSets(
        m_buffer,
        pipeline_bind_point,
        layout,
        first_set,
        descriptor_set_count,
        p_descriptor_sets,
        dynamic_offset_count,
        p_dynamic_offsets
    );
}

void NativeCommandEncoder::bind_index_buffer(
    VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type
) {
    vkCmdBindIndexBuffer(m_buffer, buffer, offset, index_type);
}

void NativeCommandEncoder::bind_pipeline(
    VkPipelineBindPoint pipeline_bind_point, VkPipeline pipeline
) {
    vkCmdBindPipeline(m_buffer, pipeline_bind_point, pipeline);
}

void NativeCommandEncoder::bind_vertex_buffers(
    std::uint32_t first_binding,
    std::uint32_t binding_count,
    const VkBuffer* p_buffers,
    const VkDeviceSize* p_offsets
) {
    vkCmdBindVertexBuffers(m_buffer, first_binding, binding_count, p_buffers, p_offsets);
}

// NOLINTNEXTLINE(readability-function-size): Mirrors the native Vulkan command signature.
void NativeCommandEncoder::blit_image(
    VkImage src_image,
    VkImageLayout src_image_layout,
    VkImage dst_image,
    VkImageLayout dst_image_layout,
    std::uint32_t region_count,
    const VkImageBlit* p_regions,
    VkFilter filter
) {
    note_image_work();
    vkCmdBlitImage(
        m_buffer,
        src_image,
        src_image_layout,
        dst_image,
        dst_image_layout,
        region_count,
        p_regions,
        filter
    );
}

void NativeCommandEncoder::clear_attachments(
    std::uint32_t attachment_count,
    const VkClearAttachment* p_attachments,
    std::uint32_t rect_count,
    const VkClearRect* p_rects
) {
    note_image_work();
    vkCmdClearAttachments(m_buffer, attachment_count, p_attachments, rect_count, p_rects);
}

void NativeCommandEncoder::clear_color_image(
    VkImage image,
    VkImageLayout image_layout,
    const VkClearColorValue* p_color,
    std::uint32_t range_count,
    const VkImageSubresourceRange* p_ranges
) {
    note_image_work();
    vkCmdClearColorImage(m_buffer, image, image_layout, p_color, range_count, p_ranges);
}

void NativeCommandEncoder::clear_depth_stencil_image(
    VkImage image,
    VkImageLayout image_layout,
    const VkClearDepthStencilValue* p_depth_stencil,
    std::uint32_t range_count,
    const VkImageSubresourceRange* p_ranges
) {
    note_image_work();
    vkCmdClearDepthStencilImage(
        m_buffer, image, image_layout, p_depth_stencil, range_count, p_ranges
    );
}

void NativeCommandEncoder::copy_buffer(
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    std::uint32_t region_count,
    const VkBufferCopy* p_regions
) {
    renderer::graph::require_recording();
    vkCmdCopyBuffer(m_buffer, src_buffer, dst_buffer, region_count, p_regions);
}

void NativeCommandEncoder::copy_buffer_to_image(
    VkBuffer src_buffer,
    VkImage dst_image,
    VkImageLayout dst_image_layout,
    std::uint32_t region_count,
    const VkBufferImageCopy* p_regions
) {
    note_image_work();
    vkCmdCopyBufferToImage(
        m_buffer, src_buffer, dst_image, dst_image_layout, region_count, p_regions
    );
}

// NOLINTNEXTLINE(readability-function-size): Mirrors the native Vulkan command signature.
void NativeCommandEncoder::copy_image(
    VkImage src_image,
    VkImageLayout src_image_layout,
    VkImage dst_image,
    VkImageLayout dst_image_layout,
    std::uint32_t region_count,
    const VkImageCopy* p_regions
) {
    note_image_work();
    vkCmdCopyImage(
        m_buffer, src_image, src_image_layout, dst_image, dst_image_layout, region_count, p_regions
    );
}

void NativeCommandEncoder::copy_image_to_buffer(
    VkImage src_image,
    VkImageLayout src_image_layout,
    VkBuffer dst_buffer,
    std::uint32_t region_count,
    const VkBufferImageCopy* p_regions
) {
    renderer::graph::require_recording();
    vkCmdCopyImageToBuffer(
        m_buffer, src_image, src_image_layout, dst_buffer, region_count, p_regions
    );
}

void NativeCommandEncoder::dispatch(
    std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z
) {
    note_image_work();
    vkCmdDispatch(m_buffer, group_count_x, group_count_y, group_count_z);
}

void NativeCommandEncoder::draw(
    std::uint32_t vertex_count,
    std::uint32_t instance_count,
    std::uint32_t first_vertex,
    std::uint32_t first_instance
) {
    note_image_work();
    vkCmdDraw(m_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void NativeCommandEncoder::draw_indexed(
    std::uint32_t index_count,
    std::uint32_t instance_count,
    std::uint32_t first_index,
    std::int32_t vertex_offset,
    std::uint32_t first_instance
) {
    note_image_work();
    vkCmdDrawIndexed(
        m_buffer, index_count, instance_count, first_index, vertex_offset, first_instance
    );
}

void NativeCommandEncoder::draw_indexed_indirect(
    VkBuffer buffer, VkDeviceSize offset, std::uint32_t draw_count, std::uint32_t stride
) {
    note_image_work();
    vkCmdDrawIndexedIndirect(m_buffer, buffer, offset, draw_count, stride);
}

void NativeCommandEncoder::end_debug_utils_label() {
    vkCmdEndDebugUtilsLabelEXT(m_buffer);
}

bool NativeCommandEncoder::supports_end_debug_utils_label() {
    return vkCmdEndDebugUtilsLabelEXT != nullptr;
}

void NativeCommandEncoder::end_rendering() {
    vkCmdEndRendering(m_buffer);
}

void NativeCommandEncoder::execute_commands(
    std::uint32_t command_buffer_count, const VkCommandBuffer* p_command_buffers
) {
    vkCmdExecuteCommands(m_buffer, command_buffer_count, p_command_buffers);
}

// NOLINTNEXTLINE(readability-function-size): Mirrors the native Vulkan command signature.
void NativeCommandEncoder::pipeline_barrier(
    VkPipelineStageFlags src_stage_mask,
    VkPipelineStageFlags dst_stage_mask,
    VkDependencyFlags dependency_flags,
    std::uint32_t memory_barrier_count,
    const VkMemoryBarrier* p_memory_barriers,
    std::uint32_t buffer_memory_barrier_count,
    const VkBufferMemoryBarrier* p_buffer_memory_barriers,
    std::uint32_t image_memory_barrier_count,
    const VkImageMemoryBarrier* p_image_memory_barriers
) {
    vkCmdPipelineBarrier(
        m_buffer,
        src_stage_mask,
        dst_stage_mask,
        dependency_flags,
        memory_barrier_count,
        p_memory_barriers,
        buffer_memory_barrier_count,
        p_buffer_memory_barriers,
        image_memory_barrier_count,
        p_image_memory_barriers
    );
}

void NativeCommandEncoder::pipeline_barrier2(const VkDependencyInfo& dependency) {
    vkCmdPipelineBarrier2(m_buffer, &dependency);
}

void NativeCommandEncoder::push_constants(
    VkPipelineLayout layout,
    VkShaderStageFlags stage_flags,
    std::uint32_t offset,
    std::uint32_t size,
    const void* p_values
) {
    vkCmdPushConstants(m_buffer, layout, stage_flags, offset, size, p_values);
}

void NativeCommandEncoder::reset_query_pool(
    VkQueryPool query_pool, std::uint32_t first_query, std::uint32_t query_count
) {
    vkCmdResetQueryPool(m_buffer, query_pool, first_query, query_count);
}

// NOLINTNEXTLINE(readability-function-size): Mirrors the native Vulkan command signature.
void NativeCommandEncoder::resolve_image(
    VkImage src_image,
    VkImageLayout src_image_layout,
    VkImage dst_image,
    VkImageLayout dst_image_layout,
    std::uint32_t region_count,
    const VkImageResolve* p_regions
) {
    note_image_work();
    vkCmdResolveImage(
        m_buffer, src_image, src_image_layout, dst_image, dst_image_layout, region_count, p_regions
    );
}

void NativeCommandEncoder::set_blend_constants(const float blend_constants[4]) {
    vkCmdSetBlendConstants(m_buffer, blend_constants);
}

void NativeCommandEncoder::set_checkpoint(const void* p_checkpoint_marker) {
    vkCmdSetCheckpointNV(m_buffer, p_checkpoint_marker);
}

bool NativeCommandEncoder::supports_set_checkpoint() {
    return vkCmdSetCheckpointNV != nullptr;
}

void NativeCommandEncoder::set_cull_mode(VkCullModeFlags cull_mode) {
    vkCmdSetCullMode(m_buffer, cull_mode);
}

void NativeCommandEncoder::set_depth_compare_op(VkCompareOp depth_compare_op) {
    vkCmdSetDepthCompareOp(m_buffer, depth_compare_op);
}

void NativeCommandEncoder::set_depth_test_enable(VkBool32 depth_test_enable) {
    vkCmdSetDepthTestEnable(m_buffer, depth_test_enable);
}

void NativeCommandEncoder::set_depth_write_enable(VkBool32 depth_write_enable) {
    vkCmdSetDepthWriteEnable(m_buffer, depth_write_enable);
}

void NativeCommandEncoder::set_front_face(VkFrontFace front_face) {
    vkCmdSetFrontFace(m_buffer, front_face);
}

void NativeCommandEncoder::set_scissor(
    std::uint32_t first_scissor, std::uint32_t scissor_count, const VkRect2D* p_scissors
) {
    vkCmdSetScissor(m_buffer, first_scissor, scissor_count, p_scissors);
}

void NativeCommandEncoder::set_stencil_compare_mask(
    VkStencilFaceFlags face_mask, std::uint32_t compare_mask
) {
    vkCmdSetStencilCompareMask(m_buffer, face_mask, compare_mask);
}

void NativeCommandEncoder::set_stencil_op(
    VkStencilFaceFlags face_mask,
    VkStencilOp fail_op,
    VkStencilOp pass_op,
    VkStencilOp depth_fail_op,
    VkCompareOp compare_op
) {
    vkCmdSetStencilOp(m_buffer, face_mask, fail_op, pass_op, depth_fail_op, compare_op);
}

void NativeCommandEncoder::set_stencil_reference(
    VkStencilFaceFlags face_mask, std::uint32_t reference
) {
    vkCmdSetStencilReference(m_buffer, face_mask, reference);
}

void NativeCommandEncoder::set_stencil_test_enable(VkBool32 stencil_test_enable) {
    vkCmdSetStencilTestEnable(m_buffer, stencil_test_enable);
}

void NativeCommandEncoder::set_stencil_write_mask(
    VkStencilFaceFlags face_mask, std::uint32_t write_mask
) {
    vkCmdSetStencilWriteMask(m_buffer, face_mask, write_mask);
}

void NativeCommandEncoder::set_viewport(
    std::uint32_t first_viewport, std::uint32_t viewport_count, const VkViewport* p_viewports
) {
    vkCmdSetViewport(m_buffer, first_viewport, viewport_count, p_viewports);
}

void NativeCommandEncoder::write_buffer_marker(
    VkPipelineStageFlagBits pipeline_stage,
    VkBuffer dst_buffer,
    VkDeviceSize dst_offset,
    std::uint32_t marker
) {
    vkCmdWriteBufferMarkerAMD(m_buffer, pipeline_stage, dst_buffer, dst_offset, marker);
}

bool NativeCommandEncoder::supports_write_buffer_marker() {
    return vkCmdWriteBufferMarkerAMD != nullptr;
}

void NativeCommandEncoder::write_timestamp(
    VkPipelineStageFlagBits pipeline_stage, VkQueryPool query_pool, std::uint32_t query
) {
    vkCmdWriteTimestamp(m_buffer, pipeline_stage, query_pool, query);
}

}  // namespace openrac::vkgl
