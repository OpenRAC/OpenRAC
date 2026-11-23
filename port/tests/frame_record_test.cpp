// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame recorder's data (renderer/frame/frame_record.h, docs/port/FRAME_RECORDER.md): a
// recorded frame keeps its call order with no DMA data at all; anchors find their place among a
// display list's tags; the recording a list was built with is found again when the list is drawn a
// tick late or sent again, and a place the list never reached drops its commands.

#include <cstdint>
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

}  // namespace

int main() {
    order_without_dma();
    anchors_place_commands();
    late_and_resent_lists();
    screen_size();
    views();
    return openrac::test::result();
}
