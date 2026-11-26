// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the real host/program.c dispatch wrapper with synthetic records and a simulated loader
// boundary. Entry expectations are the owned program-record/catalogue facts, not retail bytes.
// Runtime registration, translated game code and LoadExec are not executed by this fixture.

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "check.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

extern "C" {
gaddr func_0013AFF8(void);
std::uint32_t rac3_program_loads(void);
void rac3_program_forget_levels(void);
void rac3_program_set_overlay(int overlay);
void rac3_program_set_range(gaddr gp, gaddr low, gaddr high);
void rac3_program_keep_level(int level, gaddr buffer_word);
int rac3_start_level(int level);
}

namespace {

// The boot front-end function and frontbin program entries differ on NTSC-U (owned record facts).
#if defined(RAC3_NTSC)
constexpr gaddr kFrontbinEntry = 0x0037D200u;
constexpr gaddr kBootFrontendEntry = 0x00381280u;
constexpr gaddr kLevelEntry = 0x003EE988u;
#else
constexpr gaddr kFrontbinEntry = 0x00381250u;
constexpr gaddr kBootFrontendEntry = 0x00381250u;
constexpr gaddr kLevelEntry = 0x003F08C8u;
#endif

// State words and SP overlay contract documented in rac3-pal/host/program.c.
constexpr gaddr kCurrentLevel = RAC3_ADDR(0x001D545Cu);
constexpr gaddr kNextLevel = RAC3_ADDR(0x001D9D04u);
constexpr gaddr kExit = RAC3_ADDR(0x001D5B74u);
constexpr gaddr kBufferWord = RAC3_ADDR(0x001D4B50u);
constexpr gaddr kGp = 0x001DC8B0u;
constexpr gaddr kProgramLow = 0x001D5680u;
constexpr gaddr kProgramHigh = 0x00700000u;
constexpr int kFrontbin = 100;

// Synthetic buffer, one four-byte record and its payload; no game data is used.
constexpr gaddr kBuffer = 0x00210000u;
constexpr gaddr kRecord = kBuffer + 16u;
constexpr gaddr kPayload = kRecord + 16u;
constexpr gaddr kDestination = 0x00280000u;
constexpr std::uint32_t kValue = 0x12345678u;
constexpr gaddr kUnsupportedEntry = 0x00710000u;   // Synthetic unknown input, not a game entry.
constexpr gaddr kPadState = 0x001CD0C0u + 0x198u;  // Core state below the program range.

// Owns RAM; runtime callbacks borrow it on the test thread until the case returns.
struct Context {
    std::vector<std::uint8_t> ram =
        std::vector<std::uint8_t>(0x300000u);  // Covers synthetic records.
    std::string order;                         // Relocation, range, keep, loader events.
    int kept = 0;                              // Program pieces captured.
    int kept_level = -1;                       // Last capture's level number.
    std::uint32_t kept_value = 0;              // Owned copy of the captured word.
    int before_overlay = OPENRAC_OVERLAY_EXE;  // Overlay observed inside the loader boundary.

