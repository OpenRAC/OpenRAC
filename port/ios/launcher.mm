// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// The desktop launcher's actual Svelte interface, with a native Files/engine bridge.
#import "ios/launcher.h"

#import "ios/controller.h"
#import "ios/prepare.h"
#import "ios/save_transfer.h"
#include "openrac_launcher.h"

#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <WebKit/WebKit.h>

NSDictionary* openrac_ios_bundle() {
    static NSDictionary* bundle;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
      NSURL* file =
          [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:@"ios-bundle.json"];
      bundle = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfURL:file]
                                               options:0
                                                 error:nil];
    });
    return bundle;
}

namespace {
NSString* json(id object) {
    NSData* data = [NSJSONSerialization dataWithJSONObject:object ?: NSNull.null
                                                   options:NSJSONWritingFragmentsAllowed
                                                     error:nil];
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] ?: @"null";
}

}  // namespace

@interface OpenRACWebBridge : NSObject <
                                  WKScriptMessageHandler,
                                  WKURLSchemeHandler,
                                  WKNavigationDelegate,
                                  UIDocumentPickerDelegate>
@property(nonatomic, weak) WKWebView* web;
@property(nonatomic, strong) OpenRACControllerNavigation* controller;
@property(nonatomic, strong) OpenRACSaveTransfer* saves;
@property(nonatomic, weak) UIViewController* owner;
@property(nonatomic, strong) NSURL* files;
@property(nonatomic, strong) NSURL* data;
@property(nonatomic, strong) NSMutableDictionary<NSString*, NSURL*>* selections;
@property(nonatomic, copy) void (^play)();
@property(nonatomic, copy) void (^importDisc)
    (NSURL*, void (^)(NSString*), void (^)(BOOL, NSString*));
@property(nonatomic) NSInteger pickerReply;
@property(nonatomic) NSInteger nextJob;
@property(nonatomic) NSInteger importJob;
@property(nonatomic, strong) NSSet<NSString*>* engines;
@end

@implementation OpenRACWebBridge

- (void)webView:(WKWebView*)web didFinishNavigation:(WKNavigation*)navigation {
    [self.controller start];
}

- (void)reply:(NSInteger)request ok:(BOOL)ok value:(id)value {
    NSString* script = [NSString stringWithFormat:@"window.__openracReply(%ld,%@,%@)",
                                                  (long)request,
                                                  ok ? @"true" : @"false",
                                                  json(value)];
    [self.web evaluateJavaScript:script completionHandler:nil];
}

- (void)emit:(NSString*)name payload:(id)payload {
    NSString* script = [NSString
        stringWithFormat:@"window.dispatchEvent(new CustomEvent('openrac-event',{detail:%@}))",
                         json(@{@"name": name, @"payload": payload})];
    [self.web evaluateJavaScript:script completionHandler:nil];
}

- (void)webView:(WKWebView*)web startURLSchemeTask:(id<WKURLSchemeTask>)task {
    NSString* path = task.request.URL.path.stringByRemovingPercentEncoding;
    if ([path.pathComponents containsObject:@".."]
        || ![task.request.URL.host isEqualToString:@"launcher"]) {
        [task didFailWithError:[NSError errorWithDomain:NSCocoaErrorDomain
                                                   code:NSFileReadNoPermissionError
                                               userInfo:nil]];
        return;
    }
    NSURL* file = [self.files
        URLByAppendingPathComponent:
            [path stringByTrimmingCharactersInSet:[NSCharacterSet
                                                      characterSetWithCharactersInString:@"/"]]];
    NSData* bytes = [NSData dataWithContentsOfURL:file];
    if (!bytes) {
        [task didFailWithError:[NSError errorWithDomain:NSCocoaErrorDomain
                                                   code:NSFileReadNoSuchFileError
                                               userInfo:nil]];
        return;
    }
    NSDictionary* types = @{
        @"html": @"text/html",
        @"js": @"text/javascript",
        @"css": @"text/css",
        @"svg": @"image/svg+xml",
        @"png": @"image/png",
        @"webp": @"image/webp",
        @"woff2": @"font/woff2",
        @"json": @"application/json"
    };
    [task didReceiveResponse:[[NSURLResponse alloc] initWithURL:task.request.URL
                                                       MIMEType:types[file.pathExtension]
                                                                    ?: @"application/octet-stream"
                                          expectedContentLength:bytes.length
                                               textEncodingName:@"utf-8"]];
    [task didReceiveData:bytes];
    [task didFinish];
}

- (void)webView:(WKWebView*)web stopURLSchemeTask:(id<WKURLSchemeTask>)task {
}

