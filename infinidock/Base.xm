
#import "Infinidock.h"
#import "infinishared/Infinishared.h"

@interface IFScrollView : UIScrollView { }
@end

@implementation IFScrollView
- (void)_didRemoveSubview:(id)subview {
	[super _didRemoveSubview:subview];
	[[IFBase sharedInstance] fixScrollWidth];
}
- (void)didAddSubview:(id)subview {
	[super didAddSubview:subview];
	[[IFBase sharedInstance] fixScrollWidth];
}
@end

static NSDictionary *prefsDict = nil;
static Class specificClass;

@implementation IFBase
@synthesize scrollView;

+ (void)setClass:(Class)someclass {
	specificClass = someclass;
}
+ (id)sharedInstance {
	static id shared;
	if (shared == nil)
		shared = [[specificClass alloc] init];
	
	return shared;
}
- (id)init {
	if ((self = [super init])) {
		// pass;
	}
	
	return self;
}
- (void)applyPreferences {
	[dock addSubview:scrollView];
	[scrollView setScrollEnabled:SCROLL_ENABLED];
	[scrollView setPagingEnabled:PAGING_ENABLED];
	[scrollView setDelegate:self];
	[scrollView setDelaysContentTouches:NO];
	[scrollView setAlwaysBounceHorizontal:YES];
	[scrollView setShowsHorizontalScrollIndicator:NO];
	[scrollView setClipsToBounds:NO];
	[scrollView setFrame:[dock bounds]];
	[self fixScrollWidth];
}
- (IFSpacingMethod)spacingMethod {
	NSUInteger currentCount = [self currentIconCount];
	NSUInteger selectedCount = [self selectedIconCount];
	NSUInteger defaultCount = [self defaultIconCount];
	
	
	BOOL lessThanDefaultSelected	= selectedCount	<	defaultCount;
	BOOL moreThanDefaultSelected	= selectedCount	> 	defaultCount;
	BOOL exactlyDefaultSelected		= selectedCount	== 	defaultCount;
	
	BOOL exactlyOnePage		= currentCount	==	selectedCount;
	BOOL moreThanOnePage	= currentCount	>	selectedCount;
	BOOL lessThanOnePage	= currentCount	<	selectedCount;
	
	BOOL exactlyDefault		= currentCount	==	defaultCount;
	BOOL lessThanDefault	= currentCount	<	defaultCount;
	BOOL moreThanDefault	= currentCount	>	defaultCount;
	
	if (PAGING_ENABLED) {
		if (moreThanDefaultSelected)
			return kSpacingMethodEven;
		else
			return kSpacingMethodPaged;
	} else {
		BOOL fixDefault = (moreThanOnePage || exactlyOnePage) && (moreThanDefaultSelected || exactlyDefaultSelected);

		if (fixDefault || lessThanOnePage)
			return kSpacingMethodDefault;
		else
			return kSpacingMethodEven;
	}
}
- (CGFloat)dockWidth {
	return [dock frame].size.width;
}
- (NSUInteger)visibleCount {
	return MIN([self selectedIconCount], [self currentIconCount]);
}
- (NSUInteger)pageCount {
	int pageCount = [self currentIconCount] / [self selectedIconCount];
	if ([self currentIconCount] % [self selectedIconCount])
		pageCount += 1;
		
	return pageCount;
}
- (CGFloat)scrollWidth {
	CGFloat width;
	CGFloat frameWidth = [self dockWidth];
	
	if (PAGING_ENABLED) {
		int pages = [self pageCount];
		width = pages * frameWidth;
	} else {		
		width = [self originForIcon:[self currentIconCount] - 1].x;
		width += ICON_WIDTH;
		width += [self originForIcon:0].x;
		
		if (width < frameWidth) width = frameWidth;
	}
	
	return width;
}
- (void)fixScrollWidth {
	CGFloat width = [self scrollWidth];
	
	if (scrollView.contentSize.width != width) {
		CGPoint offset = [scrollView contentOffset];
		if (offset.x + [self dockWidth] > width)
			offset.x = width - [self dockWidth];
		
		[UIView beginAnimations:nil context:NULL];
		[UIView setAnimationDuration:0.3f];
		[scrollView setContentSize:CGSizeMake(width, [dock bounds].size.height)];
		[UIView commitAnimations];
		
		[scrollView setContentOffset:offset animated:NO];
	}
}
- (NSUInteger)currentIconCount {
	return [[dock icons] count];
}
- (BOOL)isWildcat {
	// NOTE: overridden by subclasses
	return NO;
}
- (NSUInteger)defaultIconCount; {
	return 4;
}
- (NSUInteger)selectedIconCount {
	return MAX_PERPAGE;
}
- (CGFloat)leftInset {
	IFSpacingMethod spacing = [self spacingMethod];
	
	if (spacing == kSpacingMethodEven) {
		return 0.0f;
	} else {
		return [self origInset];
	}
}
- (CGPoint)originForIcon:(NSInteger)idx { 
	CGPoint origin;
	IFSpacingMethod spacing = [self spacingMethod];
	
	int page, basex;
	
	basex = idx % [self selectedIconCount];
	page = idx / [self selectedIconCount];
	
	if (spacing == kSpacingMethodPaged) {
		origin = [self origOrigin:basex];
		
		origin.x += [self dockWidth] * page;
	} else if (spacing == kSpacingMethodEven) {
		origin = [self origOrigin:basex];
		
		int divisor = MIN([self currentIconCount], [self selectedIconCount]);
		int sections = divisor * 2;
		int which = ((basex + 1) * 2) - 1;
		CGFloat sectionWidth = [self dockWidth] / sections;
		
		origin.x = sectionWidth * which;
		
		origin.x -= ICON_WIDTH / 2;
		origin.x = roundf(origin.x);
		
		origin.x += [self dockWidth] * page;
	} else if (spacing == kSpacingMethodDefault) {
		origin = [self origOrigin:idx];
	}
	
	return origin;
}
- (NSUInteger)maxColumns {
	// Completely arbitrary balance of performance and email reduction.
	return 50;
}
- (NSInteger)columnAtPoint:(CGPoint)point {
	int ret;
	
	// Convert from absolute points to scrollview points
	point.x += scrollView.contentOffset.x;
	
	IFSpacingMethod spacing = [self spacingMethod];
	if (spacing == kSpacingMethodPaged) {
		int page = ((int) point.x) / ((int) [self dockWidth]);
		CGPoint base = CGPointMake(fmodf(point.x, [self dockWidth]), point.y);
		
		ret = [self origColumn:base];
		ret += page * MAX_PERPAGE;
	} else {
		// This doesn't work in all cases.
		ret = [self origColumn:point];
		
		/* Inefficient alert!
		ret = 0;
		SBIcon *icon;
		for (int i = 0; i < [[dock icons] count]; i++) {
			icon = [[dock icons] objectAtIndex:i];
			
			if (icon.frame.origin.x + (ICON_WIDTH / 2) > point.x)
				break;
			
			ret = i;
		}*/
	}
	
	return MIN(ret, [self currentIconCount] - 1);
}
- (BOOL)containsIcon:(SBIcon *)icon {
	// This is more accurate than using the array, for reasons unknown (?)
	return NSNotFound != [[scrollView subviews] indexOfObjectIdenticalTo:icon];
}
- (SBDestinationHole *)destinationHole {
	Class hole = objc_getClass("SBDestinationHole");
	return [hole respondsToSelector:@selector(sharedInstance)] ? [hole sharedInstance] : [hole destinationHole];
}

