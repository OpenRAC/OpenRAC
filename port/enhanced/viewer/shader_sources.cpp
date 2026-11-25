// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Context-free composition of probe, water, atmosphere and mesh lighting sources.

#include "enhanced/viewer/shader_sources.h"

#include "enhanced_viewer_shaders.h"
#include "renderer/gl/shader.h"
#include "renderer_shaders.h"

namespace openrac::lighting::shader_sources {
const std::string& local_light_source() {
    const static std::string source = [] {
        std::string result = shaders::probes_glsl;
        result.insert(result.find("// Default local lights"), shaders::local_lights_glsl);
        return result;
    }();
    return source;
}

std::string probe_shader_source(std::string source, const Options& options) {
    if (!options.enhanced) {
        return source;
    }

    source.insert(source.find('\n') + 1, local_light_source());
    source.insert(source.find('\n') + 1, shaders::shading_brdf_glsl);
    source.insert(source.find('\n') + 1, renderer::shaders::shading_normal_glsl);

    if (options.vulkan && options.gather) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_SHADOW_GATHER 1\n");
    }

    if (options.filter_reference) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_SHADOW_FILTER_REFERENCE 1\n");
    }

    if (options.zero_weight_reference) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_PROBE_ZERO_WEIGHT_REFERENCE 1\n");
    }

    return source;
}

std::string probe_mesh_fragment(std::string source, const Options& options) {
    if (!options.enhanced) {
        return source;
    }

    source.insert(source.find('\n') + 1, shaders::environment_glsl);
    source += std::string("\n#if !defined(OPENRAC_SOLID)\n") + std::string(shaders::thin_glass_glsl)
              + "\n#endif\n";
    source = probe_shader_source(std::move(source), options);
    // Shared sunlight code reads the cloud attenuation declared by this prelude.
    source.insert(source.find('\n') + 1, shaders::cloud_shadows_glsl);
    return source;
}

std::string probe_early_fragment(std::string source, const Options& options) {
    if (options.enhanced) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_EARLY_LIGHTING 1\n");
        source += shaders::early_lighting_glsl;
    }

    return probe_mesh_fragment(std::move(source), options);
}

std::string sampling_source() {
    return std::string(shaders::atmosphere_transport_glsl) + "\n"
           + std::string(shaders::atmosphere_sample_glsl) + "\n";
}

std::string atmosphere_fragment(std::string source, const Options& options) {
    if (!options.vulkan || !options.enhanced) {
        return source;
    }

    source.insert(source.find('\n') + 1, "#define OPENRAC_ATMOSPHERE 1\n");
    source.insert(source.find("void main()"), sampling_source());
    return source;
}

std::string water_vertex(std::string source, const Options& options) {
    if (!options.vulkan || !options.enhanced) {
        return source;
    }

    source.replace(0, source.find('\n'), "#version 430 core");
    source.insert(
        source.find('\n') + 1,
        "#define OPENRAC_WATER_FLOAT 1\n#define OPENRAC_WATER_FLOAT_FILTERED 1\n"
    );
    std::string helpers = std::string(shaders::water_window_glsl) + "\n"
                          + std::string(shaders::water_swell_glsl) + "\n"
                          + std::string(shaders::water_displacement_glsl) + "\n"
                          + std::string(shaders::water_float_glsl) + "\n";

    if (options.water_local) {
        helpers.insert(0, "#define OPENRAC_WATER_LOCAL 1\n");
        helpers = renderer::shader_constants(std::move(helpers), {{"water_sim_local", 1}});
    }

    source.insert(source.find("void main()"), helpers);
    return source;
}

std::string hero_only_source(const char* source) {
    return std::string("#ifndef OPENRAC_STATIC_COLOUR\n") + source + "\n#endif\n";
}

std::string hero_shader(
    std::string source, const char* fur_source, bool fragment, const Options& options
) {
    if (options.vulkan && options.enhanced) {
        source.replace(0, source.find('\n'), "#version 430 core");
        source.insert(source.find('\n') + 1, "#define OPENRAC_HERO_WETNESS 1\n");
        // Static world variants cannot contain the live, skinned hero: keep their fur path absent.
        source.insert(
            source.find('\n') + 1,
            "#ifndef OPENRAC_STATIC_COLOUR\n#define OPENRAC_HERO_FUR 1\n#endif\n"
        );
        source.insert(source.find('\n') + 1, hero_only_source(fur_source));
        source.insert(source.find('\n') + 1, hero_only_source(shaders::fur_shell_glsl));

        if (fragment) {
            source.insert(source.find('\n') + 1, "#define OPENRAC_FUR_FRAGMENT 1\n");
        }
    }

    return source;
}

std::string hero_fragment(std::string source, const Options& options) {
    if (options.vulkan && options.enhanced) {
        const auto surface = source.find("vec3 probe_surface(");

        // Material response shares existing lighting helpers and never changes coverage or depth.
        if (surface != std::string::npos) {
            source.insert(surface, shaders::foliage_glsl);
        }

        const auto fur_surface = source.find("vec3 probe_surface(");

        if (fur_surface != std::string::npos) {
            source.insert(
                fur_surface,
                hero_only_source(shaders::fur_lighting_glsl)
                    + hero_only_source(shaders::fur_environment_glsl)
            );
        }

        source.insert(source.find('\n') + 1, hero_only_source(shaders::fur_detail_glsl));
    }

    return hero_shader(std::move(source), shaders::fur_glsl, true, options);
}

std::string mesh_vertex(std::string source, const Options& options) {
    return water_vertex(
        hero_shader(std::move(source), shaders::fur_vertex_glsl, false, options), options
    );
}

std::string mesh_fragment(std::string source, const Options& options) {
    return atmosphere_fragment(
        hero_fragment(probe_mesh_fragment(std::move(source), options), options), options
    );
}

std::string motion_fragment(std::string source, const Options& options) {
    return hero_shader(std::move(source), shaders::fur_glsl, true, options);
}

std::string early_fragment(std::string source, const Options& options) {
    return atmosphere_fragment(
        hero_fragment(probe_early_fragment(std::move(source), options), options), options
    );
}
}  // namespace openrac::lighting::shader_sources
