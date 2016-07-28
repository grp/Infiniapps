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

/* Imports {{{ */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>

#import <dlfcn.h>
#import <objc/runtime.h>

#import <substrate.h>

#import "Preferences.h"
#import "iPhonePrivate.h"

/* }}} */

/* Configuration Macros {{{ */

#define IFMacroQuote_(x) #x
#define IFMacroQuote(x) IFMacroQuote_(x)

#define IFMacroConcat_(x, y) x ## y
#define IFMacroConcat(x, y) IFMacroConcat_(x, y)

#ifndef IFConfigurationTweakIdentifier
    #error "You must define a IFConfigurationTweakIdentifier."
#endif

#ifndef IFConfigurationListClass
    #define IFConfigurationListClass SBIconListView
#endif

#ifndef IFConfigurationListClassObject
    #define IFConfigurationListClassObject NSClassFromString(@IFMacroQuote(IFConfigurationListClass))
#endif

#ifndef IFConfigurationScrollViewClass
    #define IFConfigurationScrollViewClass UIScrollView
#endif

#ifndef IFConfigurationExpandWhenEditing
    #define IFConfigurationExpandWhenEditing YES
#endif

#ifndef IFConfigurationFullPages
    #define IFConfigurationFullPages NO
#endif

#ifndef IFConfigurationExpandHorizontally
    #define IFConfigurationExpandHorizontally NO
#endif

#ifndef IFConfigurationExpandVertically
    #define IFConfigurationExpandVertically YES
#endif

#ifndef IFConfigurationDynamicColumns
    #define IFConfigurationDynamicColumns NO
#endif

#ifndef IFConfigurationExpandedDimension
    // Must be less than sqrt(INT32_MAX).
    #define IFConfigurationExpandedDimension 10000
#endif

/* }}} */

/* Flags {{{ */

// Custom control structure for managing flags safely.
// Usage: IFFlag(IFFlagNamedThis) { /* code with flag enabled */ }
// Do not return out of this structure, or the flag is stuck.
#define IFFlag_(flag, c) \
    if (1) { \
        flag += 1; \
        goto IFMacroConcat(body, c); \
    } else \
        while (1) \
            if (1) { \
                flag -= 1; \
                break; \
            } else \
                IFMacroConcat(body, c):
#define IFFlag(flag) IFFlag_(flag, __COUNTER__)

static NSUInteger IFFlagExpandedFrame = 0;
static NSUInteger IFFlagDefaultDimensions = 0;

#ifndef kCFCoreFoundationVersionNumber_iOS_9_3
#define kCFCoreFoundationVersionNumber_iOS_9_3 1280.30
#endif

/* }}} */

/* Conveniences {{{ */

__attribute__((unused)) static NSUInteger IFMinimum(NSUInteger x, NSUInteger y) {
    return (x < y ? x : y);
}

__attribute__((unused)) static NSUInteger IFMaximum(NSUInteger x, NSUInteger y) {
    return (x > y ? x : y);
}

__attribute__((unused)) static SBIconView *IFIconViewForIcon(SBIcon *icon) {
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_9_3) {
        SBIconViewMap *iconViewMP = [[objc_getClass("SBIconController") sharedInstance] homescreenIconViewMap];
        return [iconViewMP iconViewForIcon:icon];
    } else {
        Class map = NSClassFromString(@"SBIconViewMap");
        return [[map homescreenMap] iconViewForIcon:icon];
    }
}

__attribute__((unused)) static SBIconController *IFIconControllerSharedInstance() {
    return (SBIconController *) [NSClassFromString(@"SBIconController") sharedInstance];
}

__attribute__((unused)) static BOOL IFIconListIsValid(SBIconListView *listView) {
    return [listView isMemberOfClass:IFConfigurationListClassObject];
}

__attribute__((unused)) static NSUInteger IFIconListLastIconIndex(SBIconListView *listView) {
    NSArray *icons = [listView icons];
    SBIcon *lastIcon = nil;

    for (SBIcon *icon in [icons reverseObjectEnumerator]) {
        if ([icon respondsToSelector:@selector(isPlaceholder)] && ![icon isPlaceholder]) {
            lastIcon = icon;
            break;
        } else if ([icon respondsToSelector:@selector(isNullIcon)] && ![icon isNullIcon]) {
            lastIcon = icon;
            break;
        } else if ([icon respondsToSelector:@selector(isDestinationHole)] && ![icon isDestinationHole]) {
            lastIcon = icon;
            break;
        }
    }

    SBIconListModel *model = [listView model];
    return [model indexForIcon:lastIcon];
}

