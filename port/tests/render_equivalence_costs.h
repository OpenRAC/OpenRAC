// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Equivalence scheduling estimates in milliseconds, keyed by registry enum rather than order.

#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "renderer/render_opt.h"

namespace openrac::test {

// Software Vulkan scene costs with immutable program retention, rounded to 100 milliseconds.
// Startup is common to both shards; first state-only entries use the warm state-skip cost.
inline constexpr std::array kEquivalenceMeasuredCosts{
    std::pair{renderer::render_opt::Opt::RegistrySelftest, 5100u},
    std::pair{renderer::render_opt::Opt::VkDrawClock, 5100u},
    std::pair{renderer::render_opt::Opt::VkStateSkip, 5100u},
    std::pair{renderer::render_opt::Opt::StaticBuffersDeviceLocal, 5100u},
    std::pair{renderer::render_opt::Opt::StaticBuffersForceStaging, 5000u},
    std::pair{renderer::render_opt::Opt::TexturePrepare, 4900u},
    std::pair{renderer::render_opt::Opt::VkDrawCache, 26700u},
    std::pair{renderer::render_opt::Opt::AttachmentDiscard, 5200u},
    std::pair{renderer::render_opt::Opt::GpuResidentInstances, 5200u},
    std::pair{renderer::render_opt::Opt::ShadowStaticCache, 5200u},
    std::pair{renderer::render_opt::Opt::VkTightSync, 5200u},
    std::pair{renderer::render_opt::Opt::PipelinePrewarm, 27400u},
    std::pair{renderer::render_opt::Opt::VkExtendedDynamicState, 35800u},
    std::pair{renderer::render_opt::Opt::MobyPoseInPlace, 5200u},
    std::pair{renderer::render_opt::Opt::ShadowAtlasDiscard, 4600u},
    std::pair{renderer::render_opt::Opt::SpirvDiskCache, 34700u},
    std::pair{renderer::render_opt::Opt::VkPersistentDescriptors, 48300u},
    std::pair{renderer::render_opt::Opt::VkLastPipeline, 36500u},
    std::pair{renderer::render_opt::Opt::ShadowDrawCache, 5800u},
    std::pair{renderer::render_opt::Opt::MobyGlowSkip, 19100u},
    std::pair{renderer::render_opt::Opt::GtaoDirectDepth, 5100u},
    std::pair{renderer::render_opt::Opt::VkBufferDirectFill, 5600u},
    std::pair{renderer::render_opt::Opt::ShadowFrameReuse, 5000u},
    std::pair{renderer::render_opt::Opt::ProbeTranslucentTiles, 4400u},
    std::pair{renderer::render_opt::Opt::ShadowDryShaders, 4800u},
    std::pair{renderer::render_opt::Opt::ShadowGeometryCull, 6200u},
    std::pair{renderer::render_opt::Opt::ShadowStaticGeometryCull, 8600u},
    std::pair{renderer::render_opt::Opt::PrepassRigid, 4700u},
    std::pair{renderer::render_opt::Opt::ProbeSelectionVariants, 4400u},
    std::pair{renderer::render_opt::Opt::EarlyProbeSetup, 8500u},
    std::pair{renderer::render_opt::Opt::ShaderVariants, 55500u},
    std::pair{renderer::render_opt::Opt::ShaderFloat16, 19200u},
    std::pair{renderer::render_opt::Opt::CameraVisibilityReuse, 6500u},
    std::pair{renderer::render_opt::Opt::ShadowDepthOnly, 6500u},
    std::pair{renderer::render_opt::Opt::AttachmentUntouched, 5700u},
    std::pair{renderer::render_opt::Opt::PassRenderArea, 8800u},
    std::pair{renderer::render_opt::Opt::ProbeFastBlended, 8000u},
};

// Returns one positive cost per registry entry; unmeasured entries use the median.
inline auto equivalence_entry_costs() {
    constexpr unsigned kNewEntryCost = 5600;  // Median milliseconds for unmeasured entries.
    std::array<unsigned, renderer::render_opt::kOptCount> costs;
    costs.fill(kNewEntryCost);
    for (const auto& [opt, milliseconds] : kEquivalenceMeasuredCosts) {
        costs[static_cast<std::size_t>(opt)] = milliseconds;
    }
    return costs;
}

}  // namespace openrac::test
