// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the real rac3 host/pad.c and shared lib/pad.c with synthetic RAM and input, not game code.
// Field offsets and report sizes are the contracts documented in host/pad.c and pad_report.h.
// Fake game readers record which copy was called and what the mod/callback pipeline handed it.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "check.h"
#include "game/common/lib/pad_report.h"
#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "rac3_version.h"

extern "C" {
void func_003A2FC0(void);
void func_003A3B40(gaddr pad);
#if defined(RAC3_PAL)
void func_003A2BC0__frontbin(void);
int func_L01_00460868(void);
#endif
void func_003A3740__frontbin(gaddr pad);
int func_L01_004613E8(int pad);
}

namespace {

// The SP pad record and globals, as named in rac3-pal/host/pad.c (seen in game code).
constexpr gaddr kPads = 0x001CD0C0u;
constexpr gaddr kActive = 0x001D52FCu;
constexpr gaddr kPortOne = 0x001D5300u;
constexpr gaddr kPortOneToo = 0x001D5304u;
constexpr gaddr kNotReady = 0x001D4CF4u;
constexpr gaddr kNone = 0x001D6DF0u;
constexpr gaddr kPadBytes = 0x5C0u;
constexpr gaddr kState = 0x198u;
constexpr gaddr kLibraryState = 0x19Cu;
constexpr gaddr kHeld = 0x1A0u;
constexpr gaddr kPreviousHeld = 0x1ACu;
constexpr gaddr kRawHeld = 0x1B0u;
constexpr gaddr kPreviousRawHeld = 0x1BCu;
constexpr gaddr kLatchedHeld = 0x348u;
constexpr gaddr kPort = 0x54Cu;
constexpr gaddr kSlot = 0x557u;
constexpr gaddr kSeen = 0x558u;
constexpr gaddr kCallback = 0x56Cu;
constexpr gaddr kCallbackArgument = 0x570u;
constexpr gaddr kReport = 0x574u;
constexpr gaddr kSmallMotor = 0x188u;
constexpr gaddr kLargeMotor = 0x189u;
constexpr int kReady = 99;
constexpr int kReadyBytes = 20;
constexpr int kEmptyBytes = 32;

// Synthetic callback identity and observable byte edits at successive stages, not retail data.
constexpr gaddr kCallbackAddress = 0x10000u;
constexpr std::uint32_t kCallbackArgumentValue = 0x1234u;
constexpr std::uint8_t kModAxis = 0x21;
constexpr std::uint8_t kAfterAxis = 0x43;
constexpr std::uint8_t kCallbackAxis = 0x65;

// The wrapper copy whose game reader or clear was invoked.
enum class Copy {
    Boot,
    Frontbin,
    Level
};

// Owns synthetic RAM; callbacks borrow it on the test thread until the test returns.
struct Context {
    std::vector<std::uint8_t> ram = std::vector<std::uint8_t>(0x300000u);  // Covers the SP globals.
    bool connected = true;                    // Whether the platform supplies port zero.
    int polls = 0;                            // Platform samples requested.
    int mods = 0;                             // Mod report callbacks.
    int after_mods = 0;                       // Post-mod callbacks.
    int callbacks = 0;                        // Game pad callbacks.
    int readers = 0;                          // Game decoder invocations.
    int motors = 0;                           // Motor commands forwarded.
    int clears = 0;                           // Game clear invocations at startup.
    Copy reader_copy = Copy::Boot;            // Last decoder chosen.
    gaddr reader_address = 0;                 // Last report's game address.
    int reader_bytes = 0;                     // Last report's length.
    std::array<int, 3> motor_values{};        // Port, small motor, large motor.
    std::string order;                        // Platform, mods, callback, reader, motors.
    std::uint8_t first_axis = kCallbackAxis;  // Expected first axis after the chosen pipeline.
    std::uint8_t second_axis = kAfterAxis;    // Expected second axis after the chosen pipeline.
};

// The mocked C input/game callbacks carry no context pointer.
Context* g_context = nullptr;

// Installs the RAM owner before a real wrapper can read its globals.
void use(Context& context) {
    g_context = &context;
    openrac_guest_base = context.ram.data();
}

// Records one mod edit before the post-mod layer and game callback run.
void modify_report(int port, std::uint8_t* report, int bytes) {
    CHECK(port == 0 && bytes == OPENRAC_PAD_REPORT);
    ++g_context->mods;
    g_context->order += 'm';
    report[2] = kModAxis;  // First stick axis in the shared payload contract.
}

// Clears counters without disturbing the pad's state between consecutive updates.
void reset_observations(Context& context) {
    context.polls = 0;
    context.mods = 0;
    context.after_mods = 0;
    context.callbacks = 0;
    context.readers = 0;
    context.motors = 0;
    context.order.clear();
}

// Sets up a connected pad through the real startup and first, quiet update.
void ready_pad(Context& context) {
    use(context);
    openrac_game_pad_report_hook = modify_report;
    func_003A2FC0();
    func_003A3B40(kPads);
    reset_observations(context);
}

// Records a decoder's identity and preserves the report delivered after all callback edits.
void read_report(Copy copy, gaddr pad, gaddr report, int bytes) {
    CHECK(pad == kPads || pad == kPads + kPadBytes);
    g_context->reader_copy = copy;
    g_context->reader_address = report;
    g_context->reader_bytes = bytes;
    ++g_context->readers;
    g_context->order += 'r';

    // Ready reports include status/id before the shared 18-byte payload.
    if (bytes == kReadyBytes) {
        CHECK(GREF(std::uint8_t, report) == 0);
        CHECK(GREF(std::uint8_t, report + 1) == 0x79);  // Analog report id with pressures.
        CHECK(GREF(std::uint8_t, report + 4) == g_context->first_axis);
        CHECK(GREF(std::uint8_t, report + 5) == g_context->second_axis);
    }
}

// A ready update samples once and passes each edit through mods, callback, decoder and motors.
void test_ready_update_samples_and_modifies_once() {
    Context context;
    ready_pad(context);
    GREF(gaddr, kPads + kCallback) = kCallbackAddress;
    GREF(std::uint32_t, kPads + kCallbackArgument) = kCallbackArgumentValue;
    GREF(std::uint32_t, kPads + kLatchedHeld) = 1;
    GREF(std::uint32_t, kPads + kRawHeld) = 1;
    GREF(std::uint8_t, kPads + kSmallMotor) = 1;
    GREF(std::uint8_t, kPads + kLargeMotor) = 42;  // Synthetic large-motor intensity.
    func_003A3B40(kPads);

    CHECK(context.polls == 1 && context.mods == 1 && context.after_mods == 1);
    CHECK(context.callbacks == 1 && context.readers == 1);
    CHECK(context.order == "pmacrv");
    CHECK(context.reader_copy == Copy::Boot);
    CHECK(context.reader_address == kPads + kReport && context.reader_bytes == kReadyBytes);
    CHECK(GREF(std::uint32_t, kPads + kPreviousHeld) == 1);
    CHECK(GREF(std::uint32_t, kPads + kPreviousRawHeld) == 1);
    CHECK(GREF(std::uint32_t, kPads + kHeld) == 0 && GREF(std::uint32_t, kPads + kRawHeld) == 0);
    CHECK(GREF(std::uint8_t, kNotReady) == 0);
    CHECK((context.motor_values == std::array<int, 3>{0, 1, 42}));
    CHECK(context.motors == 1);
    CHECK(GREF(std::uint8_t, kPads + kSmallMotor) == 0);
    CHECK(GREF(std::uint8_t, kPads + kLargeMotor) == 0);
}

// The connection/setup frame is quiet: no mod report, game callback, decoder or motor write.
void test_connection_setup_does_not_consume_a_mod_report() {
    Context context;
    use(context);
    openrac_game_pad_report_hook = modify_report;
    func_003A2FC0();
    func_003A3B40(kPads);

    CHECK(context.clears == 8);  // Startup clears all four slots on each of two ports.
    CHECK(context.polls == 1 && context.mods == 0 && context.after_mods == 0);
    CHECK(context.readers == 0 && context.callbacks == 0 && context.motors == 0);
    CHECK(context.order == "p");
    CHECK(GREF(std::int32_t, kPads + kState) == kReady);
    CHECK(GREF(std::uint8_t, kNotReady) == 1);
    CHECK(GREF(gaddr, kActive) == kPads);
    // The second controller is the first slot after four records, not a multitap slot.
    CHECK(GREF(std::uint8_t, kPads + kPort) == 0);
    CHECK(GREF(std::uint8_t, kPads + 4 * kPadBytes + kPort) == 1);
    CHECK(GREF(gaddr, kPortOne) == kPads + 4 * kPadBytes);
    CHECK(GREF(gaddr, kPortOneToo) == kPads + 4 * kPadBytes);
}

// Disconnect uses the game's empty report and readiness flag, without invoking report mods.
void test_disconnect_delivers_the_empty_report() {
    Context context;
    ready_pad(context);
    context.connected = false;
    GREF(gaddr, kPads + kCallback) = kCallbackAddress;
    GREF(std::uint32_t, kPads + kCallbackArgument) = kCallbackArgumentValue;
    func_003A3B40(kPads);

    CHECK(context.polls == 1 && context.mods == 0 && context.after_mods == 0);
    CHECK(context.callbacks == 1 && context.readers == 1 && context.motors == 0);
    CHECK(context.order == "pcr");
    CHECK(context.reader_address == kNone && context.reader_bytes == kEmptyBytes);
    CHECK(GREF(std::int32_t, kPads + kState) == 0);
    CHECK(GREF(std::int32_t, kPads + kLibraryState) == 0);
    CHECK(GREF(std::uint8_t, kPads + kSeen) == 0 && GREF(std::uint8_t, kNotReady) == 1);
}

// A slot the port never opened does not poll platform input or consume mod state.
void test_unopened_slot_never_polls() {
    Context context;
    use(context);
    openrac_game_pad_report_hook = modify_report;
    func_003A2FC0();
    func_003A3B40(kPads + kPadBytes);

    CHECK(GREF(std::uint8_t, kPads + kPadBytes + kSlot) == 1);
    CHECK(context.polls == 0 && context.mods == 0 && context.after_mods == 0);
    CHECK(context.reader_address == kNone && context.reader_bytes == kEmptyBytes);
    CHECK(GREF(std::int32_t, kPads + kPadBytes + kLibraryState) == 99);  // Unopened slot.
}

// A reconnect repeats the quiet setup frame before resuming one report per ready update.
void test_reconnect_keeps_the_quiet_frame() {
    Context context;
    ready_pad(context);
    context.connected = false;
    func_003A3B40(kPads);
    context.connected = true;
    reset_observations(context);
    func_003A3B40(kPads);
    CHECK(context.polls == 1 && context.mods == 0 && context.readers == 0);
    CHECK(GREF(std::uint8_t, kNotReady) == 1);

    context.first_axis = kModAxis;
    reset_observations(context);
    func_003A3B40(kPads);
    CHECK(context.polls == 1 && context.mods == 1 && context.readers == 1);
    CHECK(context.order == "pmarv");
    CHECK(GREF(std::uint8_t, kNotReady) == 0);
}

// Omitting report mods and the game callback does not omit or alter the game reader's input.
void test_ready_without_hooks_keeps_the_reader() {
    Context context;
    ready_pad(context);
    openrac_game_pad_report_hook = nullptr;
    context.first_axis = 0x80;  // The platform mock supplies centered axes.
    context.second_axis = 0x80;
    func_003A3B40(kPads);
    CHECK(context.polls == 1 && context.mods == 0 && context.after_mods == 0);
    CHECK(context.readers == 1 && context.callbacks == 0);
    CHECK(context.order == "prv");
    CHECK(
        GREF(std::uint8_t, kPads + kReport + 14) == 0xFF
    );  // Cross pressure in the 20-byte report.
}

#if defined(RAC3_PAL)
// The PAL frontbin and level startups retain their own decoders and clear operations.
void test_program_copies_keep_their_readers() {
    Context context;
    use(context);
    openrac_game_pad_report_hook = modify_report;
    func_003A2BC0__frontbin();
    func_003A3740__frontbin(kPads);
    GREF(gaddr, kPads + kCallback) = kCallbackAddress;
    GREF(std::uint32_t, kPads + kCallbackArgument) = kCallbackArgumentValue;
    reset_observations(context);
    func_003A3740__frontbin(kPads);
    CHECK(context.reader_copy == Copy::Frontbin && context.readers == 1 && context.mods == 1);

    CHECK(func_L01_00460868() == 0);
    CHECK(func_L01_004613E8(static_cast<int>(kPads)) == 0);
    GREF(gaddr, kPads + kCallback) = kCallbackAddress;
    GREF(std::uint32_t, kPads + kCallbackArgument) = kCallbackArgumentValue;
    reset_observations(context);
    CHECK(func_L01_004613E8(static_cast<int>(kPads)) == 0);
    CHECK(context.reader_copy == Copy::Level && context.readers == 1 && context.mods == 1);
}
#endif

#if defined(RAC3_NTSC)
// Replaces synthetic overlay memory while checking the retained core records and pointers.
void replace_overlay(Context& context) {
    // All NTSC SP records begin here, above the pad records and core pointers (disc record layout).
    constexpr gaddr kOverlayLow = 0x001D5680u;
    constexpr gaddr kPadEnd = kPads + 8 * kPadBytes;
    static_assert(kPadEnd <= kOverlayLow && kActive < kOverlayLow);
    const std::vector<std::uint8_t> saved(
        context.ram.begin() + kPads, context.ram.begin() + kPadEnd
    );
    const std::array<gaddr, 3> pointers{
        GREF(gaddr, kActive), GREF(gaddr, kPortOne), GREF(gaddr, kPortOneToo)
    };
    const std::uint8_t not_ready = GREF(std::uint8_t, kNotReady);
    std::fill(
        context.ram.begin() + kOverlayLow, context.ram.end(), 0xA5
    );  // Synthetic program data.
    CHECK(std::equal(saved.begin(), saved.end(), context.ram.begin() + kPads));
    CHECK(GREF(gaddr, kActive) == pointers[0]);
    CHECK(GREF(gaddr, kPortOne) == pointers[1] && GREF(gaddr, kPortOneToo) == pointers[2]);
    CHECK(GREF(std::uint8_t, kNotReady) == not_ready);
}

// NTSC initializes once at the real boot entry, then uses new readers on retained core records.
void test_ntsc_overlays_keep_boot_initialized_pads() {
    Context context;
    ready_pad(context);
    GREF(gaddr, kPads + kCallback) = kCallbackAddress;
    GREF(std::uint32_t, kPads + kCallbackArgument) = kCallbackArgumentValue;
    CHECK(context.clears == 8);  // The real boot wrapper cleared each record once.
    replace_overlay(context);
    func_003A3740__frontbin(kPads);
    CHECK(context.reader_copy == Copy::Frontbin && context.readers == 1);
    CHECK(context.polls == 1 && context.mods == 1 && context.order == "pmacrv");
    CHECK(context.clears == 8 && GREF(std::uint8_t, kNotReady) == 0);

    reset_observations(context);
    replace_overlay(context);
    CHECK(func_L01_004613E8(static_cast<int>(kPads)) == 0);
    CHECK(context.reader_copy == Copy::Level && context.readers == 1);
    CHECK(context.polls == 1 && context.mods == 1 && context.order == "pmacrv");
    CHECK(context.clears == 8 && GREF(std::uint8_t, kNotReady) == 0);
}
#endif

// Existing shared callers keep one mod pipeline even for a disconnected neutral report.
void test_shared_report_preserves_existing_mod_policy() {
    Context context;
    use(context);
    context.connected = false;
    openrac_game_pad_report_hook = modify_report;
    std::array<std::uint8_t, OPENRAC_PAD_REPORT> report{};
    CHECK(openrac_pad_report(0, report.data()) == 0);
    CHECK(context.polls == 1 && context.mods == 1 && context.after_mods == 1);
    CHECK(report[0] == 0xFF && report[1] == 0xFF);  // Active-low, no buttons pressed.
    CHECK(report[2] == kModAxis && report[3] == kAfterAxis);

    openrac_game_pad_report_hook = nullptr;
    reset_observations(context);
    CHECK(openrac_pad_report(0, report.data()) == 0);
    CHECK(context.polls == 1 && context.mods == 0 && context.after_mods == 0);
    CHECK(report[2] == 0x80 && report[3] == 0x80);  // Shared neutral stick centers.
}

}  // namespace

