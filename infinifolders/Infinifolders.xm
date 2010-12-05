
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>

#import <SpringBoard/SpringBoard.h>
#import <SpringBoard/SBIconList.h>
#import <SpringBoard/SBIcon.h>
#import <SpringBoard/SBIconController.h>
#import <SpringBoard/SBUIController.h>
#import <SpringBoard/SBDownloadingIcon.h>
#import <SpringBoard/SBIconModel.h>
#import <dlfcn.h>
#import <objc/runtime.h>

#import <substrate.h>

#import "infinishared/Infinishared.h"


#define idForKeyWithDefault(dict, key, default)  ([(dict) objectForKey:(key)]?:(default))
#define floatForKeyWithDefault(dict, key, default)   ({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result floatValue]:(default); })
#define NSIntegerForKeyWithDefault(dict, key, default) (NSInteger)({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result integerValue]:(default); })
#define BOOLForKeyWithDefault(dict, key, default)    (BOOL)({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result boolValue]:(default); })

#define PreferencesFilePath [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/com.chpwn.infinifolders.plist"]
#define PreferencesChangedNotification "com.chpwn.infinifolders.prefs"

#define GetPreference(name, type) type ## ForKeyWithDefault(prefsDict, @#name, (name))

#ifdef DEBUG
#define LOG(...) (NSLog(@"%s in %s:%d: %@", __PRETTY_FUNCTION__, __FILE__, __LINE__, [NSString stringWithFormat:__VA_ARGS__]))
#else
#define LOG(...)
#endif

%class SBIconController;
%class SBIcon;
%class SBIconModel;
%class SBDestinationHole;
%class SBIconListPageControl;
%class SBDownloadingIcon;
%class SBFolderIconListView;

/* Preferences */

#define ScrollEnabled YES
#define SCROLL_ENABLED GetPreference(ScrollEnabled, BOOL)

#define PagingEnabled NO
#define PAGING_ENABLED GetPreference(PagingEnabled, BOOL)

#define BOUNCE_ENABLED 0
#define BOUNCE_NOTDEFAULT 1
#define BOUNCE_DISABLED 2
#define ScrollBounce 1
#define SCROLL_BOUNCE GetPreference(ScrollBounce, NSInteger)

#define SCROLLBAR_BLACK 0
#define SCROLLBAR_WHITE 1
#define SCROLLBAR_DISABLED 2
#define ScrollbarStyle 0
#define SCROLLBAR_STYLE GetPreference(ScrollbarStyle, NSInteger)

/* Macros */

#define VALID_LIST(list) ([list isMemberOfClass:$SBFolderIconListView])
#define WILDCAT ([UIDevice instancesRespondToSelector:@selector(isWildcat)] && [[UIDevice currentDevice] performSelector:@selector(isWildcat)])
#define MAX_ICON_ROWS(list) ((int) [$SBFolderIconListView iconRowsForInterfaceOrientation:[[UIDevice currentDevice] orientation]])
#define MAX_ICON_COLUMNS(list) ((int) [$SBFolderIconListView iconColumnsForInterfaceOrientation:[[UIDevice currentDevice] orientation]])
#define DEFAULT_ROWS_FOR_ORIENTATION(o) (!!!(WILDCAT) ? 3 : (UIDeviceOrientationIsLandscape(o) ? 4 : 5))

/* Categories */

@interface SBIconList (Infinifolders)
- (int)infinifoldersDefaultRows;
@end

@interface SBIconController (Infinifolders)
- (void)infinifoldersUpdateListHeights;
@end

@interface SBFolderIconListView : SBIconList
- (float)topIconInset;
@end

/* Global variables and flags */

static NSMutableArray *listies = nil;
static NSMutableArray *scrollies = nil;
static NSDictionary *prefsDict = nil;
static Class iconListClass;

static int disableRowsFlag = 0;
static int disableOriginFlag = 0;
static int disableResizeFlag = 0;
static int disableIconsFlag = 0;

#define kBottomPadding (WILDCAT ? (UIDeviceOrientationIsLandscape([[UIDevice currentDevice] orientation]) ? -8.0f : -13.0f) : 3.0f)

/* Utility methods */

