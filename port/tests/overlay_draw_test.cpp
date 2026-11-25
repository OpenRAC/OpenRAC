// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mods draw over the game, on the GPU (renderer/ui/overlay_draw.h), on each GL (the _es and
// _vulkan tests run this one): into the frame's own kind of target, at its place, right way up,
// blended by alpha, and nothing without vertices; the mods' pictures (1.40) sampled the right way
// up, tinted, and gone once let go, one texture for a picture two draws share. Pixels are read
// back: a colour in the wrong row or a blend left out fails here on the backend that has it.

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include "check.h"
#include "renderer/frame/colour_transfer.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/ui/overlay_draw.h"
#include "renderer/ui/overlay_images.h"
#include "renderer/ui/overlay_list.h"
#include "renderer/ui/picture_blit.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac;
using namespace openrac::gl;

constexpr int kWidth = 512;
constexpr int kHeight = 448;

// The pixel at (x, y) of the picture, y down.
std::array<std::uint8_t, 4> pixel(unsigned framebuffer, int x, int y) {
    std::array<std::uint8_t, 4> out{};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glReadPixels(x, kHeight - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return out;
}

void clear_to(unsigned framebuffer, float r, float g, float b) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, kWidth, kHeight);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool near(int a, int b) {
    return a >= b - 2 && a <= b + 2;
}

// Checks an uploaded picture, a partial blit, and a cleared window blit by their pixels.
void test_picture_transfers(unsigned framebuffer) {
    constexpr int kSide = 2;
    constexpr int kCopySize = 16;
    constexpr int kOffset = 32;
    // Red on the top row, green on the bottom. The target starts blue.
    constexpr std::array<std::uint8_t, 16> kRows{
        255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255
    };
    clear_to(framebuffer, 0, 0, 1);
    renderer::ui::PictureBlit picture;
    picture.draw(framebuffer, {kRows, kSide, kSide}, {0, 0, kCopySize, kCopySize});
    CHECK(pixel(framebuffer, 1, kHeight - kCopySize)[0] == 255);
    CHECK(pixel(framebuffer, 1, kHeight - 2)[1] == 255);
    CHECK(pixel(framebuffer, kCopySize, kHeight - 2)[2] == 255);

    renderer::blit_colour(
        "letterboxed picture",
        framebuffer,
        0,
        {{0, 0},
         {kCopySize, kCopySize},
         {kOffset, kOffset},
         {kOffset + kCopySize, kOffset + kCopySize},
         false,
         std::array{0.0f, 0.0f, 0.0f, 1.0f}}
    );
    CHECK(pixel(0, kOffset + 1, kHeight - kOffset - kCopySize)[0] == 255);
    CHECK(pixel(0, 0, 0) == (std::array<std::uint8_t, 4>{0, 0, 0, 255}));
    picture.release();
}

using Pixel = std::array<std::uint8_t, 4>;

bool is(const Pixel& p, int r, int g, int b) {
    return near(p[0], r) && near(p[1], g) && near(p[2], b);
}

