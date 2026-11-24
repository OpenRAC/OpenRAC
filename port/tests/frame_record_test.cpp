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

// Does not revive old UI when a rebuilt buffer happens to reuse one of its numerical anchors.
void test_rebuilt_list_does_not_revive_stale_ui() {
    // Invented aligned addresses: the replacement has no native commands and reuses one old cursor.
    constexpr std::uint32_t kRoot = 0x6000;
    constexpr std::uint32_t kWorld = kRoot + 0x40;
    constexpr std::uint32_t kShrubs = kRoot + 0x80;
    constexpr std::uint32_t kUi = kRoot + 0xC0;
    FrameRecorder recorder;
    recorder.begin_list(kRoot);
    recorder.set_anchor(kWorld);
    recorder.push(op(FrameOp::World));
    recorder.set_anchor(kShrubs);
    recorder.push(op(FrameOp::WorldShrubs));
    recorder.set_anchor(kUi);
    recorder.push(op(FrameOp::UiQuad));
    recorder.close();

    // The producer resets this buffer but emits no native commands in its replacement build.
    recorder.begin_list(kRoot);
    recorder.close();
    const std::uint32_t rewritten_tags[]{kRoot, kRoot + 0x20, kUi};
    const FrameRecording* selected = recorder.match(kRoot, rewritten_tags);
    CHECK(selected == nullptr);

    // An incorrectly selected recording must not make its old UI a valid draw at the shared cursor.
    if (selected != nullptr) {
        const std::vector<std::size_t> places = place_commands(*selected, rewritten_tags);
        CHECK(places.back() == kDropped);
    }
}

// ---- Producer ownership ----

// Retains the entire title-like payload through delayed captures and unchanged resubmissions.
void test_owned_title_and_valid_resend_keep_payloads() {
    constexpr std::uint32_t kA = 0xA000;
    constexpr std::uint32_t kB = 0xB000;
    constexpr std::uint32_t kUi = kA + 0x40;
    constexpr std::uint32_t kColour = 0x80402010;
    FrameRecorder recorder;
    recorder.begin_list(kA);
    recorder.set_anchor(kA);
    recorder.push(op(FrameOp::World));
    recorder.close();
    UiQuad quad;
    quad.rgba = kColour;
    recorder.open().ui_quads.push_back(quad);
    recorder.set_anchor(kUi);
    recorder.push(op(FrameOp::UiQuad));
    recorder.close();
    const std::uint32_t tags[]{kA, kUi};
    const FrameRecording* title = recorder.match(kA, tags);
    CHECK(title != nullptr);

    // Do not dereference a failed ownership match after recording the test failure.
    if (title == nullptr) {
        return;
    }

    const std::uint64_t generation = title->list_generation;
    const std::uint64_t serial = title->serial;
    CHECK(title->commands.size() == 2);
    CHECK(title->ui_quads.size() == 1 && title->ui_quads[0].rgba == kColour);
    CHECK(title->list_root == kA && generation != 0);
    recorder.begin_list(kB);

    // Empty ticks are captures, not new builds of the untouched submitted buffer.
    for (std::size_t n = 0; n < FrameRecorder::kKept * 2; ++n) {
        recorder.close();
        const FrameRecording* again = recorder.match(kA, tags);
        CHECK(again != nullptr && again->list_generation == generation && again->serial == serial);
    }

    const FrameRecording* again = recorder.match(kA, tags);
    CHECK(again != nullptr && again->commands.size() == 2 && again->ui_quads[0].rgba == kColour);
}

// Replaces snapshots within one build without changing UI payload indices or dropping its prefix.
void test_owned_build_spans_capture_boundaries() {
    constexpr std::uint32_t kRoot = 0xC000;
    FrameRecorder recorder;
    recorder.begin_list(kRoot);
    recorder.set_anchor(kRoot);
    recorder.push(op(FrameOp::World));
    recorder.close();
    UiQuad first;
    first.rgba = 1;
    recorder.open().ui_quads.push_back(first);
    recorder.push(op(FrameOp::UiQuad, 0));
    recorder.close();
    UiQuad second;
    second.rgba = 2;
    recorder.open().ui_quads.push_back(second);
    recorder.push(op(FrameOp::UiQuad, 1));
    recorder.close();
    const std::uint32_t tags[]{kRoot};
    const FrameRecording* recording = recorder.match(kRoot, tags);
    CHECK(recording != nullptr);

    // The snapshot owns both payloads; neither was reconstructed from a tag or a frame counter.
    if (recording == nullptr) {
        return;
    }

    CHECK(recording->commands.size() == 3 && recording->ui_quads.size() == 2);
    CHECK(recording->commands[1].index == 0 && recording->ui_quads[0].rgba == 1);
    CHECK(recording->commands[2].index == 1 && recording->ui_quads[1].rgba == 2);
    const std::vector<std::size_t> expected{0, 0, 0};
    CHECK(place_commands(*recording, tags) == expected);
}

