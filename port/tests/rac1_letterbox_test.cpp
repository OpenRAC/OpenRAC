// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The real RAC1 PAL letterbox wrapper (game/rac1/host/letterbox.c) over a synthetic original
// operation and the actual CPU FrameRecorder commands. The words the wrapper names (enable, height,
// list cursor, level, game mode, screen extent) live in the fixture; the cursor values are
// invented RAM addresses, and no packet byte is read or written.

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "openrac/frame.h"
#include "rac1_letterbox/game_protos.h"
#include "renderer/frame/frame_record.h"
#include "tests/check.h"

namespace {

using openrac::renderer::DepthCompare;
using openrac::renderer::FrameCommand;
using openrac::renderer::FrameRecorder;

// The wrapper's data (letterbox.c, PAL v2.00 addresses) and where each lives in the fixture.
constexpr gaddr kEnable = 0x0015F544, kHeight = 0x0015F548, kCursor = 0x00161000;
constexpr gaddr kLevel = 0x0015EE84, kExtent = 0x0013E600;
constexpr std::size_t kEnableWord = 0, kHeightWord = 1, kCursorWord = 2, kLevelWord = 3;
constexpr std::size_t kExtentWord = 4;  // width, then height
constexpr gaddr kMode = 0x0015F6A8;     // level 0's game mode, a level datum
constexpr std::size_t kModeWord = 6;    // 0, play, unless a test sets it

// Invented reached list positions; the bars' packet is 0x80 bytes (draw_letterbox_bars).
constexpr gaddr kList = 0x16000;
constexpr gaddr kPacket = 0x80;
constexpr int kBlarg = 6, kKerwan = 3;
constexpr std::uint32_t kFull = 24;            // the bars' largest height
constexpr std::uint32_t kBarDepth = 0xFFFFF3;  // draw_letterbox_bars' vertex depth
constexpr int kDepthBits = 24;
constexpr int kWidth = 512, kHeight448 = 448, kHeight416 = 416;

struct Fixture;
Fixture* g_fixture = nullptr;

// Owns the game words, the recorder and the synthetic operation's bookkeeping.
struct Fixture {
    std::array<std::uint32_t, 16> words{};  // the mapped game words
    FrameRecorder recorder;                 // the actual recording implementation
    int game_calls = 0;                     // how often the original operation ran
    gaddr appended = kPacket;               // what the synthetic operation appends when it draws
    bool window = true;                     // whether openrac_frame_fill_depth draws
    std::string skip;                       // OPENRAC_UI_SKIP

    // Installs the fixture: Blarg at the 448-line screen by default, the cursor at kList.
    Fixture(int enable, std::uint32_t height, int level = kBlarg, int screen_height = kHeight448) {
        CHECK(g_fixture == nullptr);
        g_fixture = this;
        openrac_guest_base = reinterpret_cast<std::uint8_t*>(words.data());
        openrac_guest_ram_size = GUEST_RAM_SIZE;
        words[kEnableWord] = static_cast<std::uint32_t>(enable);
        words[kHeightWord] = height;
        words[kCursorWord] = kList;
        words[kLevelWord] = static_cast<std::uint32_t>(level);
        words[kExtentWord] = kWidth;
        words[kExtentWord + 1] = static_cast<std::uint32_t>(screen_height);
    }

    // Revokes the borrowed storage before its owner goes.
    ~Fixture() {
        openrac_guest_base = nullptr;
        g_fixture = nullptr;
    }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;

