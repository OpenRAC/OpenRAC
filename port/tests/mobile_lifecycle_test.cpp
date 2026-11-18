// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// A background transition drains synchronously and blocks drawing until foreground returns.
#include <atomic>
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "platform/pump.h"
#include "tests/check.h"

namespace {
int drains = 0;

void drain() {
    ++drains;
}
}  // namespace

int main() {
    CHECK(SDL_Init(SDL_INIT_EVENTS));
    openrac::platform::set_background_handler(drain);
    SDL_Event event{};
    event.type = SDL_EVENT_WILL_ENTER_BACKGROUND;
    CHECK(SDL_PushEvent(&event));
    CHECK(drains == 1);  // UIKit's notification must not return before the GPU is drained.
    CHECK(SDL_PushEvent(&event));
    CHECK(drains == 1);  // Duplicate app/scene notifications must not submit more GPU work.
    std::atomic_bool resumed = false;
    std::thread foreground([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        SDL_Event active{};
        active.type = SDL_EVENT_DID_ENTER_FOREGROUND;
        resumed = true;
        SDL_PushEvent(&active);
    });
    openrac::platform::poll_event(event);
    CHECK(resumed);  // A frame cannot be submitted while the scene is in the background.
    foreground.join();
    openrac::platform::set_background_handler(nullptr);
    event.type = SDL_EVENT_WILL_ENTER_BACKGROUND;
    CHECK(SDL_PushEvent(&event));
    CHECK(drains == 1);
    openrac::platform::poll_event(event);
    SDL_Quit();
    return openrac::test::result();
}
