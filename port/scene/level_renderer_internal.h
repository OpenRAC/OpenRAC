// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Private renderer utilities shared by scene uploads, instances, lighting and shadows.
#pragma once
#include <array>
#include <cstddef>
#include <memory>

#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/math.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif
namespace openrac::scene::render_detail {
inline bool gpu_driven() {
#if defined(OPENRAC_VULKAN)
    return vkgl::gpu_driven_active();
#else
    return false;
#endif
}

// The joint palette texture: RGBA32F, four texels (columns) per matrix, this many texels a row.
inline constexpr int kPaletteWidth = 1024;
inline constexpr gl::GLenum kRgba32f = 0x8814;
inline constexpr gl::GLenum kPaletteUnit = 1;
// The ties' lit colours: RGBA8, one texel per colour, this many a row.
inline constexpr int kLightWidth = 1024;
inline constexpr gl::GLenum kLightUnit = 2;

inline const void* offset(std::size_t bytes) {
    return reinterpret_cast<const void*>(bytes);
}

// Lets `target` and its GL objects go: a FrameBuffer frees them only in release() (its destructor
// runs where the context may be gone), so a reset alone would keep them on the GPU.
inline void release_target(std::unique_ptr<renderer::FrameBuffer>& target) {
    // Nothing made yet.
    if (!target) {
        return;
    }

    target->release();
    target.reset();
}

// The sides of what a camera sees (its view and projection, column-major), as planes a point is
// inside of when a x + b y + c z + d >= 0: left, right, bottom, top. Near and far are left out, so
// that nothing depends on the projection's depth range; they would leave out little.
inline std::array<std::array<float, 4>, 4> frustum_sides(const renderer::Mat4& m) {
    const auto row = [&](int i) {
        return std::array<float, 4>{m[i], m[4 + i], m[8 + i], m[12 + i]};
    };
    const auto x = row(0), y = row(1), w = row(3);
    std::array<std::array<float, 4>, 4> planes{};
    for (int k = 0; k < 4; ++k) {
        planes[0][k] = w[k] + x[k];
        planes[1][k] = w[k] - x[k];
        planes[2][k] = w[k] + y[k];
        planes[3][k] = w[k] - y[k];
    }
    return planes;
}

// Whether a box is wholly outside one of the sides: its corner furthest along the side's inward
// direction is outside it.
template <std::size_t N>
bool outside(
    const std::array<std::array<float, 4>, N>& planes,
    const renderer::Vec3& low,
    const renderer::Vec3& high,
    std::size_t plane_count = N
) {
    for (std::size_t i = 0; i < plane_count; ++i) {
        const auto& p = planes[i];
        const float x = p[0] >= 0.0f ? high[0] : low[0];
        const float y = p[1] >= 0.0f ? high[1] : low[1];
        const float z = p[2] >= 0.0f ? high[2] : low[2];
        if (p[0] * x + p[1] * y + p[2] * z + p[3] < 0.0f) {
            return true;
        }
    }
    return false;
}

}  // namespace openrac::scene::render_detail
