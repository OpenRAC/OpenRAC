// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shipped native compute programs and their optional development-time compiler fallback.
#include <atomic>
#include <format>

#include "renderer/vulkan/pipelines/shader_backend.h"
#include "renderer/vulkan/pipelines/shader_package.h"
#include "renderer/vulkan/pipelines/spirv_precision.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
bool compile_native_compute(
    std::string_view source,
    std::string_view definitions,
    const std::function<std::string_view(std::string_view)>& include,
    NativeComputeShader& out,
    std::string& error
) {
    const auto key = shader_key("native", {source, definitions});
    const auto audit = [&] {
        if (!detail::g) {
            return;
        }
        const auto precision = spirv_precision(out.code);
        std::atomic_ref(detail::g->submission.shader_fp16_ops).fetch_add(precision.fp16);
        std::atomic_ref(detail::g->submission.shader_fp32_ops).fetch_add(precision.fp32);
        std::atomic_ref(detail::g->submission.shader_fp16_modules).fetch_add(precision.fp16 != 0);
    };
    const auto packaged = builtin_shader_package().find(key);
    if (!packaged.empty() && decode_native_shader(packaged, out)) {
        if (detail::g) {
            std::atomic_ref(detail::g->submission.shader_package_hits).fetch_add(1);
        }
        audit();
        return true;
    }
    if (detail::g) {
        std::atomic_ref(detail::g->submission.shader_package_misses).fetch_add(1);
    }
    if (stats::probe("OPENRAC_SHADER_PACKAGE_STRICT", "Refuse runtime Vulkan shader compilation")) {
        error = std::format("missing packaged native Vulkan shader {:016x}", key.key);
        return false;
    }
#if OPENRAC_SHADER_RUNTIME_COMPILER
    const auto start = detail::Clock::now();
    const bool ok = compiler_detail::compile_native_fresh(source, definitions, include, out, error);
    if (detail::g) {
        std::atomic_ref(detail::g->submission.shader_compiles).fetch_add(1);
        std::atomic_ref(detail::g->submission.shader_compile_s)
            .fetch_add(detail::seconds_since(start));
    }
    if (ok) {
        audit();
    }
    return ok;
#else
    (void)include;
    error = std::format("missing packaged native Vulkan shader {:016x}", key.key);
    return false;
#endif
}

}  // namespace openrac::vkgl
