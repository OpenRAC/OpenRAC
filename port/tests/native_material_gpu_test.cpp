// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native effect quads (EffectQuad::native) drawn by EffectRenderer on a real GPU context
// (headless: SDL's offscreen driver): a native quad draws the same pixels as its twin drawn from
// GS words, and a native blend weighed by the target's alpha follows that alpha. Pixels are
// compared with each other and with the cleared target, never with captured values. Skipped
// (exit 77) where no OpenGL 4.1 context can be made.

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/direct/direct.h"
#include "renderer/effects/effects.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer/textures/texture.h"
#include "renderer/textures/texture_pool.h"
#include "renderer_gif_builder.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac::renderer;
using namespace openrac::gl;

// The target's side, in pixels.
constexpr int kSide = 16;

// The pool block the GS-word twin samples its texture at.
constexpr std::uint32_t kBlock = 0x100;

// An orange texel, alpha 0x40 (0x80 = 1.0): a half-weighted source.
constexpr std::uint32_t kTexel = 0x400040FFu;

// A quad over the whole target (identity camera: clip space), white at full alpha.
EffectQuad whole_quad() {
    EffectQuad quad{};

    // Corner k is at the right with bit 0 and at the top with bit 1, its texel likewise.
    for (int k = 0; k < 4; ++k) {
        quad.corner[k][0] = (k & 1) != 0 ? 1.0f : -1.0f;
        quad.corner[k][1] = (k & 2) != 0 ? 1.0f : -1.0f;
        quad.corner[k][3] = 1.0f;
        quad.rgba[k] = 0x80808080u;
        quad.st[k][0] = static_cast<float>(k & 1);
        quad.st[k][1] = static_cast<float>(k >> 1);
    }

    return quad;
}

// Draws `quads` (with `images`) over a target cleared to `clear` (RGBA, 0 to 1) and reads it back.
std::vector<std::uint8_t> draw(
    TexturePool& textures,
    std::span<const EffectQuad> quads,
    std::span<const UiImage> images,
    const std::array<float, 4>& clear
) {
    FrameBuffer target;
    std::string error;
    CHECK(target.create(kSide, kSide, error));
    RenderState state{textures, {target.id(), kSide, kSide}, {}};
    EffectRenderer effects("native material effects", EffectPart::All);
    CHECK(effects.init(state, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, kSide, kSide);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClearColor(clear[0], clear[1], clear[2], clear[3]);
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FrameInput input;
    input.effects = quads;
    input.effect_images = images;
    effects.render(input, state);
    std::vector<std::uint8_t> pixels = target.read_rgba();
    effects.release();
    target.release();
    return pixels;
}

// The one-texel image both quads sample: kept by content for the native quad, placed at kBlock
// for its twin.
UiImage texel_image(TexturePool& textures) {
    auto image = std::make_shared<Rgba8Image>(1, 1);
    image->set_texel(0, 0, kTexel);
    textures.place(kBlock, textures.add(*image, AlphaScale::Gs, "native material twin"));
    return {0x6E617469766531ull, image, nullptr};  // any hash names the content once
}

// A native quad mixing its texel over the target by the source alpha, and its twin with ALPHA
// (Cs - Cd) As + Cd, TEX0 TCC MODULATE at kBlock, draw the same pixels.
void test_a_native_quad_draws_as_its_gs_twin() {
    TexturePool textures;
    const std::array<UiImage, 1> images{texel_image(textures)};
    EffectQuad twin = whole_quad();
    twin.tex0 = openrac::test::tex0(kBlock, 1, gs::kPsmct32, 0, 0, true);
    twin.alpha = openrac::test::alpha_reg(0, 1, 0, 1);
    EffectQuad native = whole_quad();
    native.native = true;
    native.image = 1;
    native.state.blend = Blend::of(BlendMode::Mix);
    const std::array<float, 4> grey{0.25f, 0.5f, 0.75f, 1.0f};
    const std::vector<std::uint8_t> clear = draw(textures, {}, images, grey);
    const std::vector<std::uint8_t> from_words = draw(textures, {&twin, 1}, images, grey);
    const std::vector<std::uint8_t> from_material = draw(textures, {&native, 1}, images, grey);
    CHECK(from_words != clear);
    CHECK(from_material == from_words);
}

// Cs Ad + Cd adds nothing where the target's alpha is 0 and adds where it is 1 (its colour alone
// written, as the source's alpha would replace the target's); a material that keeps the colour
// and writes only alpha changes the alpha alone.
void test_destination_alpha_weighs_a_native_blend() {
    TexturePool textures;
    const std::array<UiImage, 1> images{texel_image(textures)};
    EffectQuad add = whole_quad();
    add.native = true;
    add.image = 1;
    add.state.blend = Blend::of(BlendMode::Add);
    add.state.blend.weight = BlendWeight::DestinationAlpha;
    add.state.colour_writes = kWriteRed | kWriteGreen | kWriteBlue;
    const std::array<float, 4> unweighted{0.25f, 0.25f, 0.25f, 0.0f};
    const std::array<float, 4> weighted{0.25f, 0.25f, 0.25f, 1.0f};
    CHECK(draw(textures, {&add, 1}, images, unweighted) == draw(textures, {}, images, unweighted));
    CHECK(draw(textures, {&add, 1}, images, weighted) != draw(textures, {}, images, weighted));
    EffectQuad mask = add;
    mask.state.blend.source_k = 0;
    mask.state.blend.dest_k = 0;
    mask.state.colour_writes = kWriteAlpha;
    const std::vector<std::uint8_t> before = draw(textures, {}, images, weighted);
    const std::vector<std::uint8_t> after = draw(textures, {&mask, 1}, images, weighted);
    bool colour_kept = true;
    bool alpha_written = false;

    // Each pixel keeps its colour bytes; the alpha bytes take the texel's.
    for (std::size_t i = 0; i + 3 < after.size(); i += 4) {
        colour_kept = colour_kept && after[i] == before[i] && after[i + 1] == before[i + 1]
                      && after[i + 2] == before[i + 2];
        alpha_written = alpha_written || after[i + 3] != before[i + 3];
    }

    CHECK(colour_kept && alpha_written);
}

}  // namespace

int main() {
    openrac::test::GlContext context;
    std::string why;

    // A machine without an OpenGL 4.1 context skips the test, as CTest is told.
    if (!context.open(why)) {
        std::printf("SKIP: no OpenGL 4.1 core context: %s\n", why.c_str());
        return openrac::test::kSkip;
    }

    test_a_native_quad_draws_as_its_gs_twin();
    test_destination_alpha_weighs_a_native_blend();
    return openrac::test::result();
}
