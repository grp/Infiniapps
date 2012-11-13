
#import <UIKit/UIKit.h>

@interface SBIcon : NSObject // iOS 4.x: UIView
+ (CGSize)defaultIconSize; // iOS 4.x
@end

@interface SBIconView : UIView
+ (CGSize)defaultIconSize; // iOS 5.0+
@end

@interface SBIconController : NSObject
+ (SBIconController *)sharedInstance;

- (UIInterfaceOrientation)orientation;

- (BOOL)isEditing;
- (void)setIsEditing:(BOOL)isEditing;

- (SBIconView *)grabbedIcon;
- (void)setGrabbedIcon:(SBIconView *)grabbedIcon;
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

- (NSArray *)icons;
- (SBIconListModel *)model;

- (NSUInteger)rowAtPoint:(CGPoint)point;
- (NSUInteger)columnAtPoint:(CGPoint)point;
- (CGPoint)originForIconAtX:(NSUInteger)x Y:(NSUInteger)y;

- (CGFloat)topIconInset;
- (CGFloat)bottomIconInset;
- (CGFloat)sideIconInset;

- (CGFloat)verticalIconPadding;
- (CGFloat)horizontalIconPadding;

- (CGSize)defaultIconSize; // iOS 5.0+

- (void)layoutIconsNow;
@end

@interface SBFolderIconListView : SBIconListView
@end

