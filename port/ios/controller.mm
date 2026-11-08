// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#import "ios/controller.h"

#include "ios/controller_input.h"

#import <GameController/GameController.h>

namespace {
enum Button {
    Up,
    Down,
    Left,
    Right,
    Accept,
    Back,
    Prev,
    Next,
    More,
    Count
};

NSString* const names[] =
    {@"up", @"down", @"left", @"right", @"accept", @"back", @"prev", @"next", @"more"};

uint32_t buttons() {
    uint32_t result = 0;
    auto add = [&](Button button, bool pressed) {
        if (pressed) {
            result |= 1u << button;
        }
    };
    // Polling leaves valueChangedHandlers available for SDL when the engine takes over.
    for (GCController* controller in GCController.controllers) {
        GCExtendedGamepad* pad = controller.extendedGamepad;
        if (!pad) {
            continue;
        }
        add(Up, pad.dpad.up.pressed || pad.leftThumbstick.yAxis.value >= 0.5f);
        add(Down, pad.dpad.down.pressed || pad.leftThumbstick.yAxis.value <= -0.5f);
        add(Left, pad.dpad.left.pressed || pad.leftThumbstick.xAxis.value <= -0.5f);
        add(Right, pad.dpad.right.pressed || pad.leftThumbstick.xAxis.value >= 0.5f);
        add(Accept, pad.buttonA.pressed || pad.buttonMenu.pressed);
        add(Back, pad.buttonB.pressed);
        add(Prev, pad.leftShoulder.pressed);
        add(Next, pad.rightShoulder.pressed);
        add(More, pad.buttonY.pressed);
    }
    return result;
}
}  // namespace

@interface OpenRACControllerNavigation () {
    openrac::ios::ControllerInput input;
}

@property(nonatomic, weak) WKWebView* web;
@property(nonatomic, weak) UIViewController* owner;
@property(nonatomic, strong) NSTimer* timer;
@end

@implementation OpenRACControllerNavigation

- (instancetype)initWithWebView:(WKWebView*)web owner:(UIViewController*)owner {
    self = [super init];
    if (self) {
        self.web = web;
        self.owner = owner;
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector(releaseButtons)
                                                   name:UIApplicationWillResignActiveNotification
                                                 object:nil];
    }
    return self;
}

- (void)releaseButtons {
    input.update(buttons(), false);
    [self.web evaluateJavaScript:@"window.__openracPadRelease?.()" completionHandler:nil];
}

- (void)poll {
    BOOL active = self.web.window && !self.owner.presentedViewController
                  && UIApplication.sharedApplication.applicationState == UIApplicationStateActive;
    auto edges = input.update(buttons(), active);
    NSMutableString* script = [NSMutableString string];
    // Release first so changing direction never leaves two repeat timers running.
    for (int i = 0; i < Count; ++i) {
        if (edges.up & (1u << i)) {
            [script appendFormat:@"window.__openracPad?.('up:%@');", names[i]];
        }
    }
    for (int i = 0; i < Count; ++i) {
        if (edges.down & (1u << i)) {
            [script appendFormat:@"window.__openracPad?.('down:%@');", names[i]];
        }
    }
    if (script.length) {
        [self.web evaluateJavaScript:script completionHandler:nil];
    }
}

- (void)start {
    [self stop];
    __weak OpenRACControllerNavigation* weakSelf = self;
    self.timer = [NSTimer timerWithTimeInterval:1.0 / 60.0
                                        repeats:YES
                                          block:^(NSTimer*) {
                                            [weakSelf poll];
                                          }];
    [NSRunLoop.mainRunLoop addTimer:self.timer forMode:NSRunLoopCommonModes];
}

- (void)stop {
    [self.timer invalidate];
    self.timer = nil;
    [self releaseButtons];
}

- (void)dealloc {
    [self.timer invalidate];
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

@end
