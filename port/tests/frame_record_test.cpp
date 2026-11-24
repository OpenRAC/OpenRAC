// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame recorder's data (renderer/frame/frame_record.h, docs/port/FRAME_RECORDER.md): a
// recorded frame keeps its call order with no DMA data at all; anchors find their place among a
// display list's tags; the recording a list was built with is found again when the list is drawn a
// tick late or sent again, and a place the list never reached drops its commands.

#include <cstdint>
#include <cstring>
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
    test_dropped_anchor_keeps_its_unanchored_followers_dropped();
    test_texture_paging_preserves_walk_order();
    test_unchanged_cursor_and_terminal_tag();
    test_diagnostic_command_names();
    return openrac::test::result();
}
