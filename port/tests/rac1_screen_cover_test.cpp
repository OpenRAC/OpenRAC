// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// RAC1's production C cover helper, captured through the CPU FrameRecorder with synthetic views.
// Expected geometry follows half-open intersections and screen ratios; colours use byte packing,
// not retail data. These tests do not claim a residual retail draw owns the native state it needs.

#include <cmath>
#include <cstdint>
#include <vector>

#include "game/rac1/host/screen_cover.h"
#include "openrac/frame.h"
#include "renderer/frame/frame_record.h"
#include "tests/check.h"

namespace {

// ---- Capture ----

// The existing recorder's default 2D space and an invented reached cursor for synthetic views.
constexpr int kWidth = 512;
constexpr int kHeight = 448;
constexpr std::uint32_t kCursor = 0x1000;

// The callbacks carry no user pointer; each synchronous test lends its recorder to the C helper.
openrac::renderer::FrameRecorder* g_recorder = nullptr;
// Whether openrac_frame_fill_depth draws (a window), for the helper's keep-the-packet answer.
bool g_window = true;

// The helper's test recorder, installed only for the lifetime of one test.
struct Capture {
    openrac::renderer::FrameRecorder recorder;  // Owns the captured native commands.

    // Installs the recorder before any synchronous C callback can run.
    Capture() {
        CHECK(g_recorder == nullptr);
        g_recorder = &recorder;
    }

    // Revokes the callback's borrowed pointer before its owner is destroyed.
    ~Capture() {
        CHECK(g_recorder == &recorder);
        g_recorder = nullptr;
    }

    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;
    Capture(Capture&&) = delete;
    Capture& operator=(Capture&&) = delete;