__attribute__((unused)) static UIInterfaceOrientation IFIconListOrientation(SBIconListView *listView) {
    UIInterfaceOrientation orientation = MSHookIvar<UIInterfaceOrientation>(listView, "_orientation");
    return orientation;
}

__attribute__((unused)) static CGSize IFIconDefaultSize() {
    CGSize size = [NSClassFromString(@"SBIconView") defaultIconSize];
    return size;
}

__attribute__((unused)) static SBRootFolder *IFRootFolderSharedInstance() {
    SBIconController *iconController = IFIconControllerSharedInstance();
    SBRootFolder *rootFolder = MSHookIvar<SBRootFolder *>(iconController, "_rootFolder");
    return rootFolder;
}

__attribute__((unused)) static SBIconListView *IFIconListContainingIcon(SBIcon *icon) {
    SBIconController *iconController = IFIconControllerSharedInstance();
    SBRootFolder *rootFolder = IFRootFolderSharedInstance();

    SBIconListModel *listModel = [rootFolder listContainingIcon:icon];

    if ([listModel isKindOfClass:NSClassFromString(@"SBDockIconListModel")]) {
        if ([iconController respondsToSelector:@selector(dockListView)]) {
            return [iconController dockListView];
        } else {
            return [iconController dock];
        }
    } else {
        NSUInteger index = [rootFolder indexOfList:listModel];
        return [iconController rootIconListAtIndex:index];
    }
}

/* }}} */

/* List Management {{{ */

static NSMutableArray *IFListsListViews = nil;
static NSMutableArray *IFListsScrollViews = nil;

__attribute__((constructor)) static void IFListsInitialize() {
    // Non-retaining mutable arrays, since we don't want to own these objects.
    CFArrayCallBacks callbacks = { 0, NULL, NULL, CFCopyDescription, CFEqual };
    IFListsListViews = (NSMutableArray *) CFArrayCreateMutable(NULL, 0, &callbacks);
    IFListsScrollViews = (NSMutableArray *) CFArrayCreateMutable(NULL, 0, &callbacks);
}

__attribute__((unused)) static void IFListsIterateViews(void (^block)(SBIconListView *, UIScrollView *)) {
    for (NSUInteger i = 0; i < IFMinimum([IFListsListViews count], [IFListsScrollViews count]); i++) {
        block([IFListsListViews objectAtIndex:i], [IFListsScrollViews objectAtIndex:i]);
    }
}

__attribute__((unused)) static SBIconListView *IFListsListViewForScrollView(UIScrollView *scrollView) {
    NSInteger index = [IFListsScrollViews indexOfObject:scrollView];

    if (index == NSNotFound) {
        return nil;
    }

    return [IFListsListViews objectAtIndex:index];
}

__attribute__((unused)) static UIScrollView *IFListsScrollViewForListView(SBIconListView *listView) {
    NSInteger index = [IFListsListViews indexOfObject:listView];

    if (index == NSNotFound) {
        return nil;
    }

    return [IFListsScrollViews objectAtIndex:index];
}

__attribute__((unused)) static void IFListsRegister(SBIconListView *listView, UIScrollView *scrollView) {
    [IFListsListViews addObject:listView];
    [IFListsScrollViews addObject:scrollView];
}

__attribute__((unused)) static void IFListsUnregister(SBIconListView *listView, UIScrollView *scrollView) {
    [IFListsListViews removeObject:listView];
    [IFListsScrollViews removeObject:scrollView];
}

/* }}} */

/* Preferences {{{ */

typedef enum {
    kIFScrollbarStyleBlack,
    kIFScrollbarStyleWhite,
    kIFScrollbarStyleNone
} IFScrollbarStyle;

typedef enum {
    kIFScrollBounceEnabled,
    kIFScrollBounceExtra,
    kIFScrollBounceDisabled
} IFScrollBounce;

