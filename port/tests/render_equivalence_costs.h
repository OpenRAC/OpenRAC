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

// Mesa/LLVM 21 setup and drawing in both modes, rounded to 100 milliseconds.
inline constexpr std::array kEquivalenceMeasuredCosts{
    std::pair{renderer::render_opt::Opt::RegistrySelftest, 24100u},
    std::pair{renderer::render_opt::Opt::VkDrawClock, 24000u},
    std::pair{renderer::render_opt::Opt::VkStateSkip, 25100u},
    std::pair{renderer::render_opt::Opt::StaticBuffersDeviceLocal, 24500u},
    std::pair{renderer::render_opt::Opt::StaticBuffersForceStaging, 25600u},
    std::pair{renderer::render_opt::Opt::TexturePrepare, 24200u},
    std::pair{renderer::render_opt::Opt::VkDrawCache, 25000u},
    std::pair{renderer::render_opt::Opt::AttachmentDiscard, 25700u},
    std::pair{renderer::render_opt::Opt::GpuResidentInstances, 24500u},
    std::pair{renderer::render_opt::Opt::ShadowStaticCache, 25600u},
    std::pair{renderer::render_opt::Opt::PresentSingleCopy, 26900u},
    std::pair{renderer::render_opt::Opt::VkTightSync, 26200u},
    std::pair{renderer::render_opt::Opt::PipelinePrewarm, 26800u},
    std::pair{renderer::render_opt::Opt::PresentTiming, 26000u},
    std::pair{renderer::render_opt::Opt::VkExtendedDynamicState, 31400u},
    std::pair{renderer::render_opt::Opt::MobyPoseInPlace, 27500u},
    std::pair{renderer::render_opt::Opt::ShadowAtlasDiscard, 31400u},
    std::pair{renderer::render_opt::Opt::SpirvDiskCache, 29900u},
    std::pair{renderer::render_opt::Opt::VkPersistentDescriptors, 28100u},
    std::pair{renderer::render_opt::Opt::VkLastPipeline, 27400u},
    std::pair{renderer::render_opt::Opt::ShadowDrawCache, 38000u},
    std::pair{renderer::render_opt::Opt::ShadowScrollReuse, 28400u},
    std::pair{renderer::render_opt::Opt::MotionBlurFastPath, 61500u},
    std::pair{renderer::render_opt::Opt::FsrMotionBatch, 32500u},
    std::pair{renderer::render_opt::Opt::MobyGlowSkip, 67200u},
    std::pair{renderer::render_opt::Opt::GtaoDirectDepth, 73000u},
    std::pair{renderer::render_opt::Opt::VkBufferDirectFill, 46700u},
    std::pair{renderer::render_opt::Opt::FsrPaletteReuse, 37200u},
    std::pair{renderer::render_opt::Opt::WaterStepBarriers, 41600u},
    std::pair{renderer::render_opt::Opt::FsrObjectHistoryReuse, 37300u},
    std::pair{renderer::render_opt::Opt::FsrFloat16, 41400u},
    std::pair{renderer::render_opt::Opt::NativeComputeSetup, 37400u},
    std::pair{renderer::render_opt::Opt::ShadowFrameReuse, 38600u},
    std::pair{renderer::render_opt::Opt::ProbeTranslucentTiles, 30700u},
    std::pair{renderer::render_opt::Opt::ShadowDryShaders, 41800u},
    std::pair{renderer::render_opt::Opt::MotionBlurUniqueSeeds, 36400u},
    std::pair{renderer::render_opt::Opt::MotionBlurSetupReuse, 65700u},
    std::pair{renderer::render_opt::Opt::ShadowGeometryCull, 29100u},
    std::pair{renderer::render_opt::Opt::EffectQuadShader, 64200u},
    std::pair{renderer::render_opt::Opt::EffectSetupReuse, 31000u},
    std::pair{renderer::render_opt::Opt::WaterMeshReuse, 36400u},
    std::pair{renderer::render_opt::Opt::EffectStripShader, 40900u},
    std::pair{renderer::render_opt::Opt::ShadowStaticGeometryCull, 40200u},
    std::pair{renderer::render_opt::Opt::AgxDirectOutput, 63800u},
    std::pair{renderer::render_opt::Opt::PrepassRigid, 47300u},
    std::pair{renderer::render_opt::Opt::EffectStaticControls, 49200u},
    std::pair{renderer::render_opt::Opt::WaterLocalShader, 31400u},
    std::pair{renderer::render_opt::Opt::ProbeSelectionVariants, 29400u},
    std::pair{renderer::render_opt::Opt::EarlyProbeSetup, 33100u},
    std::pair{renderer::render_opt::Opt::VkDrawUniform, 32000u},
    std::pair{renderer::render_opt::Opt::ShaderVariants, 41300u},
    std::pair{renderer::render_opt::Opt::ShaderFloat16, 25400u},
    std::pair{renderer::render_opt::Opt::CameraVisibilityReuse, 66300u},
    std::pair{renderer::render_opt::Opt::ShadowDepthOnly, 34200u},
    std::pair{renderer::render_opt::Opt::FsrReactiveR8, 64100u},
    std::pair{renderer::render_opt::Opt::MsaaPassResolve, 30800u},
    std::pair{renderer::render_opt::Opt::MotionBlurAdaptiveSamples, 52400u},
    std::pair{renderer::render_opt::Opt::AttachmentUntouched, 24500u},
    std::pair{renderer::render_opt::Opt::PassRenderArea, 33400u},
    std::pair{renderer::render_opt::Opt::OverwriteDiscard, 30400u},
    std::pair{renderer::render_opt::Opt::ProbeFastBlended, 32800u},
};

// Returns one positive cost per registry entry; unmeasured entries use the median.
inline auto equivalence_entry_costs() {
    constexpr unsigned kNewEntryCost = 32000;  // Median milliseconds for unmeasured entries.
    std::array<unsigned, renderer::render_opt::kOptCount> costs;
    costs.fill(kNewEntryCost);
    for (const auto& [opt, milliseconds] : kEquivalenceMeasuredCosts) {
        costs[static_cast<std::size_t>(opt)] = milliseconds;
    }
    return costs;
}

}  // namespace openrac::test
