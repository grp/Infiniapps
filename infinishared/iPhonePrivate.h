
#import <UIKit/UIKit.h>

@interface SBIcon : NSObject
@end

@interface SBFolderIcon : SBIcon
@end

@interface SBIconListModel : NSObject
- (NSUInteger)indexForIcon:(SBIcon *)icon;
@end

@interface SBFolder : NSObject
- (SBFolderIcon *)icon;
- (NSIndexPath *)indexPathForIcon:(SBIcon *)icon;
- (SBIconListModel *)listContainingIcon:(SBIcon *)icon;
- (NSUInteger)indexOfList:(SBIconListModel *)list;
@end

@interface SBRootFolder : SBFolder
@end

@interface SBIconModel : NSObject
+ (id)sharedInstance; // iOS 5.x
- (void)relayout; // iOS 5.x
- (void)layout; // iOS 6.0+
@end

@interface SBIconView : UIView
+ (CGSize)defaultIconSize; // iOS 5.0+
@end

@interface SBIconViewMap : NSObject
+ (SBIconViewMap *)homescreenMap;
- (SBIconView *)iconViewForIcon:(SBIcon *)icon;
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
- (SBIcon *)iconAtPoint:(CGPoint)point index:(NSInteger *)index;
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

@interface SBIconController : NSObject
+ (SBIconController *)sharedInstance;

- (UIInterfaceOrientation)orientation;

- (BOOL)isEditing;
- (void)setIsEditing:(BOOL)isEditing;

- (SBIconModel *)model; // iOS 6.0+
- (SBIconListView *)rootIconListAtIndex:(NSInteger)index;

- (SBFolder *)openFolder;

- (SBIconView *)grabbedIcon;
- (void)setGrabbedIcon:(SBIconView *)grabbedIcon;
@end