#ifndef IFPreferencesPagingEnabled
    #define IFPreferencesPagingEnabled @"PagingEnabled", NO
#endif

#ifndef IFPreferencesScrollEnabled
    #define IFPreferencesScrollEnabled @"ScrollEnabled", YES
#endif

#ifndef IFPreferencesScrollBounce
    #define IFPreferencesScrollBounce @"ScrollBounce", kIFScrollBounceEnabled
#endif

#ifndef IFPreferencesScrollbarStyle
    #define IFPreferencesScrollbarStyle @"ScrollbarStyle", kIFScrollbarStyleBlack
#endif

#ifndef IFPreferencesClipsToBounds
    #define IFPreferencesClipsToBounds @"ClipsToBounds", YES
#endif

static void IFPreferencesApplyToList(SBIconListView *listView) {
    UIScrollView *scrollView = IFListsScrollViewForListView(listView);

    BOOL scroll = IFPreferencesBoolForKey(IFPreferencesScrollEnabled);
    IFScrollBounce bounce = (IFScrollBounce) IFPreferencesIntForKey(IFPreferencesScrollBounce);
    IFScrollbarStyle bar = (IFScrollbarStyle) IFPreferencesIntForKey(IFPreferencesScrollbarStyle);
    BOOL page = IFPreferencesBoolForKey(IFPreferencesPagingEnabled);
    BOOL clips = IFPreferencesBoolForKey(IFPreferencesClipsToBounds);

    [scrollView setShowsVerticalScrollIndicator:YES];
    [scrollView setShowsHorizontalScrollIndicator:YES];
    if (bar == kIFScrollbarStyleBlack) {
        [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleDefault];
    } else if (bar == kIFScrollbarStyleWhite) {
        [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleWhite];
    } else if (bar == kIFScrollbarStyleNone) {
        [scrollView setShowsVerticalScrollIndicator:NO];
        [scrollView setShowsHorizontalScrollIndicator:NO];
    }

    [scrollView setAlwaysBounceVertical:IFConfigurationExpandVertically && (bounce == kIFScrollBounceEnabled)];
    [scrollView setAlwaysBounceHorizontal:IFConfigurationExpandHorizontally && (bounce == kIFScrollBounceEnabled)];
    [scrollView setBounces:(bounce != kIFScrollBounceDisabled)];

    [scrollView setScrollEnabled:scroll];
    [scrollView setPagingEnabled:page];
    [scrollView setClipsToBounds:clips];
    [listView setClipsToBounds:clips];

    if (bounce == kIFScrollBounceExtra) {
        NSUInteger idx = 0;
        NSUInteger max = 0;

        IFFlag(IFFlagDefaultDimensions) {
            idx = IFIconListLastIconIndex(listView);
            max = [listView iconRowsForCurrentOrientation] * [listView iconColumnsForCurrentOrientation];
        }

        [scrollView setAlwaysBounceVertical:IFConfigurationExpandVertically && (idx > max)];
        [scrollView setAlwaysBounceHorizontal:IFConfigurationExpandHorizontally && (idx > max)];
    }
}

static void IFPreferencesApply() {
    IFListsIterateViews(^(SBIconListView *listView, UIScrollView *scrollView) {
        IFPreferencesApplyToList(listView);
    });
}

/* }}} */

/* List Sizing {{{ */

typedef struct {
    NSUInteger rows;
    NSUInteger columns;
} IFIconListDimensions;

static IFIconListDimensions IFIconListDimensionsZero = { 0, 0 };

/* Defaults {{{ */

static IFIconListDimensions _IFSizingDefaultDimensionsForOrientation(UIInterfaceOrientation orientation) {
    IFIconListDimensions dimensions = IFIconListDimensionsZero;

    IFFlag(IFFlagDefaultDimensions) {
        dimensions.rows = [IFConfigurationListClassObject iconRowsForInterfaceOrientation:orientation];
        dimensions.columns = [IFConfigurationListClassObject iconColumnsForInterfaceOrientation:orientation];
    }

    return dimensions;
}

static IFIconListDimensions _IFSizingDefaultDimensions(SBIconListView *listView) {
    return _IFSizingDefaultDimensionsForOrientation(IFIconListOrientation(listView));
}

