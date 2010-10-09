
#import "Infinidock.h"

static int disableVisibleFlag;
static int disableOriginFlag;
static int disableColumnFlag;
static int forceColumnFlag;
static int disableInsetFlag;
static int disablePointFlag;

@interface SBDockIconListView
- (float)sideIconInset;
- (float)_additionalSideInsetToCenterIcons;
- (float)horizontalIconPadding;
@end

@interface IFFour : IFBase { }
@end

@implementation IFFour
- (CGPoint)origOrigin:(NSInteger)idx {
	disableOriginFlag += 1;
	CGPoint ret = [[self dock] originForIconAtX:idx Y:0];
	disableOriginFlag -= 1;
	return ret;
}
- (NSInteger)origColumn:(CGPoint)point {
	disablePointFlag += 1;
	NSInteger ret = [[self dock] columnAtPoint:point];
	disablePointFlag -= 1;
	return ret;
}
- (CGFloat)origInset {
	disableInsetFlag += 1;
	CGFloat ret = [dock sideIconInset];
	disableInsetFlag -= 1;
	return ret;
}
- (IFSpacingMethod)spacingMethod {
	IFSpacingMethod spacing = [super spacingMethod];
	
	if (spacing == kSpacingMethodPaged) {
		return kSpacingMethodPaged;
	} else {
		if ((([self currentIconCount] < [self defaultIconCount]) && ([self currentIconCount] < [self selectedIconCount])) || ([self selectedIconCount] == [self defaultIconCount]) || (([self selectedIconCount] > [self defaultIconCount]) && ([self currentIconCount] == [self defaultIconCount]))) {
			return kSpacingMethodDefault;
		}
		
		return kSpacingMethodEven;
	}
}
@end

%group Four

%hook SBDockIconListModel
- (int)maxIcons {
	if (!disableColumnFlag)
		return [[IFFour sharedInstance] maxColumns];
	
	return MAX([[IFFour sharedInstance] selectedIconCount], [[IFFour sharedInstance] defaultIconCount]);
}
%end

%hook SBDockIconListView
- (id)initWithFrame:(CGRect)frame {
	self = %orig;
	[[IFFour sharedInstance] setDock:self];
	return self;
}
+ (int)iconColumnsForInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	if (forceColumnFlag)
		return [[IFBase sharedInstance] currentIconCount];
	
	if (!disableColumnFlag)
		return [[IFBase sharedInstance] maxColumns];
		
	return [[IFBase sharedInstance] defaultIconCount];
}
- (int)visibleIconsInDock {
	if (!disableVisibleFlag) {
		return [[IFBase sharedInstance] visibleCount];
	}
	
	return %orig;
}
- (int)iconsInRowForSpacingCalculation {
	if (!disableVisibleFlag) {
		return [[IFBase sharedInstance] visibleCount];
	}
	
	return %orig;
}
- (CGPoint)originForIconAtX:(int)x Y:(int)y {
	if (!disableOriginFlag) {
		CGPoint origin = %orig;
		origin.x = [[IFBase sharedInstance] originForIcon:x].x;
		return origin;
	}
	
	disableColumnFlag += 1;
	CGPoint ret = %orig;
	disableColumnFlag -= 1;
	
	return ret;
}
- (int)columnAtPoint:(CGPoint)point {
	if (!disablePointFlag) {
		int col = [[IFBase sharedInstance] columnAtPoint:point];
		return col;
	}
	
	int row = 0;

	SBIcon *icon;
	for (int i = 0; i < [[self icons] count]; i++) {
		icon = [[self icons] objectAtIndex:i];
		
		if (icon.center.x > point.x)
			break;
		
		row = i;
	}
	
	/*CGFloat left = [self sideIconInset] + [self _additionalSideInsetToCenterIcons];
	CGFloat padding = [self horizontalIconPadding];
	CGFloat icon = [objc_getClass("SBIcon") defaultIconSize].width;
	CGFloat cur = left + icon + padding;

	while (cur < point.x) {
		row += 1;
		cur += icon + padding;
	}*/
		
	// row = %orig;
	
	return row;
}
- (void)addSubview:(UIView *)subview {
	if (subview == [[IFBase sharedInstance] scrollView]) {
		%orig;
	} else {
		[[[IFBase sharedInstance] scrollView] addSubview:subview];
	}
}
- (void)removeAllIcons {
	%orig;
	[[[[IFBase sharedInstance] dock] icons] makeObjectsPerformSelector:@selector(removeFromSuperview)];
}
- (void)setFrame:(CGRect)frame {
	%orig;
	[[IFBase sharedInstance] applyPreferences];
}
+ (float)sideIconInset {
	if ([[IFBase sharedInstance] spacingMethod] == kSpacingMethodEven)
		return [[IFBase sharedInstance] leftInset];
	
	return %orig;
}
- (float)_additionalSideInsetToCenterIcons {
	if ([[IFBase sharedInstance] spacingMethod] == kSpacingMethodEven)
		return 0.0f;
	
	return %orig;
}
- (void)setOrientation:(int)orientation duration:(double)duration {
	%orig;
	[self setIconsNeedLayout];
}
%end

%hook SBIconController
- (id)insertIcon:(SBIcon *)icon intoListView:(SBIconList *)list iconIndex:(int)index moveNow:(BOOL)now {
	id ret;

	if (list == [[IFBase sharedInstance] dock]) disableVisibleFlag += 1;
	ret = %orig;
	if (list == [[IFBase sharedInstance] dock]) disableVisibleFlag -= 1;
	
	return ret;
}
%end

%hook SBUIController
- (void)restoreIconListAnimated:(BOOL)animated {
	%orig;
	[[IFBase sharedInstance] restoreToPage];
}
- (void)restoreIconListAnimated:(BOOL)animated animateWallpaper:(BOOL)wallpaper {
	%orig;
	[[IFBase sharedInstance] restoreToPage];
}
- (void)restoreIconListAnimated:(BOOL)animated animateWallpaper:(BOOL)wallpaper keepSwitcher:(BOOL)switcher {
	%orig;
	[[IFBase sharedInstance] restoreToPage];
}
%end

%end

__attribute__((constructor)) static void four_init() {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	if ([IFFirmwareVersion() hasPrefix:@"4.0"] || [IFFirmwareVersion() hasPrefix:@"4.1"]) {
		%init(Four);
		[IFBase setClass:[IFFour class]];
		[IFFour sharedInstance];
	}
	
	[pool release];
}

