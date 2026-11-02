// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the Vulkan backend's pipeline pre-warm (R-10, renderer/vulkan/vkgl_internal.h's
// PipelinePrewarm and PipelineManifest): the pipelines one run makes are made again in the next
// run on the worker, once their program links, so its drawing thread makes none of them; a damaged
// manifest, and one whose program is no longer built, are ignored and the draws make their own;
// with the registry entry pipeline-prewarm off nothing is made ahead.
//
// Each run is a context of its own, opened and closed in turn, on one private pipeline cache file
// and its manifest (OPENRAC_VULKAN_PIPELINE_CACHE, set before anything reads the environment). The
// expected counts follow from what each run draws: kStates pipelines of one program. The last
// tests link them from parts (vk-pipeline-library) and compare the pictures with the whole ones'.

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {

namespace fs = std::filesystem;
namespace render_opt = openrac::renderer::render_opt;
using namespace openrac::gl;
namespace vkgl = openrac::vkgl;

// The states each run draws with, each its own pipeline: blend off, blend on, and a colour mask
// (each baked into the pipeline whether or not the depth and cull state is dynamic, R-11).
constexpr int kStates = 3;

// The size of the picture drawn into.
constexpr int kSide = 4;

// The fragment colours of the two programs: one the first runs build, one a run builds instead.
constexpr std::string_view kFirstColour = "vec4(0.25, 0.5, 0.75, 1.0)";
constexpr std::string_view kOtherColour = "vec4(0.75, 0.5, 0.25, 1.0)";
// The colour of the program the pipeline-library tests build, which no other run builds.
constexpr std::string_view kLinkedColour = "vec4(0.5, 0.25, 0.75, 0.5)";

// What one run did: the pipelines the worker had made by the first draw, and those the drawing
// thread made itself while drawing.
struct RunCounts {
    std::uint64_t made_ahead = 0;     // wait_for_pipeline_prewarm's count once its jobs are done.
    std::uint64_t made_by_draws = 0;  // pipelines_created over the draws.
    std::uint64_t linked = 0;         // pipelines_linked over the draws (vk-pipeline-library).
    std::vector<std::uint8_t> pixels;  // The picture the draws left.
    bool ran = false;                  // The context opened on Vulkan.
};

// Sets the environment variable `name` to `value` for this process, before anything reads it:
// common/env.h reads the environment once, at the first question.
void set_variable(const char* name, const std::string& value) {
#if defined(_WIN32)
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

// Draws one triangle into `target` in each of the kStates states.
void draw_states(const openrac::renderer::FrameBuffer& target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, kSide, kSide);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Each state, from the base: blend off, every channel written.
    for (int state = 0; state < kStates; ++state) {
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // State 1 blends, state 2 leaves alpha alone; state 0 is the base.
        if (state == 1) {
            glEnable(GL_BLEND);
        } else if (state == 2) {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        }

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

// Runs one context: builds the program of colour `colour`, waits for the pre-warm, draws the
// kStates states and closes. Not `ran` where Vulkan is not here.
RunCounts run(std::string_view colour) {
    RunCounts counts;
    openrac::test::GlContext context;
    std::string error;

    // No Vulkan here: nothing to pre-warm.
    if (!context.open(error) || !vkgl::active()) {
        std::printf("SKIP: Vulkan unavailable: %s\n", error.c_str());
        return counts;
    }

    openrac::renderer::Shader shader;
    const std::string fragment = std::string(
                                     "#version 410 core\n"
                                     "layout(location = 0) out vec4 result;\n"
                                     "void main() { result = "
                                 )
                                 + std::string(colour) + "; }\n";
    CHECK(shader.build(
        "pre-warm fixture",
        "#version 410 core\n"
        "void main() {\n"
        "    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);\n"
        "}\n",
        fragment,
        error
    ));
    openrac::renderer::FrameBuffer target;
    CHECK(target.create(kSide, kSide, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    shader.use();
    counts.made_ahead = vkgl::wait_for_pipeline_prewarm();
    const std::uint64_t before = vkgl::submission_counters().pipelines_created;
    const std::uint64_t linked_before = vkgl::submission_counters().pipelines_linked;
    draw_states(target);
    counts.made_by_draws = vkgl::submission_counters().pipelines_created - before;
    counts.linked = vkgl::submission_counters().pipelines_linked - linked_before;
    counts.pixels = target.read_rgba();
    CHECK(counts.pixels.size() == static_cast<std::size_t>(kSide * kSide * 4));
    CHECK(vkgl::submission_counters().validation_errors == 0);
    glDeleteVertexArrays(1, &vao);
    counts.ran = true;
    return counts;
}

// Tests that a second run of the same program makes its pipelines ahead and its draws make none.
// False when Vulkan is not here.
bool test_a_run_s_pipelines_are_made_ahead_in_the_next() {
    const RunCounts first = run(kFirstColour);

    // No Vulkan: the whole test is skipped.
    if (!first.ran) {
        return false;
    }

    const RunCounts second = run(kFirstColour);
    CHECK(first.made_ahead == 0);
    CHECK(first.made_by_draws >= static_cast<std::uint64_t>(kStates));
    CHECK(second.made_ahead >= static_cast<std::uint64_t>(kStates));
    CHECK(second.made_by_draws == 0);
    return true;
}

// Tests that a run that builds another program finds nothing of it in the manifest.
void test_a_manifest_of_a_program_no_longer_built_is_ignored() {
    const RunCounts other = run(kOtherColour);
    CHECK(other.made_ahead == 0);
    CHECK(other.made_by_draws >= static_cast<std::uint64_t>(kStates));
}

// Tests that a damaged manifest is dropped: nothing is made ahead and the draws make their own.
void test_a_damaged_manifest_is_ignored(const fs::path& manifest) {
    CHECK(fs::exists(manifest));

    // The manifest's first bytes overwritten: its envelope's hash no longer matches.
    {
        std::fstream file(manifest, std::ios::binary | std::ios::in | std::ios::out);
        constexpr std::array<char, 4> kGarbage = {'X', 'X', 'X', 'X'};
        file.seekp(0);
        file.write(kGarbage.data(), kGarbage.size());
    }

    const RunCounts damaged = run(kFirstColour);
    CHECK(damaged.made_ahead == 0);
    CHECK(damaged.made_by_draws >= static_cast<std::uint64_t>(kStates));
}

// Tests that with pipeline-prewarm off nothing is made ahead, though the manifest has the keys.
void test_nothing_is_made_ahead_with_the_entry_off() {
    render_opt::set(render_opt::Opt::PipelinePrewarm, false);
    const RunCounts off = run(kFirstColour);
    render_opt::set(render_opt::Opt::PipelinePrewarm, true);
    CHECK(off.made_ahead == 0);
    CHECK(off.made_by_draws >= static_cast<std::uint64_t>(kStates));
}

// Tests that with vk-pipeline-library on, a program's first draws in new states link their
// pipelines instead of making them, and draw what the whole pipelines draw. Where the device
// cannot link pipelines fast nothing is linked, and the draws make their own as before.
void test_new_pipelines_are_linked_from_parts() {
    // Nothing made ahead from the first run's manifest keys, nor in place of a linked one: the
    // second run's draws link their pipelines, or make them where the device links none.
    render_opt::set(render_opt::Opt::PipelinePrewarm, false);
    render_opt::set(render_opt::Opt::VkPipelineLibrary, false);
    const RunCounts whole = run(kLinkedColour);
    render_opt::set(render_opt::Opt::VkPipelineLibrary, true);
    const RunCounts linked = run(kLinkedColour);
    render_opt::set(render_opt::Opt::VkPipelineLibrary, false);
    render_opt::set(render_opt::Opt::PipelinePrewarm, true);
    CHECK(whole.linked == 0);
    CHECK(linked.pixels == whole.pixels);

    // A device without fast linking: the draws made their pipelines.
    if (linked.linked == 0) {
        std::printf("note: the device links no pipelines from parts\n");
        CHECK(linked.made_by_draws >= static_cast<std::uint64_t>(kStates));
        return;
    }

    CHECK(linked.linked >= static_cast<std::uint64_t>(kStates));
    CHECK(linked.made_by_draws == 0);
}

// The two vertex layouts of the shared-part test: a triangle of two floats a vertex, packed, and
// a triangle that covers the picture, each vertex's two floats followed by two that are not read.
// Read with the packed layout's stride, the second's vertices would be (-1, -1) twice and (3, -1):
// a triangle of no area, which draws nothing.
constexpr std::array<float, 6> kPacked = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f};
constexpr std::array<float, 12> kSpaced =
    {-1.0f, -1.0f, -1.0f, -1.0f, 3.0f, -1.0f, -1.0f, -1.0f, -1.0f, 3.0f, -1.0f, -1.0f};

// Draws the packed triangle, then the spaced one, each into a cleared picture, with one program;
// returns the two pictures one after the other, and the pipelines the draws linked in `linked`.
// Empty where Vulkan is not here.
std::vector<std::uint8_t> draw_layouts(std::uint64_t& linked) {
    openrac::test::GlContext context;
    std::string error;

    // No Vulkan here: nothing to draw.
    if (!context.open(error) || !vkgl::active()) {
        return {};
    }

    openrac::renderer::Shader shader;
    CHECK(shader.build(
        "vertex layout fixture",
        "#version 410 core\n"
        "layout(location = 0) in vec2 position;\n"
        "void main() { gl_Position = vec4(position, 0, 1); }\n",
        "#version 410 core\n"
        "layout(location = 0) out vec4 result;\n"
        "void main() { result = vec4(0.25, 0.75, 0.5, 1.0); }\n",
        error
    ));
    openrac::renderer::FrameBuffer target;
    CHECK(target.create(kSide, kSide, error));
    std::array<unsigned, 2> vaos{};
    std::array<unsigned, 2> buffers{};
    glGenVertexArrays(2, vaos.data());
    glGenBuffers(2, buffers.data());
    glBindVertexArray(vaos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof kPacked, kPacked.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(vaos[1]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof kSpaced, kSpaced.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    shader.use();
    const std::uint64_t linked_before = vkgl::submission_counters().pipelines_linked;
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, kSide, kSide);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    std::vector<std::uint8_t> pixels;

    // Each layout into a cleared picture, which is kept.
    for (const unsigned vao : vaos) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        const std::vector<std::uint8_t> picture = target.read_rgba();
        pixels.insert(pixels.end(), picture.begin(), picture.end());
    }

    linked = vkgl::submission_counters().pipelines_linked - linked_before;
    CHECK(vkgl::submission_counters().validation_errors == 0);
    glDeleteBuffers(2, buffers.data());
    glDeleteVertexArrays(2, vaos.data());
    return pixels;
}

// Tests that pipelines linked from a part they share draw what their whole pipelines draw: two
// draws of one program in one state but two vertex layouts share the output part, which must not
// carry the first draw's vertex input into the second's pipeline (as NVIDIA's driver let it).
void test_a_shared_part_keeps_only_its_own_state() {
    std::uint64_t none = 0;
    std::uint64_t linked = 0;
    // Nothing made whole ahead or in place of a linked one: every pipeline of the second run is
    // linked, as it is until a worker's whole one replaces it.
    render_opt::set(render_opt::Opt::PipelinePrewarm, false);
    render_opt::set(render_opt::Opt::VkPipelineLibrary, false);
    const std::vector<std::uint8_t> whole = draw_layouts(none);
    render_opt::set(render_opt::Opt::VkPipelineLibrary, true);
    const std::vector<std::uint8_t> parts = draw_layouts(linked);
    render_opt::set(render_opt::Opt::VkPipelineLibrary, false);
    render_opt::set(render_opt::Opt::PipelinePrewarm, true);
    CHECK(none == 0);
    CHECK(whole.size() == static_cast<std::size_t>(2 * kSide * kSide * 4));
    CHECK(parts == whole);

    // A device without fast linking links nothing; where it does, both draws were linked.
    if (linked == 0) {
        std::printf("note: the device links no pipelines from parts\n");
        return;
    }

    CHECK(linked == 2);
}

}  // namespace

int main() {
    const fs::path folder =
        fs::temp_directory_path() / ("openrac_vulkan_prewarm_test_" + std::to_string(std::rand()));
    const fs::path cache = folder / "pipelines.bin";
    std::error_code ec;
    fs::remove_all(folder, ec);
    fs::create_directories(folder, ec);
    set_variable("OPENRAC_VULKAN_PIPELINE_CACHE", cache.string());

    // The entry under test is on, whatever OPENRAC_RENDER_OPT_OFF says (ctest runs the Vulkan set
    // with all entries off too); one test switches it off itself.
    render_opt::set(render_opt::Opt::PipelinePrewarm, true);
    // The tests of R-10 count whole pipelines: none is linked from parts until the last test.
    render_opt::set(render_opt::Opt::VkPipelineLibrary, false);

    // Without Vulkan nothing below can run.
    if (!test_a_run_s_pipelines_are_made_ahead_in_the_next()) {
        fs::remove_all(folder, ec);
        return openrac::test::kSkip;
    }

    test_nothing_is_made_ahead_with_the_entry_off();
    test_a_manifest_of_a_program_no_longer_built_is_ignored();
    test_a_damaged_manifest_is_ignored(fs::path(cache.string() + ".manifest"));
    test_new_pipelines_are_linked_from_parts();
    test_a_shared_part_keeps_only_its_own_state();
    fs::remove_all(folder, ec);
    return openrac::test::result();
}