static void firstFreeSlot(id iconList, int *x, int *y) {
    if ([iconList respondsToSelector:@selector(firstFreeSlotX:Y:)]) {
        [iconList firstFreeSlotX:x Y:y];
    } else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex:)]) {
        int idx;
        [iconList firstFreeSlotIndex:&idx];
        [iconList getX:x Y:y forIndex:idx forOrientation:[[UIDevice currentDevice] orientation]];
    } else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex)]) {
        [iconList getX:x Y:y forIndex:(int)[iconList firstFreeSlotIndex] forOrientation:[[UIDevice currentDevice] orientation]];
    }
}
static void lastIconPosition(id iconList, int *xptr, int *yptr) {
    int x, y;

    firstFreeSlot(iconList, &x, &y);

    // We want the /last/ icon, not the next free one
    if (x == 0) {
        y -= 1;
        x = MAX_ICON_COLUMNS(iconList);
    } else {
        x -= 1;
    }

    *xptr = x;
    *yptr = y;
}
static void applyPreferences() {
    for (int i = 0; i < MIN([listies count], [scrollies count]); i++) {
        UIScrollView *scrollView = [scrollies objectAtIndex:i];
        id iconList = [listies objectAtIndex:i];

        [iconList addSubview:scrollView];

        [scrollView setShowsVerticalScrollIndicator:YES];
        if (SCROLLBAR_STYLE == SCROLLBAR_BLACK)
            [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleDefault];
        else if (SCROLLBAR_STYLE == SCROLLBAR_WHITE)
            [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleWhite];
        else if (SCROLLBAR_STYLE == SCROLLBAR_DISABLED)
            [scrollView setShowsVerticalScrollIndicator:NO];

        [scrollView setScrollEnabled:SCROLL_ENABLED];
        [scrollView setAlwaysBounceVertical:!SCROLL_BOUNCE];
        [scrollView setBounces:SCROLL_BOUNCE != BOUNCE_DISABLED];
        [scrollView setPagingEnabled:PAGING_ENABLED];

        // Note: There is now stacking, so it works fine.
        if (SCROLL_BOUNCE == BOUNCE_NOTDEFAULT) {
            int x, y, rows;
            lastIconPosition(iconList, &x, &y);

            rows = [iconList infinifoldersDefaultRows];

            [scrollView setAlwaysBounceVertical:(y > rows)];
        }
    }
}
static CGFloat topIconPadding(id list) {
    NSInvocation *invocation = [[NSInvocation alloc] init];
    [invocation setTarget:list];
    [invocation setSelector:@selector(topIconInset)];
    [invocation invoke];
    CGFloat ret;
    [invocation getReturnValue:&ret];
    [invocation release];

    return ret;
}
static void fixListHeights() {
    if (disableResizeFlag)
        return;

    for (int i = 0; i < MIN([listies count], [scrollies count]); i++) {
        UIScrollView *scrollView = [scrollies objectAtIndex:i];
        CGPoint offset = [scrollView contentOffset];
        id iconList = [listies objectAtIndex:i];

        if (![[iconList icons] count])
            continue;

        CGSize newSize, oldSize;
        CGPoint farthestOffset;
        oldSize = [scrollView contentSize];

        int x, y;
        if (![[$SBIconController sharedInstance] isEditing])
            lastIconPosition(iconList, &x, &y);
        else
            firstFreeSlot(iconList, &x, &y);

        if (![scrollView isPagingEnabled]) {
            farthestOffset = [iconList originForIconAtX:x Y:y];

            if ([scrollView frame].size.height < farthestOffset.y) {
                newSize = CGSizeMake(scrollView.frame.size.width, farthestOffset.y + [scrollView frame].size.height - [iconList originForIconAtX:0 Y:(DEFAULT_ROWS_FOR_ORIENTATION([[UIDevice currentDevice] orientation])) - 1].y);
            } else {
                newSize = CGSizeMake(scrollView.frame.size.width, scrollView.frame.size.height);
            }
        } else {
            CGFloat totalHeight = (ceil(y / [iconList infinifoldersDefaultRows]) + 1) * ([scrollView frame].size.height);
            newSize = CGSizeMake(scrollView.frame.size.width, totalHeight);
        }

        if (!CGSizeEqualToSize(oldSize, newSize)) {
            [UIView beginAnimations:nil context:NULL];
            [UIView setAnimationDuration:0.4f];
            [scrollView setContentSize:newSize];
            [UIView commitAnimations];
            [scrollView setContentOffset:offset animated:NO];
        }
    }
}
static void preferenceChangedCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    [prefsDict release];
    prefsDict = [[NSDictionary alloc] initWithContentsOfFile:PreferencesFilePath];
    [[$SBIconModel sharedInstance] relayout];
}