    // Returns the commands owned by this capture.
    const std::vector<openrac::renderer::FrameCommand>& commands() {
        return recorder.open().commands;
    }
};

// Returns an explicitly owned native screen; no retail render state is assumed by this fixture.
rac1_cover_view screen() {
    return {kWidth, kHeight, nullptr, kCursor, 1};
}

// Checks geometry, not only callback counts, against a half-open expected rectangle.
void check_rect(const openrac::renderer::FrameCommand& command, const float (&expected)[4]) {
    CHECK(command.op == openrac::renderer::FrameOp::Fill);
    CHECK(!command.whole);

    // Ratios in a 416-line view need tolerance for float screen-coordinate conversion.
    constexpr float kTolerance = 0.0001f;

    // Check every edge so an empty or widened rectangle cannot pass on its colour alone.
    for (std::size_t n = 0; n < command.rect.size(); ++n) {
        CHECK(std::fabs(command.rect[n] - expected[n]) < kTolerance);
    }
}

// ---- Tests ----

// Preserves all byte-valued colour/alpha limits without clamping game full alpha to a byte maximum.
void test_colour_limits() {
    std::uint32_t rgba = 0;
    CHECK(rac1_cover_colour(0, 0, 0, 0, &rgba) && rgba == 0);
    // Alpha 128 means full; 255 remains a distinct over-full game value.
    CHECK(rac1_cover_colour(255, 128, 1, 128, &rgba) && rgba == 0x800180FFu);
    CHECK(rac1_cover_colour(255, 255, 255, 255, &rgba) && rgba == UINT32_MAX);

    // Out-of-byte inputs require the retail path, rather than a new clamp or field-spill policy.
    constexpr std::uint32_t kUnchanged = 0x12345678;
    rgba = kUnchanged;
    CHECK(!rac1_cover_colour(-1, 0, 0, 0, &rgba));
    CHECK(!rac1_cover_colour(0, 256, 0, 0, &rgba));
    CHECK(!rac1_cover_colour(0, 0, -1, 0, &rgba));
    CHECK(!rac1_cover_colour(0, 0, 0, 256, &rgba));
    CHECK(rgba == kUnchanged);
    CHECK(!rac1_cover_colour(0, 0, 0, 0, nullptr));
}

// Distinguishes a whole-window cover from a same-sized cover confined to the 4:3 picture.
void test_whole_target_and_picture_scope() {
    Capture capture;
    rac1_cover_view view = screen();
    CHECK(rac1_cover_fill(&view, nullptr, 0, OPENRAC_BLEND_MIX));
    view.whole_target = 0;
    CHECK(rac1_cover_fill(&view, nullptr, UINT32_MAX, OPENRAC_BLEND_OPAQUE));
    CHECK(capture.commands().size() == 2);
    const auto& whole = capture.commands().at(0);
    const auto& picture = capture.commands().at(1);
    CHECK(whole.whole && whole.wide && whole.rgba == 0);
    CHECK(whole.blend == openrac::renderer::BlendMode::Mix);
    CHECK(!picture.wide && picture.rgba == UINT32_MAX);
    CHECK(picture.blend == openrac::renderer::BlendMode::Opaque);
    const float expected[]{0, 0, static_cast<float>(kWidth), static_cast<float>(kHeight)};
    check_rect(picture, expected);
    CHECK(whole.anchor == kCursor && picture.anchor == kCursor);
}

// Retains zero, full and over-full game alpha on blended covers without byte-range normalization.
void test_mix_alpha_limits() {
    Capture capture;
    const rac1_cover_view view = screen();

    // Game alpha is stored unchanged: 0 transparent, 128 full, 255 above full.
    for (const std::uint32_t alpha : {0u, 128u, 255u}) {
        const std::uint32_t rgba = alpha << 24;
        CHECK(rac1_cover_fill(&view, nullptr, rgba, OPENRAC_BLEND_MIX));
        const auto& command = capture.commands().back();
        CHECK(command.rgba == rgba);
        CHECK(command.blend == openrac::renderer::BlendMode::Mix);
        CHECK(command.whole && command.wide);
    }
}

// Intersects requested geometry, the screen and an explicit native clip without widening it.
void test_clipped_fill() {
    Capture capture;
    rac1_cover_view view = screen();
    // The native clip removes all four requested edges, including its negative screen coordinates.
    const int clip[]{10, 20, 70, 90};
    const int rectangle[]{-20, -4, 80, 100};
    view.clip = clip;
    CHECK(rac1_cover_fill(&view, rectangle, 0x7F302010, OPENRAC_BLEND_MIX));
    CHECK(capture.commands().size() == 1);
    const auto& command = capture.commands().at(0);
    const float expected[]{10, 20, 70, 90};
    check_rect(command, expected);
    CHECK(command.wide);
    CHECK(command.rgba == 0x7F302010);
    CHECK(command.blend == openrac::renderer::BlendMode::Mix);
}

// Treats outside, zero-area and inverted rectangles as empty, never as whole-target fills.
void test_empty_fill() {
    Capture capture;
    rac1_cover_view view = screen();
    const int outside[]{kWidth, 0, kWidth + 1, kHeight};
    const int zero_width[]{1, 0, 1, kHeight};
    const int inverted[]{10, 20, 5, 15};
    CHECK(rac1_cover_fill(&view, outside, UINT32_MAX, OPENRAC_BLEND_OPAQUE));
    CHECK(rac1_cover_fill(&view, zero_width, UINT32_MAX, OPENRAC_BLEND_OPAQUE));
    CHECK(rac1_cover_fill(&view, inverted, UINT32_MAX, OPENRAC_BLEND_OPAQUE));
    view.clip = inverted;
    CHECK(rac1_cover_fill(&view, nullptr, UINT32_MAX, OPENRAC_BLEND_OPAQUE));
    CHECK(capture.commands().empty());
}

// Emits opaque black bars from an already-updated height, without running the retail update again.
void test_letterbox_rectangles() {
    Capture capture;
    const rac1_cover_view view = screen();
    // The retail PAL bar stops at 24 pixels; 0 is retracted.
    constexpr int kBarHeight = 24;
    CHECK(rac1_cover_letterbox(&view, 0));
    CHECK(capture.commands().empty());
    CHECK(rac1_cover_letterbox(&view, kBarHeight));
    CHECK(capture.commands().size() == 2);
    const float top[]{0, 0, kWidth, kBarHeight};
    const float bottom[]{0, kHeight - kBarHeight, kWidth, kHeight};
    check_rect(capture.commands().at(0), top);
    check_rect(capture.commands().at(1), bottom);

    // Opaque rather than MIX preserves the source primitive's disabled blending.
    for (const auto& command : capture.commands()) {
        CHECK(command.rgba == 0x80000000u);
        CHECK(command.blend == openrac::renderer::BlendMode::Opaque);
        CHECK(command.wide && command.anchor == kCursor);
    }
}

// Preserves game-pixel bar height when a 416-line view maps into the recorder's default screen.
void test_letterbox_video_height_and_clip() {
    Capture capture;
    rac1_cover_view view = screen();
    // PAL 60 Hz uses 416 game lines; a clip keeps only the lower half of the top bar.
    view.height = 416;
    const int clip[]{0, 12, kWidth, 24};
    view.clip = clip;
    CHECK(rac1_cover_letterbox(&view, 24));
    CHECK(capture.commands().size() == 1);
    const float expected[]{0, 12.0f * kHeight / 416.0f, kWidth, 24.0f * kHeight / 416.0f};
    check_rect(capture.commands().at(0), expected);
}

// Keeps colour/alpha outside the clear mask and preserves the exact before/after-world call order.
void test_depth_only_clear_and_order() {
    Capture capture;
    rac1_cover_view view = screen();
    CHECK(rac1_cover_fill(&view, nullptr, UINT32_MAX, OPENRAC_BLEND_MIX));
    openrac::renderer::FrameCommand world;
    world.op = openrac::renderer::FrameOp::World;
    capture.recorder.push(world);
    // Invented later cursor: the clear is after the world, not attached to its earlier fill.
    view.anchor += 16;
    CHECK(rac1_cover_clear_depth(&view));
    const auto& command = capture.commands().at(2);
    CHECK(command.op == openrac::renderer::FrameOp::Clear);
    CHECK(command.clear == OPENRAC_CLEAR_DEPTH);
    CHECK((command.clear & OPENRAC_CLEAR_COLOUR) == 0);
    CHECK(command.whole && command.wide);
    CHECK(command.anchor == view.anchor);
    const std::uint32_t tags[]{kCursor, view.anchor};
    const std::vector<std::size_t> expected{0, 0, 1};
    CHECK(openrac::renderer::place_commands(capture.recorder.open(), tags) == expected);
}

// Refuses depth-clear regions the existing native Clear cannot express rather than widening them.
void test_depth_clear_scope_fallback() {
    Capture capture;
    rac1_cover_view view = screen();
    const int partial[]{1, 0, kWidth, kHeight};
    view.clip = partial;
    CHECK(!rac1_cover_clear_depth(&view));
    view.clip = nullptr;
    view.whole_target = 0;
    CHECK(!rac1_cover_clear_depth(&view));
    const int empty[]{0, 0, 0, kHeight};
    view.clip = empty;
    CHECK(rac1_cover_clear_depth(&view));
    CHECK(capture.commands().empty());
}

// Emits the bars at their own depth, tested and written, and says when no window drew them.
void test_letterbox_depth() {
    constexpr std::uint32_t kBarDepth = 0xFFFFF3;  // draw_letterbox_bars' vertex depth
    constexpr int kDepthBits = 24;
    constexpr int kBarHeight = 24;
    Capture capture;
    const rac1_cover_view view = screen();
    CHECK(
        rac1_cover_letterbox_depth(&view, kBarHeight, kBarDepth, kDepthBits, OPENRAC_DEPTH_GEQUAL)
    );
    CHECK(capture.commands().size() == 2);
    const float top[]{0, 0, kWidth, kBarHeight};
    const float bottom[]{0, kHeight - kBarHeight, kWidth, kHeight};
    check_rect(capture.commands().at(0), top);
    check_rect(capture.commands().at(1), bottom);

    // The packet's own depth policy and colour, every channel written.
    for (const auto& command : capture.commands()) {
        CHECK(command.rgba == 0x80000000u && command.wide && command.anchor == kCursor);
        CHECK(command.blend == openrac::renderer::BlendMode::Opaque);
        CHECK(command.depth.enabled && command.depth_write && command.depth_value == kBarDepth);
        CHECK(command.depth.compare == openrac::renderer::DepthCompare::GreaterEqual);
        CHECK(command.depth_bits == kDepthBits);
        CHECK(command.writes == openrac::renderer::kWriteColour);
    }

    // No window: nothing drawn, so the caller keeps its packet; 0 and 25 behave as for plain bars.
    g_window = false;
    CHECK(
        !rac1_cover_letterbox_depth(&view, kBarHeight, kBarDepth, kDepthBits, OPENRAC_DEPTH_GEQUAL)
    );
    g_window = true;
    CHECK(rac1_cover_letterbox_depth(&view, 0, kBarDepth, kDepthBits, OPENRAC_DEPTH_GEQUAL));
    CHECK(!rac1_cover_letterbox_depth(
        &view, kBarHeight + 1, kBarDepth, kDepthBits, OPENRAC_DEPTH_GEQUAL
    ));
    CHECK(capture.commands().size() == 2);
}

// Leaves unsupported dimensions, counter values and blend policies to the caller's original path.
void test_invalid_inputs_fall_back() {
    Capture capture;
    rac1_cover_view view = screen();
    CHECK(!rac1_cover_fill(nullptr, nullptr, 0, OPENRAC_BLEND_MIX));
    CHECK(!rac1_cover_fill(&view, nullptr, 0, OPENRAC_BLEND_ADD));
    CHECK(!rac1_cover_letterbox(&view, -1));
    CHECK(!rac1_cover_letterbox(&view, 25));
    view.width = 0;
    CHECK(!rac1_cover_fill(&view, nullptr, 0, OPENRAC_BLEND_MIX));
    CHECK(!rac1_cover_clear_depth(&view));
    CHECK(!rac1_cover_letterbox(&view, 0));
    view = screen();
    view.height = -1;
    CHECK(!rac1_cover_fill(&view, nullptr, 0, OPENRAC_BLEND_MIX));
    CHECK(capture.commands().empty());
}

}  // namespace

