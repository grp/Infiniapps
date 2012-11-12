
#import <UIKit/UIKit.h>

@interface UIDevice (Private)
- (BOOL)isWildcat;
@end

@interface SBIcon : NSObject
@end

@interface SBIconView : UIView
+ (CGSize)defaultIconSize;
@end

@interface SBIconController : NSObject
+ (SBIconController *)sharedInstance;
- (SBIconView *)grabbedIcon;
- (UIInterfaceOrientation)orientation;
- (BOOL)isEditing;
@end

@interface SBIconViewMap : NSObject
+ (SBIconViewMap *)homescreenMap;
- (SBIconView *)iconViewForIcon:(SBIcon *)icon;
@end

@interface SBIconListModel : NSObject
- (NSUInteger)indexForIcon:(SBIcon *)icon;
@end

@interface SBIconListView : UIView
+ (NSUInteger)iconRowsForInterfaceOrientation:(UIInterfaceOrientation)orientation;
+ (NSUInteger)iconColumnsForInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (NSUInteger)iconRowsForCurrentOrientation;
- (NSUInteger)iconColumnsForCurrentOrientation;

- (SBIconListModel *)model;
- (NSArray *)icons;

- (NSUInteger)rowAtPoint:(CGPoint)point;
- (NSUInteger)columnAtPoint:(CGPoint)point;
- (CGPoint)originForIconAtX:(NSUInteger)x Y:(NSUInteger)y;

- (NSUInteger)firstFreeSlotIndex;
- (void)firstFreeSlotIndex:(NSUInteger *)index;
- (void)firstFreeSlotX:(NSUInteger *)x Y:(NSUInteger *)y;

- (void)getX:(NSUInteger *)x Y:(NSUInteger *)y forIndex:(NSUInteger)index forOrientation:(UIInterfaceOrientation)orientation;

- (CGFloat)topIconInset;
- (CGFloat)bottomIconInset;
- (CGFloat)sideIconInset;

- (CGFloat)verticalIconPadding;
- (CGFloat)horizontalIconPadding;

- (CGSize)defaultIconSize;

- (void)layoutIconsNow;
@end

