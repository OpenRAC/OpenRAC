// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enhanced postprocessing and water source assembly shared by renderers and build recipes.

#pragma once
#include <utility>

#include "enhanced/viewer/shader_sources.h"
#include "enhanced_texture_shaders.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/shader.h"
#include "renderer_shaders.h"

namespace openrac::enhanced::recipe_sources {
inline std::pair<std::string, std::string> gtao(
    int scale, bool normals, bool performance, bool hi_z
) {
    std::string evaluate(renderer::shaders::gtao_evaluate_comp);
    evaluate.insert(
        evaluate.find('\n') + 1, std::string("#define OPENRAC_GTAO_HIZ ") + (hi_z ? "1\n" : "0\n")
    );
    std::string composite(renderer::shaders::gtao_composite_frag);
    evaluate.insert(evaluate.find('\n') + 1, renderer::shaders::surface_normal_glsl);
    composite.insert(composite.find('\n') + 1, renderer::shaders::surface_normal_glsl);
    evaluate.insert(evaluate.find('\n') + 1, renderer::shaders::gtao_common_glsl);
    composite.insert(composite.find('\n') + 1, renderer::shaders::gtao_common_glsl);
    const auto definitions = "#define OPENRAC_GTAO_SCALE " + std::to_string(scale)
                             + "\n#define OPENRAC_SURFACE_NORMALS " + (normals ? "1\n" : "0\n");
    evaluate.insert(evaluate.find('\n') + 1, definitions);
    composite.insert(composite.find('\n') + 1, definitions);
    const auto constants =
        {renderer::ShaderIntConstant{"evaluation_scale", scale},
         renderer::ShaderIntConstant{"surface_normals_enabled", normals ? 1 : 0}};
    evaluate = renderer::shader_constants(std::move(evaluate), constants);
    composite = renderer::shader_constants(std::move(composite), constants);

    if (performance) {
        evaluate.insert(evaluate.find('\n') + 1, "#define GTAO_REFINEMENT_STEPS 3\n");
    }

    return {std::move(evaluate), std::move(composite)};
}

inline std::string early_lighting(
    int s, bool fast, bool setup, const lighting::shader_sources::Options& options
) {
    std::string source = lighting::shaders::early_lighting_comp;
    source = lighting::shader_sources::probe_shader_source(std::move(source), options);
    source.insert(source.find('\n') + 1, lighting::shaders::cloud_shadows_glsl);

    if (setup) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_PROBE_RECEIVER_SETUP 1\n");
    }

    if (fast) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_CHEAP_SHADOWS 1\n");
    }

    source.insert(source.find('\n') + 1, renderer::shaders::surface_normal_glsl);
    source.insert(
        source.find('\n') + 1, "#define OPENRAC_PROBE_PIXEL early_pixel\nivec2 early_pixel;\n"
    );
    source.insert(source.find('\n') + 1, "#define OPENRAC_EARLY_SCALE " + std::to_string(s) + "\n");
    return source;
}

inline std::string atmosphere(bool no_volumes) {
    std::string source = "#version 430 core\n#define OPENRAC_PROBE_PIXEL ivec2(-1)\n"
                         + std::string(lighting::shaders::probes_glsl) + "\n"
                         + std::string(lighting::shaders::atmosphere_transport_glsl) + "\n"
                         + std::string(lighting::shaders::atmosphere_lighting_glsl) + "\n"
                         + std::string(lighting::shaders::atmosphere_comp);

    if (no_volumes) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_ATMOSPHERE_NO_VOLUMES 1\n");
        source = renderer::shader_constants(std::move(source), {{"atmosphere_volume_count", 0}});
    }

    return source;
}

inline std::string water_float() {
    std::string source(lighting::shaders::water_float_comp);
    const std::string helpers = std::string(lighting::shaders::water_window_glsl) + "\n"
                                + std::string(lighting::shaders::water_swell_glsl) + "\n"
                                + std::string(lighting::shaders::water_displacement_glsl) + "\n"
                                + std::string(lighting::shaders::water_float_glsl) + "\n";
    source.insert(source.find("void main()"), helpers);
    return source;
}

inline std::string wetness() {
    std::string source(lighting::shaders::wetness_comp);
    source.insert(
        source.find('\n') + 1,
        std::string(lighting::shaders::water_window_glsl) + "\n"
            + std::string(lighting::shaders::water_swell_glsl) + "\n"
    );
    return source;
}

inline std::string water_uv() {
    std::string source(lighting::shaders::water_uv_comp);
    source.insert(source.find('\n') + 1, lighting::shaders::water_uv_glsl);
    return source;
}

inline std::string texture_upscale(unsigned wrap) {
    std::string source(enhanced::shaders::texture_upscale_comp);
    source = renderer::shader_constants(std::move(source), {{"wrap_mode", int(wrap)}});
    return source;
}

inline std::string water_fragment_source(bool ocean, bool spectrum, bool local) {
    std::string source = std::string(lighting::shaders::water_window_glsl) + "\n"
                         + std::string(lighting::shaders::water_swell_glsl) + "\n"
                         + std::string(lighting::shaders::shading_brdf_glsl) + "\n"
                         + lighting::shader_sources::local_light_source() + "\n"
                         + std::string(lighting::shaders::environment_glsl) + "\n"
                         + std::string(lighting::shaders::water_transmission_glsl) + "\n"
                         + std::string(lighting::shaders::water_glsl);

    if (ocean) {
        source.insert(0, "#define OPENRAC_OCEAN 1\n");
    }

    if (spectrum) {
        source.insert(0, "#define OPENRAC_OCEAN_SPECTRUM 1\n#define OPENRAC_OCEAN_FINE 1\n");
    }

    if (local) {
        source.insert(0, "#define OPENRAC_WATER_LOCAL 1\n");
        return renderer::shader_constants(std::move(source), {{"water_sim_local", 1}});
    }

    return source;
}

inline std::string water_vertex_source(bool ocean, bool spectrum, bool local) {
    std::string source = std::string(lighting::shaders::water_uv_glsl) + "\n"
                         + std::string(lighting::shaders::water_mesh_glsl) + "\n"
                         + std::string(lighting::shaders::water_window_glsl) + "\n"
                         + std::string(lighting::shaders::water_swell_glsl) + "\n"
                         + std::string(lighting::shaders::water_displacement_glsl) + "\n"
                         + std::string(lighting::shaders::water_vertex_glsl);

    if (ocean) {
        source.insert(0, "#define OPENRAC_OCEAN 1\n");
    }

    if (spectrum) {
        source.insert(0, "#define OPENRAC_OCEAN_SPECTRUM 1\n");
    }

    if (local) {
        source.insert(0, "#define OPENRAC_WATER_LOCAL 1\n");
        return renderer::shader_constants(std::move(source), {{"water_sim_local", 1}});
    }

    return source;
}
}  // namespace openrac::enhanced::recipe_sources