/* Scrolling Snap */
- (void)scrollViewDidEndDragging:(UIScrollView *)_scrollView willDecelerate:(BOOL)decelerate {
	// NOTE: -scrollViewDidEndDecelerating isn't called if you manually stop the scrolling,
	//       so this calls it in that case (won't decelerate). This allows the scrolling snap
	//       code to all be in one place
	
	if (!decelerate) {
		[self scrollViewDidEndDecelerating:scrollView];
	}
}
- (void)scrollViewDidEndDecelerating:(UIScrollView *)_scrollView {
	// NEW: Check for no icons to prevent crashing. (There's no snapping w/o icons anyway, so it's fine.)
	if (disableDecelerateFlag || !SNAP_ENABLED || PAGING_ENABLED || [[objc_getClass("SBIconController") sharedInstance] isEditing] || ![[[self dock] icons] count])
		return;
	
	CGFloat dockWidth = [self dockWidth];
	CGFloat fullWidth = scrollView.contentSize.width;
	CGPoint offset = [scrollView contentOffset];	
	
	// columnAtPoint: expects screen coords
	NSInteger position = [self columnAtPoint:CGPointZero];
		
	SBIcon *icon = [[dock icons] objectAtIndex:position];
	CGFloat leftOrigin = icon.frame.origin.x;
	CGFloat leftOffset = [self originForIcon:0].x;
	
	if (offset.x - leftOrigin < ICON_WIDTH / 2) {
		offset.x = leftOrigin - leftOffset;
	} else {
		CGFloat iconPadding = [self originForIcon:1].x - ICON_WIDTH - leftOffset;
		// Right side of the icon plus just enough so that the next icon is shown correctly
		offset.x = leftOrigin + ICON_WIDTH + (iconPadding - leftOffset);
	}
	
	// Ensure that we never set the offset so that (offset + width) greater than scrolling width
	if (offset.x + dockWidth > fullWidth) {
		offset.x = fullWidth - dockWidth;
	}
	
	disableDecelerateFlag += 1;
	[scrollView setContentOffset:offset animated:YES];
	disableDecelerateFlag -= 1;
}

