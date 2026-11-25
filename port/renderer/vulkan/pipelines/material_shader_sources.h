// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Transforms GL material uniforms into the native draw/material buffer interfaces.

#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "renderer/vulkan/pipelines/shader_specialization.h"

namespace openrac::vkgl::shader_transforms {
// Material samplers share one indexed texture array; slots match kMaterialTextureWords.
inline constexpr std::array<std::string_view, 3> kMaterialSamplers =
    {"tex_T0", "emission_texture", "upscaled_texture_colours"};
// Texture indices occupy resources.xy and texture_indices.x in the GLSL material record.
inline constexpr std::array<unsigned, 3> kMaterialTextureWords = {12, 13, 20};

// Whether `c` is whitespace as std::regex's \s reads it.
inline bool glsl_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// GLSL declaration keyword; matching keeps the existing source bytes outside removed declarations.
inline constexpr std::string_view kUniformKeyword = "uniform";

// Advances through GLSL whitespace and optionally requires at least one character.
inline bool skip_uniform_spaces(const std::string& source, std::size_t& i, bool required) {
    const std::size_t from = i;

    while (i < source.size() && glsl_space(source[i])) {
        ++i;
    }

    return !required || i > from;
}

// Finds the end of one matching scalar declaration, without allocating a regex for each material.
inline std::size_t uniform_declaration_end(
    const std::string& source,
    std::size_t at,
    std::span<const std::string_view> types,
    std::string_view name
) {
    std::size_t i = at + kUniformKeyword.size();

    if (!skip_uniform_spaces(source, i, true)) {
        return std::string::npos;
    }

    const auto type = std::find_if(types.begin(), types.end(), [&](std::string_view t) {
        return source.compare(i, t.size(), t) == 0 && i + t.size() < source.size()
               && glsl_space(source[i + t.size()]);
    });

    if (type == types.end()) {
        return std::string::npos;
    }

    i += type->size();
    skip_uniform_spaces(source, i, true);

    if (source.compare(i, name.size(), name) != 0) {
        return std::string::npos;
    }

    i += name.size();
    skip_uniform_spaces(source, i, false);
    return i < source.size() && source[i] == ';' ? i + 1 : std::string::npos;
}

// Removes every declaration `uniform <type> <name>;` of `name` from `source`, with one of `types`
// and any whitespace between (at least one space after `uniform` and the type, any before the
// semicolon): what regex_replace with uniform\s+(type|...)\s+name\s*; made of it, the same text,
// so the SPIR-V cache keys stay. The regex took most of a level's load with the enhanced mesh
// shaders, whose sources are large (about 50 ms a program).
inline std::string remove_uniform(
    const std::string& source, std::span<const std::string_view> types, std::string_view name
) {
    std::string out;
    out.reserve(source.size());
    std::size_t copied = 0;
    std::size_t at = source.find(kUniformKeyword);

    while (at != std::string::npos) {
        const auto end = uniform_declaration_end(source, at, types, name);

        if (end == std::string::npos) {
            at = source.find(kUniformKeyword, at + 1);
            continue;
        }

        out.append(source, copied, at - copied);
        copied = end;
        at = source.find(kUniformKeyword, end);
    }

    out.append(source, copied, std::string::npos);
    return out;
}

// Returns whether the sampler is selected through the per-draw material record.
inline bool material_sampler(std::string_view name) {
    return std::find(kMaterialSamplers.begin(), kMaterialSamplers.end(), name)
           != kMaterialSamplers.end();
}

// A material uniform a mesh program reads from its draw's record instead: its name, the GLSL
// that reads it, and its place in the record.
struct MaterialField {
    const char* name;
    const char* value;
    unsigned offset;
};

inline constexpr MaterialField kMaterialFields[] =
    {{"material_colour", "O_MATERIAL.colour", 0},
     {"material_emission", "O_MATERIAL.emission_diffuse.xyz", 16},
     {"material_diffuse_scale", "O_MATERIAL.emission_diffuse.w", 28},
     // Original packed words retain their offsets as the record grows to include optics.
     {"material_emission_contrast", "uintBitsToFloat(O_MATERIAL.texture_indices.y)", 84},
     // Two bounded half pairs retain the foliage coverage and finish parameters.
     {"foliage_parameters0", "int(O_MATERIAL.texture_indices.z)", 88},
     {"foliage_parameters1", "int(O_MATERIAL.texture_indices.w)", 92},
     {"material_dielectric", "O_MATERIAL.optics.xyz", 96},
     {"material_metallic", "O_MATERIAL.optics.w", 108},
     {"material_dielectric_tint", "O_MATERIAL.reflection_tint.xyz", 112},
     {"foliage_reflectivity", "O_MATERIAL.reflection_tint.w", 124},
     {"material_fallback", "int(O_MATERIAL.optical_flags.x)", 128},
     {"cutout", "int(O_MATERIAL.flags.x)", 32},
     {"emission_mask", "int(O_MATERIAL.flags.y)", 36},
     {"emission_vertex_colour", "int(O_MATERIAL.flags.z)", 40},
     {"probe_effect", "int(O_MATERIAL.flags.w)", 44},
     {"hero_shadow_receiver", "int(O_MATERIAL.resources.z)", 56},
     // Depth programs use the spare resource word for their atlas view;
     // colour programs use it for terrain albedo. They never coexist.
     {"shadow_view", "int(O_MATERIAL.resources.w)", 60},
     {"probe_terrain_albedo", "uintBitsToFloat(O_MATERIAL.resources.w)", 60},
     {"skinning", "int(O_MATERIAL.draw_flags.x)", 64},
     {"lighting", "int(O_MATERIAL.draw_flags.y)", 68},
     {"probe_receiver", "int(O_MATERIAL.draw_flags.z)", 72},
     {"probe_mode", "int(O_MATERIAL.draw_flags.w)", 76}};

// Specialised colour programs already separate static and skinned work.
// Receiver/mode are common to each run: retaining real uniforms allows
// scalar branches instead of divergent per-draw buffer loads. Lighting
// remains per draw so terrain and lit static layers can still batch.
inline bool material_specialised(const std::string& vertex) {
    return vertex.find("#define OPENRAC_COLOUR_SPECIALISED") != std::string::npos;
}

// Whether a material program reads field `name` per draw.
inline bool material_per_draw(bool specialised, std::string_view name) {
    return !specialised || (name != "probe_mode" && name != "probe_receiver" && name != "skinning");
}

inline bool material_takes(const std::string& vertex, std::string_view name) {
    if (vertex.find("in mat4 model;") == std::string::npos) {
        return false;
    }

    const bool field = std::any_of(
        std::begin(kMaterialFields), std::end(kMaterialFields), [&](const MaterialField& f) {
            return name == f.name;
        }
    );
    return field && material_per_draw(material_specialised(vertex), name);
}

inline bool material_sources(
    const CompiledProgram& original,
    const std::string& vertex,
    const std::string& fragment,
    std::string& vs,
    std::string& fs,
    std::string& error
) {
    const bool specialised = material_specialised(vertex);
    const auto per_draw = [&](std::string_view name) {
        return material_per_draw(specialised, name);
    };
    vs = vertex;
    fs = fragment;
    std::string macros;

    for (const auto& field : kMaterialFields) {
        if (!per_draw(field.name)) {
            continue;
        }

        if (std::none_of(original.uniforms.begin(), original.uniforms.end(), [&](const auto& u) {
                return u.name == field.name;
            })) {
            continue;
        }

        constexpr std::array<std::string_view, 4> types = {"vec4", "vec3", "float", "int"};
        vs = remove_uniform(vs, types, field.name);
        fs = remove_uniform(fs, types, field.name);
        macros += std::string("#define ") + field.name + " (" + field.value + ")\n";
    }

    // Upscaled colour sampling must not split otherwise compatible mesh draws.
    for (const auto name : kMaterialSamplers) {
        constexpr std::array<std::string_view, 1> samplers = {"sampler2D"};
        fs = remove_uniform(fs, samplers, name);
    }
    const std::string vertex_header = R"(#extension GL_ARB_shader_draw_parameters : require



layout(push_constant) uniform OpenracDrawBase {

    uint base;

}



openrac_base;

flat out uint openrac_draw;

)";
    const std::string fragment_header =
        std::string(R"(#extension GL_EXT_nonuniform_qualifier : require

flat in uint openrac_draw;



struct OpenracMaterial {

    vec4 colour;

    vec4 emission_diffuse;

    uvec4 flags;

    uvec4 resources;

    uvec4 draw_flags;

    uvec4 texture_indices;

    vec4 optics;

    vec4 reflection_tint;

    uvec4 optical_flags;

};



layout(std430, set = 1, binding = 1) readonly buffer OpenracMaterials {

    uint openrac_draws[32768];

    OpenracMaterial openrac_materials[];

};

)")
        + "layout(constant_id = " + std::to_string(detail::kMaterialTextureCapacityConstant)
        + R"() const uint openrac_material_texture_capacity = 1024;

layout(set=1,binding=0) uniform sampler2D openrac_textures[openrac_material_texture_capacity];

#define O_MATERIAL (openrac_materials[openrac_draws[openrac_draw]])

#define tex_T0 openrac_textures[nonuniformEXT(O_MATERIAL.resources.x)]

#define emission_texture openrac_textures[nonuniformEXT(O_MATERIAL.resources.y)]

#define upscaled_texture_colours openrac_textures[nonuniformEXT(O_MATERIAL.texture_indices.x)]

)" + macros;
    vs.replace(0, vs.find('\n'), "#version 450 core");
    fs.replace(0, fs.find('\n'), "#version 450 core");
    vs.insert(vs.find('\n') + 1, vertex_header);

    // Shadow view selection and colour-layer flags are read per draw in the
    // vertex stage as well as by fragment material/lighting evaluation.
    if (std::any_of(original.uniforms.begin(), original.uniforms.end(), [](const auto& u) {
            return u.name == "shadow_view" || u.name == "skinning" || u.name == "probe_receiver"
                   || u.name == "probe_mode";
        })) {
        const auto begin = fragment_header.find("struct OpenracMaterial");
        const auto end = fragment_header.find("layout(set=1,binding=0)");
        const auto main_at = vs.find("void main() {");
        vs.insert(
            main_at,
            fragment_header.substr(begin, end - begin)
                + "\n#define O_MATERIAL (openrac_materials[openrac_draws[openrac_draw]])\n" + macros
        );
    }

    fs.insert(fs.find('\n') + 1, fragment_header);
    const auto main = vs.find("void main() {");

    if (main == std::string::npos) {
        error = "material vertex entry missing";
        return false;
    }

    vs.insert(
        main + std::strlen("void main() {"),
        "\nopenrac_draw = openrac_base.base + uint(gl_DrawIDARB);\n"
    );
    return true;
}
}  // namespace openrac::vkgl::shader_transforms