    // The native commands recorded so far.
    const std::vector<FrameCommand>& commands() { return recorder.open().commands; }
};

// Checks one recorded bar: the packet's depth, test, writes and colour, over the whole width.
void check_bar(const FrameCommand& c, float y0, float y1, gaddr anchor) {
    constexpr float kTolerance = 0.0001f;
    CHECK(c.op == openrac::renderer::FrameOp::Fill && !c.whole && c.wide);
    CHECK(c.rect[0] == 0.0f && c.rect[2] == static_cast<float>(kWidth));
    CHECK(c.rect[1] > y0 - kTolerance && c.rect[1] < y0 + kTolerance);
    CHECK(c.rect[3] > y1 - kTolerance && c.rect[3] < y1 + kTolerance);
    CHECK(c.rgba == 0x80000000u && c.blend == openrac::renderer::BlendMode::Opaque);
    CHECK(c.depth.enabled && c.depth.compare == DepthCompare::GreaterEqual && c.depth_write);
    CHECK(c.depth_bits == kDepthBits && c.depth_value == kBarDepth);
    CHECK(c.writes == openrac::renderer::kWriteColour && c.anchor == anchor);
}

// Enabled on Blarg: one game step (18 to 19), the packet taken back, both bars recorded natively.
void test_enabled_bars_are_recorded_once() {
    constexpr std::uint32_t kSeed = 18;  // breakable_window_update's seed
    Fixture fixture(1, kSeed);
    func_001F5148();
    CHECK(fixture.game_calls == 1);
    CHECK(fixture.words[kHeightWord] == kSeed + 1);
    CHECK(fixture.words[kCursorWord] == kList);
    CHECK(fixture.commands().size() == 2);

    // The top bar then the bottom one, both at the packet's first byte.
    if (fixture.commands().size() == 2) {
        const float bar = static_cast<float>(kSeed + 1);
        check_bar(fixture.commands()[0], 0.0f, bar, kList);
        check_bar(fixture.commands()[1], kHeight448 - bar, kHeight448, kList);
    }

    // At the full height the game's step holds 24.
    fixture.words[kHeightWord] = kFull;
    func_001F5148();
    CHECK(fixture.game_calls == 2 && fixture.words[kHeightWord] == kFull);
    CHECK(fixture.commands().size() == 4 && fixture.words[kCursorWord] == kList);
}

// Shrinking in play: the HUD ran first and left the bars' state (measured in game), so one game
// step (24 to 23), the packet taken back and both bars recorded at the shrunk height.
void test_shrinking_bars_in_play_are_recorded() {
    Fixture fixture(0, kFull);
    func_001F5148();
    CHECK(fixture.game_calls == 1 && fixture.words[kHeightWord] == kFull - 1);
    CHECK(fixture.words[kCursorWord] == kList);
    CHECK(fixture.commands().size() == 2);

    // The top bar then the bottom one, at the height the game's step left.
    if (fixture.commands().size() == 2) {
        const float bar = static_cast<float>(kFull - 1);
        check_bar(fixture.commands()[0], 0.0f, bar, kList);
        check_bar(fixture.commands()[1], kHeight448 - bar, kHeight448, kList);
    }

    // The last step (1 to 0) appends nothing: nothing to take back or to record.
    fixture.words[kHeightWord] = 1;
    func_001F5148();
    CHECK(fixture.game_calls == 2 && fixture.words[kHeightWord] == 0);
    CHECK(fixture.words[kCursorWord] == kList && fixture.commands().size() == 2);
}

// The freeze dialog's shrinking frames (game mode 4) were not measured: they keep the packet. While
// the bars are enabled its frames are the proved edge, recorded.
void test_freeze_dialog_shrink_keeps_its_packet() {
    constexpr std::uint32_t kFreezeDialog = 4;
    Fixture fixture(0, kFull);
    fixture.words[kModeWord] = kFreezeDialog;
    func_001F5148();
    CHECK(fixture.game_calls == 1 && fixture.words[kHeightWord] == kFull - 1);
    CHECK(fixture.words[kCursorWord] == kList + kPacket);
    CHECK(fixture.commands().empty());

    // Enabled again in the same mode: the packet appended after the kept one is taken back.
    fixture.words[kEnableWord] = 1;
    func_001F5148();
    CHECK(fixture.words[kHeightWord] == kFull && fixture.words[kCursorWord] == kList + kPacket);
    CHECK(fixture.commands().size() == 2);
}

// Runs the wrapper once on a fixture prepared by `setup` and checks that the packet stayed.
template <typename Setup>
void check_packet_kept(int level, gaddr appended, Setup setup) {
    Fixture fixture(1, kFull, level);
    fixture.appended = appended;
    setup(fixture);
    func_001F5148();
    CHECK(fixture.game_calls == 1 && fixture.words[kCursorWord] == kList + appended);
    CHECK(fixture.commands().empty());
}

// Another level, no window, the comparison switch and an unexpected packet keep the packet.
void test_unproved_cases_keep_their_packet() {
    constexpr gaddr kOtherPacket = 0x40;  // not the bars' packet
    check_packet_kept(kKerwan, kPacket, [](Fixture&) {});
    check_packet_kept(kBlarg, kPacket, [](Fixture& f) { f.window = false; });
    check_packet_kept(kBlarg, kPacket, [](Fixture& f) { f.skip = "func_001F5800,func_001F5148"; });
    check_packet_kept(kBlarg, kOtherPacket, [](Fixture&) {});

    // The same while the bars shrink in play.
    check_packet_kept(kKerwan, kPacket, [](Fixture& f) { f.words[kEnableWord] = 0; });
    check_packet_kept(kBlarg, kPacket, [](Fixture& f) {
        f.words[kEnableWord] = 0;
        f.skip = "func_001F5148";
    });
}

// An aliased cursor anchors at its RAM address and is put back as the game wrote it; 416 lines keep
// the bars' game-pixel height in the recorder's 448-line space.
void test_aliased_cursor_and_416_lines() {
    Fixture fixture(1, kFull - 1, kBlarg, kHeight416);
    fixture.words[kCursorWord] = GUEST_RAM_UNCACHED + kList;
    func_001F5148();
    CHECK(fixture.words[kCursorWord] == GUEST_RAM_UNCACHED + kList);
    CHECK(fixture.commands().size() == 2);

    // The 416-line bars, scaled into the 448-line space as the residual triangles were.
    if (fixture.commands().size() == 2) {
        const float bar = static_cast<float>(kFull) * kHeight448 / kHeight416;
        check_bar(fixture.commands()[0], 0.0f, bar, kList);
        check_bar(fixture.commands()[1], kHeight448 - bar, kHeight448, kList);
    }
}

}  // namespace