static CGSize _IFSizingDefaultPadding(SBIconListView *listView) {
    CGSize padding = CGSizeZero;

    IFFlag(IFFlagDefaultDimensions) {
        padding.width = [listView horizontalIconPadding];
        padding.height = [listView verticalIconPadding];
    }

    return padding;
}

static UIEdgeInsets _IFSizingDefaultInsets(SBIconListView *listView) {
    UIEdgeInsets insets = UIEdgeInsetsZero;

    IFFlag(IFFlagDefaultDimensions) {
        insets.top = [listView topIconInset];
        insets.bottom = [listView bottomIconInset];
        insets.left = [listView sideIconInset];
        insets.right = [listView sideIconInset];
    }

    return insets;
}

/* }}} */

/* Dimensions {{{ */

static IFIconListDimensions IFSizingMaximumDimensionsForOrientation(UIInterfaceOrientation orientation) {
    IFIconListDimensions dimensions = _IFSizingDefaultDimensionsForOrientation(orientation);

    if (IFConfigurationExpandVertically) {
        dimensions.rows = IFConfigurationExpandedDimension;
    }

    if (IFConfigurationExpandHorizontally) {
        dimensions.columns = IFConfigurationExpandedDimension;
    }

    return dimensions;
}

static IFIconListDimensions IFSizingContentDimensions(SBIconListView *listView) {
    IFIconListDimensions dimensions = IFIconListDimensionsZero;
    UIInterfaceOrientation orientation = IFIconListOrientation(listView);

    if ([[listView icons] count] > 0) {
        NSUInteger idx = IFIconListLastIconIndex(listView);

        if (IFConfigurationExpandWhenEditing && [IFIconControllerSharedInstance() isEditing]) {
            // Add room to drop the icon into.
            idx += 1;
        }

        IFIconListDimensions maximumDimensions = IFSizingMaximumDimensionsForOrientation(orientation);
        dimensions.columns = (idx % maximumDimensions.columns);
        dimensions.rows = (idx / maximumDimensions.columns);

        // Convert from index to sizing information.
        dimensions.rows += 1;
        dimensions.columns += 1;

        if (!IFConfigurationDynamicColumns) {
            // If we have more than one row, we necessarily have the
            // maximum number of columns at some point above the bottom.
            dimensions.columns = maximumDimensions.columns;
        }
    } else {
        dimensions = _IFSizingDefaultDimensionsForOrientation(orientation);
    }

    IFIconListDimensions defaultDimensions = _IFSizingDefaultDimensions(listView);

    if (IFConfigurationFullPages || IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
        // This is ugly, but we need to round up here.
        dimensions.rows = ceilf((float) dimensions.rows / (float) defaultDimensions.rows) * defaultDimensions.rows;
        dimensions.columns = ceilf((float) dimensions.columns / (float) defaultDimensions.columns) * defaultDimensions.columns;
    }

    // Make sure we have at least the default number of icons.
    dimensions.rows = (dimensions.rows > defaultDimensions.rows) ? dimensions.rows : defaultDimensions.rows;
    dimensions.columns = (dimensions.columns > defaultDimensions.columns) ? dimensions.columns : defaultDimensions.columns;

    return dimensions;
}

/* }}} */

/* Information {{{ */

// Prevent conflicts between multiple users of Infinilist.
#define IFIconListSizingInformation IFMacroConcat(IFIconListSizingInformation, IFConfigurationTweakIdentifier)

@interface IFIconListSizingInformation : NSObject {
    IFIconListDimensions defaultDimensions;
    CGSize defaultPadding;
    UIEdgeInsets defaultInsets;
    IFIconListDimensions contentDimensions;
}

@property (nonatomic, assign) IFIconListDimensions defaultDimensions;
@property (nonatomic, assign) CGSize defaultPadding;
@property (nonatomic, assign) UIEdgeInsets defaultInsets;
@property (nonatomic, assign) IFIconListDimensions contentDimensions;

@end

@implementation IFIconListSizingInformation

@synthesize defaultDimensions;
@synthesize defaultPadding;
@synthesize defaultInsets;
@synthesize contentDimensions;

