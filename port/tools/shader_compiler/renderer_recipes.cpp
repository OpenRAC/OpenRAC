// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enumerates world, effects and overlay sources using the same assembly as their renderers.

#include "renderer/gl/shader.h"
#include "renderer/gl/shader_recipe.h"
#include "renderer/ui/backdrop_blur_sources.h"
#include "renderer/ui/imgui_gl_sources.h"
#include "renderer/ui/overlay_draw_sources.h"
#include "renderer/ui/touch_overlay_sources.h"
#include "renderer/world/shader_sources.h"
#include "renderer_shaders.h"
#include "world_shaders.h"

namespace openrac::renderer {
namespace {
std::string define(std::string source, std::string_view text) {
    source.insert(source.find('\n') + 1, text);
    return source;
}

std::string world_source(std::string_view source) {
    return world::compose(
        source,
        {world::shaders::world_common_glsl,
         world::shaders::world_lights_glsl,
         world::shaders::display_blend_glsl}
    );
}
}  // namespace

void renderer_shader_recipes(ShaderRecipes& recipes, bool /*enhanced*/) {
    recipes.graphics("debug lines", shaders::debug_lines_vert, shaders::debug_lines_frag);
    recipes.graphics("direct", shaders::direct_vert, shaders::direct_frag);
    recipes.graphics(
        "direct masked",
        shaders::direct_vert,
        define(shaders::direct_frag, "#define OPENRAC_SAMPLE_MASK 1\n")
    );
    recipes.graphics("shadow volumes", shaders::shadow_vert, shaders::shadow_frag);
    recipes.graphics("shadow composite", shaders::shadow_composite_vert, shaders::shadow_frag);
    recipes.graphics("Heat haze", shaders::haze_vert, shaders::haze_frag);
    recipes.graphics("opaque effects depth", shaders::effect_vert, shaders::effect_depth_frag);
    const auto effect = define(
        shaders::direct_frag,
        "#define OPENRAC_UPSCALED_TEXTURE_COLOURS 1\n#define OPENRAC_EFFECT_FOG 1\n"
    );
    recipes.graphics("effect tested quads", shaders::effect_vert, effect);
    recipes.graphics("effect", shaders::effect_vert, effect);
    recipes.graphics(
        "effect fixed",
        shaders::effect_vert,
        shader_constants(
            define(effect, "#define OPENRAC_EFFECT_FIXED 1\n"),
            {{"textured", 1}, {"distance_field", 0}, {"alpha_test", 0}}
        )
    );
    recipes.graphics(
        "effect untextured",
        shaders::effect_vert,
        define(shaders::direct_frag, "#define OPENRAC_UPSCALED_TEXTURE_COLOURS 1\n")
    );
    recipes.graphics(
        "tfrag", world_source(world::shaders::tfrag_vert), world_source(world::shaders::tfrag_frag)
    );
    recipes.graphics(
        "tie", world_source(world::shaders::tie_vert), world_source(world::shaders::tie_frag)
    );
    using namespace openrac::shader_sources;
    recipes.graphics("imgui", renderer_ui_imgui_gl::kVertex, renderer_ui_imgui_gl::kFragment);
    recipes.graphics(
        "touch overlay", renderer_ui_touch_overlay::kVertex, renderer_ui_touch_overlay::kFragment
    );
    recipes.graphics(
        "Backdrop blur", renderer_ui_backdrop_blur::kVertex, renderer_ui_backdrop_blur::kFragment
    );
    recipes.graphics(
        "mod overlay", renderer_ui_overlay_draw::kVertex, renderer_ui_overlay_draw::kFragment
    );
}
}  // namespace openrac::renderer
