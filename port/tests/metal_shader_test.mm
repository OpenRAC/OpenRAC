// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The renderer's shaders as Metal reads them (renderer/metal/shader_translate.h): each program the
// game builds, compiled to SPIR-V as for Vulkan, translated to MSL, and compiled by the Metal
// device for macOS 12's MSL 2.4, with the clip control constant both ways. Each binding of the
// program has its Metal slot, and the vertex stage flips its rows. Skipped (exit 77) where there
// is no Metal device.

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>

#include "check.h"
#include "renderer/metal/shader_translate.h"
#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "renderer/vulkan/vkgl_internal.h"
#include "renderer/world/gpu.h"
#include "renderer_shaders.h"
#include "scene_shaders.h"
#include "world_shaders.h"

#import <Metal/Metal.h>

namespace {

using openrac::mtlgl::MslProgram;
using openrac::vkgl::CompiledProgram;

id<MTLDevice> g_device;

// `source` with `defines` after its version line, as the renderers make their variants.
std::string with(std::string_view source, std::string_view defines) {
    std::string out(source);
    out.insert(out.find('\n') + 1, defines);
    return out;
}

// Shader::build's stand-in for the upscaled colours when enhanced rendering is off.
std::string plain_colours(std::string_view fragment) {
    constexpr std::string_view marker = "// OPENRAC_UPSCALED_TEXTURE_COLOURS_SOURCE";
    std::string out(fragment);
    if (const auto at = out.find(marker); at != std::string::npos) {
        out.replace(at, marker.size(),
                    "vec4 sample_upscaled_texture_colours(vec2 uv) { return texture(tex_T0, uv); }\n");
    }
    return out;
}

bool has_buffer(const MslProgram& msl, int binding) {
    return std::ranges::find(msl.buffers, binding) != msl.buffers.end();
}

const openrac::mtlgl::MslTexture* texture(const MslProgram& msl, int binding) {
    const auto found = std::ranges::find(msl.textures, binding, &openrac::mtlgl::MslTexture::binding);
    return found == msl.textures.end() ? nullptr : &*found;
}

// Compiles `source` on the device and makes its `entry` with the clip control constant `clip`.
bool metal_compiles(const char* name, const std::string& source, const std::string& entry, bool clip) {
    MTLCompileOptions* options = [MTLCompileOptions new];
    options.languageVersion = MTLLanguageVersion2_4;
    NSError* error = nil;
    id<MTLLibrary> library = [g_device newLibraryWithSource:@(source.c_str())
                                                    options:options
                                                      error:&error];
    if (library == nil) {
        std::fprintf(stderr, "%s: %s\n%s\n", name, error.localizedDescription.UTF8String,
                     source.c_str());
        return false;
    }
    MTLFunctionConstantValues* constants = [MTLFunctionConstantValues new];
    [constants setConstantValue:&clip
                           type:MTLDataTypeBool
                        atIndex:openrac::vkgl::detail::kClipZeroToOneConstant];
    id<MTLFunction> function = [library newFunctionWithName:@(entry.c_str())
                                             constantValues:constants
                                                      error:&error];
    if (function == nil) {
        std::fprintf(stderr, "%s: %s: %s\n", name, entry.c_str(),
                     error.localizedDescription.UTF8String);
        return false;
    }
    return true;
}

void program(const char* name, std::string_view vertex, std::string_view fragment,
             std::string_view compute = {}) {
    CompiledProgram spirv;
    std::string log;
    if (!openrac::vkgl::compile_program(vertex, fragment, spirv, log, compute)) {
        std::fprintf(stderr, "%s: glslang: %s\n", name, log.c_str());
        CHECK(false);
        return;
    }
    for (const bool apple : {false, true}) {
        MslProgram msl;
        if (!openrac::mtlgl::translate_program(spirv, apple, msl, log)) {
            std::fprintf(stderr, "%s: %s\n", name, log.c_str());
            CHECK(false);
            return;
        }
        if (spirv.default_binding >= 0) {
            CHECK(has_buffer(msl, spirv.default_binding));
        }
        for (const auto& block : spirv.blocks) {
            CHECK(has_buffer(msl, block.binding));
        }
        for (const auto& block : spirv.storage) {
            CHECK(has_buffer(msl, block.binding));
        }
        for (const auto& uniform : spirv.uniforms) {
            if (uniform.sampler) {
                const auto* slot = texture(msl, uniform.binding);
                CHECK(slot != nullptr);
                if (slot != nullptr) {
                    const bool texel_buffer = uniform.kind == openrac::vkgl::SamplerKind::Buffer;
                    CHECK((slot->sampler >= 0) != texel_buffer);
                }
            }
        }
        for (const bool clip : {false, true}) {
            if (compute.empty()) {
                CHECK(msl.vertex.find("gl_Position.y = -(") != std::string::npos);
                CHECK(metal_compiles(name, msl.vertex, msl.vertex_entry, clip));
                CHECK(metal_compiles(name, msl.fragment, msl.fragment_entry, clip));
            } else {
                CHECK(metal_compiles(name, msl.compute, msl.compute_entry, clip));
            }
        }
    }
}

}  // namespace

int main() {
    @autoreleasepool {
        g_device = MTLCreateSystemDefaultDevice();
        if (g_device == nil) {
            std::puts("No Metal device: skipped");
            return 77;
        }
        namespace renderer_src = openrac::renderer::shaders;
        namespace scene_src = openrac::scene::shaders;
        namespace world_src = openrac::renderer::world::shaders;
        using openrac::renderer::world::compose;

        program("direct", renderer_src::direct_vert, plain_colours(renderer_src::direct_frag));
        program("direct masked", renderer_src::direct_vert,
                plain_colours(with(renderer_src::direct_frag, "#define OPENRAC_SAMPLE_MASK 1\n")));
        program("effect", renderer_src::effect_vert, plain_colours(renderer_src::direct_frag));
        program("effect fixed", renderer_src::effect_vert,
                plain_colours(with(renderer_src::direct_frag, "#define OPENRAC_EFFECT_FIXED 1\n")));
        program("debug lines", renderer_src::debug_lines_vert, renderer_src::debug_lines_frag);
        program("heat haze", renderer_src::haze_vert, renderer_src::haze_frag);
        program("shadow volumes", renderer_src::shadow_vert, renderer_src::shadow_frag);
        program("shadow composite", renderer_src::shadow_composite_vert, renderer_src::shadow_frag);

        const std::string mesh = plain_colours(scene_src::mesh_frag);
        program("mesh", scene_src::mesh_vert, mesh);
        program("mesh (solid)", scene_src::mesh_vert, with(mesh, "#define OPENRAC_SOLID 1\n"));
        program("sky", scene_src::sky_vert, plain_colours(scene_src::sky_frag));

        const std::initializer_list<std::string_view> parts = {world_src::world_common_glsl, world_src::world_lights_glsl,
                            world_src::display_blend_glsl};
        program("tfrag", compose(world_src::tfrag_vert, parts), compose(world_src::tfrag_frag, parts));
        program("tie", compose(world_src::tie_vert, parts), compose(world_src::tie_frag, parts));

        std::string horizons(renderer_src::gtao_evaluate_comp);
        horizons.insert(horizons.find('\n') + 1, renderer_src::surface_normal_glsl);
        horizons.insert(horizons.find('\n') + 1, renderer_src::gtao_common_glsl);
        program("GTAO horizons", {}, {},
                with(horizons, "#define OPENRAC_GTAO_SCALE 1\n#define OPENRAC_SURFACE_NORMALS 1\n"));
    }
    return openrac::test::result();
}