@interface IFDebug
@end
@implementation IFDebug
+ (id)scrollies {return scrollies;}
+ (id)listies {return listies;}
+ (Class)listClass {return iconListClass;}
@end

%group IFGroup

%hook SBFolder
- (BOOL)isFull { return NO; }
%end

%hook SBFolderIconListModel
+ (int)maxIcons {
    if (disableRowsFlag)
        return %orig;

    return 50 * (int) [objc_getClass("SBIconListView") iconColumnsForInterfaceOrientation:[[UIDevice currentDevice] orientation]];
}
%end

%hook SBFolderIconListView
+ (int)maxIcons {
    if (disableRowsFlag)
        return %orig;

    return 50 * (int) [objc_getClass("SBIconListView") iconColumnsForInterfaceOrientation:[[UIDevice currentDevice] orientation]];
}
- (id)initWithFrame:(CGRect)frame {
    self = %orig;

    if (VALID_LIST(self)) {
        UIScrollView *scrollView = [[UIScrollView alloc] init];
        [self addSubview:scrollView];

        [scrollies addObject:scrollView];
        [scrollView release];
        [listies addObject:self];
        [self release];

        [scrollView setDelegate:(id<UIScrollViewDelegate>) self];
        [scrollView setDelaysContentTouches:NO];
        [scrollView setContentSize:[self bounds].size];

        [self setFrame:frame];

        applyPreferences();
        cache_init(self, MAX_ICON_ROWS(self), MAX_ICON_COLUMNS(self));
    }

    return self;
}
- (void)dealloc {
    if (VALID_LIST(self)) {
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];
        [scrollView removeFromSuperview];
        [scrollView setDelegate:nil];
        [[scrollView subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];

        [self retain];
        [listies removeObject:self];
        [scrollies removeObject:scrollView];

        cache_destroy(self);
    }

    %orig;
}
- (void)setFrame:(CGRect)frame {
    %orig;

    if (VALID_LIST(self)) {
        if (![listies containsObject:self] || [scrollies count] < [listies indexOfObject:self]) 
            return;

        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];

        frame = [self bounds];
        frame.size.height -= kBottomPadding;
        [scrollView setFrame:frame];

        [[$SBIconController sharedInstance] infinifoldersUpdateListHeights];
    }

    fixListHeights();
}
- (void)addSubview:(UIView *)subview {
    if (VALID_LIST(self) && [subview isKindOfClass:$SBIcon]) {
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];
        [scrollView addSubview:subview];
    } else {
        %orig;
    }

    fixListHeights();
}
- (void)_didRemoveSubview:(id)subview {
    %orig;

    fixListHeights();
}
- (void)setOrientation:(int)orientation {
    %orig;

    [[$SBIconController sharedInstance] infinifoldersUpdateListHeights];
}
- (void)cleanupAfterRotation {
    %orig;

    [self layoutIconsNow];
    [[$SBIconController sharedInstance] infinifoldersUpdateListHeights];
}
- (void)removeAllIcons {
    %orig;

    if (VALID_LIST(self)) {
        for (id iconList in listies) {
            // Huh? Why does this make the whole icon list blank?
            // [[iconList icons] makeObjectsPerformSelector:@selector(removeFromSuperview)];
        }
    }
}
- (CGPoint)originForIconAtX:(int)x Y:(int)y {
    if (cache_ready(self)) return cache_point(self, x, y);

    if (VALID_LIST(self) && !disableOriginFlag) {
        disableRowsFlag += 1;
        CGPoint ret;
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];

        if (PAGING_ENABLED) {
            int page = y / [self infinifoldersDefaultRows];
            ret = %orig(x, y % [self infinifoldersDefaultRows]);
            ret.y += ([scrollView frame].size.height) * page;
        } else {
            ret = %orig;
        }

        disableRowsFlag -= 1;
        return ret;
    } else {
        return %orig;
    }
}
- (int)maxIconRows {
    if (disableRowsFlag || !VALID_LIST(self))
        return %orig;

    return 50;
}
+ (int)iconRowsForInterfaceOrientation:(int)interfaceOrientation {
    if (disableRowsFlag)
        return %orig;

    return 50;
}
- (int)rowAtPoint:(CGPoint)point {
    if (VALID_LIST(self)) {
        disableRowsFlag += 1;
        point.y += [[scrollies objectAtIndex:[listies indexOfObject:self]] contentOffset].y;

        int row = 0;
        CGFloat top = [self topIconInset];
        CGFloat padding = [self verticalIconPadding];
        CGFloat icon = [$SBIcon defaultIconSize].height;
        CGFloat cur = top + icon + padding;

        while (cur < point.y) {
            row += 1;
            cur += icon + padding;
        }

        disableRowsFlag -= 1;
        return row;
    } else {
        return %orig;
    }
}
- (NSArray *)icons {
    NSArray *icons = %orig;

    if (VALID_LIST(self)) 
        icons = [icons subarrayWithRange:NSMakeRange(0, MIN(MAX_ICON_ROWS(self) * MAX_ICON_COLUMNS(self), [icons count]))];

    return icons;
}
%new(i@:)
- (int)infinifoldersDefaultRows {
    disableRowsFlag += 1;
    int ret = MAX_ICON_ROWS(self);
    disableRowsFlag -= 1;

    return ret;
}
%end

