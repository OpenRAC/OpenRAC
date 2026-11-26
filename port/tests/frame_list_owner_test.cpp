// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The real RAC1 C list-reset/swap wrappers through a synthetic original operation and the actual
// CPU FrameRecorder matching used by the frontend. Invented cursor/tag addresses expose ownership
// collisions without game bytes, packet headers, frame numbers, textures or screenshots.

#include <array>
#include <cstdint>

#include "frame_list_owner/game_protos.h"
#include "openrac/frame.h"
#include "renderer/frame/frame_record.h"
#include "tests/check.h"

namespace {

// Invented RAM addresses; only the fixture's cursor word is dereferenced, never a destination list.
constexpr gaddr kCursorWord = 0x10;
constexpr gaddr kA = 0x16000;
constexpr gaddr kB = 0x17000;
constexpr gaddr kUi = kA + 0x60;

// An unrelated effect of each synthetic original operation, retained by the production wrapper.
constexpr std::uint32_t kInitEffect = 0x12;
constexpr std::uint32_t kSwapEffect = 0x34;

// The C callbacks carry no context pointer; each synchronous fixture lends them its owned state.
struct Fixture;
Fixture* g_fixture = nullptr;

// Owns the invented game word and recorder without a runtime memory reservation or a game program.
struct Fixture {
    std::array<std::uint32_t, 16> words{};      // Readable synthetic cursor storage.
    openrac::renderer::FrameRecorder recorder;  // The actual recording/ownership implementation.
    gaddr next = kA;                            // Root the next original operation selects.
    int init_calls = 0;                         // Number of delegated initialization operations.
    int swap_calls = 0;                         // Number of delegated swap operations.
    std::uint32_t side_effect = 0;              // An original operation's unrelated output.

    // Installs the fixture for the synchronous C wrapper and native recorder callback.
    Fixture() {
        CHECK(g_fixture == nullptr);
        g_fixture = this;
        openrac_guest_base = reinterpret_cast<std::uint8_t*>(words.data());
        openrac_guest_ram_size = GUEST_RAM_SIZE;
        words[kCursorWord / sizeof(std::uint32_t)] = kB;
    }

    // Revokes the borrowed game storage and callback state before their owner is destroyed.
    ~Fixture() {
        openrac_guest_base = nullptr;
        g_fixture = nullptr;
    }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;

    // Records a synthetic UI payload at the caller's invented reached cursor.
    void record_ui(std::uint32_t colour) {
        openrac::renderer::UiQuad quad;
        quad.rgba = colour;
        recorder.open().ui_quads.push_back(quad);
        recorder.set_anchor(kUi);
        openrac::renderer::FrameCommand command;
        command.op = openrac::renderer::FrameOp::UiQuad;
        recorder.push(command);
        recorder.close();
    }
};

// Reads the root selected after, not before, the real reset wrapper's delegated operation.
void test_real_reset_wrapper_retires_old_ui() {
    Fixture fixture;
    func_002348E8();
    CHECK(fixture.init_calls == 1 && fixture.swap_calls == 0);
    CHECK(fixture.side_effect == kInitEffect);
    CHECK(fixture.recorder.open().list_root == kA);
    fixture.record_ui(kInitEffect);
    const std::uint32_t tags[]{kA, kUi};
    CHECK(fixture.recorder.match(kA, tags) != nullptr);
    const std::uint64_t generation = fixture.recorder.open().list_generation;

    // A replacement with identical tag addresses records no UI; numerical membership is not ownership.
    func_002348E8();
    fixture.recorder.close();
    CHECK(fixture.init_calls == 2 && fixture.side_effect == kInitEffect);
    CHECK(fixture.recorder.open().list_generation != generation);
    CHECK(fixture.recorder.match(kA, tags) == nullptr);
    CHECK(fixture.recorder.latest_unanchored() == nullptr);
}

// Starts ownership in the newly selected buffer while the old submitted buffer remains intact.
void test_real_swap_wrapper_preserves_delayed_submission() {
    Fixture fixture;
    func_002348E8();
    fixture.record_ui(kInitEffect);
    fixture.next = kB;
    func_00234948();
    fixture.recorder.close();
    CHECK(fixture.init_calls == 1 && fixture.swap_calls == 1);
    CHECK(fixture.side_effect == kSwapEffect);
    CHECK(fixture.words[kCursorWord / sizeof(std::uint32_t)] == kB);
    CHECK(fixture.recorder.open().list_root == kB);
    const std::uint32_t a_tags[]{kA, kUi};
    const std::uint32_t b_tags[]{kB, kUi};
    const auto* delayed = fixture.recorder.match(kA, a_tags);
    CHECK(delayed != nullptr && delayed->ui_quads[0].rgba == kInitEffect);
    CHECK(fixture.recorder.match(kB, b_tags) == nullptr);
}

// Uses the C boundary's shared RAM normalization and rejects invalid roots without standalone escape.
void test_root_alias_and_invalid_root_are_not_new_matching_heuristics() {
    Fixture fixture;
    fixture.next = GUEST_RAM_UNCACHED + kA;
    func_002348E8();
    fixture.record_ui(kInitEffect);
    const std::uint32_t tags[]{kA, kUi};
    CHECK(fixture.recorder.open().list_root == kA);
    CHECK(fixture.recorder.match(kA, tags) != nullptr);
    fixture.next = 0;
    func_002348E8();
    fixture.recorder.push({.op = openrac::renderer::FrameOp::Fill});
    fixture.recorder.close();
    CHECK(fixture.recorder.open().list_root == UINT32_MAX);
    CHECK(fixture.recorder.match(kB, tags) == nullptr);
    CHECK(fixture.recorder.latest_unanchored() == nullptr);
}

}  // namespace

extern "C" {

// The real C wrapper reads this fixture's word through the normal guest-memory contract.
std::uint8_t* openrac_guest_base = nullptr;
std::uint32_t openrac_guest_ram_size = GUEST_RAM_SIZE;

// Maps the production wrapper's datum without copying any game's address into this test.
gaddr openrac_test_data_address(gaddr /*canonical*/) {
    return kCursorWord;
}

// An independent original-operation side effect, not a reproduced game initializer.
void func_002348E8__game(void) {
    ++g_fixture->init_calls;
    g_fixture->side_effect = kInitEffect;
    g_fixture->words[kCursorWord / sizeof(std::uint32_t)] = g_fixture->next;
}

// A distinct original-operation side effect exposes a swap that was skipped or called twice.
void func_00234948__game(void) {
    ++g_fixture->swap_calls;
    g_fixture->side_effect = kSwapEffect;
    g_fixture->words[kCursorWord / sizeof(std::uint32_t)] = g_fixture->next;
}

// Captures the C API as game_host.cpp does: normalized ownership enters the real recorder.
void openrac_frame_list_begin(gaddr root) {
    g_fixture->recorder.begin_list(openrac_frame_ram_cursor(root));
}

}  // extern "C"

// Runs the production C producer-edge checks against the same core selection used by the frontend.
int main() {
    test_real_reset_wrapper_retires_old_ui();
    test_real_swap_wrapper_preserves_delayed_submission();
    test_root_alias_and_invalid_root_are_not_new_matching_heuristics();
    return openrac::test::result();
}