    gaddr gp = 0;               // Last configured small-data base.
    gaddr low = 0;              // Last configured program range.
    gaddr high = 0;             // Last configured program range.
    int calls = 0;              // Requests to call a game routine.
    bool change_level = false;  // Stress-tests capture of pre-load caller state.
};

// The mocked C runtime callbacks have no user pointer.
Context* g_context = nullptr;

// Resets only the program wrapper's documented state before installing synthetic input.
void use(Context& context) {
    g_context = &context;
    openrac_guest_base = context.ram.data();
    rac3_program_forget_levels();
    rac3_program_set_overlay(OPENRAC_OVERLAY_EXE);
}

// Gives the real wrapper a format-correct, one-word synthetic record followed by a different tag.
void prepare(int level, gaddr entry) {
    GREF(std::int32_t, kCurrentLevel) = level;
    GREF(gaddr, kBufferWord) = kBuffer;
    GREF(std::uint32_t, kBuffer) = kRecord - kBuffer;
    const std::array<std::uint32_t, 5> record{kDestination, sizeof(kValue), 1, entry, kValue};
    std::memcpy(G(kRecord), record.data(), sizeof(record));
}

// A returned frontbin record selects its own table, including NTSC's distinct program entry.
void test_frontbin_return_selects_frontbin() {
    Context context;
    use(context);
    prepare(-1, kFrontbinEntry);
    GREF(std::uint32_t, kPadState) = 99;  // Synthetic ready pad in retained core memory.
    const std::uint32_t before = rac3_program_loads();
    CHECK(func_0013AFF8() == kFrontbinEntry);
    CHECK(openrac_game_loaded_overlay() == kFrontbin);
    CHECK(context.before_overlay == OPENRAC_OVERLAY_EXE);
    CHECK(context.order == "rgl" && context.kept == 0);
    CHECK(context.gp == kGp && context.low == kProgramLow && context.high == kProgramHigh);
    CHECK(rac3_program_loads() == before + 1);
    CHECK(GREF(std::uint32_t, kDestination) == kValue);
    CHECK(GREF(std::uint32_t, kPadState) == 99);
    CHECK(
        rac3_start_level(2) == 1
    );  // A front-end request writes the game's words, not a level call.
    CHECK(GREF(std::int32_t, kNextLevel) == 2 && GREF(std::int32_t, kExit) == 1);
    CHECK(context.calls == 0);
}

// The real boot front-end function remains executable-owned, not NTSC frontbin's identity.
void test_boot_frontend_identity_stays_separate() {
    Context context;
    use(context);
    CHECK(openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE && context.order.empty());

#if defined(RAC3_NTSC)
    CHECK(kBootFrontendEntry != kFrontbinEntry);
    prepare(-1, kBootFrontendEntry);
    CHECK(func_0013AFF8() == kBootFrontendEntry);
    CHECK(openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE);
#else
    // PAL's addresses coincide; its initial executable identity is installed outside this wrapper.
    CHECK(kBootFrontendEntry == kFrontbinEntry);
#endif
}

// Captures the pre-load level and its bytes before the simulated loader changes the source.
void test_level_capture_precedes_load_and_uses_caller_state() {
    Context context;
    use(context);
    prepare(1, kLevelEntry);
    context.change_level = true;
    const std::uint32_t before = rac3_program_loads();
    CHECK(func_0013AFF8() == kLevelEntry);
    CHECK(context.order == "rgkl");
    CHECK(context.kept == 1 && context.kept_level == 1 && context.kept_value == kValue);
    CHECK(
        GREF(std::uint32_t, kPayload) == 0
    );  // The capture owns its word before the source changes.
    CHECK(GREF(std::int32_t, kCurrentLevel) == -1);
    CHECK(openrac_game_loaded_overlay() == 1);
    CHECK(rac3_program_loads() == before + 1);
}

// Returning from a level changes table only after copying, without reconfiguring SP relocation.
void test_level_to_frontbin_keeps_handoff_order() {
    Context context;
    use(context);
    prepare(1, kLevelEntry);
    CHECK(func_0013AFF8() == kLevelEntry);
    context.order.clear();
    prepare(-1, kFrontbinEntry);
    CHECK(func_0013AFF8() == kFrontbinEntry);
    CHECK(context.before_overlay == 1 && context.order == "l");
    CHECK(openrac_game_loaded_overlay() == kFrontbin);
    CHECK(context.kept == 1);
}

// Executable switches may forget cached programs/ranges before explicitly selecting the new table.
void test_forget_and_explicit_overlay_selection_are_independent() {
    Context context;
    use(context);
    prepare(1, kLevelEntry);
    CHECK(func_0013AFF8() == kLevelEntry);
    rac3_program_set_overlay(
        39
    );  // A synthetic MP-menu handoff using the existing public boundary.
    context.order.clear();
    rac3_program_forget_levels();
    CHECK(openrac_game_loaded_overlay() == 39 && context.order.empty());
    rac3_program_set_overlay(OPENRAC_OVERLAY_EXE);
    prepare(1, kLevelEntry);
    CHECK(func_0013AFF8() == kLevelEntry);
    CHECK(context.kept == 2 && context.order == "rgkl");
    openrac_game_restore_overlay(kFrontbin);
    CHECK(openrac_game_loaded_overlay() == kFrontbin);
    openrac_game_restore_overlay(
        -2
    );  // An unsupported snapshot value falls back to the executable.
    CHECK(openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE);
}

// An executable's explicit range remains installed until its owner forgets that program context.
void test_explicit_range_is_kept_until_forget() {
    Context context;
    use(context);
    // Synthetic alternate-executable values exercise the same boundary MP uses, not MP game data.
    constexpr gaddr kOtherGp = 0x00200000u;
    constexpr gaddr kOtherLow = 0x00400000u;
    constexpr gaddr kOtherHigh = 0x00600000u;
    rac3_program_set_overlay(39);
    rac3_program_set_range(kOtherGp, kOtherLow, kOtherHigh);
    rac3_program_set_range(kGp, kProgramLow, kProgramHigh);
    CHECK(context.order == "rg" && context.gp == kOtherGp);
    CHECK(context.low == kOtherLow && context.high == kOtherHigh);
    rac3_program_forget_levels();
    CHECK(openrac_game_loaded_overlay() == 39);
    rac3_program_set_range(kGp, kProgramLow, kProgramHigh);
    CHECK(context.order == "rgrg" && context.gp == kGp);
    CHECK(context.low == kProgramLow && context.high == kProgramHigh);
}

// The entry correction does not change the existing fallback for unsupported negative-level input.
void test_unknown_entry_keeps_existing_fallback() {
    Context context;
    use(context);
    prepare(-1, kUnsupportedEntry);
    CHECK(func_0013AFF8() == kUnsupportedEntry);
    CHECK(openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE && context.kept == 0);
}

}  // namespace

