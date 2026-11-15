// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "enhanced/lighting/probes.h"
#include "renderer/gl/shader.h"
#include "renderer/renderer.h"

namespace openrac::lighting {
// Compute writes complete per-tile candidate masks. Fragments consume the
// same GPU buffer through a texture-buffer view, without selection readback.
class ProbeSelection {
public:
    static constexpr unsigned capacity = 1024, selected_capacity = 128, tile_size = 16;
    static bool supported();
    bool prepare(
        std::span<const lighting::ProbeGrid> grids,
        std::span<const unsigned> offsets,
        std::string& error
    );
    void dispatch(unsigned depth, const renderer::Camera& camera, int width, int height);
    // Depth-matched receivers use opaque masks; others may use depth-independent tile masks.
    void bind(
        const renderer::Shader& shader,
        const renderer::Camera* camera = nullptr,
        bool depth_matched = true
    ) const;

    bool available() const { return m_cull.id() != 0; }

    bool active() const { return available() && m_started; }

    void release();

private:
    // Records metadata-only selection, shared by the main and secondary cameras.
    void dispatch_shortlist(const renderer::Mat4& clip, const renderer::Vec3& position) const;
    // Records depth-bounded and optional depth-independent masks after the shortlist is visible.
    void dispatch_tiles(unsigned width, unsigned height) const;
#if defined(OPENRAC_VULKAN)
    // Records the shortlist prefix and tile tail as separate dependent graph passes.
    bool dispatch_mask_graph(const renderer::Camera& camera, unsigned width, unsigned height);
#endif
    // Orders the two dispatches through Vulkan graph dependencies or OpenGL compatibility barriers.
    bool dispatch_masks(const renderer::Camera& camera, unsigned width, unsigned height);
    renderer::Shader m_cull, m_select;
    mutable std::array<unsigned, 3> m_buffers{};
    mutable unsigned m_mask_texture = 0, m_secondary_texture = 0;
    mutable renderer::Mat4 m_secondary_clip = renderer::identity();
    mutable std::array<float, 3> m_secondary_position{};
    mutable bool m_secondary_valid = false;
    int m_width = 0, m_height = 0;
    renderer::Mat4 m_clip = renderer::identity();
    void ensure_buffers() const;
    unsigned m_count = 0;
    bool m_started = false;
    bool m_translucent_tiles = false;
};
}  // namespace openrac::lighting
