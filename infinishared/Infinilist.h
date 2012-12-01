
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

#ifndef IFConfigurationListClass
    #define IFConfigurationListClass SBIconListView
#endif

#define IFConfigurationListClassObject NSClassFromString(@IFMacroQuote(IFConfigurationListClass))

#ifndef IFConfigurationExpandWhenEditing
    #define IFConfigurationExpandWhenEditing YES
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

/* }}} */

/* Flags {{{ */

// Custom control structure for managing flags safely.
// Usage: IFFlag(IFFlagNamedThis) { /* code with flag enabled */ }
// Do not return out of this structure, or the flag is stuck.
#define IFFlag(flag) \
    if (1) { \
        flag += 1; \
        goto body; \
    } else \
        while (1) \
            if (1) { \
                flag -= 1; \
                break; \
            } else \
                body:

static NSUInteger IFFlagExpandedFrame = 0;
static NSUInteger IFFlagDefaultDimensions = 0;

/* }}} */

/* Conveniences {{{ */

__attribute__((unused)) static NSUInteger IFMinimum(NSUInteger x, NSUInteger y) {
    return (x < y ? x : y);
}

__attribute__((unused)) static NSUInteger IFMaximum(NSUInteger x, NSUInteger y) {
    return (x > y ? x : y);
}

__attribute__((unused)) static SBIconView *IFIconViewForIcon(SBIcon *icon) {
    Class map = NSClassFromString(@"SBIconViewMap");
    return [[map homescreenMap] iconViewForIcon:icon];
}

__attribute__((unused)) static SBIconController *IFIconControllerSharedInstance() {
    return (SBIconController *) [NSClassFromString(@"SBIconController") sharedInstance];
}

__attribute__((unused)) static BOOL IFIconListIsValid(SBIconListView *listView) {
    return [listView isMemberOfClass:IFConfigurationListClassObject];
}

__attribute__((unused)) static NSUInteger IFIconListLastIconIndex(SBIconListView *listView) {
    SBIcon *icon = [[listView icons] lastObject];
    SBIconListModel *model = [listView model];
    return [model indexForIcon:icon];
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
    NSInteger index = [rootFolder indexOfList:listModel];

    SBIconListView *listView = [iconController rootIconListAtIndex:index];
    return listView;
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
    if (bar == kIFScrollbarStyleBlack)
        [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleDefault];
    else if (bar == kIFScrollbarStyleWhite)
        [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleWhite];
    else if (bar == kIFScrollbarStyleNone)
        [scrollView setShowsVerticalScrollIndicator:NO];

    [scrollView setScrollEnabled:scroll];
    [scrollView setAlwaysBounceVertical:IFConfigurationExpandVertically && (bounce == kIFScrollBounceEnabled)];
    [scrollView setAlwaysBounceHorizontal:IFConfigurationExpandHorizontally && (bounce == kIFScrollBounceEnabled)];
    [scrollView setBounces:bounce != kIFScrollBounceDisabled];
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

        [scrollView setAlwaysBounceVertical:(idx > max)];
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
        dimensions.rows = NSIntegerMax;
    }

    if (IFConfigurationExpandHorizontally) {
        dimensions.columns = NSIntegerMax;
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

    if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
        IFIconListDimensions defaultDimensions = _IFSizingDefaultDimensions(listView);

        // This is ugly, but we need to round up here.
        dimensions.rows = ceilf((float) dimensions.rows / (float) defaultDimensions.rows) * defaultDimensions.rows;
        dimensions.columns = ceilf((float) dimensions.columns / (float) defaultDimensions.columns) * defaultDimensions.columns;
    }

    return dimensions;
}

/* }}} */

/* Information {{{ */

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

    if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
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
    CGSize oldSize = [scrollView contentSize];
    CGSize newSize = IFIconListSizingEffectiveContentSize(listView);

    // Make sure the content offset is never below the bottom of the scroll view.
    if (offset.y + [scrollView bounds].size.height > newSize.height) {
        // But not if the scroll view is only a few rows.
        if (newSize.height >= [scrollView bounds].size.height) {
            offset.y = newSize.height - [scrollView bounds].size.height;
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

- (id)initWithFrame:(CGRect)frame {
    if ((self = %orig)) {
        if (IFIconListIsValid(self)) {
            UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:frame];
            [scrollView setDelegate:(id<UIScrollViewDelegate>) self];
            [scrollView setDelaysContentTouches:NO];

            IFListsRegister(self, scrollView);
            [self addSubview:scrollView];

            IFIconListSizingUpdateIconList(self);
            IFPreferencesApplyToList(self);
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

        NSUInteger page = 0;

        if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
            CGPoint offset = [scrollView contentOffset];
            CGRect bounds = [self bounds];

            page = (offset.y / bounds.size.height);
        }

        %orig;

        [scrollView setFrame:[self bounds]];
        IFIconListSizingUpdateIconList(self);

        if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
            CGPoint offset = [scrollView contentOffset];
            CGRect bounds = [self bounds];

            offset.y = (page * bounds.size.height);
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
            return NSIntegerMax;
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
        if (IFFlagExpandedFrame) {
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

- (CGPoint)originForIconAtX:(NSUInteger)x Y:(NSUInteger)y {
    if (IFIconListIsValid(self)) {
        CGPoint origin = CGPointZero;

        IFFlag(IFFlagExpandedFrame) {
            UIScrollView *scrollView = IFListsScrollViewForListView(self);

            if (IFPreferencesBoolForKey(IFPreferencesPagingEnabled)) {
                IFIconListDimensions dimensions = IFSizingDefaultDimensionsForIconList(self);

                NSUInteger px = (x / dimensions.columns), py = (y / dimensions.rows);
                NSUInteger ix = (x % dimensions.columns), iy = (y % dimensions.rows);

                origin = %orig(ix, iy);

                CGSize size = [scrollView frame].size;
                origin.x += (size.width) * px;
                origin.y += (size.height) * py;
            } else {
                origin = %orig;
            }
        }

        return origin;
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

- (void)moveIconFromWindow:(SBIcon *)icon toIconList:(SBIconListView *)listView {
    %orig;

    if (IFIconListIsValid(listView)) {
        UIScrollView *scrollView = IFListsScrollViewForListView(listView);
        SBIconView *iconView = IFIconViewForIcon(icon);

        CGRect frame = [iconView frame];
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