/* Getters and setters */
- (id)dock {
	return dock;
}
- (void)setDock:(id)_dock {
	if (dock != _dock) {
		[dock autorelease];
		dock = [_dock retain];
		
		[scrollView autorelease];
		scrollView = [[IFScrollView alloc] init];
		
		[self applyPreferences];
		[self fixScrollWidth];
	}
}

/* Restore to Page */
- (void)restoreToPage {
	[self applyPreferences];
	
	// Check for disabled, then bail.
	// NEW: Check for no icons (to prevent crashes). There's no point w/o any icons anyway.
	if (RESTORE_PAGE == 0 || ![[[self dock] icons] count]) return;

	CGPoint newOffset = CGPointZero;
	CGFloat pageWidth = [self dockWidth];
	int restorePage = RESTORE_PAGE - 1; // Allow for disabled state.
	int iconCount = [self currentIconCount];
	int selectedCount = [self selectedIconCount];
	int pageCount = [self pageCount];
	
	// No point calculating if we won't be moving
	if (iconCount <= selectedCount) return;
	
	// The first page is always at the beginning
	if (restorePage == 0) {
		[scrollView setContentOffset:CGPointZero animated:NO];
		return;
	}
	
	if (PAGING_ENABLED) {
		if (restorePage >= pageCount) restorePage = pageCount - 1;
		
		newOffset.x = restorePage * pageWidth;
	} else {
		int startIndex = selectedCount * restorePage;
		
		if (iconCount < startIndex + selectedCount) {
			startIndex = iconCount - selectedCount;
		}
		
		SBIcon *icon = [[dock icons] objectAtIndex:startIndex];
		newOffset.x = icon.frame.origin.x;
		
		CGFloat iconPadding;
		CGPoint one = [dock originForIconAtX:0 Y:0];
		CGPoint two = [dock originForIconAtX:1 Y:0];
		iconPadding = (two.x - one.x) - ICON_WIDTH;
		
		newOffset.x -= iconPadding / 2;
	}
	
	[scrollView setContentOffset:newOffset animated:NO];
}

@end

/* Standard Hooks */

%group Base

%hook SBUIController
- (void)finishLaunching {
	%orig;
	
	[[IFBase sharedInstance] applyPreferences];
	[[IFBase sharedInstance] fixScrollWidth];
}
%end

%hook SBIconModel
- (void)relayout {
	%orig;
	[[IFBase sharedInstance] applyPreferences];
}
%end

