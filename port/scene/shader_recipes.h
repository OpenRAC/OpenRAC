// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Scene shader assembly shared by runtime upload and the CPU-only variant compiler.
#pragma once
#include <cstring>
#include <string>

#include "renderer/gl/shader.h"
#include "renderer_shaders.h"
#include "scene_shaders.h"

namespace openrac::scene {
// Owns the scene/provider assemblies used by every corresponding program slot.
struct SceneShaderInputs {
    std::string vertex;          // Mesh positions and surface interfaces.
    std::string fragment;        // Mesh colour with the chosen lighting provider.
    std::string early_fragment;  // Optional cached early-lighting colour path.
    std::string depth_vertex;    // Matching deformation for the depth prepass.
    bool lighting = false;       // Probe lighting declarations are available.
    bool early = false;          // Early-lighting source and outputs are available.
    bool enhanced = false;       // Matching surface-normal output is required.
    bool materials = false;      // Native batch material paths are supported.
};

namespace shader_recipe_detail {

// Emits the base, lighting diagnostics and early-lighting slots.
template <class Emit>
bool base_recipes(
    const SceneShaderInputs& input, const std::string& solid, Emit& emit, std::string& error
) {
    const auto& mesh_vertex = input.vertex;
    const auto& fragment = input.fragment;

    if (!emit(0, "mesh", mesh_vertex, fragment, error)
        || !emit(1, "mesh (solid)", mesh_vertex, solid, error)
        || !emit(2, "sky", shaders::sky_vert, shaders::sky_frag, error)) {
        return false;
    }

    if (input.lighting) {
        std::string coverage(fragment), coverage_solid(solid);
        coverage.insert(coverage.find('\n') + 1, "#define OPENRAC_PROBE_COVERAGE 1\n");
        coverage_solid.insert(coverage_solid.find('\n') + 1, "#define OPENRAC_PROBE_COVERAGE 1\n");
        std::string fast(fragment);
        fast.insert(fast.find('\n') + 1, "#define OPENRAC_PROBE_FAST 1\n");

        if (!emit(3, "mesh (cheaper probe light)", mesh_vertex, fast, error)
            || !emit(4, "mesh (probe coverage)", mesh_vertex, coverage, error)
            || !emit(5, "mesh (solid, probe coverage)", mesh_vertex, coverage_solid, error)) {
            return false;
        }
    }

    if (input.early) {
        std::string early = input.early_fragment, early_solid(early);
        early_solid.insert(early_solid.find('\n') + 1, "#define OPENRAC_SOLID 1\n");

        if (!emit(6, "early lit mesh", mesh_vertex, early, error)
            || !emit(7, "early lit solid", mesh_vertex, early_solid, error)) {
            return false;
        }
    }

    return true;
}

// Emits native opaque batches only when the renderer supports them.
template <class Emit>
bool batch_recipes(const SceneShaderInputs& input, Emit& emit, std::string& error) {
    const auto& mesh_vertex = input.vertex;
    const auto& fragment = input.fragment;

    if (input.materials) {
        // The batch's opaque, non-solid geometry never draws an added effect.
        const auto opaque_source = [](std::string source) {
            const auto effect = source.find("uniform int probe_effect;");

            if (effect != std::string::npos) {
                source.replace(
                    effect, std::strlen("uniform int probe_effect;"), "const int probe_effect = 0;"
                );
            }

            return source;
        };

        if (!emit(8, "opaque colour batch", mesh_vertex, opaque_source(fragment), error)
            || (input.early
                && !emit(
                    9,
                    "early opaque colour batch",
                    mesh_vertex,
                    opaque_source(input.early_fragment),
                    error
                ))) {
            return false;
        }
    }

    return true;
}

// Makes both stages agree on static/solid feature interfaces and constant pass controls.
inline std::string colour_fragment(std::string source, unsigned variant) {
    source.insert(source.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");

    // Both stages must agree when static variants remove material-specific interfaces.
    if (variant < 2) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_STATIC_COLOUR 1\n");
    }

    if (variant == 0) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_SOLID 1\n");
    }

    for (const auto* name :
         {"metal_pass", "glow_pass", "translucent_pass", "effect_pass", "probe_effect"}) {
        source = renderer::shader_constants(std::move(source), {{name, 0}});
    }

