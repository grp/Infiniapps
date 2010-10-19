
#import "Infinidock.h"

static int disableVisibleFlag;
static int disableOriginFlag;
static int disableColumnFlag;
static int disableInsetFlag;

@interface IFWildcat : IFBase { }
@end

@implementation IFWildcat
- (BOOL)isWildcat {
	return YES;
}
- (NSUInteger)defaultIconCount; {
	return 6;
}
- (CGPoint)origOrigin:(NSInteger)idx {
	disableOriginFlag += 1;
	CGPoint ret = [[self dock] originForIconAtX:idx Y:0];
	disableOriginFlag -= 1;
	return ret;
}
- (NSInteger)origColumn:(CGPoint)point {
	disableColumnFlag += 1;
	NSInteger ret = [[self dock] columnAtPoint:point];
	disableColumnFlag -= 1;
	return ret;
}
- (CGFloat)origInset {
	disableInsetFlag += 1;
	CGFloat ret = [dock horizontalIconInset];
	disableInsetFlag -= 1;
	return ret;
}
@end


%group Wildcat

%hook SBButtonBar
- (id)initWithFrame:(CGRect)frame {
	self = %orig;
	[[IFWildcat sharedInstance] setDock:self];
	return self;
}
- (void)cleanupAfterRotation {
	%orig;
	[self setIconsNeedLayout];
}
+ (int)iconColumnsForInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	return [[IFWildcat sharedInstance] maxColumns];
}
- (int)visibleIconsInButtonBar {
	if (!disableVisibleFlag) {
		return [[IFWildcat sharedInstance] visibleCount];
	}
	
	return %orig;
}
- (CGPoint)originForIconAtX:(int)x Y:(int)y {
	if (!disableOriginFlag) {
		CGPoint origin = %orig;
		origin.x = [[IFWildcat sharedInstance] originForIcon:x].x;
		return origin;
	}
	
	return %orig;
}
- (int)columnAtPoint:(CGPoint)point {
	if (!disableColumnFlag) {
		int col = [[IFWildcat sharedInstance] columnAtPoint:point];
		return col;
	}
	
	return %orig;
}
- (void)addSubview:(UIView *)subview {
	if (subview == [[IFWildcat sharedInstance] scrollView]) {
		%orig;
	} else {
		[[[IFWildcat sharedInstance] scrollView] addSubview:subview];
	}
}
- (void)removeAllIcons {
	%orig;
	[[[[IFBase sharedInstance] scrollView] subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
}
- (void)setFrame:(CGRect)frame {
	%orig;
	[[IFBase sharedInstance] applyPreferences];
}
- (float)horizontalIconInset {
	if (!disableInsetFlag)
		return [[IFBase sharedInstance] leftInset];
	
	return %orig;
}
- (float)horizontalInsetForCenteringIcons {
	if ([[IFBase sharedInstance] spacingMethod] == kSpacingMethodEven)
		return 0.0f;
	
	return %orig;
}
%end

%hook SBIconController
- (id)insertIcon:(SBIcon *)icon intoIconList:(SBIconList *)list index:(int)index moveNow:(BOOL)now duration:(float)duration {
	id ret;
	
	disableVisibleFlag += 1;
	ret = %orig;
	disableVisibleFlag -= 1;
	
	return ret;
}
%end

%hook SBUIController
- (void)restoreIconListAnimated:(BOOL)animated {
	%orig;
	[[IFBase sharedInstance] restoreToPage];
}
- (void)restoreIconListAnimated:(BOOL)animated animateWallpaper:(BOOL)animateWallpaper {
	%orig;
	[[IFBase sharedInstance] restoreToPage];
}
%end

%end


__attribute__((constructor)) static void wildcat_init() {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	if ([IFFirmwareVersion() hasPrefix:@"3.2"]) {
		%init(Wildcat);
		[IFBase setClass:[IFWildcat class]];
		[IFWildcat sharedInstance];
	}
	
	[pool release];
}
