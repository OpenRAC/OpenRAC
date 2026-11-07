// SPDX-License-Identifier: GPL-3.0-or-later
#import <UIKit/UIKit.h>

UIView* openrac_launcher_view(UIViewController* owner, NSURL* data, void (^play)(),
    void (^importDisc)(NSURL*, void (^)(NSString*), void (^)(BOOL, NSString*)));

NSDictionary* openrac_ios_bundle();
