
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


#define idForKeyWithDefault(dict, key, default)	 ([(dict) objectForKey:(key)]?:(default))
#define floatForKeyWithDefault(dict, key, default)   ({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result floatValue]:(default); })
#define NSIntegerForKeyWithDefault(dict, key, default) (NSInteger)({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result integerValue]:(default); })
#define BOOLForKeyWithDefault(dict, key, default)    (BOOL)({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result boolValue]:(default); })

#define PreferencesFilePath [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/com.chpwn.infiniboard.plist"]
#define PreferencesChangedNotification "com.chpwn.infiniboard.prefs"

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
%class SBIconListView;

// Hacks!
%class CategoryView;
%class OverBoardPageView;

@interface SBIconListView : SBIconList
- (float)topIconInset;
@end

/* Preferences */

#define ScrollEnabled YES
#define SCROLL_ENABLED GetPreference(ScrollEnabled, BOOL)

#define RestoreEnabled NO
#define RESTORE_ENABLED GetPreference(RestoreEnabled, BOOL)

#define FastRestoreEnabled NO
#define FAST_RESTORE_ENABLED GetPreference(FastRestoreEnabled, BOOL)

#define ConsistentRows NO
#define CONSISTENT_ROWS GetPreference(ConsistentRows, BOOL)

#define ClipsToBounds YES
#define CLIPS_TO_BOUNDS GetPreference(ClipsToBounds, BOOL)

#define PagingEnabled NO
#define PAGING_ENABLED GetPreference(PagingEnabled, BOOL)

#define BOUNCE_ENABLED 0
#define BOUNCE_NOTDEFAULT 1
#define BOUNCE_DISABLED 2
#define ScrollBounce 0
#define SCROLL_BOUNCE GetPreference(ScrollBounce, NSInteger)

#define SCROLLBAR_BLACK 0
#define SCROLLBAR_WHITE 1
#define SCROLLBAR_DISABLED 2
#define ScrollbarStyle 0
#define SCROLLBAR_STYLE GetPreference(ScrollbarStyle, NSInteger)

/* Macros */

#define ICON_HEIGHT ((CGFloat) [$SBIcon defaultIconSize].height)
#define VALID_LIST(list) ([list class] == iconListClass && [listies containsObject:list])
#define WILDCAT ([UIDevice instancesRespondToSelector:@selector(isWildcat)] && [[UIDevice currentDevice] performSelector:@selector(isWildcat)])
#define NEW_STYLE (WILDCAT || !!objc_getClass("SBIconListView"))
#define MAX_ICON_ROWS(list) ((int) (NEW_STYLE ? (int) [list iconRowsForCurrentOrientation] : (int) [list maxIconRows]))
#define MAX_ICON_COLUMNS(list) ((int) (NEW_STYLE ? (int) [list iconColumnsForCurrentOrientation] : (int) [list maxIconColumns]))

/* Categories */

@interface SBIconList (Infiniboard)
- (int)infiniboardDefaultRows;
@end

@interface SBIconController (Infiniboard)
- (void)infiniboardUpdateListHeights;
@end

#if TARGET_IPHONE_SIMULATOR
static NSString *IBFirmwareVersion() { return @"4.0"; }
#else
extern "C" id lockdown_connect();
extern "C" void lockdown_disconnect(id port);
extern "C" NSString *lockdown_copy_value(id port, int idk, CFStringRef value);

extern "C" CFStringRef kLockdownDeviceNameKey;
extern "C" CFStringRef kLockdownBuildVersionKey;    // buildVersion
extern "C" CFStringRef kLockdownProductVersionKey;    // systemVersion
extern "C" CFStringRef kLockdownDeviceClassKey;      // model
extern "C" CFStringRef kLockdownProductTypeKey;
extern "C" CFStringRef kLockdownUniqueDeviceIDKey;

static NSString *IBFirmwareVersion() {
	id port = nil;
	NSString *val = nil;
	if((port = lockdown_connect())) {
		val = lockdown_copy_value(port, 0, kLockdownProductVersionKey);
		[val autorelease];
		lockdown_disconnect(port);
	}

	return val;
}
#endif

/* Global variables and flags */

static NSMutableArray *listies = nil;
static NSMutableArray *scrollies = nil;
static NSDictionary *prefsDict = nil;
static Class iconListClass;

static int disableRowsFlag = 0;
static int disableOriginFlag = 0; 
static int disableResizeFlag = 0;
static int disableIconsFlag = 0;

#define kBottomPadding 0.0f

/* Utility methods */