extern "C" {

std::uint8_t* openrac_guest_base = nullptr;

int openrac_game_pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]) {
    CHECK(port == 0);
    ++g_context->polls;
    g_context->order += 'p';

    // A disconnected device leaves the caller's neutral defaults unchanged.
    if (g_context->connected) {
        *buttons = 0xBFFF;             // Synthetic Cross press: bit 14 clear.
        std::memset(analog, 0x80, 4);  // Four centered stick axes.
    }

    return g_context->connected;
}

void openrac_game_pad_after_mods(int port, std::uint8_t* report, int bytes) {
    CHECK(port == 0 && bytes == OPENRAC_PAD_REPORT && report[2] == kModAxis);
    ++g_context->after_mods;
    g_context->order += 'a';
    report[3] = kAfterAxis;  // The next layer's edit must also reach the game reader.
}

void openrac_game_set_motors(int port, int small, int large) {
    ++g_context->motors;
    g_context->motor_values = {port, small, large};
    g_context->order += 'v';
}

std::uint64_t openrac_guest_call_regs(
    gaddr address, const std::uint64_t* ints, const float*, float*, int
) {
    CHECK(address == kCallbackAddress && ints[0] == kPads);
    CHECK(ints[3] == kCallbackArgumentValue);
    ++g_context->callbacks;
    g_context->order += 'c';

    // The game callback may edit a ready report before the decoder consumes it.
    if (ints[2] == kReadyBytes) {
        const gaddr report = static_cast<gaddr>(ints[1]);
        CHECK(GREF(std::uint8_t, report + 4) == kModAxis);
        GREF(std::uint8_t, report + 4) = kCallbackAxis;
    }

    return 0;
}

