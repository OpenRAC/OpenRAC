// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Opt-in test retention of immutable modules, layouts and pipelines across fresh GL programs.
// Exact bytecode and layout modes must match; uniforms and descriptor state are never reused.

#include <algorithm>
#include <utility>

#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
namespace {

// Checks every input to module/layout creation, without accepting a hash collision.
bool compatible(const Program& a, const Program& b) {
    return a.compiled.vertex == b.compiled.vertex && a.compiled.fragment == b.compiled.fragment
           && a.compiled.compute == b.compiled.compute
           && a.compiled.default_binding == b.compiled.default_binding
           && a.compiled.default_size == b.compiled.default_size
           && a.gpu_materials == b.gpu_materials && a.material_start == b.material_start
           && a.dynamic_uniforms == b.dynamic_uniforms
           && a.draw_uniform_offset == b.draw_uniform_offset
           && std::equal(
               a.test_layout_bindings.begin(),
               a.test_layout_bindings.end(),
               b.test_layout_bindings.begin(),
               b.test_layout_bindings.end(),
               [](const auto& x, const auto& y) {
                   return x.binding == y.binding && x.descriptorType == y.descriptorType
                          && x.descriptorCount == y.descriptorCount && x.stageFlags == y.stageFlags
                          && x.pImmutableSamplers == nullptr && y.pImmutableSamplers == nullptr;
               }
           );
}

// Transfers ownership while leaving the donor unable to destroy the borrowed objects.
void move_objects(Program& to, Program& from) {
    to.vertex = std::exchange(from.vertex, VK_NULL_HANDLE);
    to.fragment = std::exchange(from.fragment, VK_NULL_HANDLE);
    to.compute = std::exchange(from.compute, VK_NULL_HANDLE);
    to.compute_pipeline = std::exchange(from.compute_pipeline, VK_NULL_HANDLE);
    to.set_layout = std::exchange(from.set_layout, VK_NULL_HANDLE);
    to.layout = std::exchange(from.layout, VK_NULL_HANDLE);
    to.identity = from.identity;
    from.last_pipeline = VK_NULL_HANDLE;
    from.last_pipeline_key.clear();
}

}  // namespace

bool reuse_test_program(Program& program) {
    // Normal player programs retain their existing lifetime and compilation behaviour.
    if (!g->test_program_reuse || !g->test_program_borrow) {
        return false;
    }
    const auto found = std::find_if(g->test_programs.begin(), g->test_programs.end(), [&](auto& p) {
        return compatible(program, p);
    });
    // Changed shader arithmetic, descriptor mode or push constants need fresh driver objects.
    if (found == g->test_programs.end()) {
        return false;
    }
    g->pipelines.rename_program(found->serial, program.serial);
    move_objects(program, *found);
    g->test_programs.erase(found);
    ++g->submission.test_program_reuses;
    return true;
}

bool retain_test_program(Program& program) {
    constexpr std::size_t kRetainedPrograms = 256;  // Bounds test-only shader permutations.
    // Partial links and a full pool use the ordinary deferred destruction path.
    if (!g->test_program_reuse || !program.linked || !program.layout
        || g->test_programs.size() >= kRetainedPrograms) {
        return false;
    }
    // A pool copy already owns these objects; normal retirement avoids duplicate retained entries.
    if (std::any_of(g->test_programs.begin(), g->test_programs.end(), [&](const auto& p) {
            return compatible(program, p);
        })) {
        return false;
    }
    Program retained;
    retained.compiled = program.compiled;
    retained.test_layout_bindings = program.test_layout_bindings;
    retained.serial = program.serial;
    retained.linked = true;
    retained.gpu_materials = program.gpu_materials;
    retained.material_start = program.material_start;
    retained.dynamic_uniforms = program.dynamic_uniforms;
    retained.draw_uniform_offset = program.draw_uniform_offset;
    move_objects(retained, program);
    g->test_programs.push_back(std::move(retained));
    return true;
}

void clear_test_programs() {
    g->test_program_reuse = false;
    g->test_program_borrow = false;
    for (auto& program : g->test_programs) {
        release_program(program);
    }
    g->test_programs.clear();
}

}  // namespace openrac::vkgl::detail

namespace openrac::vkgl {

void test_reuse_programs(bool on, bool borrow) {
    // Tests may request this only after opening their Vulkan context.
    if (detail::g) {
        detail::g->test_program_reuse = on;
        detail::g->test_program_borrow = on && borrow;
    }
}

}  // namespace openrac::vkgl
