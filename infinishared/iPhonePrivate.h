
#import <UIKit/UIKit.h>

typedef struct SBIconCoordinate {
	NSInteger row;
	NSInteger col;
} SBIconCoordinate;

@interface ISIconSupport : NSObject
+ (id)sharedInstance;
- (void)addExtension:(NSString *)name;
@end

@interface SBIcon : NSObject
- (BOOL)isPlaceholder; // iOS 6.0+
- (BOOL)isDestinationHole; // iOS 5.x
- (BOOL)isNullIcon; // iOS 5.x
@end

@class SBFolder;
@interface SBFolderIcon : SBIcon
- (SBFolder *)folder;

+ (NSUInteger)_maxIconsInGridImage; // iOS 6.0+
- (NSUInteger)_maxIcons; // iOS 5.x

- (NSUInteger)_numberOfExcessIcons; // iOS 5.x
- (NSUInteger)_gridColumns; // iOS 5.x
- (id)_miniIconGridWithSkipping:(BOOL)skipping; // iOS 5.x
@end

@interface SBIconListModel : NSObject
- (NSUInteger)indexForIcon:(SBIcon *)icon;
@end

@interface SBFolder : NSObject
- (SBFolderIcon *)icon;
- (NSArray *)allIcons;
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
- (NSUInteger)iconsInRowForSpacingCalculation;

- (NSArray *)icons;
- (SBIconListModel *)model;

- (NSUInteger)rowAtPoint:(CGPoint)point;
- (NSUInteger)columnAtPoint:(CGPoint)point;
- (NSUInteger)rowForIcon:(SBIcon *)icon;
- (SBIcon *)iconAtPoint:(CGPoint)point index:(NSInteger *)index;

- (CGPoint)originForIconAtCoordinate:(SBIconCoordinate)coordinate; // iOS 7.0+
- (CGPoint)originForIconAtX:(NSUInteger)x Y:(NSUInteger)y; // iOS 5.x and 6.x
- (CGPoint)originForIconAtIndex:(NSUInteger)index;

- (CGSize)defaultIconSize; // iOS 5.0+
- (CGFloat)topIconInset;
- (CGFloat)bottomIconInset;
- (CGFloat)sideIconInset;
- (CGFloat)verticalIconPadding;
- (CGFloat)horizontalIconPadding;

- (void)updateEditingStateAnimated:(BOOL)animated; // iOS 7.0+

- (void)setOrientation:(UIInterfaceOrientation)orientation;
- (void)cleanupAfterRotation;

- (void)layoutIconsNow;
@end

@interface SBRootIconListView : SBIconListView
@end

@interface SBFolderIconListView : SBIconListView
@end

@interface SBDockIconListView : SBIconListView
- (NSArray *)visibleIcons;
- (NSUInteger)visibleIconsInDock; // iOS 5.x and 6.x

- (void)_updateForOrientation:(UIInterfaceOrientation)orientation duration:(NSTimeInterval)interval;
@end

@interface SBIconController : NSObject
+ (SBIconController *)sharedInstance;

- (UIInterfaceOrientation)orientation;

- (BOOL)isEditing;
- (void)setIsEditing:(BOOL)isEditing;

- (SBIconModel *)model; // iOS 6.0+

- (SBDockIconListView *)dockListView; // iOS 7.0+
- (SBDockIconListView *)dock; // iOS 5.x and 6.x

- (SBIconListView *)currentRootIconList;
- (SBIconListView *)rootIconListAtIndex:(NSInteger)index;
- (SBFolderIconListView *)currentFolderIconList;

- (CGRect)_contentViewRelativeFrameForIcon:(SBIcon *)icon;

- (SBIconView *)grabbedIcon;
- (void)setGrabbedIcon:(SBIconView *)grabbedIcon;
- (void)_dropIconIntoOpenFolder:(SBIcon *)icon withInsertionPath:(NSIndexPath *)path; // iOS 6.0+
- (void)moveIconFromWindow:(SBIcon *)icon toIconList:(SBIconListView *)listView;

- (SBFolder *)openFolder;

- (void)openFolder:(SBFolder *)folder animated:(BOOL)animated; // iOS 7.0+
- (void)setOpenFolder:(SBFolder *)folder; // iOS 5.x and 6.x

- (void)_animateFolder:(SBFolder *)folder open:(BOOL)open animated:(BOOL)animated; // iOS 7.0+
- (void)_slideFolderOpen:(BOOL)open animated:(BOOL)animated; // iOS 5.x and 6.x

- (void)_folderDidFinishOpenClose:(BOOL)_folder animated:(BOOL)animated; // iOS 7.0+
- (void)_openCloseFolderAnimationEnded:(id)ended finished:(id)finished context:(void *)context; // iOS 5.x and 6.x

- (NSUInteger)_folderRowsForFolder:(SBFolder *)folder inOrientation:(UIInterfaceOrientation)orientation;
@end

@interface SBIconZoomAnimator : NSObject // iOS 7.0+

- (void)prepare; // iOS 7.0+

@end

typedef NSUInteger SBNotchInfoDirection;
typedef struct {
    SBNotchInfoDirection direction;
    CGRect rect;
} SBNotchInfo;

