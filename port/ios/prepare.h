// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#import <Foundation/Foundation.h>

bool openrac_prepare_disc(
    NSURL* data, NSDictionary* engine, void (^progress)(NSString*), NSError** error
);
void openrac_reset_import_cancel();
void openrac_cancel_import();
bool openrac_import_cancelled();
