// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame recorder's data (renderer/frame/frame_record.h, docs/port/FRAME_RECORDER.md): a
// recorded frame keeps its call order with no DMA data at all; anchors find their place among a
// display list's tags; the recording a list was built with is found again when the list is drawn a
// tick late or sent again, and a place the list never reached drops its commands.

#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "check.h"
#include "renderer/frame/frame_record.h"

using namespace openrac::renderer;

namespace {

FrameCommand op(FrameOp kind, std::uint32_t index = 0) {
    FrameCommand c;
    c.op = kind;
    c.index = index;
    return c;
}

// A frame with no display list: its commands, in call order, are the frame.
void order_without_dma() {
    FrameRecorder recorder;
    recorder.push(op(FrameOp::Clear));
    recorder.push(op(FrameOp::World, 0x0F));
    recorder.push(op(FrameOp::WorldMobys));
    recorder.open().ui_items.push_back({});
    recorder.push(op(FrameOp::UiItem, 0));
    recorder.push(op(FrameOp::Fill));
    recorder.close();
    const FrameRecording* r = recorder.latest_unanchored();
    CHECK(r != nullptr);
    if (r == nullptr) {
        return;
    }
    CHECK(r->serial == recorder.latest_serial());
    CHECK(r->commands.size() == 5);
    const FrameOp expected[] =
        {FrameOp::Clear, FrameOp::World, FrameOp::WorldMobys, FrameOp::UiItem, FrameOp::Fill};
    for (std::size_t i = 0; i < 5 && i < r->commands.size(); ++i) {
        CHECK(r->commands[i].op == expected[i]);
        CHECK(r->commands[i].anchor == 0);
    }
    // No list: placed at its start, in order.
    const auto places = place_commands(*r, {});
    CHECK(places.size() == 5);
    for (const std::size_t p : places) {
        CHECK(p == 0);
    }
    CHECK(recorder.match(std::vector<std::uint32_t>{0x1000}) == nullptr);
}

// Anchors put each command before the list's tag at its address, in call order; commands with no
// anchor follow the one before; an anchor the list never reached drops its command.
void anchors_place_commands() {
    FrameRecorder recorder;
    recorder.push(op(FrameOp::Fill));  // before any anchor: the list's start
    recorder.set_anchor(0x2000);
    recorder.push(op(FrameOp::World));
    recorder.push(op(FrameOp::UiQuad, 0));
    recorder.set_anchor(0x2040);
    recorder.push(op(FrameOp::UiQuad, 1));
    recorder.set_anchor(0x9990);  // never in the list
    recorder.push(op(FrameOp::UiQuad, 2));
    recorder.set_anchor(0x2080);
    recorder.push(op(FrameOp::Mobys));
    recorder.close();
    // The list: tags at 0x2000 (after a tag elsewhere, a called packet), 0x2020, 0x2040, 0x2080.
    const std::vector<std::uint32_t> tags{0x00500000, 0x2000, 0x2020, 0x2040, 0x00600000, 0x2080};
    const FrameRecording* r = recorder.match(tags);
    CHECK(r != nullptr);
    if (r == nullptr) {
        return;
    }
    const auto places = place_commands(*r, tags);
    CHECK(places.size() == 6);
    CHECK(places[0] == 0);
    CHECK(places[1] == 1 && places[2] == 1);
    CHECK(places[3] == 3);
    CHECK(places[4] == kDropped);
    CHECK(places[5] == 5);
    // A tag whose address repeats later (a subroutine called twice) is found from the last place.
    const std::vector<std::uint32_t> again{0x2040, 0x2000, 0x2040, 0x2080};
    const auto later = place_commands(*r, again);
    CHECK(later[1] == 1 && later[3] == 2 && later[5] == 3);
}

// The list drawn is the one the game built a tick before (Going Commando's and Up Your Arsenal's
// double buffers): its recording is found among the kept ones by its anchors, the newest first,
// and an empty recording (a fade's ticks, which send the last list again) does not hide it.
void late_and_resent_lists() {
    FrameRecorder recorder;
    // Tick 1 builds in buffer A, tick 2 in buffer B.
    recorder.set_anchor(0xA000);
    recorder.push(op(FrameOp::World, 1));
    recorder.close();
    recorder.set_anchor(0xB000);
    recorder.push(op(FrameOp::World, 2));
    recorder.close();
    const std::vector<std::uint32_t> list_a{0xA000, 0xA010};
    const std::vector<std::uint32_t> list_b{0xB000};
    const FrameRecording* a = recorder.match(list_a);
    const FrameRecording* b = recorder.match(list_b);
    CHECK(a != nullptr && a->commands[0].index == 1);
    CHECK(b != nullptr && b->commands[0].index == 2);
    // A fade: nothing recorded for two ticks, list A sent again.
    recorder.close();
    recorder.close();
    a = recorder.match(list_a);
    CHECK(a != nullptr && a->commands[0].index == 1);
    // Tick 3 builds in buffer A again: the newest recording of A is drawn with A.
    recorder.set_anchor(0xA000);
    recorder.push(op(FrameOp::World, 3));
    recorder.close();
    a = recorder.match(list_a);
    CHECK(a != nullptr && a->commands[0].index == 3);
    // Only kKept recordings are kept: after four more, tick 2's is gone.
    for (int i = 0; i < 4; ++i) {
        recorder.set_anchor(0xC000);
        recorder.push(op(FrameOp::World, 10));
        recorder.close();
    }
    CHECK(recorder.match(list_b) == nullptr);
    // close() forgets the anchor.
    CHECK(recorder.anchor() == 0);
    recorder.reset();
    CHECK(recorder.match(std::vector<std::uint32_t>{0xC000}) == nullptr);
}

// The game's 2D screen goes with each command recorded after it is given, across recordings.
void screen_size() {
    FrameRecorder recorder;
    recorder.push(op(FrameOp::Fill));
    recorder.set_screen(512.0f, 416.0f);
    recorder.push(op(FrameOp::Scissor));
    recorder.set_screen(0.0f, 0.0f);  // no size: kept
    recorder.close();
    recorder.push(op(FrameOp::Fill));
    recorder.close();
    const FrameRecording* r = recorder.latest_unanchored();
    CHECK(r != nullptr && r->commands.size() == 1 && r->commands[0].screen_height == 416.0f);
    recorder.reset();
    CHECK(recorder.screen()[1] == 448.0f);
}

// Split-screen views: in the recording in the order they begin, each with its rectangle, the
// screen and the first camera given while it was open; a view begun while one is open ends that
// one; a view still open when the recording closes ends with it.
void views() {
    FrameRecorder recorder;
    recorder.set_screen(512.0f, 448.0f);
    FrameViewCamera camera;
    camera.position = {1.0f, 2.0f, 3.0f};
    camera.tan_half_fov_y = 0.25f;
    recorder.view_camera(camera);  // no view open: nobody's
    recorder.view_begin(0, {0.0f, 0.0f, 512.0f, 224.0f});
    recorder.push(op(FrameOp::World, 0x0F));
    recorder.view_camera(camera);
    FrameViewCamera later = camera;
    later.position = {9.0f, 9.0f, 9.0f};
    recorder.view_camera(later);  // the first is kept
    recorder.view_end();
    CHECK(!recorder.view_open());
    recorder.view_begin(1, {0.0f, 224.0f, 512.0f, 448.0f});
    recorder.push(op(FrameOp::World, 0x0F));
    recorder.view_begin(2, {0.0f, 0.0f, 256.0f, 224.0f});
    recorder.close();
    const FrameRecording* r = recorder.latest_unanchored();
    CHECK(r != nullptr);
    if (r == nullptr) {
        return;
    }
    CHECK(r->views.size() == 3);
    if (r->views.size() == 3) {
        CHECK(r->views[0].view == 0 && r->views[0].has_camera);
        CHECK(r->views[0].camera == camera);
        CHECK(r->views[0].rect[3] == 224.0f && r->views[0].screen_height == 448.0f);
        CHECK(r->views[1].view == 1 && !r->views[1].has_camera);
        CHECK(r->views[2].view == 2 && !r->views[2].has_camera);
    }
    const FrameOp expected[] = {
        FrameOp::ViewBegin,
        FrameOp::World,
        FrameOp::ViewEnd,
        FrameOp::ViewBegin,
        FrameOp::World,
        FrameOp::ViewEnd,
        FrameOp::ViewBegin,
        FrameOp::ViewEnd,
    };
    CHECK(r->commands.size() == 8);
    for (std::size_t i = 0; i < 8 && i < r->commands.size(); ++i) {
        CHECK(r->commands[i].op == expected[i]);
    }
    CHECK(!recorder.view_open());
    // The next recording has none: a full-screen frame.
    recorder.push(op(FrameOp::World, 0x0F));
    recorder.close();
    r = recorder.latest_unanchored();
    CHECK(r != nullptr && r->views.empty());
}

// Rejects non-finite rectangles before split-screen placement can round them into target pixels.
void test_invalid_view_rectangles() {
    FrameRecorder recorder;
    const float infinity = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    recorder.view_begin(0, {0.0f, 0.0f, infinity, 1.0f});
    recorder.view_begin(0, {nan, 0.0f, 1.0f, 1.0f});
    recorder.view_begin(0, {1.0f, 0.0f, 0.0f, 1.0f});
    recorder.view_begin(0, {0.0f, 1.0f, 1.0f, 1.0f});
    recorder.view_begin(kFrameViews, {0.0f, 0.0f, 1.0f, 1.0f});
    CHECK(!recorder.view_open());
    CHECK(recorder.open().views.empty());
    CHECK(recorder.open().commands.empty());
}

// Keeps alternating players' pictures only while their rectangles belong to the same layout.
void test_retained_view_layouts() {
    FrameView upper;
    upper.rect = {0.0f, 0.0f, 512.0f, 224.0f};  // Upper half of the default screen.
    FrameView lower = upper;
    lower.view = 1;
    lower.rect = {0.0f, 224.0f, 512.0f, 448.0f};  // Lower half, sharing only an edge.
    CHECK(upper.compatible_with(upper));
    CHECK(upper.compatible_with(lower));
    CHECK(lower.compatible_with(upper));

    FrameView quarter = lower;
    quarter.view = 2;  // A former four-player layout's lower-left quarter overlaps the new half.
    quarter.rect[2] = 256.0f;
    CHECK(!lower.compatible_with(quarter));
    quarter.view = lower.view;
    CHECK(!lower.compatible_with(quarter));

    FrameView resized = upper;
    resized.screen_height *= 2.0f;  // Doubled source pixels, unchanged screen fractions.
    resized.rect[3] *= 2.0f;
    CHECK(upper.compatible_with(resized));
}

// Keeps each view's World layer union separate and distinguishes layout-only calls from drawing.
void test_view_world_layers() {
    FrameRecorder recorder;
    recorder.push(op(FrameOp::World, 1));  // The unscoped world belongs to no split-screen view.
    recorder.view_begin(0, {0.0f, 0.0f, 512.0f, 224.0f});
    recorder.push(op(FrameOp::World, 1));
    recorder.push(op(FrameOp::World, 2));  // A second pass adds only this view's next layer.
    recorder.view_begin(1, {0.0f, 224.0f, 512.0f, 448.0f});
    recorder.push(op(FrameOp::World, 4));  // The other player's layer is not part of view zero.
    recorder.view_begin(2, {0.0f, 0.0f, 256.0f, 224.0f});
    const auto& views = recorder.open().views;
    CHECK(views.size() == 3);

    // The last layout has not drawn anything and must not replace a retained picture.
    if (views.size() == 3) {
        CHECK(views[0].has_world && views[0].layers == (1u | 2u));
        CHECK(views[1].has_world && views[1].layers == 4u);
        CHECK(!views[2].has_world && views[2].layers == 0);
    }
}

// Drops an unanchored follower of an unreachable command until a later anchor is reached.
void test_dropped_anchor_keeps_its_unanchored_followers_dropped() {
    // These invented addresses model a resent list cut short before its final native draw.
    constexpr std::uint32_t kReached = 0x1000;
    constexpr std::uint32_t kMissing = 0x2000;
    constexpr std::uint32_t kResumed = 0x3000;
    FrameRecorder recorder;
    recorder.set_anchor(kReached);
    recorder.push(op(FrameOp::World));
    recorder.set_anchor(kMissing);
    recorder.push(op(FrameOp::TargetBegin));
    recorder.set_anchor(0);
    recorder.push(op(FrameOp::Fill));
    recorder.push(op(FrameOp::TargetEnd));
    recorder.set_anchor(kResumed);
    recorder.push(op(FrameOp::UiQuad));

    const std::uint32_t tags[]{kReached, kResumed};
    const std::vector<std::size_t> places = place_commands(recorder.open(), tags);
    const std::vector<std::size_t> expected{0, kDropped, kDropped, kDropped, 1};
    CHECK(places == expected);
}

// Follows walk order through a texture-paging splice rather than sorting cursor addresses.
void test_texture_paging_preserves_walk_order() {
    // Uploads can run before earlier allocations (seen in game code: DmaTfragTextures, DoGifPaging).
    constexpr std::uint32_t kUpload = 0x3000;
    constexpr std::uint32_t kDraw = 0x2000;
    constexpr std::uint32_t kExit = 0x4000;
    FrameRecorder recorder;
    recorder.set_anchor(kUpload);
    recorder.push(op(FrameOp::Picture));
    recorder.set_anchor(kDraw);
    recorder.push(op(FrameOp::World));
    recorder.set_anchor(kExit);
    recorder.push(op(FrameOp::WorldShrubs));
    recorder.set_anchor(kDraw);
    recorder.push(op(FrameOp::UiQuad));
    recorder.set_anchor(0);
    recorder.push(op(FrameOp::Fill));

    const std::uint32_t tags[]{kUpload, kDraw, kExit};
    const std::vector<std::size_t> places = place_commands(recorder.open(), tags);
    const std::vector<std::size_t> expected{0, 1, 2, kDropped, kDropped};
    CHECK(places == expected);
}

// Keeps multiple native draws at an unchanged cursor, including the list's terminating tag.
void test_unchanged_cursor_and_terminal_tag() {
    // Native draws append no packet, so both draws can precede the same eventual END tag.
    constexpr std::uint32_t kEnd = 0x2000;
    FrameRecorder recorder;
    recorder.set_anchor(kEnd);
    recorder.push(op(FrameOp::UiQuad));
    recorder.push(op(FrameOp::UiQuad));
    recorder.close();

    const std::uint32_t tags[]{kEnd};
    const FrameRecording* recording = recorder.match(tags);
    CHECK(recording != nullptr);

    // A failed match has no placement to inspect.
    if (recording == nullptr) {
        return;
    }

    const std::vector<std::size_t> expected{0, 0};
    CHECK(place_commands(*recording, tags) == expected);
}

// Gives dropped-command diagnostics native operation names instead of packet encodings.
void test_diagnostic_command_names() {
    CHECK(std::strcmp(frame_op_name(FrameOp::World), "World") == 0);
    CHECK(std::strcmp(frame_op_name(FrameOp::WorldShrubs), "WorldShrubs") == 0);
    CHECK(std::strcmp(frame_op_name(FrameOp::Mobys), "Mobys") == 0);
    CHECK(std::strcmp(frame_op_name(FrameOp::PictureBand), "PictureBand") == 0);
    // A corrupt enum must still produce a printable diagnostic.
    CHECK(std::strcmp(frame_op_name(static_cast<FrameOp>(UINT8_MAX)), "Unknown") == 0);
}

}  // namespace

int main() {
    order_without_dma();
    anchors_place_commands();
    late_and_resent_lists();
    screen_size();
    views();
    test_invalid_view_rectangles();
    test_retained_view_layouts();
    test_view_world_layers();
    test_dropped_anchor_keeps_its_unanchored_followers_dropped();
    test_texture_paging_preserves_walk_order();
    test_unchanged_cursor_and_terminal_tag();
    test_diagnostic_command_names();
    return openrac::test::result();
}
