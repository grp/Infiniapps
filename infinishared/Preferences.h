
#import "iPhonePrivate.h"

/* Preferences {{{ */
static NSDictionary *preferences = nil;
static NSString *identifier = nil;
static void (*callback)() = NULL;

static void IFPreferencesLoad() {
    [preferences release];
    preferences = [[NSDictionary alloc] initWithContentsOfFile:[NSString stringWithFormat:[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/%@.plist"], identifier]];
}

static BOOL IFPreferencesBoolForKey(NSString *key, BOOL def) {
    id obj = [preferences objectForKey:key];

    if (obj != nil) return [obj boolValue];
    else return def;
}

static int IFPreferencesIntForKey(NSString *key, int def) {
    id obj = [preferences objectForKey:key];

    if (obj != nil) return [obj intValue];
    else return def;
}

static id IFPreferencesObjectForKey(NSString *key, id def) {
    return [preferences objectForKey:key] ?: def;
}

static SBIconModel *IFPreferencesSharedIconModel() {
    Class modelClass = NSClassFromString(@"SBIconModel");

    if ([modelClass respondsToSelector:@selector(sharedInstance)]) {
        return [modelClass sharedInstance];
    } else {
        Class controllerClass = NSClassFromString(@"SBIconController");
        SBIconController *controller = [controllerClass sharedInstance];

        return [controller model];
    }
}

static void IFPreferencesIconModelLayout(SBIconModel *model) {
    if ([model respondsToSelector:@selector(relayout)]) {
        [model relayout];
    } else {
        [model layout];
    }
}

static void IFPreferencesChangedCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    IFPreferencesLoad();

    if (callback != NULL) {
        callback();
    }

    IFPreferencesIconModelLayout(IFPreferencesSharedIconModel());
}

static void IFPreferencesInitialize(NSString *bundleIdentifier, void (*cb)()) {
    identifier = [bundleIdentifier copy];
    callback = cb;

    IFPreferencesLoad();

    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, IFPreferencesChangedCallback, (CFStringRef) [NSString stringWithFormat:@"%@.preferences-changed", bundleIdentifier], NULL, CFNotificationSuspensionBehaviorCoalesce);
}
/* }}} */

