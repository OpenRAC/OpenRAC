// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Arm ASR context creation and dispatch over renderer-owned Vulkan images.
#include "enhanced/asr/asr_context.h"

#include <cstring>
#include <string_view>
#include <vector>

#include "common/env.h"
#include "common/log.h"
#include "renderer/gl.h"
#include "renderer/vulkan/vkgl.h"

#include <host/backends/vk/ffxm_vk.h>
#include <host/ffxm_fsr2.h>

namespace openrac::enhanced::asr_detail {
namespace {
// The library's names for the formats the temporal module allocates; others are rejected.
arm::FfxmSurfaceFormat surface_format(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return arm::FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return arm::FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return arm::FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return arm::FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return arm::FFXM_SURFACE_FORMAT_R11G11B10_FLOAT;
        case VK_FORMAT_R16G16_SFLOAT:
            return arm::FFXM_SURFACE_FORMAT_R16G16_FLOAT;
        case VK_FORMAT_R32_SFLOAT:
            return arm::FFXM_SURFACE_FORMAT_R32_FLOAT;
        case VK_FORMAT_R16_SFLOAT:
            return arm::FFXM_SURFACE_FORMAT_R16_FLOAT;
        case VK_FORMAT_R8_UNORM:
            return arm::FFXM_SURFACE_FORMAT_R8_UNORM;
        default:
            return arm::FFXM_SURFACE_FORMAT_UNKNOWN;
    }
}

// Describes a borrowed renderer image; the output is drawn by ASR's fragment passes.
arm::FfxmResource resource(const vkgl::NativeImage& image, bool write) {
    arm::FfxmResourceDescription description{};
    description.type = arm::FFXM_RESOURCE_TYPE_TEXTURE2D;
    description.format = surface_format(image.format);
    description.width = image.width;
    description.height = image.height;
    description.depth = 1;
    description.mipCount = 1;
    description.usage =
        write ? static_cast<arm::FfxmResourceUsage>(
                    arm::FFXM_RESOURCE_USAGE_UAV | arm::FFXM_RESOURCE_USAGE_RENDERTARGET
                )
              : arm::FFXM_RESOURCE_USAGE_READ_ONLY;
    // The states match record_native_compute's layouts, which ASR restores after its passes.
    return arm::ffxmGetResourceVK(
        reinterpret_cast<void*>(image.image),
        description,
        nullptr,
        write ? arm::FFXM_RESOURCE_STATE_UNORDERED_ACCESS
              : arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ
    );
}
}  // namespace

bool preferred() {
    if (const auto chosen = env::value(env::Var::TemporalUpscaler)) {
        return *chosen == "asr";
    }
    return vkgl::tile_based();
}

// Library-owned histories and pipelines; the renderer owns all imported frame images.
struct AsrContext::State {
    arm::FfxmFsr2Context context{};     // Live only after successful creation; about 200 KB.
    std::vector<std::byte> scratch;     // Backend bookkeeping kept at a stable address while live.
    std::array<int, 2> render_size{};   // Active render extent.
    std::array<int, 2> display_size{};  // Active display extent.
    bool live = false;                  // Whether the library context needs destruction.

    bool create(const vkgl::NativeDevice& device, std::string& error) {
        arm::VkDeviceContext native{device.device, device.physical, device.device_proc};
        scratch.resize(arm::ffxmGetScratchMemorySizeVK(device.physical, 1));
        arm::FfxmFsr2ContextDescription description{};
        description.qualityMode = arm::FFXM_FSR2_SHADER_QUALITY_MODE_QUALITY;
        description.flags = arm::FFXM_FSR2_ENABLE_DEPTH_INVERTED;
        description.maxRenderSize =
            {static_cast<unsigned>(render_size[0]), static_cast<unsigned>(render_size[1])};
        description.displaySize =
            {static_cast<unsigned>(display_size[0]), static_cast<unsigned>(display_size[1])};
        auto result = arm::ffxmGetInterfaceVK(
            &description.backendInterface,
            arm::ffxmGetDeviceVK(&native),
            scratch.data(),
            scratch.size(),
            1
        );

        // A failed interface must not be handed to the context constructor.
        if (result != arm::FFXM_OK) {
            error = "cannot create Arm ASR Vulkan interface: " + std::to_string(result);
            return false;
        }

        result = arm::ffxmFsr2ContextCreate(&context, &description);
        live = result == arm::FFXM_OK;

        if (!live) {
            error = "cannot create Arm ASR context: " + std::to_string(result);
        }

        return live;
    }

