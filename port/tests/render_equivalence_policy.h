// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Explicit owners for optimisations absent from the generic world fixture.
// CTest requires these existing output/counter comparisons before the scene shards run.

#pragma once

#include <array>
#include <string_view>
#include <utility>

#include "renderer/render_opt.h"

namespace openrac::test {

// Feature fixtures exercise real motion, temporal history, effects and swapchain behaviour.
inline constexpr std::array kFocusedEquivalence{
    std::pair{renderer::render_opt::Opt::MsaaPassResolve, "render_equivalence_support"},
    std::pair{renderer::render_opt::Opt::MotionBlurFastPath, "motion_blur"},
    std::pair{renderer::render_opt::Opt::MotionBlurUniqueSeeds, "motion_blur"},
    std::pair{renderer::render_opt::Opt::MotionBlurSetupReuse, "motion_blur"},
    std::pair{renderer::render_opt::Opt::MotionBlurAdaptiveSamples, "motion_blur"},
    std::pair{renderer::render_opt::Opt::FsrMotionBatch, "fsr3_motion"},
    std::pair{renderer::render_opt::Opt::FsrPaletteReuse, "fsr3_frame"},
    std::pair{renderer::render_opt::Opt::FsrObjectHistoryReuse, "fsr3_frame"},
    std::pair{renderer::render_opt::Opt::FsrReactiveR8, "fsr3_frame"},
    std::pair{renderer::render_opt::Opt::FsrFloat16, "fsr3_sdk"},
    std::pair{renderer::render_opt::Opt::NativeComputeSetup, "fsr3_sdk"},
    std::pair{renderer::render_opt::Opt::OverwriteDiscard, "motion_blur"},
    std::pair{renderer::render_opt::Opt::AgxDirectOutput, "fsr3_frame"},
    std::pair{renderer::render_opt::Opt::WaterStepBarriers, "water_runtime"},
    std::pair{renderer::render_opt::Opt::EffectQuadShader, "effect_material"},
    std::pair{renderer::render_opt::Opt::EffectSetupReuse, "effect_material"},
    std::pair{renderer::render_opt::Opt::EffectStripShader, "effect_material"},
    std::pair{renderer::render_opt::Opt::WaterMeshReuse, "effect_material"},
    std::pair{renderer::render_opt::Opt::EffectStaticControls, "effect_material"},
    std::pair{renderer::render_opt::Opt::WaterLocalShader, "effect_material"},
    std::pair{renderer::render_opt::Opt::VkDrawUniform, "vulkan_uniform"},
    std::pair{renderer::render_opt::Opt::PresentSingleCopy, "vulkan_present"},
    std::pair{renderer::render_opt::Opt::PresentTiming, "vulkan_present_timing"},
    std::pair{renderer::render_opt::Opt::ShadowScrollReuse, "shadow_scroll_vulkan"},
};

// Returns the CTest owner, or empty when the generic scene retains the comparison.
inline std::string_view focused_equivalence_test(renderer::render_opt::Opt opt) {
    for (const auto& [entry, owner] : kFocusedEquivalence) {
        if (entry == opt) {
            return owner;
        }
    }
    return {};
}

// Keeps compilation/cache/lifecycle checks independent of the test-only immutable object pool.
inline bool may_reuse_test_programs(renderer::render_opt::Opt opt) {
    using renderer::render_opt::Opt;
    return opt != Opt::PipelinePrewarm && opt != Opt::SpirvDiskCache && opt != Opt::VkDrawCache
           && opt != Opt::VkPersistentDescriptors && opt != Opt::VkLastPipeline
           && opt != Opt::ShaderVariants && opt != Opt::ShaderFloat16
           && opt != Opt::VkExtendedDynamicState;
}

}  // namespace openrac::test
