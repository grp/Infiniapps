
@interface SBIconModel : NSObject { }
+ (id)sharedInstance;
- (void)relayout;
@end

/* Preferences {{{ */
static NSDictionary *preferences = nil;
static NSString *identifier = nil;
static void (*callback)() = NULL;

static void IFPreferencesLoad() {
    [preferences release];
    NSLog(@"IF:Preferences: Loading");
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

static void IFPreferencesChangedCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    IFPreferencesLoad();

    NSLog(@"IF:Preferences: Changed");

    if (callback != NULL) {
        callback();
    }

    [[objc_getClass("SBIconModel") sharedInstance] relayout];
}

static void IFPreferencesInitialize(NSString *bundleIdentifier, void (*cb)()) {
    identifier = [bundleIdentifier copy];
    callback = cb;

    IFPreferencesLoad();

    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, IFPreferencesChangedCallback, (CFStringRef) [NSString stringWithFormat:@"%@.preferences-changed", bundleIdentifier], NULL, CFNotificationSuspensionBehaviorCoalesce);
}
/* }}} */

