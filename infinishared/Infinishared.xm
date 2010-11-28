
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>

#import <SpringBoard/SpringBoard.h>
#import <SpringBoard/SBIconList.h>
#import <SpringBoard/SBIcon.h>
#import <SpringBoard/SBIconController.h>
#import <SpringBoard/SBUIController.h>
#import <SpringBoard/SBDownloadingIcon.h>
#import <SpringBoard/SBIconModel.h>
#import <dlfcn.h>
#import <objc/runtime.h>

#import <substrate.h>

%class SBIconController;
%class SBIcon;
%class SBIconModel;
%class SBDownloadingIcon;

%group Infinishared

static SBIcon *iconWithIdentifier(NSString *identifier) {
    SBIcon *icon;
    SBIconModel *iconModel = [$SBIconModel sharedInstance];

    if ([iconModel respondsToSelector:@selector(leafIconForIdentifier:)])
        icon = [iconModel leafIconForIdentifier:identifier];
    else
        icon = [iconModel iconForDisplayIdentifier:identifier];

    return icon;
}
static NSString *downloadingIconBundleID(SBIcon *icon) {
    if ([icon respondsToSelector:@selector(bundleID)]) 
        return [icon bundleID];
    else
        return [icon applicationBundleID];
}

%hook SBIconModel
- (void)replaceDownloadingDisplayIdentifiers:(NSArray *)identifiers withDisplayIdentifiers:(NSArray *)displayIdentifiers {
    for (NSString *identifier in identifiers) {
        [iconWithIdentifier(identifier) performSelector:@selector(removeFromSuperview) withObject:nil afterDelay:0.1];
    }

    %orig;
}
- (void)removeAppForDownloadingIcon:(SBDownloadingIcon *)downloadingIcon {
    [iconWithIdentifier(downloadingIconBundleID(downloadingIcon)) performSelector:@selector(removeFromSuperview) withObject:nil afterDelay:0.1];

    %orig;
}
- (void)replaceDownloadingIconIdentifiers:(id)identifiers withAppIconIdentifiers:(id)appIconIdentifiers {
    for (NSString *identifier in identifiers) {
        [iconWithIdentifier(identifier) performSelector:@selector(removeFromSuperview) withObject:nil afterDelay:0.1];
    }

    %orig;
}
- (void)removeApplicationIconForDownloadingIcon:(SBDownloadingIcon *)downloadingIcon {
    [iconWithIdentifier(downloadingIconBundleID(downloadingIcon)) performSelector:@selector(removeFromSuperview) withObject:nil afterDelay:0.1];

    %orig;
}
%end

%end

void infinishared_init() {
    %init(Infinishared);
}

@interface IconListHelper
- (CGPoint)originForIconAtX:(int)x Y:(int)y;
@end

static NSMutableDictionary *cache;

static inline CGPoint point_from_value(NSValue *value) {
    return [value CGPointValue];
}

static inline NSValue *value_from_point(CGPoint point) {
    return [NSValue valueWithCGPoint:point];
}

void cache_init(id list, int r, int c) {
    NSNumber *key = [NSNumber numberWithInt:(int) list];
    NSMutableArray *rows = [NSMutableArray array];

    for (int y = 0; y < c; y++) {
        NSMutableArray *row = [NSMutableArray array];
        for (int x = 0; x < r; x++) {
            [row addObject:value_from_point([((IconListHelper *)list) originForIconAtX:x Y:y])];
        }
        [rows addObject:row];
    }

    [cache setObject:rows forKey:key];
}

BOOL cache_ready(id list) {
    return [cache objectForKey:[NSNumber numberWithInt:(int) list]] != nil;
}

CGPoint cache_point(id list, int x, int y) {
    NSArray *rows = [cache objectForKey:[NSNumber numberWithInt:(int) list]];
    if (rows == nil) return CGPointMake(-1337.0, -1337.0);
    return point_from_value([[rows objectAtIndex:y] objectAtIndex:x]);
}

void cache_destroy(id list) {
    [cache removeObjectForKey:[NSNumber numberWithInt:(int) list]];
}
