// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Production recipes pass through the real Vulkan program linker with runtime compilation refused
// and user caches disabled. Package omissions cannot be hidden by a previous test run.
#include <cstdio>
#include <exception>

#include "renderer/gl/shader.h"
#include "renderer/gl/shader_recipe.h"
#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
// Links one production recipe and checks that no component silently uses a fallback.
void test_recipe(const openrac::renderer::ShaderRecipe& recipe, std::string& error) {
    using namespace openrac;
    const auto misses = vkgl::submission_counters().shader_package_misses;
    bool ok;

    if (recipe.native) {
        vkgl::NativeComputeShader shader;
        ok = vkgl::compile_native_compute(recipe.compute, recipe.definitions, {}, shader, error);
    } else if (recipe.raw) {
        vkgl::CompiledProgram program;
        ok = vkgl::compile_program(recipe.vertex, recipe.fragment, program, error, recipe.compute);
    } else {
        renderer::Shader shader;
        ok = recipe.compute.empty()
                 ? shader.build(recipe.name, recipe.vertex, recipe.fragment, error)
                 : shader.build_compute(recipe.name, recipe.compute, error);
        shader.release();
    }

    if (!ok) {
        std::fprintf(stderr, "%s: %s\n", recipe.name.c_str(), error.c_str());
    }

    if (vkgl::submission_counters().shader_package_misses != misses) {
        std::fprintf(stderr, "Package miss during %s\n", recipe.name.c_str());
    }

    CHECK(ok);
}

// Runs the invocation while the executable boundary retains exception reporting.
int run_tests() {
    using namespace openrac;
    test::GlContext context;
    std::string error;

    if (!context.open(error) || !gl::is_vulkan()) {
        std::printf("SKIP: Vulkan context unavailable: %s\n", error.c_str());
        return test::kSkip;
    }

    renderer::ShaderRecipes recipes;
    const bool enhanced = gl::enhanced_rendering_available();
    renderer::renderer_shader_recipes(recipes, enhanced);
    renderer::scene_shader_recipes(recipes, enhanced);

    if (enhanced) {
        renderer::enhanced_shader_recipes(recipes);
    }

    renderer::native_shader_recipes(recipes);
    const auto before = vkgl::submission_counters().shader_compiles;
    const auto misses_before = vkgl::submission_counters().shader_package_misses;

    for (const auto& recipe : recipes.entries) {
        // Atlas shaders stay packaged on devices whose runtime selects separate shadow maps.
        if (recipe.vertex.find("GL_ARB_shader_viewport_layer_array") != std::string::npos
            && !vkgl::indexed_viewports_supported(1)) {
            continue;
        }

        test_recipe(recipe, error);
    }

    CHECK(vkgl::submission_counters().shader_compiles == before);
    CHECK(vkgl::submission_counters().shader_package_misses == misses_before);
    std::printf("Checked %zu production recipes\n", recipes.entries.size());
    return test::result();
}
}  // namespace

int main() {
    try {
        return run_tests();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "unexpected shader test exception: %s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected shader test exception\n");
        return 1;
    }
}
