// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Build-time scene, object-preview and shadow recipe enumeration using the shared scene assembly.
#include <stdexcept>

#include "enhanced/viewer/object_preview_shader_source.h"
#include "enhanced/viewer/shader_sources.h"
#include "renderer/gl/shader_recipe.h"
#include "scene/shader_recipes.h"

namespace openrac::renderer {
namespace {
// Assembles matching colour, early-lighting and depth sources for one capability choice.
scene::SceneShaderInputs mesh_inputs(
    const lighting::shader_sources::Options& options, bool lighting
) {
    const bool enhanced = options.enhanced;
    scene::SceneShaderInputs input;
    input.vertex = lighting
                       ? lighting::shader_sources::mesh_vertex(scene::shaders::mesh_vert, options)
                       : scene::shaders::mesh_vert;
    input.fragment =
        lighting ? lighting::shader_sources::mesh_fragment(scene::shaders::mesh_frag, options)
                 : scene::shaders::mesh_frag;
    input.depth_vertex =
        lighting ? lighting::shader_sources::mesh_vertex(scene::shaders::mesh_depth_vert, options)
                 : scene::shaders::mesh_depth_vert;
    input.lighting = lighting;
    input.early = enhanced && lighting;
    input.enhanced = enhanced;
    // Include batch shaders even when the player's device uses individual materials.
    input.materials = true;

    if (input.early) {
        input.early_fragment =
            lighting::shader_sources::early_fragment(scene::shaders::mesh_frag, options);
    }

    return input;
}

// Emits the viewer's neutral and probe-lit material sources.
void preview_recipes(ShaderRecipes& recipes) {
    for (const auto& code :
         {std::string(
              "vec3 preview_illumination(vec3 a,vec3 n,vec3 p,vec3 v,float d) {return "
              "preview_neutral(a,n);}"
          ),
          enhanced::object_preview_lighting_source()}) {
        std::string fragment(scene::shaders::object_preview_frag);
        fragment.insert(fragment.find("// PREVIEW_LIGHTING"), code);
        recipes.graphics("object viewer", scene::shaders::object_preview_vert, fragment);
    }
}

// Emits scene slots before shadow permutations for the same lighting configuration.
void mesh_recipes(ShaderRecipes& recipes, const scene::SceneShaderInputs& input) {
    std::string error;

    if (!scene::scene_shader_recipes(
            input,
            [&](unsigned,
                std::string_view name,
                const std::string& vs,
                const std::string& fs,
                std::string&) {
                recipes.graphics(name, vs, fs);
                return true;
            },
            error
        )) {
        throw std::runtime_error(error);
    }
}

// Emits each solid/alpha and deformation combination for one shadow target layout.
void shadow_variants(
    ShaderRecipes& recipes,
    const lighting::shader_sources::Options& options,
    bool lighting,
    bool atlas
) {
    for (bool solid : {false, true}) {
        for (bool deform : {false, true}) {
            std::string vs = deform && lighting ? lighting::shader_sources::mesh_vertex(
                                                      scene::shaders::shadow_depth_vert, options
                                                  )
                                                : scene::shaders::shadow_depth_vert;
            std::string fs = scene::shaders::hero_shadow_frag;

            if (atlas) {
                vs.insert(
                    vs.find('\n') + 1,
                    "#extension GL_ARB_shader_viewport_layer_array : require\n#define "
                    "OPENRAC_SHADOW_ATLAS 1\n"
                );
            }

            if (solid) {
                vs.insert(vs.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
                fs.insert(fs.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
            }

            recipes.graphics("dry shadow", vs, fs);
        }
    }
}

// Emits solid, atlas and deformed shadow depth combinations.
void shadow_recipes(
    ShaderRecipes& recipes, const lighting::shader_sources::Options& options, bool lighting
) {
    recipes.graphics(
        "Ratchet shadow depth",
        lighting ? lighting::shader_sources::mesh_vertex(scene::shaders::shadow_depth_vert, options)
                 : scene::shaders::shadow_depth_vert,
        scene::shaders::hero_shadow_frag
    );

    for (bool atlas : {false, true}) {
        shadow_variants(recipes, options, lighting, atlas);
    }
}

// Emits static depth programs with each normal and alpha-coverage choice.
void rigid_recipes(ShaderRecipes& recipes) {
    for (bool normals : {false, true}) {
        for (bool solid : {false, true}) {
            std::string vs = scene::shaders::mesh_depth_vert, fs = scene::shaders::mesh_depth_frag;
            vs.insert(vs.find('\n') + 1, "#define OPENRAC_DEPTH_RIGID 1\n");

            if (normals) {
                vs.insert(vs.find('\n') + 1, "#define OPENRAC_DEPTH_NORMALS 1\n");
                fs.insert(fs.find('\n') + 1, shaders::shading_normal_glsl);
                fs.insert(fs.find('\n') + 1, "#define OPENRAC_DEPTH_NORMALS 1\n");
            }

            if (solid) {
                vs.insert(vs.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
                fs.insert(fs.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
            }

            recipes.graphics("rigid prepass", vs, fs);
        }
    }
}
}  // namespace

void scene_shader_recipes(ShaderRecipes& recipes, bool enhanced) {
    preview_recipes(recipes);

    for (bool lighting : {false, true}) {
        for (bool water_local : {true, false}) {
            if ((!lighting && !water_local) || (enhanced && !lighting)) {
                continue;
            }

            lighting::shader_sources::Options options;
            options.enhanced = enhanced;
            options.water_local = water_local;
            mesh_recipes(recipes, mesh_inputs(options, lighting));
            shadow_recipes(recipes, options, lighting);
            rigid_recipes(recipes);
        }
    }
}
}  // namespace openrac::renderer
