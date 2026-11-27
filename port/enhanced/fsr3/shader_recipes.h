// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Pinned FidelityFX pass identities and definition assembly shared by runtime and build recipes.

#pragma once
#include <array>
#include <bit>
#include <string>

// The SDK's private declarations require its public context types first.
// clang-format off

#include <FidelityFX/host/ffx_fsr3upscaler.h>

#include <ffx_fsr3upscaler_private.h>

#include <ffx_shader_blobs.h>
// clang-format on

namespace openrac::enhanced::fsr3_detail {
// The pinned half pyramid callbacks are stubs. Select only the complete vendor
// history-sampling path in accumulation; reprojection and other passes stay FP32.
inline constexpr unsigned kShaderSwitches = (1u << 6) - 1;
inline constexpr unsigned kHalf = FSR3UPSCALER_SHADER_PERMUTATION_ALLOW_FP16;

// This integration supplies LDR colour, unjittered render-resolution motion and reversed depth.
inline constexpr unsigned kBasePermutation = FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS
                                             | FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED;

// Runtime and package generation share the four Lanczos/sharpening combinations they can select.
inline constexpr std::array<unsigned, 4> kSupportedPermutations{
    kBasePermutation,
    kBasePermutation | FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE,
    kBasePermutation | FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING,
    kBasePermutation | FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE
        | FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING
};

inline bool half_history_pass(unsigned pass) {
    return pass == FFX_FSR3UPSCALER_PASS_ACCUMULATE
           || pass == FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN;
}

// Maps the upscaler's supported passes to unmodified AMD sources.
inline constexpr std::array<const char*, FFX_FSR3UPSCALER_PASS_TCR_AUTOGENERATE> kPassSources{
    "prepare_inputs",
    "luma_pyramid",
    "shading_change_pyramid",
    "shading_change",
    "prepare_reactivity",
    "luma_instability",
    "accumulate",
    "accumulate",
    "rcas",
    "debug_view",
    "autogen_reactive"
};

// Translates the SDK's permutation flags to its original GLSL preprocessor switches.
inline std::string shader_definitions(unsigned permutation) {
    std::string result = "#define FFX_GPU 1\n#define FFX_GLSL 1\n";
    result += (permutation & kHalf) ? "#define FFX_HALF 1\n#define "
                                      "FFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF 1\n"
                                    : "#define FFX_HALF 0\n";
    constexpr std::array<const char*, 6> kSwitches{
        "REPROJECT_USE_LANCZOS_TYPE",
        "HDR_COLOR_INPUT",
        "LOW_RESOLUTION_MOTION_VECTORS",
        "JITTERED_MOTION_VECTORS",
        "INVERTED_DEPTH",
        "APPLY_SHARPENING"
    };

    // These positions are the pinned SDK's Fs3UpscalerShaderPermutationOptions bits.
    for (unsigned bit = 0; bit < kSwitches.size(); ++bit) {
        result += "#define FFX_FSR3UPSCALER_OPTION_";
        result += kSwitches[bit];
        result += (permutation & (1u << bit)) ? " 1\n" : " 0\n";
    }

    return result;
}

}  // namespace openrac::enhanced::fsr3_detail