// The frame cleared to (r, g, b, a), 0 to 255.
void clear_to_bytes(unsigned framebuffer, int r, int g, int b, int a) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, kWidth, kHeight);
    glClearColor(
        static_cast<float>(r) / 255.0f,
        static_cast<float>(g) / 255.0f,
        static_cast<float>(b) / 255.0f,
        static_cast<float>(a) / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Alpha, add and subtract over a frame that is not black, for rectangles and text: the
// subtraction is the frame's colour minus the colour times its alpha (not the colour minus the
// frame's: that is what a swapped pair of factors would give, and a colour above the frame's would
// then show), nothing goes below 0 or above 255, and the frame's alpha is whatever it was.
void blends(renderer::ui::OverlayDraw& draw, unsigned frame) {
    using renderer::ui::OverlayList;
    constexpr std::uint32_t kAlpha = 0;
    constexpr std::uint32_t kAdd = 1;
    constexpr std::uint32_t kSubtract = 2;
    OverlayList list;

    const auto draw_over = [&](int r, int g, int b, int a) {
        clear_to_bytes(frame, r, g, b, a);
        draw.draw(frame, kWidth, kHeight, list.vertices(), list.batches());
    };

    // Subtract the game's way, cyan at half alpha from a mid colour: red untouched, green and blue
    // lose half of 255.
    list.begin(1.0f);
    CHECK(list.set_blend(kSubtract));
    list.rect(100, 100, 40, 40, 0x00FFFF80u);
    draw_over(100, 150, 200, 255);
    Pixel p = pixel(frame, 120, 120);
    CHECK(is(p, 100, 150 - 128, 200 - 128));
    CHECK(p[3] == 255);
    CHECK(is(pixel(frame, 99, 120), 100, 150, 200));  // outside: as it was
    CHECK(is(pixel(frame, 141, 120), 100, 150, 200));

    // The direction: a colour above the frame's leaves 0, not "colour minus frame" (170 here).
    list.begin(1.0f);
    CHECK(list.set_blend(kSubtract));
    list.rect(100, 100, 40, 40, 0xC8C8C8FFu);
    draw_over(30, 30, 30, 255);
    CHECK(is(pixel(frame, 120, 120), 0, 0, 0));
    // A colour below it takes it down by that much, at full alpha; at no alpha, nothing.
    list.begin(1.0f);
    CHECK(list.set_blend(kSubtract));
    list.rect(100, 100, 40, 40, 0x0A141EFFu);
    list.rect(200, 100, 40, 40, 0xFFFFFF00u);
    draw_over(100, 100, 100, 255);
    CHECK(is(pixel(frame, 120, 120), 90, 80, 70));
    CHECK(is(pixel(frame, 220, 120), 100, 100, 100));

    // Add: the frame's colour plus the colour times alpha, held at 255.
    list.begin(1.0f);
    CHECK(list.set_blend(kAdd));
    list.rect(100, 100, 40, 40, 0x32641480u);
    list.rect(200, 100, 40, 40, 0xFFFFFFFFu);
    draw_over(100, 150, 200, 255);
    p = pixel(frame, 120, 120);
    CHECK(is(p, 100 + 25, 150 + 50, 200 + 10));
    CHECK(p[3] == 255);
    CHECK(is(pixel(frame, 220, 120), 255, 255, 255));

    // Alpha (the default) is what it was: colour over the frame by its alpha.
    list.begin(1.0f);
    list.rect(100, 100, 40, 40, 0xFFFFFF80u);
    draw_over(0, 0, 0, 255);
    CHECK(is(pixel(frame, 120, 120), 128, 128, 128));

    // The frame's alpha is kept by all three, also when it is not 1.
    for (const std::uint32_t blend : {kAlpha, kAdd, kSubtract}) {
        list.begin(1.0f);
        CHECK(list.set_blend(blend));
        list.rect(100, 100, 40, 40, 0x80808080u);
        draw_over(100, 100, 100, 128);
        CHECK(near(pixel(frame, 120, 120)[3], 128));
    }

    // Order and what is set when: the blend of a rectangle is the one set when it was asked, and
    // goes on until it is set again.
    list.begin(1.0f);
    list.rect(100, 100, 100, 40, 0x0000FFFFu);  // alpha: the blue over the frame
    CHECK(list.set_blend(kSubtract));
    list.rect(120, 100, 20, 40, 0x00001EFFu);   // blue 30 less
    list.text(150, 110, 16, 0x00FFFFFFu, "I");  // text subtracts too: a bar of "I"
    CHECK(list.set_blend(kAlpha));
    list.rect(180, 100, 20, 40, 0xFFFFFFFFu);  // alpha again, white
    draw_over(100, 150, 200, 255);
    CHECK(is(pixel(frame, 105, 120), 0, 0, 255));      // alpha
    CHECK(is(pixel(frame, 130, 120), 0, 0, 225));      // subtracted from the blue just drawn
    CHECK(is(pixel(frame, 190, 120), 255, 255, 255));  // alpha again
    // The text's strokes took all of the blue the frame had there (it is over the blue rectangle),
    // between them the frame is as it was.
    bool stroked = false;
    bool bare = false;
    for (int y = 110; y < 126; ++y) {
        for (int x = 150; x < 170; ++x) {
            const Pixel t = pixel(frame, x, y);
            stroked = stroked || is(t, 0, 0, 0);
            bare = bare || is(t, 0, 0, 255);
        }
    }
    CHECK(stroked && bare);

    // Draw after a subtract frame: an alpha rectangle is drawn by alpha, not by what was left.
    list.begin(1.0f);
    list.rect(100, 100, 40, 40, 0xFFFFFF80u);
    draw_over(0, 0, 0, 255);
    CHECK(is(pixel(frame, 120, 120), 128, 128, 128));
}

// A mod's picture (interface 1.40): a 2 x 2 of four colours, drawn by the nearest texel over a
// square, each quarter its colour the right way up; tinted; turned; gone once let go.
void pictures(renderer::ui::OverlayDraw& draw, unsigned frame) {
    using renderer::ui::OverlayList;
    namespace images = renderer::ui::overlay_images;
    static const int owner = 0;
    const std::array<std::uint8_t, 16> rgba{
        255,
        0,
        0,
        255,
        0,
        255,
        0,
        255,  // red, green: the top row
        0,
        0,
        255,
        255,
        255,
        255,
        255,
        255  // blue, white: the bottom
    };
    const int id = images::create(&owner, 2, 2, rgba, false);
    CHECK(id > 0);
    OverlayList list;
    auto draw_list = [&] {
        clear_to_bytes(frame, 0, 0, 0, 255);
        draw.draw(frame, kWidth, kHeight, list.vertices(), list.batches());
    };
    // 1:1 aspect is 448/512 of a unit down across: 100 down is 100 * 512 / 448 across.
    const float aspect = 512.0f / 448.0f;
    list.begin(aspect);
    list.image(id, 256, 224, 100, 100, 0, 0xFFFFFFFFu);
    list.rect(0, 0, 10, 10, 0xFFFF00FFu);  // a rectangle after it samples white, not the picture
    draw_list();
    CHECK(is(pixel(frame, 230, 200), 255, 0, 0));      // top left: red
    CHECK(is(pixel(frame, 280, 200), 0, 255, 0));      // top right: green
    CHECK(is(pixel(frame, 230, 250), 0, 0, 255));      // bottom left: blue
    CHECK(is(pixel(frame, 280, 250), 255, 255, 255));  // bottom right: white
    CHECK(is(pixel(frame, 5, 5), 255, 255, 0));
    CHECK(is(pixel(frame, 150, 224), 0, 0, 0));  // outside it: the frame

    // Tinted half red at half alpha: the white quarter becomes (128, 0, 0) over black.
    list.begin(aspect);
    list.image(id, 256, 224, 100, 100, 0, 0xFF000080u);
    draw_list();
    CHECK(is(pixel(frame, 280, 250), 128, 0, 0));

    // A half turn counter-clockwise: white is now top left.
    list.begin(aspect);
    list.image(id, 256, 224, 100, 100, 3.14159265f, 0xFFFFFFFFu);
    draw_list();
    CHECK(is(pixel(frame, 230, 200), 255, 255, 255));
    CHECK(is(pixel(frame, 280, 250), 255, 0, 0));

    // Let go: the same list draws nothing of it.
    CHECK(images::destroy(&owner, id));
    list.begin(aspect);
    list.image(id, 256, 224, 100, 100, 0, 0xFFFFFFFFu);
    draw_list();
    CHECK(is(pixel(frame, 230, 200), 0, 0, 0));
}

// One picture drawn by two draws that share their textures, as the overlay's top layer and its
// layer under the HUD do: one texture for it, which the release of either draw leaves to the
// other, and gone from both once let go.
void shared_pictures(
    renderer::ui::OverlayTextures& textures, renderer::ui::OverlayDraw& top, unsigned frame
) {
    using renderer::ui::OverlayList;
    namespace images = renderer::ui::overlay_images;
    static const int owner = 0;
    constexpr std::array<std::uint8_t, 4> kRed{255, 0, 0, 255};
    const int id = images::create(&owner, 1, 1, kRed, false);
    CHECK(id > 0);
    renderer::ui::OverlayDraw under_hud(textures);
    OverlayList list;
    list.begin(static_cast<float>(kWidth) / static_cast<float>(kHeight));
    list.image(id, 256, 224, 100, 100, 0, 0xFFFFFFFFu);
    const auto drawn_by = [&](renderer::ui::OverlayDraw& draw) {
        clear_to_bytes(frame, 0, 0, 0, 255);
        draw.draw(frame, kWidth, kHeight, list.vertices(), list.batches());
        return is(pixel(frame, 256, 224), 255, 0, 0);
    };
    CHECK(drawn_by(under_hud));
    CHECK(drawn_by(top));
    CHECK(textures.size() == 1);  // uploaded once, for both

    // One draw let go: the other still samples the picture, from the same texture.
    under_hud.release();
    CHECK(drawn_by(top));
    CHECK(textures.size() == 1);

    // The picture let go: the next draw, either, frees its texture; neither draws it.
    CHECK(images::destroy(&owner, id));
    CHECK(!drawn_by(top));
    CHECK(textures.size() == 0);
    CHECK(!drawn_by(under_hud));
    CHECK(textures.size() == 0);
    under_hud.release();
}

}  // namespace

