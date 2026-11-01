// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Authored unshadowed lights and render-thread Forward+ resources.
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include "common/result.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"

namespace openrac::enhanced {
// Shared probe/direct-light source for mesh and water materials, including the light iterator.
const std::string& local_light_shader_source();

// World-space point/spot light; RGB is linear radiance, angles are radians from the spot axis.
struct LocalLight {
    renderer::Vec3 position{}, colour{1, 1, 1}, direction{0, 0, -1};
    float intensity = 1, range = 10;
    float inner_angle = .35f, outer_angle = .6f;
    bool spot = false;
    bool operator==(const LocalLight&) const = default;
};

// Complete masks cover up to 4096 authored lights plus the eight original game slots.
class LocalLights {
public:
    // Copies a complete authoring snapshot; rejects invalid input without changing the old lights.
    Result<void> set_lights(std::span<const LocalLight> lights);
    // Per-instance object attachments are additive to the existing authored snapshot.
    Result<void> set_attached_lights(std::span<const LocalLight> lights);
    // Captures pending edits and original lights once per game frame; called on the render thread.
    void prepare(std::uint64_t frame, const std::array<float, 64>& game, bool enabled);
    // Generates opaque-depth and depth-independent tile masks for this view.
    bool dispatch(
        unsigned depth,
        const renderer::Camera& camera,
        int width,
        int height,
        bool force_tiles = false
    );
    // Binds all-light fallback for secondary views, compute receivers and mismatching viewports.
    void bind(
        const renderer::Shader& shader, const renderer::Camera* camera, bool depth_matched
    ) const;
    // Releases device resources and clears the authoring snapshot at level unload.
    void release();
    // Diagnostic reference switch for correctness/profiling; leaves the light table unchanged.
    std::atomic<bool> reference{false};

private:
    // Four vec4 records: position/range, radiance/backlight, direction/outer cosine, inner cosine/type.
    using Record = std::array<std::array<float, 4>, 4>;
    // Copy pending authored/attached snapshots at a captured frame boundary.
    bool capture_edits(bool new_frame, std::vector<LocalLight>& pending);
    // Packs a validated authoring record into the four-vec4 GPU interface.
    static Record pack_light(const LocalLight& light);
    // Builds the optional culler and sizes masks, preserving the all-light fallback on failure.
    bool prepare_culling(int width, int height);
    // Creates descriptor-safe empty views, even before lights or a depth prepass exist.
    void ensure_buffers() const;
    mutable std::mutex m_mutex;  // Protects authoring edits, never held while issuing GPU commands.
    std::vector<LocalLight> m_pending, m_attached;
    std::vector<Record> m_records;
    std::array<float, 64> m_game{};  // Detects explicit game-bank updates within a captured tick.
    std::uint64_t m_revision = 0, m_uploaded_revision = ~std::uint64_t{0};
    std::uint64_t m_frame = ~std::uint64_t{0};
    mutable std::array<unsigned, 2> m_buffers{}, m_textures{};
    renderer::Shader m_cull;
    renderer::Mat4 m_clip{};
    int m_width = 0, m_height = 0;
    unsigned m_count = 0;
    std::size_t m_mask_words = 0;
    bool m_ready = false, m_enabled = false, m_failed = false;
};
}  // namespace openrac::enhanced