void func_003A31C8(gaddr pad, gaddr report, int bytes) {
    read_report(Copy::Boot, pad, report, bytes);
}

void func_003A2DC8__frontbin(gaddr pad, gaddr report, int bytes) {
    read_report(Copy::Frontbin, pad, report, bytes);
}

void func_L01_00460A70(int pad, int report, int bytes) {
    read_report(Copy::Level, static_cast<gaddr>(pad), static_cast<gaddr>(report), bytes);
}

void func_003A30D8(gaddr pad) {
    ++g_context->clears;
    std::memset(G(pad), 0, kPadBytes);
}

#if defined(RAC3_PAL)
void func_003A2CD8__frontbin(gaddr pad) {
    func_003A30D8(pad);
}

void func_L01_00460980(gaddr pad) {
    func_003A30D8(pad);
}
#endif

}  // extern "C"

// Runs the synthetic state-machine checks without a game, renderer or generated host code.
int main() {
    test_ready_update_samples_and_modifies_once();
    test_connection_setup_does_not_consume_a_mod_report();
    test_disconnect_delivers_the_empty_report();
    test_unopened_slot_never_polls();
    test_reconnect_keeps_the_quiet_frame();
    test_ready_without_hooks_keeps_the_reader();
#if defined(RAC3_PAL)
    test_program_copies_keep_their_readers();
#endif
#if defined(RAC3_NTSC)
    test_ntsc_overlays_keep_boot_initialized_pads();
#endif
    test_shared_report_preserves_existing_mod_policy();
    return openrac::test::result();
}
