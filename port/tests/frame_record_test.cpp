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

// A screen effect is recorded with its values beside the commands; closing keeps them with the
// recording, and a new recording starts with none.
void screen_effects_kept() {
    FrameRecorder recorder;
    ScreenEffect soften;
    soften.kind = ScreenEffectKind::Soften;
    soften.strength = 0x80;
    recorder.push_screen_effect(soften);
    recorder.close();
    const FrameRecording* r = recorder.latest_unanchored();
    CHECK(r != nullptr);
    if (r != nullptr) {
        CHECK(r->commands.size() == 1 && r->commands[0].op == FrameOp::ScreenEffect);
        CHECK(r->screen_effects.size() == 1 && r->screen_effects[0] == soften);
        FrameRecording copy = *r;
        copy.clear();
        CHECK(copy.screen_effects.empty() && copy.empty());
    }
    CHECK(recorder.open().screen_effects.empty());
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
    CHECK(std::strcmp(frame_op_name(FrameOp::ViewBegin), "ViewBegin") == 0);
    CHECK(std::strcmp(frame_op_name(FrameOp::ViewEnd), "ViewEnd") == 0);
    CHECK(std::strcmp(frame_op_name(FrameOp::ScreenEffect), "ScreenEffect") == 0);
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

// A depth rectangle (openrac_frame_fill_depth) keeps its producer's depth, test and writes in native
// values; values its buffer or the contract cannot hold leave it to the caller.
void test_depth_fill_command() {
    constexpr std::uint32_t kBar = 0xFFFFF3;  // the letterbox bars' 24-bit depth
    constexpr int kDepth24 = 24;
    const float rect[]{0.0f, 0.0f, 512.0f, 24.0f};
    const auto bar = depth_fill_command(
        rect,
        0x80000000u,
        BlendMode::Opaque,
        true,
        kBar,
        kDepth24,
        DepthCompare::GreaterEqual,
        true,
        kWriteColour
    );
    CHECK(bar.has_value());

    // What it carries, when it was made.
    if (bar) {
        CHECK(bar->op == FrameOp::Fill && !bar->whole && bar->wide);
        CHECK(bar->rect[3] == 24.0f && bar->rgba == 0x80000000u);
        CHECK(bar->depth.enabled && bar->depth.compare == DepthCompare::GreaterEqual);
        CHECK(bar->depth_write && bar->depth_bits == kDepth24 && bar->depth_value == kBar);
        CHECK(bar->writes == kWriteColour);
    }

    // Written without a test is still a depth pass; neither tested nor written is none.
    const auto stamp = depth_fill_command(
        nullptr, 0, BlendMode::Mix, true, 0, kDepth24, DepthCompare::Always, true, kWriteAlpha
    );
    const auto tint = depth_fill_command(
        nullptr, 0, BlendMode::Mix, true, 0, kDepth24, DepthCompare::Always, false, kWriteColour
    );
    CHECK(stamp && stamp->whole && stamp->depth.enabled && stamp->writes == kWriteAlpha);
    CHECK(tint && !tint->depth.enabled && !tint->depth_write);

    // Another bit count, a depth past the buffer, Never and a channel past the colour's.
    constexpr int kOddBits = 20;
    constexpr std::uint32_t kPastZ24 = 0x1000000;
    constexpr std::uint8_t kPastColour = 0x10;
    CHECK(!depth_fill_command(
        rect,
        0,
        BlendMode::Opaque,
        false,
        kBar,
        kOddBits,
        DepthCompare::GreaterEqual,
        true,
        kWriteColour
    ));
    CHECK(!depth_fill_command(
        rect,
        0,
        BlendMode::Opaque,
        false,
        kPastZ24,
        kDepth24,
        DepthCompare::GreaterEqual,
        true,
        kWriteColour
    ));
    CHECK(!depth_fill_command(
        rect, 0, BlendMode::Opaque, false, kBar, kDepth24, DepthCompare::Never, true, kWriteColour
    ));
    CHECK(!depth_fill_command(
        rect,
        0,
        BlendMode::Opaque,
        false,
        kBar,
        kDepth24,
        DepthCompare::GreaterEqual,
        true,
        kPastColour
    ));

    // A plain fill command keeps no depth of its own.
    const FrameCommand plain = op(FrameOp::Fill);
    CHECK(plain.depth_bits == 0 && !plain.depth.enabled && !plain.depth_write);
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

// Up Your Arsenal's split-screen maps record one World call a view, at its terrain; the sky before
// it and the ties, shrubs and mobys after it are named only by openrac_game_draw. Those are the
// open view's layers too, or the player's picture would hold terrain alone (no sky, no player).
void test_view_drawn_layers() {
    FrameRecorder recorder;
    recorder.view_layers(16);  // No view open: nobody's.
    recorder.view_begin(0, {0.0f, 0.0f, 341.0f, 224.0f});
    recorder.view_layers(1);
    recorder.push(op(FrameOp::World, 2));
    recorder.view_layers(2 | 4 | 8 | 16);
    recorder.view_begin(1, {170.0f, 224.0f, 512.0f, 448.0f});
    recorder.view_layers(16);  // A layout's own draws: still no World call.
    recorder.view_end();
    recorder.view_layers(4);  // After the views: the screen's, nobody's.
    const auto& views = recorder.open().views;
    CHECK(views.size() == 2);
    if (views.size() == 2) {
        CHECK(views[0].has_world && views[0].layers == 0x1Fu);
        CHECK(!views[1].has_world && views[1].layers == 16u);
    }
}

// Up Your Arsenal's two-player layout is staggered: player 0 top left and player 1 bottom right,
// each two thirds of the screen wide. They share x 170 to 341 and touch at y 224 (208 on NTSC-U's
// 416-line screen) without overlapping, so both players' pictures stay in one layout.
void test_staggered_views() {
    FrameView upper;
    upper.view = 0;
    upper.rect = {0.0f, 0.0f, 341.0f, 224.0f};
    FrameView lower;
    lower.view = 1;
    lower.rect = {170.0f, 224.0f, 512.0f, 448.0f};
    CHECK(upper.compatible_with(lower) && lower.compatible_with(upper));
    upper.rect[3] = lower.rect[1] = 208.0f;
    lower.rect[3] = upper.screen_height = lower.screen_height = 416.0f;
    CHECK(upper.compatible_with(lower) && lower.compatible_with(upper));
}

// The same maps send each player's view in a display list of its own with no vertical blank
// between them, so every send ends a tick: each capture closes one player's view while the list
// it walks is the other player's, sent before and built with the recording before. That list
// matches the older recording; the views a tick draws are the latest recording's. A capture with
// nothing recorded keeps the latest.
void test_latest_views() {
    FrameRecorder recorder;
    CHECK(recorder.latest() == nullptr);
    recorder.view_begin(0, {0.0f, 0.0f, 341.0f, 224.0f});
    recorder.set_anchor(0x0068F1A0u);
    recorder.push(op(FrameOp::World, 2));
    recorder.close();  // Player 0's view, into list B.
    recorder.view_begin(1, {170.0f, 224.0f, 512.0f, 448.0f});
    recorder.set_anchor(0x0056F1A0u);
    recorder.push(op(FrameOp::World, 2));
    recorder.close();  // Player 1's view, into list A, while the window walks list B.
    const std::vector<std::uint32_t> list_b = {0x0068F000u, 0x0068F1A0u};
    const FrameRecording* matched = recorder.match(list_b);
    const FrameRecording* latest = recorder.latest();
    CHECK(matched != nullptr && latest != nullptr && matched != latest);
    if (matched != nullptr && latest != nullptr && matched->views.size() == 1
        && latest->views.size() == 1) {
        CHECK(matched->views[0].view == 0);
        CHECK(latest->views[0].view == 1 && latest->views[0].has_world);
        CHECK(latest->serial == recorder.latest_serial());
    }
    recorder.close();  // Nothing recorded since.
    CHECK(recorder.latest() == latest);
    recorder.reset();
    CHECK(recorder.latest() == nullptr);
}

// Keeps the payload with its command when appending an effect closes a full recording.
void test_screen_effect_rollover() {
    FrameRecorder recorder;
    recorder.open().commands.resize(FrameRecorder::kMostCommands);
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    recorder.push_screen_effect(glow);

    CHECK(recorder.open().commands.size() == 1);
    CHECK(recorder.open().screen_effects.size() == 1);

    // A missing payload is already a failure; do not read it while reporting that failure.
    if (recorder.open().commands.size() != 1 || recorder.open().screen_effects.size() != 1) {
        return;
    }

    CHECK(recorder.open().commands.front().index == 0);
    CHECK(recorder.open().screen_effects.front() == glow);
    const FrameRecording* previous = recorder.latest_unanchored();
    CHECK(previous != nullptr);

    // The full recording contains no command for the newly appended effect.
    if (previous != nullptr) {
        CHECK(previous->commands.size() == FrameRecorder::kMostCommands);
        CHECK(previous->screen_effects.empty());
    }
}

// Builds the same reachable command subset as the list walker, using synthetic tag addresses.
std::vector<FrameSplice> submitted_commands(
    const FrameRecording& recording, std::span<const std::uint32_t> tags
) {
    const std::vector<std::size_t> places = place_commands(recording, tags);
    std::vector<FrameSplice> submitted;

    // Missing and inherited drops must not become drawable views or post effects.
    for (std::size_t n = 0; n < places.size(); ++n) {
        if (places[n] != kDropped) {
            submitted.push_back({0, static_cast<std::uint32_t>(n)});
        }
    }

    return submitted;
}

// A cut list selects only reachable views and layers, even when later explicit anchors resume.
void test_submitted_views_exclude_unreachable_worlds() {
    FrameRecorder recorder;
    recorder.set_anchor(0x1000);  // First reachable view and layer.
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    recorder.set_anchor(0x2000);  // A missing extra layer and the whole next view.
    recorder.push(op(FrameOp::World, 2));
    recorder.view_begin(1, {0, 224, 512, 448});
    recorder.push(op(FrameOp::World, 4));
    recorder.set_anchor(0x3000);  // A later explicit anchor resumes, despite the preceding drops.
    recorder.view_begin(2, {0, 0, 256, 224});
    recorder.push(op(FrameOp::World, 8));
    FrameRecording recording = recorder.open();
    const std::uint32_t tags[]{0x1000, 0x3000};
    CHECK(recording.resolve_views(submitted_commands(recording, tags)));
    CHECK(recording.views[0].has_world && recording.views[0].layers == 1);
    CHECK(!recording.views[1].has_world && recording.views[1].layers == 0);
    CHECK(recording.views[2].has_world && recording.views[2].layers == 8);
    // A fully cut list keeps old pictures, not a new full-screen world.
    CHECK(recording.resolve_views({}));
    CHECK(recording.has_split_world());
    CHECK(!recording.views[0].has_world && !recording.views[2].has_world);
}

// A reachable World retains its captured camera and viewport even if the begin marker was truncated.
void test_world_owner_does_not_depend_on_begin_reachability() {
    FrameRecorder recorder;
    // The transitional begin marker is absent from the submitted list.
    recorder.set_anchor(0x2000);
    recorder.view_begin(1, {0, 224, 512, 448});
    FrameViewCamera camera;
    camera.position = {1, 2, 3};  // Synthetic camera, owned by the recording rather than game RAM.
    recorder.view_camera(camera);
    recorder.set_anchor(0x3000);  // A later explicit world anchor is reached.
    recorder.push(op(FrameOp::World, 8));
    FrameRecording recording = recorder.open();
    const std::uint32_t tags[]{0x3000};
    CHECK(recording.resolve_views(submitted_commands(recording, tags)));
    CHECK(recording.views[0].has_world && recording.views[0].layers == 8);
    CHECK(recording.views[0].view == 1 && recording.views[0].camera == camera);
    CHECK(recording.views[0].rect[1] == 224.0f);
}

// A post-effect payload remains diagnostic evidence but is not scheduled when its anchor is absent.
void test_unreachable_view_effect_is_not_scheduled() {
    FrameRecorder recorder;
    recorder.set_anchor(0x1000);  // Only the world reaches the submitted list.
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    recorder.set_anchor(0x2000);  // Neither this effect nor its unanchored followers may execute.
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    recorder.push_screen_effect(glow);
    recorder.set_anchor(0);
    recorder.view_end();
    FrameRecording recording = recorder.open();
    const std::uint32_t tags[]{0x1000};
    CHECK(recording.resolve_views(submitted_commands(recording, tags)));
    CHECK(recording.views[0].has_world);
    CHECK(recording.views[0].screen_effects.empty());
    CHECK(recording.screen_effects[0] == glow);
    CHECK(!recording.commands[2].view_effect_prepared);
}

// Each terminal post pass belongs to one view, while a later unscoped effect remains global.
void test_view_effect_submission_owns_payloads() {
    FrameRecorder recorder;
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    ScreenEffect soften;
    soften.kind = ScreenEffectKind::Soften;
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    recorder.push_screen_effect(glow);
    recorder.view_end();
    recorder.view_begin(1, {0, 224, 512, 448});
    recorder.push(op(FrameOp::World, 1));
    recorder.push_screen_effect(soften);
    recorder.view_end();
    recorder.push_screen_effect(glow);
    FrameRecording recording = recorder.open();
    const std::vector<FrameSplice> submitted = submitted_commands(recording, {});
    CHECK(recording.resolve_views(submitted));
    CHECK(recording.commands[2].view == 0 && recording.commands[2].view_effect_prepared);
    CHECK(recording.commands[6].view == 1 && recording.commands[6].view_effect_prepared);
    CHECK(recording.commands[8].view == kNoFrameView);
    CHECK(!recording.commands[8].view_effect_prepared);
    CHECK((recording.views[0].screen_effects == std::vector<std::uint32_t>{0}));
    CHECK((recording.views[1].screen_effects == std::vector<std::uint32_t>{1}));
    recorder.open().screen_effects[0].kind = ScreenEffectKind::None;
    CHECK(recording.screen_effects[0] == glow);
    CHECK(recording.resolve_views(submitted));  // Resolving a copied tick twice is idempotent.
    CHECK(recording.views[0].screen_effects.size() == 1);
}

// Explicit panel effects keep their recorded target rather than being routed to the player's world.
void test_view_panel_effect_keeps_its_target_scope() {
    FrameRecorder recorder;
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    recorder.push(op(FrameOp::TargetBegin, kRecordedTargets));
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    recorder.push_screen_effect(glow);
    recorder.push(op(FrameOp::TargetEnd));
    recorder.push_screen_effect(glow);
    FrameRecording recording = recorder.open();
    CHECK(recording.resolve_views(submitted_commands(recording, {})));
    CHECK(!recording.commands[3].view_effect_prepared);
    CHECK(recording.commands[5].view_effect_prepared);
    CHECK((recording.views[0].screen_effects == std::vector<std::uint32_t>{1}));
}

// Unsupported ordering fails before publishing a partial schedule or inferring a global effect.
void test_view_effect_rejects_orphans_and_interleaved_drawing() {
    FrameRecorder recorder;
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    recorder.push_screen_effect(glow);
    FrameRecording orphan = recorder.open();
    const FrameSplice effect_only{0, 2};  // The effect is reachable, but its world was truncated.
    CHECK(!orphan.resolve_views({&effect_only, 1}));
    CHECK(!orphan.commands[2].view_effect_prepared);
    recorder.push(op(FrameOp::World, 2));  // Later drawing cannot move ahead of the first effect.
    FrameRecording interleaved = recorder.open();
    CHECK(!interleaved.resolve_views(submitted_commands(interleaved, {})));
    CHECK(!interleaved.commands[2].view_effect_prepared);
    recorder.reset();
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    // Native UI before the effect needs a richer per-view stream.
    recorder.push(op(FrameOp::Fill));
    recorder.push_screen_effect(glow);
    FrameRecording ui_before = recorder.open();
    CHECK(!ui_before.resolve_views(submitted_commands(ui_before, {})));
}

// Rollover keeps a post effect's view identity and payload instead of promoting it to the whole frame.
void test_view_effect_rollover_preserves_owner() {
    FrameRecorder recorder;
    recorder.set_anchor(0x1000);  // The owner and its resumed effect still belong to this list tag.
    recorder.view_begin(1, {0, 224, 512, 448});
    recorder.push(op(FrameOp::World, 1));
    recorder.open().commands.resize(FrameRecorder::kMostCommands);
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    recorder.push_screen_effect(glow);
    CHECK(recorder.open().commands.size() == 1);
    CHECK(recorder.open().views.size() == 1);
    CHECK(recorder.open().views[0].view == 1);
    CHECK(recorder.open().commands[0].view == 0);
    CHECK(recorder.open().commands[0].anchor == 0x1000);
    CHECK(recorder.open().screen_effects[0] == glow);
    FrameRecording orphan = recorder.open();
    const std::uint32_t tags[]{0x1000};
    CHECK(!orphan.resolve_views(submitted_commands(orphan, tags)));
    recorder.close();  // Closing a full open view must not recurse through push() indefinitely.
}

// Unproved colour ordering fails explicitly; global effects retain their existing context policy.
void test_view_effect_context_contract() {
    FrameRecorder recorder;
    recorder.view_begin(0, {0, 0, 512, 224});
    recorder.push(op(FrameOp::World, 1));
    ScreenEffect glow;
    glow.kind = ScreenEffectKind::Glow;
    recorder.push_screen_effect(glow);
    FrameRecording recording = recorder.open();
    CHECK(recording.resolve_views(submitted_commands(recording, {})));
    CHECK(recording.validate_view_effect_context({true, false, false}));
    CHECK(!recording.validate_view_effect_context({false, false, false}));
    CHECK(!recording.validate_view_effect_context({true, true, false}));
    CHECK(!recording.validate_view_effect_context({true, false, true}));
    recorder.reset();
    // An unscoped effect remains valid with the legacy 2D stream.
    recorder.push_screen_effect(glow);
    CHECK(recorder.open().validate_view_effect_context({false, true, true}));
}

}  // namespace

int main() {
    order_without_dma();
    anchors_place_commands();
    late_and_resent_lists();
    screen_effects_kept();
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
    test_depth_fill_command();
    screen_size();
    views();
    test_invalid_view_rectangles();
    test_retained_view_layouts();
    test_view_world_layers();
    test_view_drawn_layers();
    test_staggered_views();
    test_latest_views();
    test_screen_effect_rollover();
    test_submitted_views_exclude_unreachable_worlds();
    test_world_owner_does_not_depend_on_begin_reachability();
    test_unreachable_view_effect_is_not_scheduled();
    test_view_effect_submission_owns_payloads();
    test_view_panel_effect_keeps_its_target_scope();
    test_view_effect_rejects_orphans_and_interleaved_drawing();
    test_view_effect_rollover_preserves_owner();
    test_view_effect_context_contract();
    return openrac::test::result();
}