// ---- C callbacks ----

extern "C" {

// Captures the adapter's reached list cursor using the real recorder's placement state.
void openrac_frame_anchor(gaddr cursor) {
    g_recorder->set_anchor(cursor);
}

// Captures the Fill contract's values without a graphics device or game memory.
void openrac_frame_fill(const float* rect, std::uint32_t rgba, int blend) {
    openrac::renderer::FrameCommand command;
    command.op = openrac::renderer::FrameOp::Fill;
    command.whole = rect == nullptr;

    // Only an explicit rectangle has coordinates to borrow for this synchronous callback.
    if (rect != nullptr) {
        // Keep every borrowed edge before the production C helper returns.
        for (std::size_t n = 0; n < command.rect.size(); ++n) {
            command.rect[n] = rect[n];
        }
    }

    command.wide = rect == nullptr || (blend & OPENRAC_PICTURE_WHOLE) != 0;
    command.rgba = rgba;
    command.blend = static_cast<openrac::renderer::BlendMode>(blend & UINT8_MAX);
    g_recorder->push(command);
}

// Captures the depth fill through the production command builder, as recorder.cpp does.
int openrac_frame_fill_depth(
    const float* rect,
    std::uint32_t rgba,
    int blend,
    std::uint32_t depth,
    int depth_bits,
    int depth_test,
    unsigned writes
) {
    // The helper's tests use GEQUAL only; another value is a failure of this fixture.
    CHECK(depth_test == OPENRAC_DEPTH_GEQUAL);
    const auto mode = static_cast<openrac::renderer::BlendMode>(blend & UINT8_MAX);
    const bool wide = rect == nullptr || (blend & OPENRAC_PICTURE_WHOLE) != 0;
    const auto command = openrac::renderer::depth_fill_command(
        rect,
        rgba,
        mode,
        wide,
        depth,
        depth_bits,
        openrac::renderer::DepthCompare::GreaterEqual,
        (writes & OPENRAC_WRITE_DEPTH) != 0,
        static_cast<std::uint8_t>(writes & OPENRAC_WRITE_COLOUR)
    );

    // Without a window nothing is recorded, and the helper reports it.
    if (!g_window || !command) {
        return 0;
    }

    g_recorder->push(*command);
    return 1;
}

// Captures the clear mask so tests can reject accidental colour/alpha clearing.
void openrac_frame_clear(std::uint32_t rgba, int what) {
    openrac::renderer::FrameCommand command;
    command.op = openrac::renderer::FrameOp::Clear;
    command.rgba = rgba;
    command.clear = static_cast<std::uint8_t>(what);
    command.wide = true;
    g_recorder->push(command);
}

}  // extern "C"

// Runs the production C helper with synthetic native ownership, not a claimed retail binding.
int main() {
    test_colour_limits();
    test_whole_target_and_picture_scope();
    test_mix_alpha_limits();
    test_clipped_fill();
    test_empty_fill();
    test_letterbox_rectangles();
    test_letterbox_video_height_and_clip();
    test_depth_only_clear_and_order();
    test_depth_clear_scope_fallback();
    test_invalid_inputs_fall_back();
    test_letterbox_depth();
    return openrac::test::result();
}