- (NSString *)description {
    return [NSString stringWithFormat:@"<IFIconListSizingInformation:%p defaultDimensions = {%ld, %ld} defaultPadding = %@ defaultInsets = %@ contentDimensions = {%ld, %ld}>", self, (unsigned long)defaultDimensions.rows, (unsigned long)defaultDimensions.columns, NSStringFromCGSize(defaultPadding), NSStringFromUIEdgeInsets(defaultInsets), (unsigned long)contentDimensions.rows, (unsigned long)contentDimensions.columns];
}

@end

static NSMutableDictionary *IFIconListSizingStore = nil;

__attribute__((constructor)) static void IFIconListSizingInitialize() {
    IFIconListSizingStore = [[NSMutableDictionary alloc] init];
}

static IFIconListSizingInformation *IFIconListSizingInformationForIconList(SBIconListView *listView) {
    IFIconListSizingInformation *information = [IFIconListSizingStore objectForKey:[NSValue valueWithNonretainedObject:listView]];
    return information;
}

static IFIconListDimensions IFSizingDefaultDimensionsForIconList(SBIconListView *listView) {
    return [IFIconListSizingInformationForIconList(listView) defaultDimensions];
}

static void IFIconListSizingSetInformationForIconList(IFIconListSizingInformation *information, SBIconListView *listView) {
    [IFIconListSizingStore setObject:information forKey:[NSValue valueWithNonretainedObject:listView]];
}

static void IFIconListSizingRemoveInformationForIconList(SBIconListView *listView) {
    [IFIconListSizingStore removeObjectForKey:[NSValue valueWithNonretainedObject:listView]];
}

static IFIconListSizingInformation *IFIconListSizingComputeInformationForIconList(SBIconListView *listView) {
    IFIconListSizingInformation *info = [[IFIconListSizingInformation alloc] init];
    [info setDefaultDimensions:_IFSizingDefaultDimensions(listView)];
    [info setDefaultPadding:_IFSizingDefaultPadding(listView)];
    [info setDefaultInsets:_IFSizingDefaultInsets(listView)];
    [info setContentDimensions:IFSizingContentDimensions(listView)];
    return [info autorelease];
}

/* }}} */

/* Content Size {{{ */

static CGSize IFIconListSizingEffectiveContentSize(SBIconListView *listView) {
    IFIconListSizingInformation *info = IFIconListSizingInformationForIconList(listView);

    IFIconListDimensions effectiveDimensions = [info contentDimensions];
    CGSize contentSize = CGSizeZero;

    if (IFConfigurationFullPages || IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
        IFIconListDimensions defaultDimensions = [info defaultDimensions];
        CGSize size = [listView frame].size;

        IFIconListDimensions result = IFIconListDimensionsZero;
        result.columns = (effectiveDimensions.columns / defaultDimensions.columns);
        result.rows = (effectiveDimensions.rows / defaultDimensions.rows);

        contentSize = CGSizeMake(size.width * result.columns, size.height * result.rows);
    } else {
        CGSize padding = [info defaultPadding];
        UIEdgeInsets insets = [info defaultInsets];
        CGSize iconSize = IFIconDefaultSize();

        contentSize.width = insets.left + effectiveDimensions.columns * (iconSize.width + padding.width) - padding.width + insets.right;
        contentSize.height = insets.top + effectiveDimensions.rows * (iconSize.height + padding.height) - padding.height + insets.bottom;
    }

    return contentSize;
}

static void IFIconListSizingUpdateContentSize(SBIconListView *listView, UIScrollView *scrollView) {
    CGPoint offset = [scrollView contentOffset];
    CGSize scrollSize = [scrollView bounds].size;
    CGSize oldSize = [scrollView contentSize];
    CGSize newSize = IFIconListSizingEffectiveContentSize(listView);


    if (IFConfigurationExpandHorizontally) {
        // Be sure not to have two-dimensional scrolling.
        if (newSize.height > scrollSize.height) {
            newSize.height = scrollSize.height;
        }

        // Make sure the content offset is never outside the scroll view.
        if (offset.x + scrollSize.width > newSize.width) {
            // But not if the scroll view is only a few columns.
            if (newSize.width >= scrollSize.width) {
                offset.x = newSize.width - scrollSize.width;
            }
        }
    } else if (IFConfigurationExpandVertically) {
        // Be sure not to have two-dimensional scrolling.
        if (newSize.width > scrollSize.width) {
            newSize.width = scrollSize.width;
        }

        // Make sure the content offset is never outside the scroll view.
        if (offset.y + scrollSize.height > newSize.height) {
            // But not if the scroll view is only a few rows.
            if (newSize.height >= scrollSize.height) {
                offset.y = newSize.height - scrollSize.height;
            }
        }
    }

    if (!CGSizeEqualToSize(oldSize, newSize)) {
        [UIView animateWithDuration:0.3f animations:^{
            [scrollView setContentSize:newSize];
            [scrollView setContentOffset:offset animated:NO];
        }];
    }
}