extern "C" {

// The real C wrapper reads the fixture's words through the normal guest-memory contract.
std::uint8_t* openrac_guest_base = nullptr;
std::uint32_t openrac_guest_ram_size = GUEST_RAM_SIZE;

// Maps each datum the wrapper names to its fixture word; any other address is a test failure.
gaddr openrac_test_data_address(gaddr canonical) {
    const auto at = [](std::size_t word) {
        return static_cast<gaddr>(word * sizeof(std::uint32_t));
    };

    // One word per name, the screen extent's two words from its start.
    switch (canonical) {
        case kEnable:
            return at(kEnableWord);
        case kHeight:
            return at(kHeightWord);
        case kCursor:
            return at(kCursorWord);
        case kLevel:
            return at(kLevelWord);
        case kExtent:
            return at(kExtentWord);
        default:
            CHECK(false);
            return at(kEnableWord);
    }
}

// The game mode, read as level 0's datum (OPENRAC_LDATA); any other is a test failure.
gaddr openrac_relocate_level_data(int from, gaddr address) {
    CHECK(from == 0 && address == kMode);
    return static_cast<gaddr>(kModeWord * sizeof(std::uint32_t));
}

// The original operation: the game's step of the height and, when it is not 0, its packet.
void func_001F5148__game(void) {
    Fixture& f = *g_fixture;
    ++f.game_calls;
    std::uint32_t& height = f.words[kHeightWord];

    // While enabled the bars grow to 24, otherwise they shrink to 0 (draw_letterbox_bars).
    if (f.words[kEnableWord] != 0) {
        height = height < kFull ? height + 1 : height;
    } else if (height != 0) {
        --height;
    }

    // A height of 0 appends nothing.
    if (height != 0) {
        f.words[kCursorWord] += f.appended;
    }
}

// OPENRAC_UI_SKIP, as the fixture sets it.
const char* openrac_env_string(const char* name) {
    CHECK(std::strcmp(name, "OPENRAC_UI_SKIP") == 0);
    return g_fixture->skip.empty() ? nullptr : g_fixture->skip.c_str();
}

void openrac_frame_anchor(gaddr list_cursor) {
    g_fixture->recorder.set_anchor(list_cursor);
}

// As recorder.cpp: the C values to the native command, recorded only with a window.
int openrac_frame_fill_depth(
    const float* rect,
    std::uint32_t rgba,
    int blend,
    std::uint32_t depth,
    int depth_bits,
    int depth_test,
    unsigned writes
) {
    // The wrapper asks for this one test and these writes; any other is a test failure.
    CHECK(depth_test == OPENRAC_DEPTH_GEQUAL);
    CHECK(writes == (OPENRAC_WRITE_COLOUR | OPENRAC_WRITE_DEPTH));
    const auto mode = static_cast<openrac::renderer::BlendMode>(blend & 0xFF);  // the low byte
    const bool wide = (blend & OPENRAC_PICTURE_WHOLE) != 0 || rect == nullptr;
    const auto c = openrac::renderer::depth_fill_command(
        rect,
        rgba,
        mode,
        wide,
        depth,
        depth_bits,
        DepthCompare::GreaterEqual,
        true,
        openrac::renderer::kWriteColour
    );

    // No window, or a value the recorder refuses: nothing drawn.
    if (!g_fixture->window || !c) {
        return 0;
    }

    g_fixture->recorder.push(*c);
    return 1;
}

// The wrapper never asks for a plain fill or a clear.
void openrac_frame_fill(const float* /*rect*/, std::uint32_t /*rgba*/, int /*blend*/) {
    CHECK(false);
}

void openrac_frame_clear(std::uint32_t /*rgba*/, int /*what*/) {
    CHECK(false);
}

}  // extern "C"

// Runs the production wrapper on the proved edge and on every case that keeps the packet.
int main() {
    test_enabled_bars_are_recorded_once();
    test_shrinking_bars_in_play_are_recorded();
    test_freeze_dialog_shrink_keeps_its_packet();
    test_unproved_cases_keep_their_packet();
    test_aliased_cursor_and_416_lines();
    return openrac::test::result();
}
