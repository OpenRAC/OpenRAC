// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>

#include "enhanced/renderer/gtao_depth.h"
#include "renderer/ambient_occlusion.h"
#include "renderer/framebuffer.h"
#include "renderer/shader.h"

namespace openrac::renderer {
struct FrameInput;
struct RenderTarget;

// Optional, spatial-only ground-truth ambient occlusion. The target supplies smooth normals
// from the world prepass; depth is used for positions and horizons, never to derive normals.
// The horizon estimate is filtered at full size. Modern
// lighting consumes visibility_texture() after render(..., false); the default
// colour composite is retained only for legacy prelit paths. evaluate_only
// exposes the raw horizon/guide texture for fused early-lighting denoising.
//
// The renderer's ambient occlusion hook (renderer/ambient_occlusion.h): a program registers one
// with Renderer::set_ambient_occlusion, and the tests also run it alone.
class Gtao final : public AmbientOcclusion {
public:
    // The single-sample horizon path can freeze bindings before its depth producer records.
    bool prepares_surface_chain() const override { return true; }

    // Declares depth/normal attachment writes and their copy, resolve or sampling consumers on Vulkan.
    void render_prepass(unsigned framebuffer, const std::function<void()>& record) override;
    bool render(
        const FrameInput& input,
        const RenderTarget& target,
        bool apply_to_colour = true,
        bool evaluate_only = false
    ) override;

    // Packed RGBA8 texture-buffer view, one AO/guide record per evaluation cell.
    unsigned raw_texture() const override { return m_ao_texture; }

    unsigned visibility_texture() const override { return m_visibility.colour_texture(); }

    // Depth sampled by the last successful render; may be borrowed from its target.
    unsigned depth_texture() const override { return m_sampled_depth; }

    void release() override;

private:
    // Four scale/normal specialisations and one uniform-controlled reference.
    static constexpr unsigned kProgramVariants = 5;
    // Records the depth hierarchy, horizon evaluation and optional filter with either synchronisation path.
    bool execute(const FrameInput&, const RenderTarget&, bool apply_to_colour, bool evaluate_only);
    // Declares Vulkan consumers after the copied or resolved depth producer.
    bool execute_graph(
        const FrameInput&, const RenderTarget&, bool apply_to_colour, bool evaluate_only, bool hi_z
    );
    // Records horizon shader bindings and its original dispatch without inserting barriers.
    void evaluate(const FrameInput&, const RenderTarget&, bool hi_z);
    // Records the full-resolution bilateral filter and optional colour blending.
    void filter(const FrameInput&, const RenderTarget&, bool apply_to_colour);
    // Allocates size-dependent targets, keeping caller-owned depth textures borrowed.
    bool prepare(int width, int height, int scale, bool visibility, bool copy_depth);
    // Compiles the requested quality kernel and its shared composite on first use.
    bool prepare_program(int scale, bool normals, bool performance, bool hi_z);
    FrameBuffer m_depth;
    GtaoDepth m_hi_z;              // Optional, bounded hierarchy; unused in the no-Hi-Z path.
    unsigned m_sampled_depth = 0;  // Borrowed target depth or our owned copy for this render.
    unsigned m_ao_buffer = 0, m_ao_texture = 0;
    int m_ao_width = 0, m_ao_height = 0;
    FrameBuffer m_visibility;
    // Four bounded scale/normal variants plus the uniform-controlled reference.
    // Each quality kernel is built only on its own first use, sharing the composite.
    // Each quality has separate Hi-Z/no-Hi-Z variants; both share the full-resolution filter.
    std::array<Shader, kProgramVariants * 2> m_evaluate, m_evaluate_performance;
    std::array<Shader, kProgramVariants> m_composite;
    unsigned m_program = 4;
    unsigned m_vao = 0;
    bool m_failed = false;
};
}  // namespace openrac::renderer