static void firstFreeSlot(id iconList, int *xptr, int *yptr) {
	int x, y;

	if ([iconList respondsToSelector:@selector(gridlockLastIconX:Y:)]) {
		[iconList gridlockLastIconX:&x Y:&y];

		if (x == MAX_ICON_COLUMNS(iconList) - 1) {
			x = 0;
			y += 1;
		} else {
			x += 1;
		}
	} else {
		if ([iconList respondsToSelector:@selector(firstFreeSlotX:Y:)]) {
			[iconList firstFreeSlotX:&x Y:&y];
		} else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex:)]) {
			int idx;
			[iconList firstFreeSlotIndex:&idx];
			[iconList getX:&x Y:&y forIndex:idx forOrientation:[[UIDevice currentDevice] orientation]];
		} else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex)]) {
			[iconList getX:&x Y:&y forIndex:[iconList firstFreeSlotIndex] forOrientation:[[UIDevice currentDevice] orientation]];
		}
	}

	*xptr = x;
	*yptr = y;
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
		SBIconList *iconList = [listies objectAtIndex:i];

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
		[scrollView setClipsToBounds:CLIPS_TO_BOUNDS];
		[iconList setClipsToBounds:CLIPS_TO_BOUNDS];
		[scrollView setPagingEnabled:PAGING_ENABLED];

		if (SCROLL_BOUNCE == BOUNCE_NOTDEFAULT) {
			int x, y, rows;
			lastIconPosition(iconList, &x, &y);

			rows = [iconList infiniboardDefaultRows];

			[scrollView setAlwaysBounceVertical:(y > rows)];
		}
	}
}
static CGFloat topIconPadding(id list) {
	NSInvocation *invocation = [[NSInvocation alloc] init];
	[invocation setTarget:list];
	if ([list respondsToSelector:@selector(topIconPadding)]) {
		[invocation setSelector:@selector(topIconPadding)];
	} else {
		[invocation setSelector:@selector(topIconInset)];
	}
	[invocation invoke];
	CGFloat ret;
	[invocation getReturnValue:&ret];
	[invocation release];
	return ret;
}
static CGFloat verticalIconPadding(id list) {
	NSInvocation *invocation = [[NSInvocation alloc] init];
	[invocation setTarget:list];
	[invocation setSelector:@selector(verticalIconPadding)];
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
				newSize = CGSizeMake(scrollView.frame.size.width, farthestOffset.y + ICON_HEIGHT + topIconPadding(iconList));
			} else {
				newSize = CGSizeMake(scrollView.frame.size.width, scrollView.frame.size.height - kBottomPadding);
			}
		} else {
			CGFloat totalHeight = (ceil(y / [iconList infiniboardDefaultRows]) + 1) * ([scrollView frame].size.height + kBottomPadding);
			newSize = CGSizeMake(scrollView.frame.size.width, totalHeight);
		}

		// add back what we subtracted from the bottom
		newSize.height += kBottomPadding;

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
static void restoreIconList() {
	applyPreferences();

	for (UIScrollView *scrollView in scrollies) {
		if (RESTORE_ENABLED)
			[scrollView setContentOffset:CGPointZero animated:NO];

		if (SCROLLBAR_STYLE != SCROLLBAR_DISABLED)
			[scrollView flashScrollIndicators];
	}
}
static SBIcon *iconWithIdentifier(NSString *identifier) {
	SBIcon *icon;
	SBIconModel *iconModel = [$SBIconModel sharedInstance];
	if ([iconModel respondsToSelector:@selector(leafIconForIdentifier:)])
	    icon = [iconModel leafIconForIdentifier:identifier];
	else
	    icon = [iconModel iconForDisplayIdentifier:identifier];

	return icon;
}
static id currentIconList() {
	id iconList;
	if ([$SBIconController instancesRespondToSelector:@selector(currentIconList)])
		iconList = [[$SBIconController sharedInstance] currentIconList];
	else
		iconList = [[$SBIconController sharedInstance] currentRootIconList];
	return iconList;
}
static void fixDockOrdering() {
	// FIXME: please be less lame
	UIView *dockSuperview;
	if ([$SBIconModel instancesRespondToSelector:@selector(buttonBar)])
		dockSuperview = [[[$SBIconModel sharedInstance] buttonBar] superview];
	else
		dockSuperview = [[[$SBIconController sharedInstance] dock] superview];

	id iconList = currentIconList();

	[[[[iconList superview] superview] superview] bringSubviewToFront:dockSuperview];
}

%group IFGroup

%hook SBIconListModel
+ (int)maxIcons {
	if (disableRowsFlag || [self class] != objc_getClass("SBIconListModel"))
		return %orig;

	return 50 * (int) [objc_getClass("SBIconListView") iconColumnsForInterfaceOrientation:[[UIDevice currentDevice] orientation]];
}
%end

