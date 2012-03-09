
/* Headers */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>

#import <dlfcn.h>

#import <objc/runtime.h>
#import <objc/message.h>

@interface SBIcon : NSObject { }
+ (CGSize)defaultIconSize;
@end

@interface SBDownloadingIcon : SBIcon { }
@end

@interface SBDestinationHole : SBIcon { }
@end

@interface SBApplicationIcon : SBIcon { }
@end

@interface SBIconView : UIView { }
+ (CGSize)defaultIconSize;
@end

@interface SBIconList : NSObject { }
- (CGPoint)originForIconAtX:(int)x Y:(int)y;
- (CGFloat)horizontalIconInset;
- (NSArray *)icons;
@end

@interface SBButtonBar : SBIconList { }
@end

/* Preferences */

#define idForKeyWithDefault(dict, key, default)	 ([(dict) objectForKey:(key)]?:(default))
#define floatForKeyWithDefault(dict, key, default)   ({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result floatValue]:(default); })
#define NSIntegerForKeyWithDefault(dict, key, default) (NSInteger)({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result integerValue]:(default); })
#define BOOLForKeyWithDefault(dict, key, default)    (BOOL)({ id _result = [(dict) objectForKey:(key)]; (_result)?[_result boolValue]:(default); })

#define IFPreferencesFilePath [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/com.chpwn.infinidock.plist"]
#define IFPreferencesChangedNotification "com.chpwn.infinidock.prefs"

#define GetPreference(name, type) type ## ForKeyWithDefault(prefsDict, @#name, (name))

#define IFMaxIcons 50
#define IFIconsPerPage 4
#define IFScrollEnabled YES
#define IFPagingEnabled NO
#define IFSnapEnabled NO
#define IFRestoreEnabled 1
#define IFMenuRestore NO

#define MAX_ICONS GetPreference(IFMaxIcons, NSInteger)
#define MAX_PERPAGE GetPreference(IFIconsPerPage, NSInteger)
#define SCROLL_ENABLED GetPreference(IFScrollEnabled, BOOL)
#define PAGING_ENABLED GetPreference(IFPagingEnabled, BOOL)
#define SNAP_ENABLED GetPreference(IFSnapEnabled, BOOL)
#define RESTORE_PAGE GetPreference(IFRestoreEnabled, NSInteger)
#define MENU_BUTTON_RESTORE GetPreference(IFMenuRestore, BOOL)


#define ICON_WIDTH ([objc_getClass("SBIconView") ?: objc_getClass("SBIcon") defaultIconSize].width)

NSString *IFFirmwareVersion();

/* Base Class */

typedef enum {
	kSpacingMethodPaged,
	kSpacingMethodDefault,
	kSpacingMethodEven
} IFSpacingMethod;

@interface IFBase : NSObject <UIScrollViewDelegate> {
	UIScrollView *scrollView;
	id dock;
	NSUInteger maxColumns;
	NSUInteger visibleCount;
	int disableDecelerateFlag;
}

@property (nonatomic, retain) UIScrollView *scrollView;
@property (nonatomic, retain) id dock;

+ (void)setClass:(Class)someclass;
+ (IFBase *)sharedInstance;

- (void)applyPreferences;
- (BOOL)isWildcat;

- (NSUInteger)currentIconCount;
- (NSUInteger)defaultIconCount;
- (NSUInteger)selectedIconCount;
- (NSUInteger)pageCount;
- (IFSpacingMethod)spacingMethod;
- (CGFloat)scrollWidth;
- (void)fixScrollWidth;
- (CGFloat)dockWidth;
- (CGFloat)leftInset;
- (void)restoreToPage;

- (NSUInteger)maxColumns;
- (NSUInteger)visibleCount;
- (int)columnAtPoint:(CGPoint)point;
- (CGPoint)originForIcon:(int)idx;

- (CGPoint)origOrigin:(int)idx;
- (NSInteger)origColumn:(CGPoint)point;
- (CGFloat)origInset;

@end


