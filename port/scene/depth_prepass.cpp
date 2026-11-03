// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer_shaders.h"
#include "scene/level_lighting.h"
#include "scene/level_renderer.h"
#include "scene_shaders.h"

namespace openrac::scene {
// Also clears any shaders made before a later variant failed to compile.
void LevelScene::release_rigid_depth_programs() {
    for (auto* programs : {&m_rigid_depth, &m_rigid_surface_depth}) {
        for (auto& shader : *programs) {
            shader.release();
        }
    }
}

bool LevelScene::make_rigid_depth_programs(std::string& error) {
    release_rigid_depth_programs();
    const auto source = m_lighting
                            ? m_lighting->rigid_depth_vertex(std::string(shaders::mesh_depth_vert))
                            : std::optional<std::string>{shaders::mesh_depth_vert};
    if (!source) {
        return true;
    }
    for (unsigned normals = 0; normals < 2; ++normals) {
        auto& programs = normals ? m_rigid_surface_depth : m_rigid_depth;
        for (unsigned solid = 0; solid < 2; ++solid) {
            std::string vertex(*source), fragment(shaders::mesh_depth_frag);
            vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_DEPTH_RIGID 1\n");
            if (normals) {
                vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_DEPTH_NORMALS 1\n");
                fragment.insert(fragment.find('\n') + 1, renderer::shaders::shading_normal_glsl);
                fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_DEPTH_NORMALS 1\n");
            }
            if (solid) {
                vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
                fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
            }
            if (!programs[solid].build(
                    std::string(normals ? "rigid surface prepass " : "rigid depth prepass ")
                        + (solid ? "solid" : "alpha"),
                    vertex,
                    fragment,
                    error
                )) {
                release_rigid_depth_programs();
                return false;
            }
        }
    }
    return true;
}
}  // namespace openrac::scene