/* }}} */

static void IFIconListSizingUpdateIconList(SBIconListView *listView) {
    UIScrollView *scrollView = IFListsScrollViewForListView(listView);

    IFIconListSizingSetInformationForIconList(IFIconListSizingComputeInformationForIconList(listView), listView);
    IFIconListSizingUpdateContentSize(listView, scrollView);
}

/* }}} */

%group IFBasic

%hook IFConfigurationListClass

/* View Hierarchy {{{ */

static void IFIconListInitialize(SBIconListView *listView) {
    UIScrollView *scrollView = [[IFConfigurationScrollViewClass alloc] initWithFrame:[listView frame]];
    [scrollView setDelegate:(id<UIScrollViewDelegate>) listView];
    [scrollView setDelaysContentTouches:NO];

    IFListsRegister(listView, scrollView);
    [listView addSubview:scrollView];

    IFIconListSizingUpdateIconList(listView);
    IFPreferencesApplyToList(listView);
}

- (id)initWithFrame:(CGRect)frame {
    if ((self = %orig)) {
        // Avoid hooking a sub-initializer when we hook the base initializer, but otherwise do hook it.
        if (IFIconListIsValid(self) && ![self respondsToSelector:@selector(initWithFrame:viewMap:)]) {
            IFIconListInitialize(self);
        }
    }

    return self;
}

- (id)initWithFrame:(CGRect)frame viewMap:(id)viewMap {
    if ((self = %orig)) {
        if (IFIconListIsValid(self)) {
            IFIconListInitialize(self);
        }
    }

    return self;
}

- (void)dealloc {
    if (IFIconListIsValid(self)) {
        UIScrollView *scrollView = IFListsScrollViewForListView(self);

        IFListsUnregister(self, scrollView);
        IFIconListSizingRemoveInformationForIconList(self);

        [scrollView release];
    }

    %orig;
}

- (void)setFrame:(CGRect)frame {
    if (IFIconListIsValid(self)) {
        UIScrollView *scrollView = IFListsScrollViewForListView(self);

        NSUInteger pagex = 0;
        NSUInteger pagey = 0;

        if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
            CGPoint offset = [scrollView contentOffset];
            CGRect bounds = [self bounds];

            pagex = (offset.x / bounds.size.width);
            pagey = (offset.y / bounds.size.height);
        }

        %orig;

        [scrollView setFrame:[self bounds]];
        IFIconListSizingUpdateIconList(self);

        [self layoutIconsNow];

        if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
            CGPoint offset = [scrollView contentOffset];
            CGRect bounds = [self bounds];

            offset.x = (pagex * bounds.size.width);
            offset.y = (pagey * bounds.size.height);
            [scrollView setContentOffset:offset animated:NO];
        }
    } else {
        %orig;
    }
}

- (void)addSubview:(UIView *)view {
    if (IFIconListIsValid(self)) {
        UIScrollView *scrollView = IFListsScrollViewForListView(self);

        if (view == scrollView) {
            %orig;
        } else {
            [scrollView addSubview:view];

            IFIconListSizingUpdateIconList(self);
        }
    } else {
        %orig;
    }
}

- (void)setOrientation:(UIInterfaceOrientation)orientation {
    %orig;

    if (IFIconListIsValid(self)) {
        IFIconListSizingUpdateIconList(self);
    }
}

- (void)cleanupAfterRotation {
    %orig;

    if (IFIconListIsValid(self)) {
        [self layoutIconsNow];
    }
}

/* }}} */

/* Icon Layout {{{ */
/* Dimensions {{{ */

