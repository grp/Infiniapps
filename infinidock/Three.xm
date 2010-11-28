
#import "Infinidock.h"

static int disableVisibleFlag;
static int disableOriginFlag;
static int disableColumnFlag;
static int disableInsetFlag;

@interface IFThree : IFBase { }
@end

@implementation IFThree
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


%group Three

%hook SBButtonBar
- (id)initWithFrame:(CGRect)frame {
    self = %orig;
    [[IFThree sharedInstance] setDock:self];
    return self;
}
- (int)maxIconColumns {
    if (!disableColumnFlag)
        return [[IFThree sharedInstance] maxColumns];

    return MAX([[IFThree sharedInstance] selectedIconCount], [[IFThree sharedInstance] defaultIconCount]);
}
- (int)visibleIconsInRow:(id)row {
    if (!disableVisibleFlag) {
        return [[IFThree sharedInstance] visibleCount];
    }

    return MAX([[IFThree sharedInstance] selectedIconCount], [[IFThree sharedInstance] defaultIconCount]);
}
- (CGPoint)originForIconAtX:(int)x Y:(int)y {
    if (!disableOriginFlag) {
        CGPoint origin = %orig;
        origin.x = [[IFThree sharedInstance] originForIcon:x].x;
        return origin;
    }

    disableColumnFlag += 1;
    CGPoint ret = %orig;
    disableColumnFlag -= 1;

    return ret;
}
- (int)columnAtPoint:(CGPoint)point {
    if (!disableColumnFlag) {
        int col = [[IFThree sharedInstance] columnAtPoint:point];
        return col;
    }

    return %orig;
}
- (void)addSubview:(UIView *)subview {
    if (subview == [[IFThree sharedInstance] scrollView]) {
        %orig;
    } else {
        [[[IFThree sharedInstance] scrollView] addSubview:subview];
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
%end

%hook SBUIController
- (void)restoreIconList:(BOOL)unk {
    %orig;
    [[IFBase sharedInstance] restoreToPage];
}
%end

%end


__attribute__((constructor)) static void three_init() {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if ([IFFirmwareVersion() hasPrefix:@"3.1"] || [IFFirmwareVersion() hasPrefix:@"3.0"]) {
        %init(Three);
        [IFBase setClass:[IFThree class]];
        [IFThree sharedInstance];
    }

    [pool release];
}
