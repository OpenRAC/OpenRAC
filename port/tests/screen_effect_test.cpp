// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exercises screen_effect.cpp's pass scheduling with fake GL, shader and framebuffer allocations.
// Expected sizes are successive halves; no bound sampler may name the output attachment, even
// when its branch in the shared shader is unused. No GPU or game data is needed; pixels are untested.

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "check.h"
#include "renderer/effects/screen_effect.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer_shaders.h"

namespace {

// Tracks resources and bindings for GL callbacks that have no user pointer, on the test thread.
struct Context {
    unsigned next = 1;              // Next synthetic resource name.
    unsigned unit = 0;              // Active texture unit.
    unsigned framebuffer = 0;       // Current draw framebuffer.
    unsigned read_framebuffer = 0;  // Copy source, distinct from a pass's output binding.
    std::array<int, 4> viewport{};  // Current pass extent in target-local pixels.

    // The source framebuffer and dimensions of each full input copy.
    struct Copy {
        unsigned source = 0;  // Framebuffer that owns the copied view.
        int width = 0;        // Copied columns.
        int height = 0;       // Copied rows.
    };

    std::vector<Copy> sources;      // Input isolation across consecutive view effects.
    std::vector<unsigned> outputs;  // Temporary targets and final view destinations, in pass order.
    std::array<unsigned, 2> textures{};              // Textures bound to the shader's two samplers.
    std::unordered_map<unsigned, unsigned> colours;  // Framebuffer to colour attachment.
    std::vector<std::array<int, 2>> sizes;           // Sizes allocated for blur targets.
    int passes = 0;                                  // Raster callbacks invoked.
    int copies = 0;                                  // Input frame copies made.
    int builds = 0;                                  // Shader builds attempted.
    bool reject_shader = false;                      // Injects shader creation failure.
};

// The fake GL callbacks borrow the current test's context until that test returns.
Context* g_context = nullptr;

// Allocates distinct names so framebuffer feedback cannot be hidden by accidental aliasing.
void generate(int count, unsigned* names) {
    // GL's output array contains one name per requested resource.
    for (int n = 0; n < count; ++n) {
        names[n] = g_context->next++;
    }
}

// Tracks the two texture units the screen shader declares.
void active_texture(unsigned unit) {
    g_context->unit = unit - openrac::gl::GL_TEXTURE0;
    CHECK(g_context->unit < g_context->textures.size());
}

// Tracks textures independently of the active shader branch, as GL's feedback rule requires.
void bind_texture(unsigned, unsigned texture) {
    g_context->textures.at(g_context->unit) = texture;
}

// Only the draw binding determines the raster output; read bindings are used for frame copies.
void bind_framebuffer(unsigned binding, unsigned framebuffer) {
    // ScreenEffects names its copy source explicitly; it must never inherit the preceding view.
    if (binding == openrac::gl::GL_READ_FRAMEBUFFER) {
        g_context->read_framebuffer = framebuffer;
    }

    // A read-only bind does not change where subsequent draws land.
    if (binding != openrac::gl::GL_READ_FRAMEBUFFER) {
        g_context->framebuffer = framebuffer;
    }
}

// Counts copies without pretending to validate pixels or framebuffer resolve behavior.
void copy_texture(unsigned, int, int, int, int, int, int width, int height) {
    ++g_context->copies;
    g_context->sources.push_back({g_context->read_framebuffer, width, height});
}

// Tracks the local viewport that each scheduler pass declares to the raster layer.
void viewport(int x, int y, int width, int height) {
    g_context->viewport = {x, y, width, height};
}

// Accepts GL calls whose state does not affect texture feedback, on the test thread.
template <typename... Args>
void ignore(Args...) {}

// Applies both effect kinds consecutively, including after resizing and at a one-pixel extent.
void test_consecutive_effects_do_not_sample_the_output() {
    Context context;
    g_context = &context;
    openrac::renderer::ScreenEffects effects;
    openrac::renderer::ScreenEffect effect;
    effect.kind = openrac::renderer::ScreenEffectKind::Glow;
    effect.blur = 1;
    constexpr unsigned kTarget = 10'000;
    constexpr int kWidth = 64;
    constexpr int kHeight = 32;
    const openrac::renderer::RenderTarget target{kTarget, kWidth, kHeight};
    effects.apply(effect, target, kTarget);
    effect.kind = openrac::renderer::ScreenEffectKind::Soften;
    effects.apply(effect, target, kTarget);
    effects.apply(effect, {kTarget, 1, 1}, kTarget);

    CHECK(context.copies == 3);  // One input copy for each effect, not for each blur pass.
    CHECK(context.builds == 1);
    CHECK(context.passes > context.copies);
    CHECK(context.sizes.size() == 12);  // Two buffers per size, three sizes, two extents.
    CHECK((context.sizes.front() == std::array<int, 2>{kWidth / 2, kHeight / 2}));
    CHECK((context.sizes.back() == std::array<int, 2>{1, 1}));
    effects.release();
    CHECK(context.colours.empty());
}

// Does no work for empty targets or no effect, and retries a failed shader only after release.
void test_disabled_and_failed_effects_do_not_draw() {
    Context context;
    g_context = &context;
    openrac::renderer::ScreenEffects effects;
    openrac::renderer::ScreenEffect effect;
    const openrac::renderer::RenderTarget target{10'000, 16, 16};  // Synthetic square target.
    effects.apply(effect, target, target.framebuffer);
    effect.kind = openrac::renderer::ScreenEffectKind::Glow;
    effects.apply(effect, {target.framebuffer, 0, 16}, target.framebuffer);
    CHECK(context.builds == 0);
    context.reject_shader = true;
    effects.apply(effect, target, target.framebuffer);
    context.reject_shader = false;
    effects.apply(effect, target, target.framebuffer);
    CHECK(context.builds == 1);
    CHECK(context.copies == 0 && context.passes == 0);
    effects.release();
    effects.apply(effect, target, target.framebuffer);
    CHECK(context.builds == 2);  // Explicit release clears the failure latch.
    CHECK(context.copies == 1 && context.passes != 0);
    effects.release();
}

// Two view post passes copy distinct sources and write each view target once, never the composite.
void test_independent_view_targets() {
    Context context;
    g_context = &context;
    openrac::renderer::ScreenEffects effects;
    openrac::renderer::ScreenEffect effect;
    effect.kind = openrac::renderer::ScreenEffectKind::Glow;
    const openrac::renderer::RenderTarget upper{10'000, 64, 32};  // Synthetic half-screen targets.
    const openrac::renderer::RenderTarget lower{20'000, 64, 32};
    effects.apply(effect, upper, upper.framebuffer);
    effect.kind = openrac::renderer::ScreenEffectKind::Soften;
    effects.apply(effect, lower, lower.framebuffer);
    CHECK(context.sources.size() == 2);

    // No resource or source binding from the first view can stand in for the second view's copy.
    if (context.sources.size() == 2) {
        CHECK(context.sources[0].source == upper.framebuffer);
        CHECK(context.sources[1].source == lower.framebuffer);
        CHECK(context.sources[0].width == upper.width && context.sources[0].height == upper.height);
        CHECK(context.sources[1].width == lower.width && context.sources[1].height == lower.height);
    }

    CHECK(std::count(context.outputs.begin(), context.outputs.end(), upper.framebuffer) == 1);
    CHECK(std::count(context.outputs.begin(), context.outputs.end(), lower.framebuffer) == 1);
    effects.release();
}

}  // namespace

namespace openrac::gl {

// Only screen_effect.cpp's entry points are supplied; any added GL dependency must be explicit.
PFN_glGenVertexArrays glGenVertexArrays = generate;
PFN_glGenTextures glGenTextures = generate;
PFN_glDeleteVertexArrays glDeleteVertexArrays = ignore<int, const unsigned*>;
PFN_glDeleteTextures glDeleteTextures = ignore<int, const unsigned*>;
PFN_glActiveTexture glActiveTexture = active_texture;
PFN_glBindTexture glBindTexture = bind_texture;
PFN_glBindFramebuffer glBindFramebuffer = bind_framebuffer;
PFN_glCopyTexSubImage2D glCopyTexSubImage2D = copy_texture;
PFN_glTexImage2D glTexImage2D =
    ignore<unsigned, int, int, int, int, int, unsigned, unsigned, const void*>;
PFN_glTexParameteri glTexParameteri = ignore<unsigned, unsigned, int>;
PFN_glViewport glViewport = viewport;
PFN_glUniform1i glUniform1i = ignore<int, int>;
PFN_glUniform1f glUniform1f = ignore<int, float>;
PFN_glUniform2f glUniform2f = ignore<int, float, float>;
PFN_glUniform4f glUniform4f = ignore<int, float, float, float, float>;
PFN_glDrawArrays glDrawArrays = ignore<unsigned, int, int>;
PFN_glDisable glDisable = ignore<unsigned>;
PFN_glDepthMask glDepthMask = ignore<unsigned char>;
PFN_glColorMask glColorMask = ignore<unsigned char, unsigned char, unsigned char, unsigned char>;
PFN_glBindSampler glBindSampler = ignore<unsigned, unsigned>;
PFN_glBindVertexArray glBindVertexArray = ignore<unsigned>;

}  // namespace openrac::gl

namespace openrac::renderer {

// Fake resources keep the pass scheduler independent of a real GL context and shader compiler.
FrameBuffer::~FrameBuffer() = default;

bool FrameBuffer::create_colour(int width, int height, std::string&) {
    release();
    m_framebuffer = g_context->next++;
    m_colour = g_context->next++;
    m_width = width;
    m_height = height;
    g_context->colours[m_framebuffer] = m_colour;
    g_context->sizes.push_back({width, height});
    return true;
}

void FrameBuffer::release() {
    g_context->colours.erase(m_framebuffer);
    m_framebuffer = 0;
    m_colour = 0;
    m_width = 0;
    m_height = 0;
}

Shader::~Shader() = default;

bool Shader::build(std::string_view, std::string_view, std::string_view, std::string& error) {
    ++g_context->builds;

    // A failed compile must latch the effect off until its resources are released.
    if (g_context->reject_shader) {
        error = "synthetic shader failure";
        return false;
    }

    m_program = g_context->next++;
    return true;
}

void Shader::release() {
    m_program = 0;
}

void Shader::use() const {}

int Shader::uniform(const char*) const {
    return 0;
}

// Checks resource declarations and GL feedback before invoking each real scheduler callback.
void raster_pass(
    std::string_view,
    const RenderTarget& target,
    RasterInputs inputs,
    const std::function<void()>& record
) {
    CHECK(g_context->framebuffer == target.framebuffer);
    CHECK((g_context->viewport == std::array<int, 4>{0, 0, target.width, target.height}));
    g_context->outputs.push_back(target.framebuffer);
    const auto attachment = g_context->colours.find(target.framebuffer);
    const unsigned output = attachment == g_context->colours.end() ? 0 : attachment->second;

    // Even an unused shader sampler bound to the output makes GL rendering undefined.
    for (const unsigned texture : g_context->textures) {
        CHECK(texture == 0 || texture != output);

        // Every bound input is declared to the raster graph, once per unique texture.
        if (texture != 0) {
            CHECK(std::any_of(inputs.textures.begin(), inputs.textures.end(), [texture](auto read) {
                return read.name == texture;
            }));
        }
    }

    ++g_context->passes;
    record();
}

namespace shaders {

// The fake compiler does not inspect shader text; real shader compilation is a GPU test's job.
const char* const screen_effect_vert = "";
const char* const screen_effect_frag = "";

}  // namespace shaders
}  // namespace openrac::renderer

// Runs pass-scheduling checks only; no window, driver or game is started.
int main() {
    test_consecutive_effects_do_not_sample_the_output();
    test_disabled_and_failed_effects_do_not_draw();
    test_independent_view_targets();
    return openrac::test::result();
}