%hook SBIconController
- (void)moveIconFromWindow:(SBIcon *)icon toIconList:(id)iconList {
	if (iconList == [[IFBase sharedInstance] dock]) {
		CGRect frame = [icon frame];
		UIScrollView *scrollView = [[IFBase sharedInstance] scrollView];
		frame.origin.x += scrollView.contentOffset.x;
		[icon setFrame:frame];
	}
	
	%orig;
}
- (void)setGrabbedIcon:(SBIcon *)icon {
	UIScrollView *scrollView = [[IFBase sharedInstance] scrollView];
	[scrollView setScrollEnabled:!icon];

	%orig;
}
- (void)setIsEditing:(BOOL)editing {
	%orig;
	
	UIScrollView *scrollView = [[IFBase sharedInstance] scrollView];
	
	if (editing) [scrollView setDelaysContentTouches:YES];
	else [scrollView setDelaysContentTouches:NO];
	
	if (!editing) [[scrollView delegate] scrollViewDidEndDecelerating:scrollView];
}
%end

%hook SBIcon
- (UIView *)superview{
	id dock = [[IFBase sharedInstance] dock];
	if (dock && [self isKindOfClass:objc_getClass("SBStackIcon")] && [[IFBase sharedInstance] containsIcon:self]) {
		return dock;
	}
	
	return %orig;
}
- (CGRect)frame {
	CGRect frame = %orig;
	
	id dock = [[IFBase sharedInstance] dock];
	UIScrollView *scrollView = [[IFBase sharedInstance] scrollView];
	
	// Fix for Stack v3 and Categories
	if ((dock && [[IFBase sharedInstance] containsIcon:self] && ([self isKindOfClass:objc_getClass("SBStackIcon")] || ([self isKindOfClass:objc_getClass("SBApplicationIcon")] && [[[(SBApplicationIcon *) self application] displayIdentifier] hasPrefix:@"com.bigboss.categories."])))) {
		frame.origin.x -= scrollView.contentOffset.x;
	}
	
	return frame;
}
%end

%end


/* Constructor */

#if TARGET_IPHONE_SIMULATOR
NSString *IFFirmwareVersion() { return @"3.0"; }
#else
extern "C" id lockdown_connect();
extern "C" void lockdown_disconnect(id port);
extern "C" NSString *lockdown_copy_value(id port, int idk, CFStringRef value);

extern "C" CFStringRef kLockdownDeviceNameKey;
extern "C" CFStringRef kLockdownBuildVersionKey;    // buildVersion
extern "C" CFStringRef kLockdownProductVersionKey;    // systemVersion
extern "C" CFStringRef kLockdownDeviceClassKey;      // model
extern "C" CFStringRef kLockdownProductTypeKey;
extern "C" CFStringRef kLockdownUniqueDeviceIDKey;

NSString *IFFirmwareVersion() {
	static NSString *version = nil;
	if (version) return version;
	
	NSDictionary *sys = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
	version = [[sys objectForKey:@"ProductVersion"] retain];
	return version;
	
	
	/*id port = nil;
	NSString *val = nil;
	
	if((port = lockdown_connect())) {
		val = lockdown_copy_value(port, 0, kLockdownProductVersionKey);
		[val autorelease];
		lockdown_disconnect(port);
	}
	
	return val;*/
}
#endif

void IFPreferencesChanged(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
	[prefsDict release];
	prefsDict = [[NSDictionary alloc] initWithContentsOfFile:IFPreferencesFilePath];
	[[objc_getClass("SBIconModel") sharedInstance] relayout];
}

__attribute__((constructor)) static void infinidock_init() {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// SpringBoard only!
	if (![[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.springboard"])
		return;
	
	NSLog(@"Welcome to Infinidock.");
	NSLog(@"The cake is a lie.");
	
	%init(Base);
	infinishared_init();
	
	prefsDict = [[NSDictionary alloc] initWithContentsOfFile:IFPreferencesFilePath];
	CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, IFPreferencesChanged, CFSTR(IFPreferencesChangedNotification), NULL, CFNotificationSuspensionBehaviorCoalesce);
	
	dlopen("/Library/MobileSubstrate/DynamicLibraries/IconSupport.dylib", RTLD_NOW);
	[[objc_getClass("ISIconSupport") sharedInstance] addExtension:@"infinidock"];
	
	[pool release];
}