%hook SBUIController
- (void)finishLaunching {
    %orig;
    applyPreferences();
}
%end

%hook SBIconModel
- (void)relayout {
    %orig;
    applyPreferences();
    fixListHeights();
}
%end

%hook SBIconController
- (void)_openCloseFolderAnimationEnded:(id)ended finished:(id)finished context:(void *)context { 
    if ([scrollies count]) [[scrollies objectAtIndex:0] flashScrollIndicators];
    %orig;
}
- (void)moveIconFromWindow:(SBIcon *)icon toIconList:(id)iconList {
    if (VALID_LIST(iconList)) {
        CGRect frame = [icon frame];
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:iconList]];
        frame.origin.y += [scrollView contentOffset].y;
        [icon setFrame:frame];
    }

    %orig;
}
- (void)setGrabbedIcon:(SBIcon *)icon {
    for (UIScrollView *scrollView in scrollies) {
        [scrollView setScrollEnabled:!icon];
    }

    %orig;

    if (!icon) fixListHeights();
}
- (void)setIsEditing:(BOOL)editing {
    %orig;

    [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(infinifoldersUpdateListHeights) userInfo:nil repeats:NO];
}
%new(v@:)
- (void)infinifoldersUpdateListHeights {
    fixListHeights();
}
%end

%hook SBFolderView
typedef struct { int direction; CGRect rect; } notch_info_t;
- (id)initWithRows:(int)rows notchInfo:(notch_info_t)notchInfo {
    fixListHeights();
    return %orig(MIN(rows, DEFAULT_ROWS_FOR_ORIENTATION(0)), notchInfo);
}
- (id)initWithRows:(int)rows notchInfo:(notch_info_t)notchInfo orientation:(int)orientation {
    fixListHeights();
    return %orig(MIN(rows, DEFAULT_ROWS_FOR_ORIENTATION(orientation)), notchInfo, orientation);
}
- (void)setRows:(int)rows notchInfo:(notch_info_t)notchInfo orientation:(int)orientation {
    fixListHeights();
    return %orig(MIN(rows, DEFAULT_ROWS_FOR_ORIENTATION(orientation)), notchInfo, orientation);
}
%end

%end

/* Constructor */

__attribute__((constructor)) static void infinifolders_init() {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    // SpringBoard only!
    if (![[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.springboard"])
        return;

    NSLog(@"Welcome to Infinifolders.");
    NSLog(@"Funny quote goes here.");

    prefsDict = [[NSDictionary alloc] initWithContentsOfFile:PreferencesFilePath];
    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, preferenceChangedCallback, CFSTR(PreferencesChangedNotification), NULL, CFNotificationSuspensionBehaviorCoalesce);

    dlopen("/Library/MobileSubstrate/DynamicLibraries/IconSupport.dylib", RTLD_LAZY);
    [[objc_getClass("ISIconSupport") sharedInstance] addExtension:@"infinifolders"];

    scrollies = [[NSMutableArray alloc] init];
    listies = [[NSMutableArray alloc] init];

    %init(IFGroup);
    infinishared_init();

    [pool release];
}