    return source;
}

// Emits specialized static and animated colour programs, including early-lighting versions.
template <class Emit>
bool colour_recipes(const SceneShaderInputs& input, Emit& emit, std::string& error) {
    const auto& mesh_vertex = input.vertex;

    // The same small pass shaders also serve OpenGL and direct/CPU draws. No
    // material-buffer or batching capability is required to fix these controls.
    // Keep opaque features constant per native program. In particular,
    // static geometry never needs the joint-palette path, and solid
    // geometry must not inherit the alpha shader's discard instructions.
    for (unsigned variant = 0; variant < 3u; ++variant) {
        std::string vertex(mesh_vertex);
        vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
        vertex = renderer::shader_constants(vertex, {{"metal_pass", 0}});

        if (variant < 2) {
            vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_STATIC_COLOUR 1\n");
            vertex = renderer::shader_constants(vertex, {{"skinning", 0}});
        }

        const char* names[] = {"solid static colour", "alpha static colour", "opaque moby colour"};

        if (!emit(
                10 + variant,
                names[variant],
                vertex,
                colour_fragment(input.fragment, variant),
                error
            )
            || (input.early
                && !emit(
                    13 + variant,
                    std::string("early ") + names[variant],
                    vertex,
                    colour_fragment(input.early_fragment, variant),
                    error
                ))) {
            return false;
        }
    }

    return true;
}

// Adds the early-lighting controls for static translucent geometry.
inline std::string early_translucent_fragment(std::string early) {
    for (const char* define :
         {"#define OPENRAC_EARLY_TRANSLUCENT 1\n",
          "#define OPENRAC_COLOUR_SPECIALISED 1\n",
          "#define OPENRAC_STATIC_COLOUR 1\n"}) {
        early.insert(early.find('\n') + 1, define);
    }

    for (const auto* name : {"metal_pass", "glow_pass", "effect_pass", "probe_effect"}) {
        early = renderer::shader_constants(std::move(early), {{name, 0}});
    }

    early = renderer::shader_constants(std::move(early), {{"translucent_pass", 1}});

    return early;
}

// Emits the inexpensive and reference translucent paths, then optional early lighting.
template <class Emit>
bool translucent_recipes(const SceneShaderInputs& input, Emit& emit, std::string& error) {
    const auto& mesh_vertex = input.vertex;
    const auto& fragment = input.fragment;
    std::string static_vertex(mesh_vertex), static_translucent(fragment);
    static_vertex.insert(static_vertex.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
    static_translucent
        .insert(static_translucent.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
    static_vertex.insert(static_vertex.find('\n') + 1, "#define OPENRAC_STATIC_COLOUR 1\n");
    static_translucent
        .insert(static_translucent.find('\n') + 1, "#define OPENRAC_STATIC_COLOUR 1\n");
    static_vertex = renderer::shader_constants(std::move(static_vertex), {{"skinning", 0}});
    static_vertex = renderer::shader_constants(std::move(static_vertex), {{"metal_pass", 0}});

    for (const auto* name : {"metal_pass", "glow_pass", "effect_pass", "probe_effect"}) {
        static_translucent = renderer::shader_constants(std::move(static_translucent), {{name, 0}});
    }

    static_translucent =
        renderer::shader_constants(std::move(static_translucent), {{"translucent_pass", 1}});
    // The default blended static pass reads cheaper probe light and shadows (Enhanced is
    // allowed to be lossy, probe-fast-blended); the full one stays as the reference.
    std::string static_fast(static_translucent);
    static_fast.insert(static_fast.find('\n') + 1, "#define OPENRAC_PROBE_FAST 1\n");

    if (!emit(16, "static translucent colour", static_vertex, static_fast, error)
        || !emit(
            17,
            "static translucent colour, full probe light",
            static_vertex,
            static_translucent,
            error
        )) {
        return false;
    }

    if (input.early) {
        const auto early = early_translucent_fragment(input.early_fragment);

        if (!emit(18, "static translucent colour, early lighting", static_vertex, early, error)) {
            return false;
        }
    }

    return true;
}

// Emits minimal depth programs and optional matching normal outputs.
template <class Emit>
bool depth_recipes(const SceneShaderInputs& input, Emit& emit, std::string& error) {
    const std::string depth_source = input.depth_vertex;
    std::string depth_vertex(depth_source), depth_fragment(shaders::mesh_depth_frag);
    depth_vertex.insert(depth_vertex.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
    depth_fragment.insert(depth_fragment.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");

    if (!emit(19, "mesh depth", depth_source, shaders::mesh_depth_frag, error)
        || !emit(20, "mesh depth solid", depth_vertex, depth_fragment, error)) {
        return false;
    }

    // Compatibility rendering retains the minimal depth-only shaders.
    if (input.enhanced) {
        const std::string define = "#define OPENRAC_DEPTH_NORMALS 1\n";
        std::string vertex(depth_source);
        std::string normal_fragment(shaders::mesh_depth_frag);
        normal_fragment
            .insert(normal_fragment.find('\n') + 1, renderer::shaders::shading_normal_glsl);
        depth_fragment
            .insert(depth_fragment.find('\n') + 1, renderer::shaders::shading_normal_glsl);
        vertex.insert(vertex.find('\n') + 1, define);
        normal_fragment.insert(normal_fragment.find('\n') + 1, define);
        depth_vertex.insert(depth_vertex.find('\n') + 1, define);
        depth_fragment.insert(depth_fragment.find('\n') + 1, define);

        if (!emit(21, "mesh surface depth", vertex, normal_fragment, error)
            || !emit(22, "mesh solid surface depth", depth_vertex, depth_fragment, error)) {
            return false;
        }
    }

    return true;
}
}  // namespace shader_recipe_detail

// Emits stable destination slots for LevelScene and build recipes; stops at the first failure.
template <class Emit>
bool scene_shader_recipes(const SceneShaderInputs& input, Emit emit, std::string& error) {
    // mesh.frag again with OPENRAC_SOLID, after its version line (which has to stay the first).
    // The registered lighting puts its GLSL into the mesh shader; without it the scene's own.
    const std::string fragment = input.fragment;
    std::string solid(fragment);
    solid.insert(solid.find('\n') + 1, "#define OPENRAC_SOLID 1\n");

    return shader_recipe_detail::base_recipes(input, solid, emit, error)
           && shader_recipe_detail::batch_recipes(input, emit, error)
           && shader_recipe_detail::colour_recipes(input, emit, error)
           && shader_recipe_detail::translucent_recipes(input, emit, error)
           && shader_recipe_detail::depth_recipes(input, emit, error);
}
}  // namespace openrac::scene
