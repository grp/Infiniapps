/* Imports {{{ */
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>

#import <dlfcn.h>
#import <objc/runtime.h>

#import <substrate.h>

#import "infinishared/Infinishared.h"
#import "infinishared/Preferences.h"
/* }}} */


/* Macros {{{ */
#define IFMacroQuote_(x) #x
#define IFMacroQuote(x) IFMacroQuote_(x)
#define $IFIconList objc_getClass(IFMacroQuote(IFIconList))
/* }}} */
/* Global variables and flags {{{ */

static NSMutableArray *listies = nil;
static NSMutableArray *scrollies = nil;

static int disableRowsFlag = 0;
static int disableOriginFlag = 0;
static int disableResizeFlag = 0;

/* }}} */
/* Prototypes {{{ */

@interface UIDevice (Private)
- (BOOL)isWildcat;
@end

@interface SBIconViewMap : NSObject { }
+ (SBIconViewMap *)homescreenMap;
- (id)iconViewForIcon:(id)icon;
@end

@interface SBIconController : NSObject { }
+ (SBIconController *)sharedInstance;
- (UIInterfaceOrientation)orientation;
- (BOOL)isEditing;
@end

@interface IFIcon : UIView { }
+ (CGSize)defaultSize;
@end

@interface IFIconList : UIView { }
+ (int)iconRowsForInterfaceOrientation:(UIInterfaceOrientation)orientation;
+ (int)iconColumnsForInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (NSArray *)icons;

- (CGPoint)originForIconAtX:(int)x Y:(int)y;

- (int)firstFreeSlotIndex;
- (void)firstFreeSlotIndex:(int *)index;
- (void)firstFreeSlotX:(int *)x Y:(int *)y;

- (void)getX:(int *)x Y:(int *)y forIndex:(int)index forOrientation:(UIInterfaceOrientation)orientation;

- (CGFloat)topIconInset;
- (CGFloat)verticalIconPadding;

- (void)layoutIconsNow;
@end

/* }}} */
/* IFIconController {{{ */

static SBIconController *IFIconControllerSharedInstance() {
    return (SBIconController *) [objc_getClass("SBIconController") sharedInstance];
}

/* }}} */
/* IFDevice {{{ */

static BOOL IFDeviceIsPad() {
    // XXX: this isn't a public API, might not always exist
    return [[UIDevice currentDevice] isWildcat];
}

static UIInterfaceOrientation IFDeviceCurrentInterfaceOrientation() {
    return [IFIconControllerSharedInstance() orientation];
}

/* }}} */
/* IFIconView {{{ */

static Class IFIconViewClass() {
    Class iconView = objc_getClass("SBIconView");
    Class icon = objc_getClass("SBIcon");

    if (iconView != nil) return iconView;
    if (icon != nil) return icon;

    return nil;
}


static IFIcon *IFIconViewForIcon(id icon) {
    Class map = objc_getClass("SBIconViewMap");

    if (map != nil) {
        return [[map homescreenMap] iconViewForIcon:icon];
    } else {
        return icon;
    }
}

static CGSize IFIconViewDefaultSize() {
    Class iconViewClass = IFIconViewClass();
    SEL defaultIconSize = @selector(defaultIconSize);
    CGSize size = CGSizeZero;

    NSMethodSignature *signature = [iconViewClass methodSignatureForSelector:defaultIconSize];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setTarget:iconViewClass];
    [invocation setSelector:defaultIconSize];
    [invocation invoke];
    [invocation getReturnValue:&size];

    return size;
}

/* }}} */
/* IFIconList {{{ */

#define MAX_ICON_ROWS(list) ([$IFIconList iconRowsForInterfaceOrientation:IFDeviceCurrentInterfaceOrientation()])
#define MAX_ICON_COLUMNS(list) ([$IFIconList iconColumnsForInterfaceOrientation:IFDeviceCurrentInterfaceOrientation()])
#define DEFAULT_ROWS_FOR_ORIENTATION(o) (!(IFDeviceIsPad()) ? (UIInterfaceOrientationIsLandscape(o) ? 2 : 3) : (UIInterfaceOrientationIsLandscape(o) ? 4 : 5))

