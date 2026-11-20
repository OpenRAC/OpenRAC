// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// raw_mouse.h on macOS: GCMouse's motion, the mouse's own counts with no pointer acceleration.
// Its handlers run on the main queue, which SDL's event pump drains, so on the window's thread;
// the sums are atomic all the same.

#include <atomic>

#include "platform/raw_mouse.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

namespace openrac::platform::raw_mouse {
namespace {

std::atomic<float> g_dx{0.0f};
std::atomic<float> g_dy{0.0f};
std::atomic<int> g_mice{0};
bool g_started = false;

void add(std::atomic<float>& sum, float v) {
    float was = sum.load(std::memory_order_relaxed);
    while (!sum.compare_exchange_weak(was, was + v, std::memory_order_relaxed)) {
    }
}

// Reads one mouse: GCMouse's y is up, SDL's (and the game's) down.
void watch(GCMouse* mouse) API_AVAILABLE(macos(11.0)) {
    GCMouseInput* input = mouse.mouseInput;
    if (input == nil) {
        return;
    }
    input.mouseMovedHandler = ^(GCMouseInput*, float dx, float dy) {
      add(g_dx, dx);
      add(g_dy, -dy);
    };
    g_mice.fetch_add(1);
}

}  // namespace

bool start() {
    if (g_started) {
        return g_mice.load() > 0;
    }
    g_started = true;
    if (@available(macOS 11.0, *)) {
        @autoreleasepool {
            for (GCMouse* mouse in [GCMouse mice]) {
                watch(mouse);
            }
            NSNotificationCenter* centre = [NSNotificationCenter defaultCenter];
            [centre addObserverForName:GCMouseDidConnectNotification
                                object:nil
                                 queue:[NSOperationQueue mainQueue]
                            usingBlock:^(NSNotification* note) {
                              watch(static_cast<GCMouse*>(note.object));
                            }];
            [centre addObserverForName:GCMouseDidDisconnectNotification
                                object:nil
                                 queue:[NSOperationQueue mainQueue]
                            usingBlock:^(NSNotification*) {
                              if (g_mice.load() > 0) {
                                  g_mice.fetch_sub(1);
                              }
                            }];
        }
        return true;
    }
    return false;
}

bool active() {
    return g_mice.load() > 0;
}

void take(float& dx, float& dy) {
    dx = g_dx.exchange(0.0f);
    dy = g_dy.exchange(0.0f);
}

}  // namespace openrac::platform::raw_mouse
