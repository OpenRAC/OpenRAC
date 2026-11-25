// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enumerates native Vulkan programs and the embedded error checkerboard without a GPU context.

#include "renderer/gl/shader_recipe.h"
#include "renderer/vulkan/draw/gpu_draw_sources.h"
#include "renderer/vulkan/draw/multisample_blit_sources.h"
#include "renderer/vulkan/passes/depth_combine_sources.h"
#include "renderer/vulkan/pipelines/error_shader_sources.h"
#include "renderer_shaders.h"
#include "vkgl_shaders.h"

namespace openrac::renderer {
void native_shader_recipes(ShaderRecipes& recipes) {
    using namespace openrac::shader_sources;
    recipes.graphics(
        "error checkerboard",
        vkgl::error_shader_sources::kVertex,
        vkgl::error_shader_sources::kFragment,
        true
    );
    recipes.native("shadow depth combine", renderer_vulkan_passes_depth_combine::kCombine);

    for (auto definitions :
         {"",
          "#define OPENRAC_HDR_PQ 1\n",
          "#define OPENRAC_HDR_PQ 1\n#define OPENRAC_HDR_FLOAT_TARGET 1\n"}) {
        recipes.native("HDR encode", vkgl::shaders::hdr_encode_comp, definitions);
    }

    std::string agx(shaders::agx_frag);
    agx.replace(0, agx.find('\n'), "#version 450\n#define OPENRAC_AGX_COMPUTE 1");
    recipes.native("AgX compute", agx);
    recipes.graphics(
        "multisample blit",
        renderer_vulkan_draw_multisample_blit::kVertex,
        renderer_vulkan_draw_multisample_blit::kFragment,
        true
    );

    for (unsigned pass = 0; pass < 3; ++pass) {
        std::string source(renderer_vulkan_draw_gpu_draw::kGpuCullSource);

        if (pass) {
            source.insert(
                source.find('\n') + 1,
                pass == 1 ? "#define MAKE_ARGUMENTS 1\n"
                          : "#define SUMMARIZE 1\n#define SUMMARY_AT 167936\n"
            );
        }

        recipes.compute("GPU instance culling", source, true);
    }
}
}  // namespace openrac::renderer
