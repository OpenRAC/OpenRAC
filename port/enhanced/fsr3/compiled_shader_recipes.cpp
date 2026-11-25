// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared enumeration of temporal, blur and native FidelityFX shader variants.
#include "renderer/gl/shader_recipe.h"
#ifdef OPENRAC_SHADER_FSR3
#include <map>

#include "enhanced/fsr3/shader_recipes.h"
#include "enhanced/viewer/shader_sources.h"
#include "fsr3_shaders.h"
#include "fsr3_sources.h"
#include "renderer/gl/shader.h"
#endif

namespace openrac::renderer {
namespace {
#ifdef OPENRAC_SHADER_FSR3
namespace shaders = enhanced::shaders;

// Enumerates object motion with each supported lighting source.
void motion_recipes(ShaderRecipes& recipes) {
    for (bool lighting : {false, true}) {
        for (bool local : {true, false}) {
            lighting::shader_sources::Options options;
            options.water_local = local;
            recipes.graphics(
                "temporal object motion",
                lighting ? lighting::shader_sources::mesh_vertex(shaders::motion_vert, options)
                         : shaders::motion_vert,
                lighting ? lighting::shader_sources::motion_fragment(shaders::motion_frag, options)
                         : shaders::motion_frag
            );
        }
    }
}

// Enumerates jump-flood and velocity blur specializations.
void blur_recipes(ShaderRecipes& recipes) {
    const auto define = [](std::string source, std::string_view text) {
        source.insert(source.find('\n') + 1, text);
        return source;
    };

    for (auto definitions : {"", "#define FAST_FLOOD 1\n", "#define INITIALIZE_SEEDS 1\n"}) {
        const auto source = *definitions ? define(shaders::blur_jump_frag, definitions)
                                         : std::string(shaders::blur_jump_frag);
        recipes.graphics("blur jump flood", shaders::fullscreen_vert, source);
    }

    const auto fast = define(shaders::blur_jump_frag, "#define FAST_FLOOD 1\n");
    recipes.graphics(
        "blur fast initial flood",
        shaders::fullscreen_vert,
        define(fast, "#define INITIALIZE_SEEDS 1\n")
    );
    recipes.graphics(
        "blur unique flood", shaders::fullscreen_vert, define(fast, "#define UNIQUE_SEEDS 1\n")
    );
    recipes.graphics("relative blur resolve", shaders::fullscreen_vert, shaders::blur_resolve_frag);
    recipes.graphics(
        "blur and copy resolve",
        shaders::fullscreen_vert,
        define(shaders::blur_resolve_frag, "#define COPY_UNCHANGED 1\n")
    );

    for (int mode : {0, 1}) {
        recipes.graphics(
            "blur velocity",
            shaders::fullscreen_vert,
            define(
                shader_constants(shaders::blur_velocity_frag, {{"camera_relative", mode}}),
                mode ? "#define OPENRAC_BLUR_CAMERA 1\n" : "#define OPENRAC_BLUR_GAMEPLAY 1\n"
            )
        );
    }
}

// Enumerates the pinned SDK passes and their supported FP16 permutations.
void sdk_recipes(ShaderRecipes& recipes) {
    using namespace enhanced::fsr3_detail;
    std::map<std::string, std::string> sources;

    for (const auto& entry : kSdkSources) {
        for (auto chunk : entry.chunks) {
            sources[std::string(entry.name)] += chunk;
        }
    }

    for (unsigned pass = 0; pass < kPassSources.size(); ++pass) {
        const auto name = std::string("ffx_fsr3upscaler_") + kPassSources[pass] + "_pass.glsl";

        for (unsigned permutation = 0; permutation <= kShaderSwitches; ++permutation) {
            recipes.native(
                name + " / " + std::to_string(permutation),
                sources.at(name),
                shader_definitions(permutation)
            );

            if (half_history_pass(pass)) {
                recipes.native(
                    name + " / half / " + std::to_string(permutation),
                    sources.at(name),
                    shader_definitions(permutation | kHalf)
                );
            }
        }
    }
}

#endif
}  // namespace

void fsr_shader_recipes(ShaderRecipes& recipes) {
#ifdef OPENRAC_SHADER_FSR3
    recipes
        .graphics("temporal camera motion", shaders::fullscreen_vert, shaders::camera_motion_frag);
    recipes.graphics("temporal reactive mask", shaders::fullscreen_vert, shaders::reactive_frag);
    recipes.graphics("late world depth", shaders::fullscreen_vert, shaders::late_world_depth_frag);

    motion_recipes(recipes);
    blur_recipes(recipes);
    sdk_recipes(recipes);
#else
    (void)recipes;
#endif
}
}  // namespace openrac::renderer