%hook IconList
+ (int)maxIcons {
	if (disableRowsFlag || [self class] != objc_getClass("SBIconListView"))
		return %orig;

	return 50 * (int) [objc_getClass("SBIconListView") iconColumnsForInterfaceOrientation:[[UIDevice currentDevice] orientation]];
}
- (id)initWithFrame:(CGRect)frame {
	self = %orig;

	if ([self isMemberOfClass:iconListClass]) {
		UIScrollView *scrollView = [[UIScrollView alloc] init];
		[self addSubview:scrollView];

		[scrollies addObject:scrollView];
		[listies addObject:self];
		[scrollView release];
		[self release];

		[scrollView setDelegate:(id<UIScrollViewDelegate>) self];
		[scrollView setDelaysContentTouches:NO];
		[scrollView setContentSize:[(UIView *) self bounds].size];

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

		CGRect frame = [(UIView *) self bounds];
		frame.size.height -= kBottomPadding;
		[scrollView setFrame:frame];

		[[$SBIconController sharedInstance] infiniboardUpdateListHeights];
	}
}
- (void)addSubview:(UIView *)subview {
	if (VALID_LIST(self) && [subview isKindOfClass:$SBIcon]) {
		UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];
		[scrollView addSubview:subview];

		fixListHeights();
	} else {
		%orig;
	}
}
- (void)_didRemoveSubview:(id)subview {
	%orig;

	if (VALID_LIST(self)) fixListHeights();
}
- (void)setOrientation:(int)orientation {
	%orig;

	if (VALID_LIST(self)) [[$SBIconController sharedInstance] infiniboardUpdateListHeights];
}
- (void)removeAllIcons {
	%orig;

	if (VALID_LIST(self)) {
		for (SBIconList *iconList in listies) {
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

		if (PAGING_ENABLED) {
			int row = y / (int) [self infiniboardDefaultRows];
			ret = %orig(x, y % (int) [self infiniboardDefaultRows]);
			ret.y += ([(UIView *) self frame].size.height - kBottomPadding) * row;
		} else {
			ret = %orig;
		}

		disableRowsFlag -= 1;

		return ret;
	} else if (!disableOriginFlag) {
        disableRowsFlag += 1;
		CGPoint ret = %orig;
        disableRowsFlag -= 1;
        return ret;
	} else {
        return %orig;
    }
}
- (int)maxIconRows {
	if (disableRowsFlag || !VALID_LIST(self)) {
		return %orig;
	}

	return 50;
}
- (int)iconRowsForInterfaceOrientation:(int)interfaceOrientation {
	if (disableRowsFlag || !VALID_LIST(self)) {
		return %orig;
	}

	return 50;
}
+ (int)iconRowsForInterfaceOrientation:(int)interfaceOrientation {
	if (disableRowsFlag || self != iconListClass)
		return %orig;

	return 50;
}
- (int)rowAtPoint:(CGPoint)point {
	int row = -1;

	if (VALID_LIST(self)) {
		disableRowsFlag += 1;
		point = [[[$SBIconController sharedInstance] grabbedIcon] center];

		CGFloat offset = [[scrollies objectAtIndex:[listies indexOfObject:self]] contentOffset].y;
		CGFloat top = topIconPadding(self) - offset;
		CGFloat padding = verticalIconPadding(self);
		CGFloat icon = ICON_HEIGHT;
		CGFloat cur = top + icon + padding;

		if (PAGING_ENABLED) {
			row = floorf((point.y + offset) / [(UIView *) self frame].size.height) * MAX_ICON_ROWS(self);
		} else {
			point.y += offset;
		}

		while (cur < point.y) {
			row += 1;
			cur += icon + padding;
		}

		disableRowsFlag -= 1;
	} else {
		row = %orig;
	}

	return row;
}
- (void)setTag:(int)tag {
	%orig;

	if (VALID_LIST(self) && (tag == 23954 || tag == 0x4645)) {
		UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];
        [self retain]; [scrollView retain];

		[scrollies removeObject:scrollView];
		[listies removeObject:self];

		for (UIView *subview in [scrollView subviews]) {
            [self addSubview:subview];
		}

		[scrollView removeFromSuperview];

        cache_destroy(self);
	}
}
- (NSArray *)icons {
	NSArray *icons = %orig;

    if (VALID_LIST(self) && disableIconsFlag)
		icons = [icons subarrayWithRange:NSMakeRange(0, MIN(MAX_ICON_ROWS(self) * MAX_ICON_COLUMNS(self), [icons count]))];

	return icons;
}
- (int)rowForIcon:(SBIcon *)icon {
	int ret = %orig;

	if (disableRowsFlag) {
		CGPoint point = [icon frame].origin;
		ret = 0;
		CGFloat top = topIconPadding(self);
		CGFloat padding = verticalIconPadding(self);
		CGFloat icon = [$SBIcon defaultIconSize].height;
		CGFloat cur = top + icon + padding;

		while (cur < point.y) {
			ret += 1;
			cur += icon + padding;
		}

		if (ret >= MAX_ICON_ROWS(self)) ret = MAX_ICON_ROWS(self) - 1;
	}

	return ret;
}
%new(i@:)
- (int)infiniboardDefaultRows {
	disableRowsFlag	+= 1;
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
- (void)restoreIconList:(BOOL)unk {
	%orig;
	restoreIconList();
}
- (void)restoreIconListAnimated:(BOOL)animated {
	%orig;
	restoreIconList();
}
- (void)restoreIconListAnimated:(BOOL)animated animateWallpaper:(BOOL)animateWallpaper {
	%orig;
	restoreIconList();
}
- (void)restoreIconListAnimated:(BOOL)animated animateWallpaper:(BOOL)wallpaper keepSwitcher:(BOOL)switcher {
	%orig;
	restoreIconList();
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

	[NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(infiniboardUpdateListHeights) userInfo:nil repeats:NO];
}
- (void)scrollViewDidEndDecelerating:(id)scrollView {
	if (FAST_RESTORE_ENABLED) {
		for (UIScrollView *scrollView in scrollies) {
			[scrollView setContentOffset:CGPointZero animated:NO];
		}
	}

	%orig;
}
%new(v@:)
- (void)infiniboardUpdateListHeights {
	fixListHeights();
	applyPreferences();
}
- (void)_slideFolderOpen:(BOOL)open animated:(BOOL)animated {
	disableRowsFlag += 1;
	// We disable the rows here so that the folder can slide SpringBoard "upwards" if necessary.
	%orig;
	disableRowsFlag -= 1;
}
%end

%hook SBFolderIcon
- (void)launch {
	for (int i = 0; i < [scrollies count]; i++) {
		UIScrollView *scrollView = [scrollies objectAtIndex:i];
		id iconList = [listies objectAtIndex:i];

		if ([[iconList icons] containsObjectIdenticalTo:self]) [scrollView scrollRectToVisible:[(UIView *) self frame] animated:NO];
	}

	%orig;
}
%end

%hook OverBoardPageView
- (void)setIconList:(SBIconList *)list {
	disableRowsFlag += 1;
	disableOriginFlag += 1;
	disableIconsFlag += 1;
	%orig;
	disableIconsFlag -= 1;
	disableOriginFlag -= 1;
	disableRowsFlag -= 1;
}
%end

%hook CategoryView
- (id)initWithDisplayIdentifier:(id)displayIdentifier {
	disableRowsFlag += 1;
	disableOriginFlag += 1;
	self = %orig;
	disableOriginFlag -=  1;
	disableRowsFlag -=  1;

	return self;
}
%end

%end

/* Constructor */

__attribute__((constructor)) static void infiniboard_init() {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	// SpringBoard only!
	if (![[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.springboard"])
		return;

	NSLog(@"Welcome to Infiniboard.");
	NSLog(@"IT'S A TRAP!");

	prefsDict = [[NSDictionary alloc] initWithContentsOfFile:PreferencesFilePath];
	CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, preferenceChangedCallback, CFSTR(PreferencesChangedNotification), NULL, CFNotificationSuspensionBehaviorCoalesce);

	// Load other extensions
	dlopen("/Library/MobileSubstrate/DynamicLibraries/SixRows.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/7x7SpringBoard.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/CategoriesSB.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/FCSB.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/Iconoclasm.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/FiveIRows.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/FiveIRowsPart1.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/FiveIRowsPart2.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/FiveIRowsPart3.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/OverBoard.dylib", RTLD_LAZY);
	dlopen("/Library/MobileSubstrate/DynamicLibraries/LockInfo.dylib", RTLD_LAZY);

	dlopen("/Library/MobileSubstrate/DynamicLibraries/IconSupport.dylib", RTLD_LAZY);
	[[objc_getClass("ISIconSupport") sharedInstance] addExtension:@"infiniboard"];

	scrollies = [[NSMutableArray alloc] init];
	listies = [[NSMutableArray alloc] init];

	iconListClass = objc_getClass("SBIconList") ?: objc_getClass("SBIconListView");
	static Class meta = object_getClass(iconListClass);
	%init(IFGroup, IconList=iconListClass, +IconList=meta);
	infinishared_init();

	[pool release];
}