+ (NSUInteger)maxIcons {
    if (self == IFConfigurationListClassObject) {
        if (IFFlagDefaultDimensions) {
            return %orig;
        } else {
            return IFConfigurationExpandedDimension * IFConfigurationExpandedDimension;
        }
    } else {
        return %orig;
    }
}

+ (NSUInteger)maxVisibleIconRowsInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if (self == IFConfigurationListClassObject) {
        NSUInteger rows = 0;

        IFFlag(IFFlagDefaultDimensions) {
            rows = %orig;
        }

        return rows;
    } else {
        return %orig;
    }
}

+ (NSUInteger)iconRowsForInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if (self == IFConfigurationListClassObject) {
        if (IFFlagDefaultDimensions) {
            return %orig;
        } else {
            IFIconListDimensions dimensions = IFSizingMaximumDimensionsForOrientation(interfaceOrientation);
            return dimensions.rows;
        }
    } else {
        return %orig;
    }
}

+ (NSUInteger)iconColumnsForInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if (self == IFConfigurationListClassObject) {
        if (IFFlagDefaultDimensions) {
            return %orig;
        } else {
            IFIconListDimensions dimensions = IFSizingMaximumDimensionsForOrientation(interfaceOrientation);
            return dimensions.columns;
        }
    } else {
        return %orig;
    }
}

- (NSUInteger)iconRowsForCurrentOrientation {
    if (IFIconListIsValid(self)) {
        if (IFFlagExpandedFrame) {
            IFIconListDimensions dimensions = [IFIconListSizingInformationForIconList(self) contentDimensions];
            return dimensions.rows;
        } else {
            return %orig;
        }
    } else {
        return %orig;
    }
}

- (NSUInteger)iconColumnsForCurrentOrientation {
    if (IFIconListIsValid(self)) {
        if (IFFlagExpandedFrame) {
            IFIconListDimensions dimensions = [IFIconListSizingInformationForIconList(self) contentDimensions];
            return dimensions.columns;
        } else {
            return %orig;
        }
    } else {
        return %orig;
    }
}

- (CGRect)bounds {
    if (IFIconListIsValid(self)) {
        // This check breaks icon positions on iOS 7.0+, but is needed on iOS 5.x and 6.x.
        if (kCFCoreFoundationVersionNumber < 800.0 && IFFlagExpandedFrame) {
            CGRect bounds = %orig;
            bounds.size = IFIconListSizingEffectiveContentSize(self);
            return bounds;
        } else {
            return %orig;
        }
    } else {
        return %orig;
    }
}

/* }}} */

/* Positioning {{{ */

static CGPoint IFIconListOriginForIconAtXY(SBIconListView *self, NSUInteger x, NSUInteger y, CGPoint (^orig)(NSUInteger, NSUInteger)) {
    CGPoint origin = CGPointZero;

    IFFlag(IFFlagExpandedFrame) {
        UIScrollView *scrollView = IFListsScrollViewForListView(self);

        if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
            IFIconListDimensions dimensions = IFSizingDefaultDimensionsForIconList(self);

            NSUInteger px = (x / dimensions.columns), py = (y / dimensions.rows);
            NSUInteger ix = (x % dimensions.columns), iy = (y % dimensions.rows);

            origin = orig(ix, iy);

            CGSize size = [scrollView frame].size;
            origin.x += (size.width) * px;
            origin.y += (size.height) * py;
        } else {
            origin = orig(x, y);
        }
    }

    return origin;
}

- (CGPoint)originForIconAtCoordinate:(SBIconCoordinate)coordinate {
    if (IFIconListIsValid(self)) {
        return IFIconListOriginForIconAtXY(self, coordinate.col - 1, coordinate.row - 1, ^(NSUInteger x, NSUInteger y) {
            SBIconCoordinate innerCoordinate = { .row = y + 1, .col = x + 1 };
            return %orig(innerCoordinate);
        });
    } else {
        return %orig;
    }
}

- (CGPoint)originForIconAtX:(NSUInteger)x Y:(NSUInteger)y {
    if (IFIconListIsValid(self)) {
        return IFIconListOriginForIconAtXY(self, x, y, ^(NSUInteger x, NSUInteger y) {
            return %orig(x, y);
        });
    } else {
        return %orig;
    }
}

