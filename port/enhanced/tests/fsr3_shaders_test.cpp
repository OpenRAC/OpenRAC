// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compiles AMD's supported temporal-upscaler permutations and checks their native resource reflection.
#include <bit>
#include <iostream>

#include "enhanced/fsr3/sdk_shaders.h"
#include "renderer/vulkan/spirv_precision.h"
#include "tests/check.h"

#include <FidelityFX/host/ffx_fsr3upscaler.h>

namespace {
// Every supported pass must compile before an SDK context can create Vulkan pipelines.
void test_upscaler_shaders_have_explicit_resources() {
    openrac::enhanced::fsr3_detail::ShaderLibrary shaders;
    std::string error;
    const bool ready = shaders.prepare(error);
    CHECK(ready);

    // Failed compilation is useful only when its original shader diagnostic is visible.
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }

    FfxShaderBlob blob{};
    // SDK flags: render-resolution motion (bit 2) and reversed depth (bit 4).
    constexpr unsigned kFlags = (1u << 2) | (1u << 4);
    CHECK(shaders.find(FFX_FSR3UPSCALER_PASS_PREPARE_INPUTS, kFlags, blob));
    CHECK(blob.data != nullptr && blob.size > 0);
    CHECK(blob.cbvCount == 1);
    // Prepare inputs reads motion, depth and colour, and writes five intermediate images.
    CHECK(blob.srvTextureCount == 3);
    CHECK(blob.uavTextureCount == 5);
    CHECK(std::string(blob.boundConstantBufferNames[0]) == "cbFSR3Upscaler");
    FfxShaderBlob requested_half{};
    CHECK(shaders.find(FFX_FSR3UPSCALER_PASS_PREPARE_INPUTS, kFlags | (1u << 7), requested_half));
    // The pinned GLSL half luma callbacks are stubs: requesting the SDK capability
    // must retain the supported FP32 permutation rather than select those stubs.
    CHECK(requested_half.data == blob.data && requested_half.size == blob.size);
    CHECK(!shaders.find(FFX_FSR3UPSCALER_PASS_COUNT, kFlags, blob));
}

void test_half_history_sampler() {
    openrac::enhanced::fsr3_detail::ShaderLibrary shaders;
    std::string error;
    const bool ready = shaders.prepare(error, true);
    CHECK(ready);
    if (!ready) {
        std::cerr << error << '\n';
        return;
    }
    constexpr unsigned flags = (1u << 2) | (1u << 4);
    for (unsigned extra : {0u, 1u, 32u, 33u}) {
        FfxShaderBlob full{}, half{};
        CHECK(shaders.find(FFX_FSR3UPSCALER_PASS_ACCUMULATE, flags | extra, full));
        CHECK(shaders.find(FFX_FSR3UPSCALER_PASS_ACCUMULATE, flags | extra | (1u << 7), half));
        const auto words = [](const FfxShaderBlob& b) {
            return std::span(reinterpret_cast<const std::uint32_t*>(b.data), b.size / 4);
        };
        const auto precision = openrac::vkgl::spirv_precision(words(half));
        CHECK(precision.fp16 > 0 && precision.int16 && !precision.storage16 && !precision.float64);
        CHECK(openrac::vkgl::spirv_precision(words(full)).fp16 == 0);
        CHECK(
            full.cbvCount == half.cbvCount && full.srvTextureCount == half.srvTextureCount
            && full.uavTextureCount == half.uavTextureCount
            && full.samplerCount == half.samplerCount
        );
        std::cout << "History permutation " << extra << ": " << precision.fp16
                  << " FP16 arithmetic ops\n";
    }
    for (unsigned pass :
         {unsigned(FFX_FSR3UPSCALER_PASS_LUMA_PYRAMID),
          unsigned(FFX_FSR3UPSCALER_PASS_SHADING_CHANGE_PYRAMID)}) {
        FfxShaderBlob a{}, b{};
        CHECK(shaders.find(pass, flags, a));
        CHECK(shaders.find(pass, flags | (1u << 7), b));
        CHECK(a.data == b.data);  // Never select the incomplete pyramid callbacks.
    }
}
}  // namespace

int main() {
    try {
        test_upscaler_shaders_have_explicit_resources();
        test_half_history_sampler();
        return openrac::test::result();
    } catch (...) {
        return 1;
    }
}