static void IFIconListFirstFreeSlot(IFIconList *iconList, int *xptr, int *yptr) {
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
            [iconList getX:&x Y:&y forIndex:idx forOrientation:IFDeviceCurrentInterfaceOrientation()];
        } else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex)]) {
            int idx = [iconList firstFreeSlotIndex];
            [iconList getX:&x Y:&y forIndex:idx forOrientation:IFDeviceCurrentInterfaceOrientation()];
        }
    }

    *xptr = x;
    *yptr = y;
/*
    if ([iconList respondsToSelector:@selector(firstFreeSlotX:Y:)]) {
        [iconList firstFreeSlotX:x Y:y];
    } else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex:)]) {
        int idx;
        [iconList firstFreeSlotIndex:&idx];
        [iconList getX:x Y:y forIndex:idx forOrientation:IFDeviceCurrentInterfaceOrientation()];
    } else if ([iconList respondsToSelector:@selector(firstFreeSlotIndex)]) {
        int idx = [iconList firstFreeSlotIndex];
        [iconList getX:x Y:y forIndex:idx forOrientation:IFDeviceCurrentInterfaceOrientation()];
    }
*/
}

static void IFIconListLastIconPosition(IFIconList *iconList, int *xptr, int *yptr) {
    int x, y;

    IFIconListFirstFreeSlot(iconList, &x, &y);

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

static int IFIconListDefaultRows(IFIconList *iconList) {
    disableRowsFlag += 1;
    int ret = MAX_ICON_ROWS(iconList);
    disableRowsFlag -= 1;

    return ret;
}

static void IFIconListFixHeight(IFIconList *iconList) {
    UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:iconList]];
    CGPoint offset = [scrollView contentOffset];
    CGFloat iconHeight = IFIconViewDefaultSize().height;

    if (!IFDeviceIsPad() && UIInterfaceOrientationIsLandscape(IFDeviceCurrentInterfaceOrientation())) {
        // Hack for SBRotator's fail.
        CGRect listFrame = [iconList bounds];
        int icons = [[iconList icons] count];
        int rows = icons / MAX_ICON_COLUMNS(iconList);
        if (icons % MAX_ICON_COLUMNS(iconList)) rows += 1;
        listFrame.size.height = rows * (iconHeight + [iconList verticalIconPadding]);
        [scrollView setFrame:listFrame];
    } else {
        CGRect listFrame = [iconList bounds];
        listFrame.size.height -= IFBottomPadding;
        [scrollView setFrame:listFrame];
    }

    if (![[iconList icons] count])
        return;

    CGSize newSize, oldSize;
    CGPoint farthestOffset;
    oldSize = [scrollView contentSize];

    int x, y;
    if (![IFIconControllerSharedInstance() isEditing])
        IFIconListLastIconPosition(iconList, &x, &y);
    else
        IFIconListFirstFreeSlot(iconList, &x, &y);

    if (![scrollView isPagingEnabled]) {
        farthestOffset = [iconList originForIconAtX:x Y:y];

        if ([scrollView frame].size.height < farthestOffset.y) {
            newSize = CGSizeMake(scrollView.frame.size.width, farthestOffset.y + [scrollView frame].size.height - [iconList originForIconAtX:0 Y:(DEFAULT_ROWS_FOR_ORIENTATION(IFDeviceCurrentInterfaceOrientation())) - 1].y);
        } else {
            newSize = CGSizeMake(scrollView.frame.size.width, scrollView.frame.size.height);
        }
    } else {
        CGFloat totalHeight = (ceil(y / IFIconListDefaultRows(iconList)) + 1) * ([scrollView frame].size.height);
        newSize = CGSizeMake(scrollView.frame.size.width, totalHeight);
    }

    // Make sure the content offset is never below the bottom of the scroll view.
    if (offset.y + [scrollView bounds].size.height > newSize.height) {
        offset.y = newSize.height - [scrollView bounds].size.height;
    }

    if (!CGSizeEqualToSize(oldSize, newSize)) {
        [UIView beginAnimations:nil context:NULL];
        [UIView setAnimationDuration:0.4f];
        [scrollView setContentSize:newSize];
        [scrollView setContentOffset:offset animated:NO];
        [UIView commitAnimations];
    }
}