- (void)webView:(WKWebView*)web
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                    decisionHandler:(void (^)(WKNavigationActionPolicy))decision {
    if ([action.request.URL.scheme isEqualToString:@"openrac-app"]) {
        decision(WKNavigationActionPolicyAllow);
    } else {
        decision(WKNavigationActionPolicyCancel);
    }
}

- (void)userContentController:(WKUserContentController*)content
      didReceiveScriptMessage:(WKScriptMessage*)message {
    if (!message.frameInfo.mainFrame
        || ![message.frameInfo.request.URL.scheme isEqualToString:@"openrac-app"]) {
        return;
    }
    NSDictionary* request = message.body;
    if (![request isKindOfClass:NSDictionary.class]) {
        return;
    }
    NSInteger identifier = [request[@"id"] integerValue];
    NSString* command = request[@"command"];
    NSDictionary* args = request[@"args"];
    if (![command isKindOfClass:NSString.class] || ![args isKindOfClass:NSDictionary.class]) {
        return;
    }
    __weak OpenRACWebBridge* weakSelf = self;
    if ([self.saves handleCommand:command
                             args:args
                            reply:^(BOOL ok, id value) {
                              [weakSelf reply:identifier ok:ok value:value];
                            }]) {
        return;
    }
    if ([command isEqualToString:@"dialog_open"]) {
        self.pickerReply = identifier;
        NSDictionary* options =
            [args[@"options"] isKindOfClass:NSDictionary.class] ? args[@"options"] : @{};
        BOOL folder = [options[@"directory"] boolValue];
        UTType* iso = [UTType typeWithFilenameExtension:@"iso"];
        UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc]
            initForOpeningContentTypes:@[folder ? UTTypeFolder : (iso ?: UTTypeData)]
                                asCopy:NO];
        picker.delegate = self;
        [self.owner presentViewController:picker animated:YES completion:nil];
        return;
    }
    if ([command isEqualToString:@"dialog_confirm"]) {
        UIAlertController* alert =
            [UIAlertController alertControllerWithTitle:@"OpenRAC"
                                                message:args[@"message"]
                                         preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"Annuler"
                                                  style:UIAlertActionStyleCancel
                                                handler:^(UIAlertAction*) {
                                                  [self reply:identifier ok:YES value:@NO];
                                                }]];
        [alert addAction:[UIAlertAction actionWithTitle:@"Continuer"
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction*) {
                                                  [self reply:identifier ok:YES value:@YES];
                                                }]];
        [self.owner presentViewController:alert animated:YES completion:nil];
        return;
    }
    if ([command isEqualToString:@"prepare_disc"]) {
        [self reply:identifier ok:YES value:args[@"image"]];
        return;
    }
    if ([command isEqualToString:@"install_game"]) {
        if (self.importJob != 0) {
            [self reply:identifier ok:NO value:@"A disc import is already running."];
            return;
        }
        NSURL* source = self.selections[args[@"image"]];
        if (!source
            || ![args[@"game"] isEqualToString:openrac_ios_bundle()[@"engines"][0][@"game"]]) {
            [self reply:identifier ok:NO value:@"Choose a supported disc in Files first."];
            return;
        }
        NSInteger job = ++self.nextJob;
        self.importJob = job;
        openrac_reset_import_cancel();
        [self reply:identifier
                 ok:YES
              value:@{
                  @"id": @(job),
                  @"title": @"Set up from your disc",
                  @"command": @"Local disc preparation",
                  @"detached": @NO,
                  @"output": self.data.path
              }];
        self.importDisc(
            source,
            ^(NSString* text) {
              [self emit:@"job-output"
                  payload:@{
                      @"type": @"output",
                      @"id": @(job),
                      @"stream": @"stdout",
                      @"line": text
                  }];
            },
            ^(BOOL success, NSString* error) {
              if (!success) {
                  [self emit:@"job-output"
                      payload:@{
                          @"type": @"output",
                          @"id": @(job),
                          @"stream": @"stderr",
                          @"line": error ?: @"Import failed"
                      }];
              }
              self.importJob = 0;
              [self emit:@"job-exit"
                  payload:@{
                      @"type": @"exit",
                      @"id": @(job),
                      @"code": success ? @0 : @1,
                      @"cancelled": @(!success && openrac_import_cancelled()),
                      @"outputs": @[]
                  }];
            }
        );
        return;
    }
    if ([command isEqualToString:@"cancel_job"]) {
        if (self.importJob != 0 && self.importJob == [args[@"id"] integerValue]) {
            openrac_cancel_import();
            [self emit:@"job-output"
                payload:@{
                    @"type": @"output",
                    @"id": @(self.importJob),
                    @"stream": @"stdout",
                    @"line": @"Stopping after the current copy or level finishes. Your previous "
                             @"copy is preserved."
                }];
        }
        [self reply:identifier ok:YES value:NSNull.null];
        return;
    }
    if ([command isEqualToString:@"run_action"] && [args[@"id"] isEqualToString:@"play"]) {
        NSLog(@"OpenRAC native launch: scope %@, engines %@", args[@"scope"], self.engines);
        if (self.importJob != 0) {
            [self reply:identifier ok:NO value:@"Wait for the disc import to finish first."];
            return;
        }
        if (![self.engines containsObject:args[@"scope"][@"key"]]) {
            [self reply:identifier
                     ok:NO
                  value:@"This game's native iOS engine is not included yet."];
            return;
        }
        [self reply:identifier
                 ok:YES
              value:@{
                  @"id": @0,
                  @"title": @"Play",
                  @"command": @"Native iOS engine",
                  @"detached": @YES,
                  @"output": NSNull.null
              }];
        [self.controller stop];
        dispatch_async(dispatch_get_main_queue(), self.play);
        return;
    }
    if ([command isEqualToString:@"open_url"]) {
        NSURL* url = [NSURL URLWithString:args[@"url"]];
        if ([url.scheme isEqualToString:@"https"]) {
            [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
        }
        [self reply:identifier ok:YES value:NSNull.null];
        return;
    }
    char* response = openrac_ios_launcher_command(command.UTF8String, json(args).UTF8String);
    NSDictionary* result =
        [NSJSONSerialization JSONObjectWithData:[[NSString stringWithUTF8String:response]
                                                    dataUsingEncoding:NSUTF8StringEncoding]
                                        options:0
                                          error:nil];
    openrac_ios_launcher_free(response);
    BOOL ok = [result[@"ok"] boolValue];
    [self reply:identifier ok:ok value:result[ok ? @"value" : @"error"] ?: NSNull.null];
}

