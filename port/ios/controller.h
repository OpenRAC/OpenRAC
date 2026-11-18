// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

@interface OpenRACControllerNavigation : NSObject
- (instancetype)initWithWebView:(WKWebView*)web owner:(UIViewController*)owner;
- (void)start;
- (void)stop;
@end