extern "C" {

std::uint8_t* openrac_guest_base = nullptr;
int openrac_game_start_level = -1;

// Simulates a record's returned entry and one-word copy after the production pre-load capture.
gaddr func_0013AFF8__game(void) {
    g_context->order += 'l';
    g_context->before_overlay = openrac_game_loaded_overlay();
    const gaddr entry = GREF(gaddr, kRecord + 12u);  // Entry tag in the synthetic record contract.
    GREF(std::uint32_t, kDestination) = GREF(std::uint32_t, kPayload);
    GREF(std::uint32_t, kPayload) = 0;

    // This mutation stresses the wrapper's snapshot ordering; it is not copied retail behavior.
    if (g_context->change_level) {
        GREF(std::int32_t, kCurrentLevel) = -1;
    }

    return entry;
}

void openrac_guest_set_relocation(
    gaddr low, gaddr high, gaddr gp, const std::uint8_t*, gaddr, std::uint32_t
) {
    CHECK(low == 0 && high == 0);
    g_context->gp = gp;
    g_context->order += 'r';
}

void openrac_guest_set_level_range(gaddr low, gaddr high) {
    g_context->low = low;
    g_context->high = high;
    g_context->order += 'g';
}

void openrac_guest_add_level_program(
    int level, gaddr base, const std::uint8_t* bytes, std::uint32_t size
) {
    CHECK(base == kDestination && size == sizeof(kValue));
    ++g_context->kept;
    g_context->kept_level = level;
    std::memcpy(&g_context->kept_value, bytes, sizeof(kValue));
    g_context->order += 'k';
}

std::uint64_t openrac_guest_call_regs(gaddr, const std::uint64_t*, const float*, float*, int) {
    ++g_context->calls;
    return 0;
}

const char* openrac_env_string(const char*) {
    return nullptr;
}

}  // extern "C"

// Runs both release builds against the production wrapper, not a duplicate classifier.
int main() {
    test_frontbin_return_selects_frontbin();
    test_boot_frontend_identity_stays_separate();
    test_level_capture_precedes_load_and_uses_caller_state();
    test_level_to_frontbin_keeps_handoff_order();
    test_forget_and_explicit_overlay_selection_are_independent();
    test_explicit_range_is_kept_until_forget();
    test_unknown_entry_keeps_existing_fallback();
    return openrac::test::result();
}
