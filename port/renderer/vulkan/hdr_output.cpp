// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR output (common/hdr.h, docs/port/HDR.md): the swapchain made in an HDR colour space when the
// frontend asks for it and the surface offers it, and the one compute pass that writes the default
// framebuffer's picture into it at the player's paper white (shaders/hdr_encode.comp). With HDR
// not asked for, nothing here runs: the swapchain is chosen and the picture copied as before.

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

#include "renderer/vulkan/graph_presentation.h"
#include "renderer/vulkan/surface_format.h"
#include "renderer/vulkan/vkgl_internal.h"
#include "vkgl_shaders.h"

namespace openrac::vkgl {

namespace detail {

namespace {

using hdr::Output;

// The encode pass's push constants (hdr_encode.comp's Encode).
struct EncodeConstants {
    float paper_white;
    float sdr_gamma;
    std::int32_t same_size;
    float headroom;
    float highlights;
    float sdr_hue;
    float sdr_chroma;
    float blowout;
    std::int32_t plain;
};

// Whether the encode pass can write `format`: the device stores to it. Where the surface's images
// cannot be storage images, the pass writes an image of the same format and that is copied 1:1.
bool can_store(void*, VkFormat format) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(g->physical, format, &props);
    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0;
}

void release_pipeline() {
    auto& hdr = g->hdr;
    vkDestroyPipeline(g->device, hdr.pipeline, nullptr);
    vkDestroyPipelineLayout(g->device, hdr.layout, nullptr);
    vkDestroyDescriptorSetLayout(g->device, hdr.descriptors, nullptr);
    hdr.pipeline = VK_NULL_HANDLE;
    hdr.layout = VK_NULL_HANDLE;
    hdr.descriptors = VK_NULL_HANDLE;
    hdr.pipeline_for = Output::Sdr;
}

// The encode pipeline for `output`, made the first time it is needed; one at a time.
bool make_pipeline(Output output) {
    auto& hdr = g->hdr;
    if (hdr.pipeline != VK_NULL_HANDLE && hdr.pipeline_for == output) {
        return true;
    }
    if (hdr.failed) {
        return false;
    }
    release_pipeline();
    const auto failed = [] {
        release_pipeline();
        g->hdr.failed = true;
        return false;
    };
    NativeComputeShader shader;
    std::string error;
    const std::string_view definitions =
        output == Output::Hdr10 ? "#define OPENRAC_HDR_PQ 1\n" : "";
    if (!compile_native_compute(shaders::hdr_encode_comp, definitions, {}, shader, error)) {
        log::warn("vulkan: no HDR encode pass ({}); the picture goes out in SDR", error);
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
            vkCreateDescriptorSetLayout(g->device, &descriptors, nullptr, &hdr.descriptors),
            "HDR encode descriptors"
        )) {
        return failed();
    }
    const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(EncodeConstants)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &hdr.descriptors;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    if (!check(
            vkCreatePipelineLayout(g->device, &layout, nullptr, &hdr.layout), "HDR encode layout"
        )) {
        return failed();
    }
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo code{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    code.codeSize = shader.code.size() * sizeof(std::uint32_t);
    code.pCode = shader.code.data();
    if (!check(vkCreateShaderModule(g->device, &code, nullptr, &module), "HDR encode module")) {
        return failed();
    }
    VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipeline.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipeline.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module = module;
    pipeline.stage.pName = "main";
    pipeline.layout = hdr.layout;
    const bool made = check(
        vkCreateComputePipelines(
            g->device, g->pipeline_cache, 1, &pipeline, nullptr, &hdr.pipeline
        ),
        "HDR encode pipeline"
    );
    vkDestroyShaderModule(g->device, module, nullptr);
    if (!made) {
        return failed();
    }
    hdr.pipeline_for = output;
    count_pipeline();
    return true;
}

// Prepare descriptors before recording so failure can select the SDR graph.
VkDescriptorSet prepare_encode(VkImageView view, Output output) {
    if (!g->default_colour || !view || !make_pipeline(output)) {
        return VK_NULL_HANDLE;
    }
    const VkDescriptorSet set = allocate_set(g->hdr.descriptors);
    if (!set) {
        return VK_NULL_HANDLE;
    }
    Image& colour = *g->default_colour;
    SamplerParams parameters;
    parameters.min = parameters.mag = gl::GL_LINEAR;
    parameters.wrap_s = parameters.wrap_t = gl::GL_CLAMP_TO_EDGE;
    const std::array<VkDescriptorImageInfo, 2> images{
        {{sampler_for(parameters, false),
          sample_view(colour, 0, 0),
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
         {VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL}}
    };
    std::array<VkWriteDescriptorSet, 2> writes{};
    for (unsigned i = 0; i < writes.size(); ++i) {
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType =
            i == 0 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(
        g->device, static_cast<unsigned>(writes.size()), writes.data(), 0, nullptr
    );
    g->submission.descriptor_writes += writes.size();
    return set;
}

void encode(
    VkCommandBuffer cmd, VkDescriptorSet set, VkExtent2D extent, const hdr::Levels& levels
) {
    const Image& colour = *g->default_colour;
    const EncodeConstants constants{
        levels.paper_white,
        hdr::kSdrGamma,
        colour.width == static_cast<int>(extent.width)
                && colour.height == static_cast<int>(extent.height)
            ? 1
            : 0,
        hdr::headroom(levels),
        levels.look.highlights,
        levels.look.sdr_hue,
        levels.look.sdr_chroma,
        levels.look.blowout,
        levels.look.plain() ? 1 : 0,
    };
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g->hdr.pipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g->hdr.layout, 0, 1, &set, 0, nullptr
    );
    vkCmdPushConstants(
        cmd, g->hdr.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants
    );
    vkCmdDispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);
    count_command(Command::BindPipeline);
    count_command(Command::BindDescriptors);
    count_command(Command::Other);
    count_command(Command::Dispatch);
    count_copy(extent.width, extent.height);
}

void copy_encoded(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D extent) {
    VkImageCopy copy{};
    copy.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.extent = {extent.width, extent.height, 1};
    vkCmdCopyImage(
        cmd,
        source,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy
    );
    count_command(Command::Copy);
    count_copy(extent.width, extent.height);
}

// The screen's metadata: BT.2020 primaries, the display's peak, content up to the peak (MaxCLL:
// the frame's light above white rolls off towards it) and a frame average at paper white, so that
// a television need not tone-map it down.
void set_metadata() {
    auto& hdr = g->hdr;
    if (!hdr.metadata || vkSetHdrMetadataEXT == nullptr || hdr.shown == Output::Sdr) {
        return;
    }
    VkHdrMetadataEXT m{VK_STRUCTURE_TYPE_HDR_METADATA_EXT};
    m.displayPrimaryRed = {0.708f, 0.292f};
    m.displayPrimaryGreen = {0.170f, 0.797f};
    m.displayPrimaryBlue = {0.131f, 0.046f};
    m.whitePoint = {0.3127f, 0.3290f};
    m.maxLuminance = hdr.asked.levels.peak;
    m.minLuminance = 0.0f;
    m.maxContentLightLevel = hdr.asked.levels.peak;
    m.maxFrameAverageLightLevel = hdr.asked.levels.paper_white;
    vkSetHdrMetadataEXT(g->device, 1, &g->swapchain, &m);
}

}  // namespace

VkSurfaceFormatKHR choose_hdr_format(
    std::span<const VkSurfaceFormatKHR> formats, const VkSurfaceCapabilitiesKHR& caps
) {
    auto& hdr = g->hdr;
    hdr.offered = hdr.colour_spaces && offers_hdr(formats);
    const Output wanted = hdr.colour_spaces ? hdr.asked.wanted : Output::Sdr;
    const SurfaceChoice choice = choose_surface_format(formats, wanted, can_store, nullptr);
    hdr.direct = (caps.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
    hdr.made_for = hdr.asked.wanted;
    hdr.shown = choice.output;
    if (wanted != Output::Sdr && choice.output == Output::Sdr) {
        log::warn(
            "vulkan: HDR asked for, but the screen offers no HDR format the game can write; SDR"
        );
    }
    return choice.format;
}

void release_hdr_views() {
    for (VkImageView v : g->hdr.views) {
        vkDestroyImageView(g->device, v, nullptr);
    }
    g->hdr.views.clear();
    // Only ever let go with the device idle (a swapchain made again, the context closed).
    g->hdr.staging.reset();
}

void hdr_swapchain_made(const VkSurfaceFormatKHR& format) {
    release_hdr_views();
    auto& hdr = g->hdr;
    if (hdr.shown == Output::Sdr) {
        return;
    }
    // Images that cannot be stored to: the pass writes one of its own, copied into each.
    if (!hdr.direct) {
        const Format staging{
            format.format, VK_IMAGE_ASPECT_COLOR_BIT, hdr.shown == Output::Hdr10 ? 4 : 8
        };
        hdr.staging = make_image(
            staging,
            static_cast<int>(g->swapchain_extent.width),
            static_cast<int>(g->swapchain_extent.height),
            1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_STORAGE_BIT
        );
        if (!hdr.staging) {
            hdr.shown = Output::Sdr;
            return;
        }
    }
    // A view of each swapchain image for the pass's writes, when they can be storage images.
    const std::vector<VkImage> none;
    for (VkImage image : hdr.direct ? g->swapchain_images : none) {
        VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = format.format;
        info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkImageView view = VK_NULL_HANDLE;
        if (!check(vkCreateImageView(g->device, &info, nullptr, &view), "an HDR screen view")) {
            release_hdr_views();
            hdr.shown = Output::Sdr;
            return;
        }
        hdr.views.push_back(view);
    }
    set_metadata();
    log::info(
        "vulkan: HDR output {} at {:.0f} nits paper white (peak {:.0f}){}",
        hdr::name(hdr.shown),
        hdr.asked.levels.paper_white,
        hdr.asked.levels.peak,
        hdr.direct ? "" : ", through a copy"
    );
}

void release_hdr() {
    release_hdr_views();
    release_pipeline();
    g->hdr.failed = false;
}

bool hdr_encode_into(VkCommandBuffer cmd, std::uint32_t index) {
    auto& hdr = g->hdr;
    const VkImage screen = g->swapchain_images[index];
    const VkExtent2D extent = g->swapchain_extent;
    Image* staging = hdr.direct ? nullptr : hdr.staging.get();
    const VkImageView view = hdr.direct
                                 ? (index < hdr.views.size() ? hdr.views[index] : VK_NULL_HANDLE)
                                 : (staging ? sample_view(*staging, 0, 0) : VK_NULL_HANDLE);
    const VkDescriptorSet set = prepare_encode(view, hdr.shown);
    if (!set) {
        return false;
    }
    Image destination;
    destination.image = screen;
    const auto result = record_presentation(
        cmd,
        g->default_colour,
        destination,
        true,
        [&] { copy_encoded(cmd, staging->image, screen, extent); },
        {staging, [&] { encode(cmd, set, extent, hdr.asked.levels); }}
    );
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
    return true;
}

}  // namespace detail

void set_hdr(const HdrRequest& request) {
    using namespace detail;
    if (!active()) {
        return;
    }
    auto& hdr = g->hdr;
    const bool same = hdr.asked == request;
    const bool same_metadata = hdr.asked.levels.paper_white == request.levels.paper_white
                               && hdr.asked.levels.peak == request.levels.peak;
    hdr.asked = request;
    if (same || !hdr.colour_spaces) {
        return;
    }
    // A new kind needs a new swapchain. New levels need new constants, and new metadata unless
    // only the look changed.
    if (request.wanted != hdr.made_for) {
        g->swapchain_stale = true;
    } else if (!same_metadata) {
        set_metadata();
    }
}

hdr::Output hdr_output() {
    using namespace detail;
    return active() ? g->hdr.shown : hdr::Output::Sdr;
}

bool hdr_offered() {
    using namespace detail;
    return active() && g->hdr.offered;
}

bool hdr_frame() {
    using namespace detail;
    return active() && g->hdr.shown != hdr::Output::Sdr;
}

void clamp_colour_writes(unsigned framebuffer) {
    using namespace detail;
    if (!active() || framebuffer == 0) {
        return;
    }
    if (Image* colour = target_of(framebuffer).colour) {
        colour->clamp_writes = true;
    }
}

namespace {

// test_hdr_encode's pass on the default framebuffer's picture.
bool test_encode_picture(
    hdr::Output output,
    const hdr::Levels& levels,
    int width,
    int height,
    std::vector<std::uint8_t>& out,
    bool staged
) {
    using namespace detail;
    using hdr::Output;
    const bool pq = output == Output::Hdr10;
    const Format format{
        pq ? VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        pq ? 4 : 8
    };
    const ImagePtr screen =
        make_image(format, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_STORAGE_BIT);
    if (!screen) {
        return false;
    }
    ImagePtr staging;
    if (staged) {
        staging =
            make_image(format, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_STORAGE_BIT);
        if (!staging) {
            retire(screen);
            return false;
        }
    }
    const auto bytes = static_cast<VkDeviceSize>(width) * height * format.bytes;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = bytes;
    info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;
    alloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo got{};
    if (!check(
            vmaCreateBuffer(g->allocator, &info, &alloc, &buffer, &allocation, &got), "a read-back"
        )) {
        retire(staging);
        retire(screen);
        return false;
    }
    VkCommandBuffer cmd = cmd_for(screen.get(), true);
    const VkExtent2D extent{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    const auto set = prepare_encode(sample_view(staged ? *staging : *screen, 0, 0), output);
    const bool encoded = set
                         && record_presentation(
                             cmd,
                             g->default_colour,
                             *screen,
                             false,
                             [&] { copy_encoded(cmd, staging->image, screen->image, extent); },
                             {staging.get(), [&] { encode(cmd, set, extent, levels); }}
                         );
    if (encoded) {
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {extent.width, extent.height, 1};
        vkCmdCopyImageToBuffer(
            cmd, screen->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &copy
        );
        count_command(Command::Copy);
    }
    flush_and_wait();
    if (encoded) {
        vmaInvalidateAllocation(g->allocator, allocation, 0, VK_WHOLE_SIZE);
        const auto* from = static_cast<const std::uint8_t*>(got.pMappedData);
        out.assign(from, from + bytes);
    }
    vmaDestroyBuffer(g->allocator, buffer, allocation);
    retire(staging);
    retire(screen);
    return encoded;
}

}  // namespace

bool test_hdr_encode(
    hdr::Output output,
    const hdr::Levels& levels,
    int width,
    int height,
    std::vector<std::uint8_t>& out,
    bool staged,
    unsigned source
) {
    using namespace detail;
    using hdr::Output;
    if (!active() || output == Output::Sdr || !g->default_colour) {
        return false;
    }
    end_rendering();
    // Use `source`'s picture instead of the default one for this pass. Not owned: the
    // framebuffer keeps it.
    Image* const picture = source != 0 ? target_of(source).colour : nullptr;
    if (source != 0 && picture == nullptr) {
        return false;
    }
    ImagePtr default_colour = g->default_colour;
    if (picture != nullptr) {
        g->default_colour = ImagePtr(ImagePtr{}, picture);
    }
    const bool encoded = test_encode_picture(output, levels, width, height, out, staged);
    g->default_colour = std::move(default_colour);
    return encoded;
}

}  // namespace openrac::vkgl
