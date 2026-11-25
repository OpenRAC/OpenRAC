// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned shader source requests collected by build recipes and strict package coverage tests.

#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace openrac::renderer {
// An owned source assembly request, with native Vulkan and raw-GLSL paths marked explicitly.
struct ShaderRecipe {
    std::string name, vertex, fragment, compute, definitions;
    bool native = false;
    bool raw = false;  // Internal Vulkan shaders bypass Shader's enhanced preamble.
};

// Appends source requests in a stable order shared by production assembly and package tests.
struct ShaderRecipes {
    // Keeps a graphics source pair; raw recipes bypass Shader's enhanced preamble.
    void graphics(
        std::string_view name, std::string_view vertex, std::string_view fragment, bool raw = false
    ) {
        entries.push_back(
            {std::string(name), std::string(vertex), std::string(fragment), {}, {}, false, raw}
        );
    }

    // Keeps one GL-shaped compute source and its diagnostic name.
    void compute(std::string_view name, std::string_view source, bool raw = false) {
        entries.push_back({std::string(name), {}, {}, std::string(source), {}, false, raw});
    }

    // Keeps explicit native Vulkan bindings and preprocessor definitions.
    void native(std::string_view name, std::string_view source, std::string_view definitions = {}) {
        entries.push_back(
            {std::string(name), {}, {}, std::string(source), std::string(definitions), true, true}
        );
    }

    std::vector<ShaderRecipe> entries;  // Owned requests in first-use order.
};

// Appends the renderer, scene, enhanced and native families without creating a graphics context.
void renderer_shader_recipes(ShaderRecipes& recipes, bool enhanced);
void scene_shader_recipes(ShaderRecipes& recipes, bool enhanced);
void enhanced_shader_recipes(ShaderRecipes& recipes);
void native_shader_recipes(ShaderRecipes& recipes);
void fsr_shader_recipes(ShaderRecipes& recipes);
}  // namespace openrac::renderer