- (void)documentPicker:(UIDocumentPickerViewController*)picker
    didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    NSURL* source = urls.firstObject;
    if (!source) {
        [self reply:self.pickerReply ok:YES value:NSNull.null];
        return;
    }
    NSString* token = [NSString stringWithFormat:@"openrac-selection://%@/%@",
                                                 NSUUID.UUID.UUIDString,
                                                 source.lastPathComponent];
    self.selections[token] = source;
    [self reply:self.pickerReply ok:YES value:token];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)picker {
    [self reply:self.pickerReply ok:YES value:NSNull.null];
}

@end

UIView* openrac_launcher_view(
    UIViewController* owner,
    NSURL* data,
    void (^play)(),
    void (^importDisc)(NSURL*, void (^)(NSString*), void (^)(BOOL, NSString*))
) {
    NSURL* documents = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
                                                            inDomains:NSUserDomainMask]
                           .firstObject;
    NSString* root = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"app_tools"];
    NSDictionary* bundle = openrac_ios_bundle();
    openrac_ios_launcher_init(root.UTF8String, documents.path.UTF8String, json(bundle).UTF8String);
    OpenRACWebBridge* bridge = [[OpenRACWebBridge alloc] init];
    bridge.owner = owner;
    bridge.data = data;
    bridge.play = play;
    bridge.importDisc = importDisc;
    bridge.selections = [NSMutableDictionary dictionary];
    bridge.engines = [NSSet setWithArray:[bundle[@"engines"] valueForKey:@"key"] ?: @[]];
    bridge.files = [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:@"launcher-dist"];
    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    [config setURLSchemeHandler:bridge forURLScheme:@"openrac-app"];
    [config.userContentController addScriptMessageHandler:bridge name:@"openrac"];
    [config.userContentController
        addUserScript:
            [[WKUserScript alloc]
                  initWithSource:[NSString stringWithFormat:@"window.__OPENRAC_IOS__=true;window.__"
                                                            @"OPENRAC_STORE_ARTWORK__=%@",
                                                            [bundle[@"storeArtwork"] boolValue]
                                                                ? @"true"
                                                                : @"false"]
                   injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                forMainFrameOnly:YES]];
    WKWebView* web = [[WKWebView alloc] initWithFrame:owner.view.bounds configuration:config];
    web.navigationDelegate = bridge;
    web.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    web.opaque = NO;
    web.backgroundColor = [UIColor colorWithRed:0.07 green:0.07 blue:0.1 alpha:1];
    bridge.web = web;
    bridge.saves = [[OpenRACSaveTransfer alloc] initWithOwner:owner];
    bridge.controller = [[OpenRACControllerNavigation alloc] initWithWebView:web owner:owner];
    [web
        loadRequest:[NSURLRequest
                        requestWithURL:[NSURL URLWithString:@"openrac-app://launcher/index.html"]]];
    return web;
}