    // Maps one rendered frame into the same reversed-depth, unjittered-motion contract as FSR 3.1.
    arm::FfxmErrorCode record(
        VkCommandBuffer command,
        std::span<const vkgl::NativeImage> images,
        const fsr3_detail::Dispatch& frame
    ) {
        // Combined depth/stencil has no surface format; the caller supplies a float colour copy.
        if (images[1].aspect != VK_IMAGE_ASPECT_COLOR_BIT) {
            return arm::FFXM_ERROR_INVALID_ARGUMENT;
        }

        // Unknown formats would be bound with wrong views rather than rejected by the library.
        for (const auto& image : images) {
            if (surface_format(image.format) == arm::FFXM_SURFACE_FORMAT_UNKNOWN) {
                return arm::FFXM_ERROR_INVALID_ARGUMENT;
            }
        }

        arm::FfxmFsr2DispatchDescription description{};
        description.commandList = arm::ffxmGetCommandListVK(command);
        description.color = resource(images[0], false);
        description.depth = resource(images[1], false);
        description.motionVectors = resource(images[2], false);
        description.reactive = resource(images[3], false);
        description.output = resource(images[4], true);
        description.jitterOffset = {frame.jitter[0], frame.jitter[1]};
        description.motionVectorScale = {1, 1};
        description.renderSize =
            {static_cast<unsigned>(render_size[0]), static_cast<unsigned>(render_size[1])};
        description.enableSharpening = frame.sharpness > 0;
        description.sharpness = frame.sharpness;
        description.frameTimeDelta = frame.milliseconds;
        description.preExposure = 1;
        description.reset = frame.reset;
        // The reversed-depth contract puts the farthest plane in cameraNear, as for FSR 3.1.
        description.cameraNear = frame.far_plane;
        description.cameraFar = frame.near_plane;
        description.cameraFovAngleVertical = frame.vertical_fov;
        description.viewSpaceToMetersFactor = 1;
        return arm::ffxmFsr2ContextDispatch(&context, &description);
    }
};

AsrContext::AsrContext() : m_state(std::make_unique<State>()) {}

AsrContext::~AsrContext() {
    release();
}

bool AsrContext::configure(
    std::array<int, 2> render_size, std::array<int, 2> display_size, std::string& error
) {
    if (render_size[0] <= 0 || render_size[1] <= 0 || display_size[0] < render_size[0]
        || display_size[1] < render_size[1]) {
        error = "invalid Arm ASR render or display dimensions";
        return false;
    }

    vkgl::NativeDevice device;

    // The prebuilt shaders use explicit 16-bit types (SPIR-V Float16 and Int16 capabilities),
    // also in subgroup operations, and its fragment passes write storage images.
    if (!gl::enhanced_rendering_available() || !vkgl::native_device(device) || !device.float16
        || !device.int16 || !device.subgroup_extended_types || !device.fragment_stores) {
        error = "Arm ASR requires 16-bit shader arithmetic (also in subgroups) and fragment "
                "storage writes";
        return false;
    }

    if (m_state->live && m_state->render_size == render_size
        && m_state->display_size == display_size) {
        return true;
    }

    release();
    m_state->render_size = render_size;
    m_state->display_size = display_size;

    if (!m_state->create(device, error)) {
        release();
        return false;
    }

    log::info("Arm ASR temporal upscaling: quality shaders, FP16");
    return true;
}

bool AsrContext::float16_active() const {
    return m_state->live;
}

bool AsrContext::dispatch(
    std::array<unsigned, 5> textures, const fsr3_detail::Dispatch& frame, std::string& error
) {
    if (!m_state->live) {
        error = "Arm ASR context is not configured";
        return false;
    }

    const std::array<vkgl::NativeImageUse, 5> uses{
        {{textures[0]}, {textures[1]}, {textures[2]}, {textures[3]}, {textures[4], true}}
    };
    arm::FfxmErrorCode result = arm::FFXM_OK;
    // The renderer lends the command buffer and images only for this synchronous dispatch.
    const bool recorded = vkgl::record_native_compute(uses, [&](auto command, auto images) {
        result = m_state->record(command, images, frame);
        return result == arm::FFXM_OK;
    });

    if (!recorded) {
        error = "cannot dispatch Arm ASR temporal upscaling: " + std::to_string(result);
    }

    return recorded;
}

void AsrContext::release() {
    // Histories may still be referenced by recorded or submitted GPU work.
    if (m_state->live) {
        gl::glFinish();
        arm::ffxmFsr2ContextDestroy(&m_state->context);
        m_state->live = false;
    }
}
}  // namespace openrac::enhanced::asr_detail