int main() {
    test::GlContext context;
    std::string why;
    if (!context.open(why, kWidth, kHeight)) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return test::kSkip;
    }
    renderer::FrameBuffer frame;
    CHECK(frame.create(kWidth, kHeight, why));
    renderer::ui::OverlayTextures textures;
    renderer::ui::OverlayDraw draw(textures);
    renderer::ui::OverlayList list;

    // No vertices: the frame as it was.
    clear_to(frame.id(), 0.0f, 0.0f, 0.5f);
    list.begin(1.0f);
    draw.draw(frame.id(), kWidth, kHeight, list.vertices());
    CHECK(near(pixel(frame.id(), 100, 100)[2], 128));

    // A red rectangle in the top left: there, and not at the same place from the bottom.
    list.rect(100, 40, 50, 20, 0xFF0000FFu);
    // A half white one lower down, over the blue.
    list.rect(300, 300, 50, 50, 0xFFFFFF80u);
    draw.draw(frame.id(), kWidth, kHeight, list.vertices());
    const auto red = pixel(frame.id(), 120, 50);
    CHECK(red[0] == 255 && red[1] == 0 && red[2] == 0);
    CHECK(pixel(frame.id(), 120, kHeight - 50)[0] == 0);  // the picture is the right way up
    CHECK(pixel(frame.id(), 99, 50)[0] == 0);
    CHECK(pixel(frame.id(), 151, 50)[0] == 0);
    const auto half = pixel(frame.id(), 320, 320);
    CHECK(near(half[0], 128) && near(half[1], 128) && near(half[2], 192));
    CHECK(half[3] == 255);  // the frame's own alpha stays, under a see-through colour too

    // A frame larger than the picture: it stretches, as the game's own picture does.
    renderer::FrameBuffer large;
    CHECK(large.create(kWidth * 2, kHeight * 2, why));
    glBindFramebuffer(GL_FRAMEBUFFER, large.id());
    glViewport(0, 0, kWidth * 2, kHeight * 2);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    list.begin(1.0f);
    list.rect(100, 40, 50, 20, 0x00FF00FFu);
    draw.draw(large.id(), kWidth * 2, kHeight * 2, list.vertices());
    std::array<std::uint8_t, 4> green{};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, large.id());
    glReadPixels(240, kHeight * 2 - 1 - 100, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, green.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    CHECK(green[1] == 255);

    // Over and over, as frames do.
    for (int i = 0; i < 8; ++i) {
        draw.draw(frame.id(), kWidth, kHeight, list.vertices());
    }
    CHECK(glGetError() == GL_NO_ERROR);

    test_picture_transfers(frame.id());
    CHECK(glGetError() == GL_NO_ERROR);

    blends(draw, frame.id());
    CHECK(glGetError() == GL_NO_ERROR);

    pictures(draw, frame.id());
    CHECK(glGetError() == GL_NO_ERROR);

    shared_pictures(textures, draw, frame.id());
    CHECK(glGetError() == GL_NO_ERROR);

    draw.release();
    textures.release();
    frame.release();
    large.release();
    return test::result();
}