// Does not let a newer build of another root steal commands at a shared called-packet address.
void test_owned_roots_disambiguate_shared_anchors() {
    constexpr std::uint32_t kA = 0xD000;
    constexpr std::uint32_t kB = 0xE000;
    constexpr std::uint32_t kShared = 0xF000;
    FrameRecorder recorder;
    recorder.begin_list(kA);
    recorder.set_anchor(kShared);
    recorder.push(op(FrameOp::Picture, 1));
    recorder.begin_list(kB);
    recorder.set_anchor(kShared);
    recorder.push(op(FrameOp::Picture, 2));
    recorder.close();
    const std::uint32_t a_tags[]{kA, kShared};
    const std::uint32_t b_tags[]{kB, kShared};
    const FrameRecording* a = recorder.match(kA, a_tags);
    const FrameRecording* b = recorder.match(kB, b_tags);
    CHECK(a != nullptr && a->commands[0].index == 1);
    CHECK(b != nullptr && b->commands[0].index == 2);
    CHECK(recorder.match(a_tags) == nullptr);
}

// Preserves genuine partial-list UI while dropping only the commands whose places were not reached.
void test_owned_partial_and_truncated_resends() {
    constexpr std::uint32_t kRoot = 0x11000;
    constexpr std::uint32_t kWorld = kRoot + 0x40;
    constexpr std::uint32_t kUi = kRoot + 0x80;
    FrameRecorder recorder;
    recorder.begin_list(kRoot);
    recorder.set_anchor(kWorld);
    recorder.push(op(FrameOp::World));
    recorder.set_anchor(kUi);
    recorder.push(op(FrameOp::UiQuad));
    recorder.close();
    const std::uint32_t ui_only[]{kRoot, kUi};
    const std::uint32_t truncated[]{kRoot, kWorld};
    const FrameRecording* recording = recorder.match(kRoot, ui_only);
    CHECK(recording != nullptr);

    // A partial UI pass is legitimate when its build owns the submitted root.
    if (recording == nullptr) {
        return;
    }

    const std::vector<std::size_t> ui_places{kDropped, 1};
    const std::vector<std::size_t> cut_places{1, kDropped};
    CHECK(place_commands(*recording, ui_only) == ui_places);
    CHECK(recorder.match(kRoot, truncated) == recording);
    CHECK(place_commands(*recording, truncated) == cut_places);
    recorder.close();
    CHECK(recorder.match(kRoot, ui_only) == recording);
}

// Reusing every numerical tag still cannot revive an old payload after the producer resets its root.
void test_owned_identical_tag_layout_is_a_new_generation() {
    constexpr std::uint32_t kRoot = 0x12000;
    constexpr std::uint32_t kUi = kRoot + 0x20;
    FrameRecorder recorder;
    recorder.begin_list(kRoot);
    recorder.set_anchor(kUi);
    recorder.push(op(FrameOp::UiQuad, 1));
    recorder.close();
    const std::uint32_t identical_tags[]{kRoot, kUi};
    const FrameRecording* old = recorder.match(kRoot, identical_tags);
    CHECK(old != nullptr);
    const std::uint64_t old_generation = recorder.open().list_generation;
    recorder.begin_list(kRoot);
    recorder.close();
    CHECK(recorder.match(kRoot, identical_tags) == nullptr);
    recorder.set_anchor(kUi);
    recorder.push(op(FrameOp::UiQuad, 2));
    recorder.close();
    const FrameRecording* next = recorder.match(kRoot, identical_tags);
    CHECK(
        next != nullptr && next->list_generation != old_generation && next->commands[0].index == 2
    );
}

// Owned empty builds cannot escape through a stale standalone recording, but reset restores legacy use.
void test_owned_empty_build_blocks_standalone_fallback() {
    constexpr std::uint32_t kRoot = 0x13000;
    FrameRecorder recorder;
    recorder.push(op(FrameOp::Fill));
    recorder.close();
    CHECK(recorder.latest_unanchored() != nullptr);
    recorder.begin_list(kRoot);
    recorder.close();
    const std::uint32_t tags[]{kRoot};
    CHECK(recorder.latest_unanchored() == nullptr);
    CHECK(recorder.match(kRoot, tags) == nullptr);
    recorder.push(op(FrameOp::Fill));
    recorder.close();
    CHECK(recorder.match(kRoot, tags) != nullptr);
    CHECK(recorder.latest_unanchored() == nullptr);
    recorder.reset();
    recorder.push(op(FrameOp::Clear));
    recorder.close();
    CHECK(recorder.latest_unanchored() != nullptr);
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
    test_rebuilt_list_does_not_revive_stale_ui();
    test_owned_title_and_valid_resend_keep_payloads();
    test_owned_build_spans_capture_boundaries();
    test_owned_roots_disambiguate_shared_anchors();
    test_owned_partial_and_truncated_resends();
    test_owned_identical_tag_layout_is_a_new_generation();
    test_owned_empty_build_blocks_standalone_fallback();
    return openrac::test::result();
}
