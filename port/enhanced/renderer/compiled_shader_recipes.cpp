// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared enumeration of enhanced postprocessing, lighting, water, probes and effect materials.
#include "enhanced/renderer/shader_recipe_sources.h"
#include "renderer/effects/shader_sources.h"
#include "renderer/gl/shader_recipe.h"

namespace openrac::renderer {
namespace {
namespace ls = lighting::shaders;
namespace sources = enhanced::recipe_sources;

// Enumerates normal, refinement and Hi-Z choices for one GTAO evaluation scale.
void gtao_recipes(ShaderRecipes& recipes, int scale) {
    for (bool normals : {false, true}) {
        for (bool performance : {false, true}) {
            for (bool hi_z : {false, true}) {
                const auto [evaluate, composite] = sources::gtao(scale, normals, performance, hi_z);
                recipes.compute("GTAO horizons", evaluate);
                recipes.graphics("GTAO filter", shaders::gtao_vert, composite);
            }
        }
    }
}

// Enumerates colour transforms, bloom and GTAO quality combinations.
void postprocess_recipes(ShaderRecipes& recipes) {
    recipes.graphics("AgX", shaders::gtao_vert, shaders::agx_frag);
    recipes.graphics("bloom downsample", shaders::gtao_vert, shaders::bloom_downsample_frag);
    recipes.graphics("bloom upsample", shaders::gtao_vert, shaders::bloom_upsample_frag);
    recipes.graphics("bloom composite", shaders::gtao_vert, shaders::bloom_composite_frag);
    recipes.compute("GTAO depth bounds", shaders::gtao_depth_comp);

    for (int scale : {1, 2}) {
        gtao_recipes(recipes, scale);
    }
}

// Enumerates early GI, atmosphere and local-light preparation.
void lighting_recipes(ShaderRecipes& recipes) {
    lighting::shader_sources::Options options;

    for (int scale : {2, 4}) {
        for (bool fast : {false, true}) {
            for (bool setup : {false, true}) {
                recipes.compute(
                    "early GI and cascades", sources::early_lighting(scale, fast, setup, options)
                );
            }
        }
    }

    for (bool no_volumes : {false, true}) {
        recipes.compute("atmosphere transport", sources::atmosphere(no_volumes));
    }

    recipes.graphics(
        "local fog over authored sky",
        shaders::gtao_vert,
        lighting::shader_sources::atmosphere_fragment(ls::atmosphere_sky_frag, options)
    );
    recipes.compute("Forward+ light culling", ls::local_lights_comp);
}

// Enumerates water, ocean, wetness and texture preparation.
void water_recipes(ShaderRecipes& recipes) {
    recipes.compute("water window metadata", ls::water_window_comp);
    recipes.compute("water surface waves", ls::water_comp);
    recipes.compute(
        "water lighting cache",
        std::string("#version 430 core\n#define OPENRAC_PROBE_PIXEL ivec2(-1)\n") + ls::probes_glsl
            + ls::water_lighting_comp
    );
    recipes.compute("ocean inverse FFT", ls::ocean_fft_comp);
    recipes.compute("water attachment momentum", sources::water_float());
    recipes.compute("Ratchet surface wetness", sources::wetness());
    recipes.compute("water authored UV animation", sources::water_uv());

    for (unsigned wrap = 0; wrap < 4; ++wrap) {
        recipes.compute("texture upscale", sources::texture_upscale(wrap));
    }
}

// Enumerates probe placement, reflection and shortlist kernels.
void probe_recipes(ShaderRecipes& recipes) {
    recipes.compute("probe sphere compaction", ls::probe_spheres_comp);
    recipes.graphics("probe placement spheres", ls::probe_spheres_vert, ls::probe_spheres_frag);
    recipes.compute("tiled environment-probe culling", ls::environment_selection_comp);
    recipes.graphics(
        "environment capture spheres",
        ls::environment_preview_vert,
        std::string("#version 410 core\n") + ls::environment_glsl + ls::environment_preview_frag
    );
    recipes.compute("tiled probe-volume culling", ls::probe_selection_comp);

    for (auto definition :
         {"#define OPENRAC_PROBE_SHORTLIST 1\n", "#define OPENRAC_PROBE_TILES 1\n"}) {
        std::string source(ls::probe_selection_comp);
        source.insert(source.find('\n') + 1, definition);
        recipes.compute("probe selection", source);
    }
}

// Adds tested, fixed and untextured forms of one assembled effect material.
void emit_effect(ShaderRecipes& recipes, const std::string& vertex, const std::string& material) {
    for (bool fixed : {false, true}) {
        recipes.graphics(
            "effect material",
            effect_sources::vertex(vertex),
            effect_sources::fragment(effect_sources::base_fragment(fixed), material)
        );
    }

    recipes.graphics(
        "effect material tested",
        effect_sources::vertex(vertex),
        effect_sources::fragment(effect_sources::base_fragment(false), material)
    );
    recipes.graphics(
        "effect material untextured",
        effect_sources::vertex(vertex),
        effect_sources::fragment(effect_sources::untextured(), material)
    );
}

// Combines ocean, spectral-wave and local-water choices for one atmosphere source.
void water_effect_recipes(ShaderRecipes& recipes, const std::string& medium) {
    for (bool ocean : {false, true}) {
        for (bool spectrum : {false, true}) {
            // A spectrum is available only for the ocean optics path.
            if (spectrum && !ocean) {
                continue;
            }

            for (bool local : {false, true}) {
                emit_effect(
                    recipes,
                    sources::water_vertex_source(ocean, spectrum, local),
                    sources::water_fragment_source(ocean, spectrum, local) + medium
                );
            }
        }
    }
}

// Combines water optics with both atmosphere paths for effect materials.
void effect_recipes(ShaderRecipes& recipes) {
    const std::string plain = "vec3 effect_material(vec3 rgb){return rgb;}\n";

    for (bool atmosphere : {false, true}) {
        const auto medium = atmosphere ? "#define OPENRAC_EFFECT_ATMOSPHERE 1\n"
                                             + lighting::shader_sources::sampling_source()
                                       : std::string{};
        emit_effect(recipes, {}, plain + medium);

        water_effect_recipes(recipes, medium);
    }
}
}  // namespace

void enhanced_shader_recipes(ShaderRecipes& recipes) {
    postprocess_recipes(recipes);
    lighting_recipes(recipes);
    water_recipes(recipes);
    probe_recipes(recipes);
    effect_recipes(recipes);
}
}  // namespace openrac::renderer