/* }}} */
/* Helpers {{{ */

__attribute__((unused)) static int IFMinimum(int x, int y) {
    return x < y ? x : y;
}

__attribute__((unused)) static int IFMaximum(int x, int y) {
    return x > y ? x : y;
}

static BOOL IFIconListIsValid(id iconList) {
    return [iconList isMemberOfClass:$IFIconList];
}

static void IFFixListHeights() {
    if (disableResizeFlag)
        return;

    for (int i = 0; i < IFMinimum([listies count], [scrollies count]); i++) {
        IFIconList *iconList = [listies objectAtIndex:i];
        IFIconListFixHeight(iconList);
    }
}

typedef enum {
    kIFScrollbarStyleBlack,
    kIFScrollbarStyleWhite,
    kIFScrollbarStyleNone
} IFScrollbarStyle;

typedef enum {
    kIFScrollBounceEnabled,
    kIFScrollBounceExtra,
    kIFScrollBounceDisabled
} kIFScrollBounce;

static void IFPreferencesApply() {
    for (int i = 0; i < IFMinimum([listies count], [scrollies count]); i++) {
        UIScrollView *scrollView = [scrollies objectAtIndex:i];
        id iconList = [listies objectAtIndex:i];

        [iconList addSubview:scrollView];

        BOOL scroll = IFPreferencesBoolForKey(@"ScrollEnabled", YES);
        int bounce = IFPreferencesIntForKey(@"ScrollBounce", 0);
        int bar = IFPreferencesIntForKey(@"ScrollbarStyle", 0);
        BOOL page = IFPreferencesBoolForKey(@"PagingEnabled", NO);
        BOOL clips = IFPreferencesBoolForKey(@"ClipsToBounds", YES);

        [scrollView setShowsVerticalScrollIndicator:YES];
        if (bar == kIFScrollbarStyleBlack)
            [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleDefault];
        else if (bar == kIFScrollbarStyleWhite)
            [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleWhite];
        else if (bar == kIFScrollbarStyleNone)
            [scrollView setShowsVerticalScrollIndicator:NO];

        [scrollView setScrollEnabled:scroll];
        [scrollView setAlwaysBounceVertical:bounce == kIFScrollBounceEnabled];
        [scrollView setBounces:bounce != kIFScrollBounceDisabled];
        [scrollView setPagingEnabled:page];
        [scrollView setClipsToBounds:clips];
        [iconList setClipsToBounds:clips];

        if (bounce == kIFScrollBounceExtra) {
            int x, y, rows;
            IFIconListLastIconPosition(iconList, &x, &y);

            rows = IFIconListDefaultRows(iconList);

            [scrollView setAlwaysBounceVertical:(y > rows)];
        }
    }
}

/* }}} */
/* Hooks {{{ */

%group IFBasic

%hook IFIconList

+ (int)maxIcons {
    if (disableRowsFlag)
        return %orig;

    return 50 * [$IFIconList iconColumnsForInterfaceOrientation:IFDeviceCurrentInterfaceOrientation()];
}

- (id)initWithFrame:(CGRect)frame {
    self = %orig;

    if (IFIconListIsValid(self)) {
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

        IFPreferencesApply();
        cache_init(self, MAX_ICON_ROWS(self), MAX_ICON_COLUMNS(self));
    }

    return self;
}

