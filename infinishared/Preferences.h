/* License {{{ */

/*
 * Copyright (c) 2010-2014, Xuzz Productions, LLC
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* }}} */

#import "iPhonePrivate.h"

/* Preferences {{{ */
static NSDictionary *preferences = nil;
static NSString *identifier = nil;
static void (*callback)() = NULL;

__attribute__((unused)) static void IFPreferencesLoad() {
    [preferences release];
    preferences = [[NSDictionary alloc] initWithContentsOfFile:[NSString stringWithFormat:[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/%@.plist"], identifier]];
}

__attribute__((unused)) static BOOL IFPreferencesBoolForKey(NSString *key, BOOL def) {
    id obj = [preferences objectForKey:key];

    if (obj != nil) return [obj boolValue];
    else return def;
}

__attribute__((unused)) static int IFPreferencesIntForKey(NSString *key, int def) {
    id obj = [preferences objectForKey:key];

    if (obj != nil) return [obj intValue];
    else return def;
}

__attribute__((unused)) static id IFPreferencesObjectForKey(NSString *key, id def) {
    return [preferences objectForKey:key] ?: def;
}

__attribute__((unused)) static SBIconModel *IFPreferencesSharedIconModel() {
    Class modelClass = NSClassFromString(@"SBIconModel");

    if ([modelClass respondsToSelector:@selector(sharedInstance)]) {
        return [modelClass sharedInstance];
    } else {
        Class controllerClass = NSClassFromString(@"SBIconController");
        SBIconController *controller = [controllerClass sharedInstance];

        return [controller model];
    }
}

__attribute__((unused)) static void IFPreferencesIconModelLayout(SBIconModel *model) {
    if ([model respondsToSelector:@selector(relayout)]) {
        [model relayout];
    } else {
        [model layout];
    }
}

__attribute__((unused)) static void IFPreferencesChangedCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    IFPreferencesLoad();

    if (callback != NULL) {
        callback();
    }

    IFPreferencesIconModelLayout(IFPreferencesSharedIconModel());
}

__attribute__((unused)) static void IFPreferencesInitialize(NSString *bundleIdentifier, void (*cb)()) {
    identifier = [bundleIdentifier copy];
    callback = cb;

    IFPreferencesLoad();

    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, IFPreferencesChangedCallback, (CFStringRef) [NSString stringWithFormat:@"%@.preferences-changed", bundleIdentifier], NULL, CFNotificationSuspensionBehaviorCoalesce);
}
/* }}} */

