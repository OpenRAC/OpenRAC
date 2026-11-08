// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#import "ios/save_transfer.h"

#include "openrac_launcher.h"

#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace {
NSURL* transfer_folder(NSString* area, NSError** error) {
    NSURL* support = [NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory
                                                          inDomains:NSUserDomainMask]
                         .firstObject;
    NSURL* folder = [[support URLByAppendingPathComponent:@"OpenRAC/save-transfers"]
        URLByAppendingPathComponent:area];
    folder = [folder URLByAppendingPathComponent:NSUUID.UUID.UUIDString isDirectory:YES];
    if (![NSFileManager.defaultManager createDirectoryAtURL:folder
                                withIntermediateDirectories:YES
                                                 attributes:nil
                                                      error:error]) {
        return nil;
    }
    return folder;
}

NSDictionary* invoke(NSString* command, NSDictionary* args) {
    NSData* bytes = [NSJSONSerialization dataWithJSONObject:args options:0 error:nil];
    NSString* json = [[NSString alloc] initWithData:bytes encoding:NSUTF8StringEncoding];
    char* raw = openrac_ios_launcher_command(command.UTF8String, json.UTF8String);
    NSData* response = [[NSString stringWithUTF8String:raw] dataUsingEncoding:NSUTF8StringEncoding];
    openrac_ios_launcher_free(raw);
    return [NSJSONSerialization JSONObjectWithData:response options:0 error:nil];
}

}  // namespace

@interface OpenRACSaveTransfer ()
@property(nonatomic, weak) UIViewController* owner;
@property(nonatomic, copy) void (^pickerReply)(BOOL, id);
@end

@implementation OpenRACSaveTransfer

- (instancetype)initWithOwner:(UIViewController*)owner {
    self = [super init];
    if (self) {
        self.owner = owner;
    }
    return self;
}

- (BOOL)handleCommand:(NSString*)command args:(NSDictionary*)args reply:(void (^)(BOOL, id))reply {
    if ([command isEqualToString:@"pick_saves_file"]) {
        if (self.pickerReply || self.owner.presentedViewController) {
            reply(NO, @"Close the current Files or Share window first.");
            return YES;
        }
        self.pickerReply = reply;
        UIDocumentPickerViewController* picker =
            [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeZIP]
                                                                        asCopy:YES];
        picker.delegate = self;
        [self.owner presentViewController:picker animated:YES completion:nil];
        return YES;
    }
    if (![command isEqualToString:@"export_saves"]) {
        return NO;
    }
    if (self.owner.presentedViewController) {
        reply(NO, @"Close the current Files or Share window first.");
        return YES;
    }
    NSError* error = nil;
    NSURL* folder = transfer_folder(@"exports", &error);
    if (!folder) {
        reply(NO, error.localizedDescription);
        return YES;
    }
    NSMutableDictionary* request = [args mutableCopy];
    request[@"directory"] = folder.path;
    NSDictionary* result = invoke(command, request);
    if (![result[@"ok"] boolValue]) {
        reply(NO, result[@"error"] ?: @"Could not export saves.");
        return YES;
    }
    NSDictionary* exported = result[@"value"];
    NSURL* file = [NSURL fileURLWithPath:exported[@"saved"]];
    UIActivityViewController* share =
        [[UIActivityViewController alloc] initWithActivityItems:@[file] applicationActivities:nil];
    share.popoverPresentationController.sourceView = self.owner.view;
    share.popoverPresentationController.sourceRect = CGRectMake(
        CGRectGetMidX(self.owner.view.bounds), CGRectGetMidY(self.owner.view.bounds), 1, 1
    );
    share.completionWithItemsHandler =
        ^(UIActivityType, BOOL completed, NSArray*, NSError* shareError) {
          if (shareError) {
              reply(NO, shareError.localizedDescription);
          } else {
              reply(YES, @{
                  @"saved": completed ? file.lastPathComponent : @"",
                  @"slots": completed ? exported[@"slots"] : @[]
              });
          }
        };
    [self.owner presentViewController:share animated:YES completion:nil];
    return YES;
}

- (void)finishPicker:(BOOL)ok value:(id)value {
    void (^reply)(BOOL, id) = self.pickerReply;
    self.pickerReply = nil;
    if (reply) {
        reply(ok, value);
    }
}

- (void)documentPicker:(UIDocumentPickerViewController*)picker
    didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    NSURL* source = urls.firstObject;
    if (!source) {
        [self finishPicker:YES value:NSNull.null];
        return;
    }
    NSError* error = nil;
    NSNumber* size = nil;
    [source getResourceValue:&size forKey:NSURLFileSizeKey error:&error];
    if (!size || size.unsignedLongLongValue > (80ull << 20)) {
        [self finishPicker:NO
                     value:error.localizedDescription ?: @"This save archive is too large."];
        return;
    }
    NSURL* folder = transfer_folder(@"imports", &error);
    NSURL* copy = [folder URLByAppendingPathComponent:@"saves.zip"];
    // Keep a private snapshot: preview and confirmation must read the same archive, even if its
    // provider or the original file changes. Files has already materialized the asCopy document.
    BOOL access = [source startAccessingSecurityScopedResource];
    BOOL copied =
        folder && [NSFileManager.defaultManager copyItemAtURL:source toURL:copy error:&error];
    if (access) {
        [source stopAccessingSecurityScopedResource];
    }
    [self finishPicker:copied
                 value:copied ? copy.path
                              : (error.localizedDescription ?: @"Could not read the save file.")];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)picker {
    [self finishPicker:YES value:NSNull.null];
}

@end