- (NSUInteger)rowAtPoint:(CGPoint)point {
    if (IFIconListIsValid(self)) {
        NSUInteger row = 0;

        IFFlag(IFFlagExpandedFrame) {
            UIScrollView *scrollView = IFListsScrollViewForListView(self);
            CGPoint offset = [scrollView contentOffset];
            CGSize size = [scrollView frame].size;

            if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
                row = %orig;

                NSUInteger page = (offset.y / size.height);
                IFIconListDimensions dimensions = IFSizingDefaultDimensionsForIconList(self);
                row += page * dimensions.rows;
            } else {
                point.x += offset.x;
                point.y += offset.y;

                row = %orig;
            }
        }

        return row;
    } else {
        return %orig;
    }
}

- (NSUInteger)columnAtPoint:(CGPoint)point {
    if (IFIconListIsValid(self)) {
        NSUInteger column = 0;

        IFFlag(IFFlagExpandedFrame) {
            UIScrollView *scrollView = IFListsScrollViewForListView(self);
            CGPoint offset = [scrollView contentOffset];
            CGSize size = [scrollView frame].size;

            if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
                column = %orig;

                NSUInteger page = (offset.x / size.width);
                IFIconListDimensions dimensions = IFSizingDefaultDimensionsForIconList(self);
                column += page * dimensions.columns;
            } else {
                point.x += offset.x;
                point.y += offset.y;

                column = %orig;
            }
        }

        return column;
    } else {
        return %orig;
    }
}

/* }}} */
/* }}} */

%end

/* Fixes {{{ */

%hook UIScrollView

// FIXME: this is an ugly hack
static id grabbedIcon = nil;
- (void)setContentOffset:(CGPoint)offset {
    if (grabbedIcon != nil && [IFListsScrollViews containsObject:self]) {
        // Prevent weird auto-scrolling behavior while dragging icons.
        return;
    } else {
        %orig;
    }
}

%end

%hook SBIconController

- (CGRect)_contentViewRelativeFrameForIcon:(SBIcon *)icon {
    SBIconListView *listView = IFIconListContainingIcon(icon);
    UIScrollView *scrollView = IFListsScrollViewForListView(listView);

    CGRect ret = %orig;

    // The list could, in theory, be in another list that
    // we don't care about. If it is, we won't have a scroll
    // view for it, and can safely ignore moving the icon.
    if (scrollView != nil) {
        ret.origin.x -= [scrollView contentOffset].x;
        ret.origin.y -= [scrollView contentOffset].y;
    }

    return ret;
}

- (void)moveIconFromWindow:(SBIcon *)icon toIconList:(SBIconListView *)listView {
    %orig;

    if (IFIconListIsValid(listView)) {
        UIScrollView *scrollView = IFListsScrollViewForListView(listView);
        SBIconView *iconView = IFIconViewForIcon(icon);

        CGRect frame = [iconView frame];
        frame.origin.x += [scrollView contentOffset].x;
        frame.origin.y += [scrollView contentOffset].y;
        [iconView setFrame:frame];
    }
}

- (void)_dropIconIntoOpenFolder:(SBIcon *)icon withInsertionPath:(NSIndexPath *)path {
    %orig;

    SBFolderIconListView *listView = [self currentFolderIconList];

    if (IFIconListIsValid(listView)) {
        UIScrollView *scrollView = IFListsScrollViewForListView(listView);
        SBIconView *iconView = IFIconViewForIcon(icon);

        CGRect frame = [iconView frame];
        frame.origin.x -= [scrollView contentOffset].x;
        frame.origin.y -= [scrollView contentOffset].y;
        [iconView setFrame:frame];
    }
}

- (void)setGrabbedIcon:(id)icon {
    IFListsIterateViews(^(SBIconListView *listView, UIScrollView *scrollView) {
        [scrollView setScrollEnabled:(icon == nil)];
    });

    %orig;

    if (icon != nil) {
        grabbedIcon = icon;
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            grabbedIcon = nil;
        });
    }
}

- (void)setIsEditing:(BOOL)editing {
    %orig;

    dispatch_async(dispatch_get_main_queue(), ^{
        IFListsIterateViews(^(SBIconListView *listView, UIScrollView *scrollView) {
            IFIconListSizingUpdateIconList(listView);
        });
    });
}

%end

/* }}} */

%end

