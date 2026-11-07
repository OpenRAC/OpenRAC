// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#import <UIKit/UIKit.h>

@interface OpenRACSaveTransfer : NSObject <UIDocumentPickerDelegate>
- (instancetype)initWithOwner:(UIViewController*)owner;
- (BOOL)handleCommand:(NSString*)command args:(NSDictionary*)args reply:(void (^)(BOOL, id))reply;
@end
