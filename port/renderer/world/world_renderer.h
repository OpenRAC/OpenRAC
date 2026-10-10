// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The base of every world renderer: a BucketRenderer that shares a
// WorldContext with the others, and the uniforms each draw's GS state needs.
//
// A world renderer's life: init() (shaders; the context's GL objects),
// load() for each level (uploads the level's draw data; the GL context must
// be current), render() every frame, unload() between levels, release() at
// the end.

#pragma once

#include <memory>
#include <string>

#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "renderer/world/gs_pass.h"
#include "renderer/world/world_context.h"

namespace openrac::renderer::world {

class WorldRenderer : public BucketRenderer {
public:
    WorldRenderer(std::string name, Bucket bucket, std::shared_ptr<WorldContext> context)
        : BucketRenderer(std::move(name), bucket), m_context(std::move(context)) {}

    WorldContext& context() { return *m_context; }

    const std::shared_ptr<WorldContext>& shared_context() const { return m_context; }

protected:
    // The GS state of `draw` into GL and the shader's alpha-test and output
    // uniforms (display_blend.glsl).
    static void set_gs_draw(const Shader& shader, const GsDraw& draw);

    static void count_draw(RenderState& state, std::size_t indices);

private:
    std::shared_ptr<WorldContext> m_context;
};

}  // namespace openrac::renderer::world
