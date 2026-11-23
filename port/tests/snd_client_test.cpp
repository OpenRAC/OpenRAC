// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native 989snd client's pump order, without a game, disc, bank or audio device: engine answers
// are taken when sent, callbacks wait for the next pump, and a program reset drops old callbacks.

#include <cstdint>
#include <vector>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "tests/check.h"

const char* openrac_game_disc_files = nullptr;
const char* openrac_game_mods_dir = nullptr;
unsigned openrac_game_mods_generation = 0;

// This fixture never uploads a bank; no emulated memory is needed for the unused bank-load entry.
const std::uint8_t* openrac_iop_bytes(std::uint32_t, std::size_t* available) {
    *available = 0;
    return nullptr;
}

namespace {

constexpr gaddr kCallback = 0x00100000u;
constexpr long long kArgument = 0x123456789abcdefLL;

struct Event {
    bool callback;
    int answer;
    long long argument;

    bool operator==(const Event&) const = default;
};

std::vector<Event> g_events;

int answer(const std::int32_t* args) {
    const int value = args != nullptr ? args[0] : 7;
    g_events.push_back({false, value, 0});
    return value;
}

void callback(int value, long long argument) {
    g_events.push_back({true, value, argument});
}

void begin() {
    openrac_snd_client_reset();
    g_events.clear();
}

void callbacks_wait_one_pump() {
    begin();
    std::int32_t args[] = {17};
    openrac_snd_request(answer, args, 1, kCallback, kArgument);
    args[0] = 99;
    CHECK((g_events == std::vector<Event>{{false, 17, 0}}));
    CHECK(openrac_lib_snd_Pump() == 0);
    CHECK((g_events == std::vector<Event>{{false, 17, 0}, {true, 17, kArgument}}));
    CHECK(openrac_lib_snd_Pump() == 0);
    CHECK(g_events.size() == 2);
}

void taking_a_request_pumps_the_previous_one() {
    begin();
    const std::int32_t first[] = {11};
    const std::int32_t second[] = {22};
    openrac_snd_request(answer, first, 1, kCallback, 1);
    openrac_snd_request(answer, second, 1, kCallback, 2);
    CHECK((g_events == std::vector<Event>{{false, 11, 0}, {true, 11, 1}, {false, 22, 0}}));
    openrac_lib_snd_Pump();
    CHECK((g_events.back() == Event{true, 22, 2}));
    CHECK(g_events.size() == 4);
}

void synchronous_requests_wait_for_callbacks() {
    begin();
    const std::int32_t args[] = {31};
    openrac_snd_request(answer, args, 1, kCallback, kArgument);
    CHECK(openrac_snd_request_sync(answer, nullptr) == 7);
    CHECK((g_events == std::vector<Event>{{false, 31, 0}, {true, 31, kArgument}, {false, 7, 0}}));
    CHECK(openrac_snd_request_sync(nullptr, nullptr) == 0);
    CHECK(g_events.size() == 3);
}

void empty_requests_and_reset() {
    begin();
    openrac_snd_request(answer, nullptr, 0, 0, 0);
    CHECK((g_events == std::vector<Event>{{false, 7, 0}}));
    CHECK(openrac_lib_snd_Pump() == 0);
    openrac_snd_request(nullptr, nullptr, 0, kCallback, kArgument);
    CHECK(g_events.size() == 1);
    openrac_lib_snd_Pump();
    CHECK((g_events.back() == Event{true, 0, kArgument}));
    const std::int32_t args[] = {41};
    openrac_snd_request(answer, args, 1, kCallback, kArgument);
    openrac_snd_client_reset();
    const std::size_t before = g_events.size();
    CHECK(openrac_lib_snd_Pump() == 0);
    CHECK(g_events.size() == before);

    // Invalid argument counts take neither an engine call nor a callback.
    openrac_snd_request(answer, nullptr, -1, kCallback, 0);
    openrac_snd_request(answer, nullptr, OPENRAC_SND_REQUEST_ARGS + 1, kCallback, 0);
    CHECK(openrac_lib_snd_Pump() == 0);
    CHECK(g_events.size() == before);
}

}  // namespace

int main() {
    const openrac_fn_entry entry{
        kCallback, reinterpret_cast<openrac_host_fn>(callback), "snd_test_callback"
    };
    openrac_guest_register(OPENRAC_OVERLAY_EXE, &entry, 1);
    callbacks_wait_one_pump();
    taking_a_request_pumps_the_previous_one();
    synchronous_requests_wait_for_callbacks();
    empty_requests_and_reset();
    openrac_snd_client_reset();
    openrac_guest_unregister_all();
    return openrac::test::result();
}