- (void)dealloc {
    if (IFIconListIsValid(self)) {
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

    if (IFIconListIsValid(self)) {
        if (![listies containsObject:self] || [scrollies count] < [listies indexOfObject:self]) 
            return;

        IFFixListHeights();
    }

    IFFixListHeights();
}

- (void)didAddSubview:(UIView *)subview {
    if (IFIconListIsValid(self) && [subview isKindOfClass:IFIconViewClass()]) {
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];
        [scrollView addSubview:subview];
    } else {
        %orig;
    }

    IFFixListHeights();
}

- (void)_didRemoveSubview:(id)subview {
    %orig;

    IFFixListHeights();
}

- (void)setOrientation:(int)orientation {
    %orig;

    IFFixListHeights();
}

- (void)cleanupAfterRotation {
    %orig;

    [self layoutIconsNow];
    IFFixListHeights();
}

- (void)removeAllIcons {
    %orig;

    if (IFIconListIsValid(self)) {
        for (id iconList in listies) {
            // Huh? Why does this make the whole icon list blank?
            // [[iconList icons] makeObjectsPerformSelector:@selector(removeFromSuperview)];
        }
    }
}

- (CGPoint)originForIconAtX:(int)x Y:(int)y {
    if (cache_ready(self)) return cache_point(self, x, y);

    if (IFIconListIsValid(self) && !disableOriginFlag) {
        disableRowsFlag += 1;
        CGPoint ret;
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:self]];

        if (IFPreferencesBoolForKey(@"PagingEnabled", NO)) {
            int page = y / IFIconListDefaultRows(self);
            ret = %orig(x, y % IFIconListDefaultRows(self));
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
    if (disableRowsFlag || !IFIconListIsValid(self))
        return %orig;

    return 50;
}

+ (int)iconRowsForInterfaceOrientation:(int)interfaceOrientation {
    if (disableRowsFlag)
        return %orig;

    return 50;
}

- (int)rowAtPoint:(CGPoint)point {
    if (IFIconListIsValid(self)) {
        disableRowsFlag += 1;
        point.y += [[scrollies objectAtIndex:[listies indexOfObject:self]] contentOffset].y;

        int row = 0;
        CGFloat top = [self topIconInset];
        CGFloat padding = [self verticalIconPadding];
        CGFloat icon = IFIconViewDefaultSize().height;
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

    if (IFIconListIsValid(self))
        icons = [icons subarrayWithRange:NSMakeRange(0, IFMinimum(MAX_ICON_ROWS(self) * MAX_ICON_COLUMNS(self), [icons count]))];

    return icons;
}

%end

%hook SBUIController

- (void)finishLaunching {
    %orig;
    IFPreferencesApply();
}

%end

%hook SBIconModel

- (void)relayout {
    %orig;

    IFPreferencesApply();
    IFFixListHeights();
}

%end

%hook SBIconController

- (void)moveIconFromWindow:(id)icon toIconList:(IFIconList *)iconList {
    if (IFIconListIsValid(iconList)) {
        IFIcon *iconView = IFIconViewForIcon(icon);

        CGRect frame = [iconView frame];
        UIScrollView *scrollView = [scrollies objectAtIndex:[listies indexOfObject:iconList]];
        frame.origin.y += [scrollView contentOffset].y;
        [iconView setFrame:frame];
    }

    %orig;
}

- (void)setGrabbedIcon:(id)icon {
    for (UIScrollView *scrollView in scrollies) {
        [scrollView setScrollEnabled:(icon == nil)];
    }

    %orig;

    if (icon == nil) IFFixListHeights();
}

- (void)setIsEditing:(BOOL)editing {
    %orig;

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0), dispatch_get_current_queue(), ^{
        IFFixListHeights();
    });
}

%end

%end

/* }}} */

