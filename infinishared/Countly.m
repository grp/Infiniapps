// Countly.m
//
// This code is provided under the MIT License.
//
// Please visit www.count.ly for more information.

#define COUNTLY_VERSION "1.0"

#import "Countly.h"
#import <UIKit/UIKit.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <CoreTelephony/CTCarrier.h>

#include <sys/types.h>
#include <sys/sysctl.h>

__attribute__((unused)) static NSString *CountlyStringByEscapingForURLArgument (NSString *self) {
	// Encode all the reserved characters, per RFC 3986
	// (<http://www.ietf.org/rfc/rfc3986.txt>)
	CFStringRef escaped = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, (CFStringRef)self, NULL, (CFStringRef)@"!*'();:@&=+$,/?%#[]", kCFStringEncodingUTF8);
	return [(id)escaped autorelease];
}

__attribute__((unused)) static NSString *CountlyStringByUnescapingFromURLArgument (NSString *self) {
	NSMutableString *resultString = [NSMutableString stringWithString:self];
	[resultString replaceOccurrencesOfString:@"+" withString:@" " options:NSLiteralSearch range:NSMakeRange(0, [resultString length])];
	return [resultString stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
}

__attribute__((unused)) static NSString *CountlyDeviceInfoUDID() {
	return [[UIDevice currentDevice] performSelector:@selector(uniqueIdentifier)];
}

__attribute__((unused)) static NSString *CountlyDeviceInfoDevice() {
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char *machine = malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString *platform = [NSString stringWithUTF8String:machine];
    free(machine);

    return platform;
}

__attribute__((unused)) static NSString *CountlyDeviceInfoOSVersion() {
	return [[UIDevice currentDevice] systemVersion];
}

__attribute__((unused)) static NSString *CountlyDeviceInfoCarrier() {
	if (NSClassFromString(@"CTTelephonyNetworkInfo")) {
		CTTelephonyNetworkInfo *netinfo = [[[CTTelephonyNetworkInfo alloc] init] autorelease];
		CTCarrier *carrier = [netinfo subscriberCellularProvider];
		return [carrier carrierName];
	}

	return nil;
}

__attribute__((unused)) static NSString *CountlyDeviceInfoResolution() {
	CGRect bounds = [[UIScreen mainScreen] bounds];
	CGFloat scale = [[UIScreen mainScreen] respondsToSelector:@selector(scale)] ? [[UIScreen mainScreen] scale] : 1.0f;
	CGSize res = CGSizeMake(bounds.size.width * scale, bounds.size.height * scale);
	NSString *result = [NSString stringWithFormat:@"%gx%g", res.width, res.height];

	return result;
}

__attribute__((unused)) static NSString *CountlyDeviceInfoLocale() {
	return [[NSLocale currentLocale] localeIdentifier];
}

__attribute__((unused)) static NSString *CountlyDeviceInfoAppVersion() {
	NSString *version = @CountlyQuote(CountlyAppVersion);

    NSString *appid = @CountlyQuote(CountlyAppIdentifier);
    NSString *prefix = @"/var/lib/";
    NSString *next = @"dpkg";
    NSString *info = @"/info";
    NSString *total = [NSString stringWithFormat:@"%@%@%@/%@.list", prefix, next, info, appid];
    BOOL pirated = ![[NSFileManager defaultManager] fileExistsAtPath:total];

    if (pirated) {
        version = [version stringByAppendingString:@"p"];
    }

    return version;
}

__attribute__((unused)) static NSString *CountlyDeviceInfoMetrics() {
	NSString *result = @"{";

	result = [result stringByAppendingFormat:@"\"%@\":\"%@\"", @"_device", CountlyDeviceInfoDevice()];
	result = [result stringByAppendingFormat:@",\"%@\":\"%@\"", @"_os", @"iOS"];
	result = [result stringByAppendingFormat:@",\"%@\":\"%@\"", @"_os_version", CountlyDeviceInfoOSVersion()];

	NSString *carrier = CountlyDeviceInfoCarrier();
	if (carrier != nil)
		result = [result stringByAppendingFormat:@",\"%@\":\"%@\"", @"_carrier", carrier];

	result = [result stringByAppendingFormat:@",\"%@\":\"%@\"", @"_resolution", CountlyDeviceInfoResolution()];
	result = [result stringByAppendingFormat:@",\"%@\":\"%@\"", @"_locale", CountlyDeviceInfoLocale()];
	result = [result stringByAppendingFormat:@",\"%@\":\"%@\"", @"_app_version", CountlyDeviceInfoAppVersion()];
	result = [result stringByAppendingString:@"}"];
	result = CountlyStringByEscapingForURLArgument(result);

	return result;
}

#define Event 40
@interface C(Event) : NSObject

#define countlyKey 41
@property (nonatomic, copy) NSString *C(countlyKey);
@property (nonatomic, retain) NSDictionary *C(countlySegmentation);
@property (nonatomic, assign) int C(countlyCount);
@property (nonatomic, assign) double C(countlySum);
#define countlyTimestamp 46
@property (nonatomic, assign) double C(countlyTimestamp);

@end

@implementation C(Event)

#define countlyKey_ 50
@synthesize C(countlyKey) = C(countlyKey_);
#define countlySegmentation_ 51
@synthesize C(countlySegmentation) = C(countlySegmentation_);
#define countlyCount_ 52
@synthesize C(countlyCount) = C(countlyCount_);
#define countlySum_ 53
@synthesize C(countlySum) = C(countlySum_);
#define countlyTimestamp_ 54
@synthesize C(countlyTimestamp) = C(countlyTimestamp_);

- (id)init {
    if ((self = [super init])) {
        C(countlyKey_) = nil;
        C(countlySegmentation_) = nil;
        C(countlyCount_) = 0;
        C(countlySum_) = 0;
        C(countlyTimestamp_) = 0;
    }

    return self;
}

- (void)dealloc {
    [C(countlyKey_) release];
    [C(countlySegmentation_) release];

    [super dealloc];
}

@end

@interface C(EventQueue) : NSObject {
#define countlyEvents_ 60
    NSMutableArray *C(countlyEvents_);
}

@end

@implementation C(EventQueue)

- (id)init {
    if ((self = [super init])) {
        C(countlyEvents_) = [[NSMutableArray alloc] init];
    }

    return self;
}

- (void)dealloc {
    [C(countlyEvents_) release];
    [super dealloc];
}

- (NSUInteger)C(countlyCount) {
    @synchronized (self) {
        return [C(countlyEvents_) count];
    }
}

#define countlyEvents 61
- (NSString *)C(countlyEvents) {
    NSString *result = @"[";

    @synchronized (self) {
        for (int i = 0; i < C(countlyEvents_).count; ++i) {
            C(Event) *event = [C(countlyEvents_) objectAtIndex:i];

            result = [result stringByAppendingString:@"{"];

            result = [result stringByAppendingFormat:@"\"%@\":\"%@\"", @"key", event.C(countlyKey)];

            if (event.C(countlySegmentation)) {
                NSString *segmentation = @"{";

                NSArray *keys = [event.C(countlySegmentation) allKeys];
                for (int i = 0; i < keys.count; i++) {
                    NSString *key = [keys objectAtIndex:i];
                    NSString *value = [event.C(countlySegmentation) objectForKey:key];

                    segmentation = [segmentation stringByAppendingFormat:@"\"%@\":\"%@\"", key, value];

                    if (i + 1 < keys.count)
                        segmentation = [segmentation stringByAppendingString:@","];
                }
                segmentation = [segmentation stringByAppendingString:@"}"];

                result = [result stringByAppendingFormat:@",\"%@\":%@", @"segmentation", segmentation];
            }

            result = [result stringByAppendingFormat:@",\"%@\":%d", @"count", event.C(countlyCount)];

            if (event.C(countlySum) > 0)
                result = [result stringByAppendingFormat:@",\"%@\":%g", @"sum", event.C(countlySum)];

            result = [result stringByAppendingFormat:@",\"%@\":%ld", @"timestamp", (time_t)event.C(countlyTimestamp)];

            result = [result stringByAppendingString:@"}"];

            if (i + 1 < C(countlyEvents_).count)
                result = [result stringByAppendingString:@","];
        }

        [C(countlyEvents_) release];
        C(countlyEvents_) = [[NSMutableArray alloc] init];
    }

    result = [result stringByAppendingString:@"]"];

    result = CountlyStringByEscapingForURLArgument(result);

	return result;
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlyCount):(int)count {
    @synchronized (self) {
        for (C(Event *event) in C(countlyEvents_)) {
            if ([event.C(countlyKey) isEqualToString:key]) {
                event.C(countlyCount) += count;
                event.C(countlyTimestamp) = (event.C(countlyTimestamp) + time(NULL)) / 2;
                return;
            }
        }

        C(Event) *event = [[C(Event) alloc] init];
        event.C(countlyKey) = key;
        event.C(countlyCount) = count;
        event.C(countlyTimestamp) = time(NULL);
        [C(countlyEvents_) addObject:event];
    }
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlyCount):(int)count C(countlySum):(double)sum {
    @synchronized (self) {
        for (C(Event) *event in C(countlyEvents_)) {
            if ([event.C(countlyKey) isEqualToString:key]) {
                event.C(countlyCount) += count;
                event.C(countlySum) += sum;
                event.C(countlyTimestamp) = (event.C(countlyTimestamp) + time(NULL)) / 2;
                return;
            }
        }

        C(Event) *event = [[C(Event) alloc] init];
        event.C(countlyKey) = key;
        event.C(countlyCount) = count;
        event.C(countlySum) = sum;
        event.C(countlyTimestamp) = time(NULL);
        [C(countlyEvents_) addObject:event];
    }
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlySegmentation):(NSDictionary *)segmentation C(countlyCount):(int)count {
    @synchronized (self) {
        for (C(Event) *event in C(countlyEvents_)) {
            if ([event.C(countlyKey) isEqualToString:key] && event.C(countlySegmentation) && [event.C(countlySegmentation) isEqualToDictionary:segmentation]) {
                event.C(countlyCount) += count;
                event.C(countlyTimestamp) = (event.C(countlyTimestamp) + time(NULL)) / 2;
                return;
            }
        }

        C(Event) *event = [[C(Event) alloc] init];
        event.C(countlyKey) = key;
        event.C(countlySegmentation) = segmentation;
        event.C(countlyCount) = count;
        event.C(countlyTimestamp) = time(NULL);
        [C(countlyEvents_) addObject:event];
    }
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlySegmentation):(NSDictionary *)segmentation C(countlyCount):(int)count C(countlySum):(double)sum {
    @synchronized (self) {
        for (C(Event) *event in C(countlyEvents_)) {
            if ([event.C(countlyKey) isEqualToString:key] && event.C(countlySegmentation) && [event.C(countlySegmentation) isEqualToDictionary:segmentation]) {
                event.C(countlyCount) += count;
                event.C(countlySum) += sum;
                event.C(countlyTimestamp) = (event.C(countlyTimestamp) + time(NULL)) / 2;
                return;
            }
        }

        C(Event) *event = [[C(Event) alloc] init];
        event.C(countlyKey) = key;
        event.C(countlySegmentation) = segmentation;
        event.C(countlyCount) = count;
        event.C(countlySum) = sum;
        event.C(countlyTimestamp) = time(NULL);
        [C(countlyEvents_) addObject:event];
    }
}

@end

#define ConnectionQueue 80
@interface C(ConnectionQueue) : NSObject {
#define countlyQueue_ 81
	NSMutableArray *C(countlyQueue_);
#define countlyConnection_ 82
	NSURLConnection *C(countlyConnection_);
#define countlyAppKey 83
	NSString *C(countlyAppKey);
#define countlyAppHost 84
	NSString *C(countlyAppHost);
}

@property (nonatomic, copy) NSString *C(countlyAppKey);
@property (nonatomic, copy) NSString *C(countlyAppHost);

@end

static C(ConnectionQueue) *s_sharedConnectionQueue = nil;

@implementation C(ConnectionQueue) : NSObject

@synthesize C(countlyAppKey);
@synthesize C(countlyAppHost);

+ (C(ConnectionQueue) *)C(sharedQueue) {
	if (s_sharedConnectionQueue == nil)
		s_sharedConnectionQueue = [[C(ConnectionQueue) alloc] init];

	return s_sharedConnectionQueue;
}

- (id)init {
	if ((self = [super init])) {
		C(countlyQueue_) = [[NSMutableArray alloc] init];
		C(countlyConnection_) = nil;
        C(countlyAppKey) = nil;
        C(countlyAppHost) = nil;
	}

	return self;
}

#define countlyTick 85
- (void)C(countlyTick) {
    if (C(countlyConnection_) != nil || [C(countlyQueue_) count] == 0)
        return;

    NSString *data = [C(countlyQueue_) objectAtIndex:0];
    NSString *urlString = [NSString stringWithFormat:@"%@/i?%@", self.C(countlyAppHost), data];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    C(countlyConnection_) = [NSURLConnection connectionWithRequest:request delegate:self];
    [C(countlyConnection_) start];
}

#define countlyBeginSession 86
- (void)C(countlyBeginSession) {
	NSString *data = [NSString stringWithFormat:@"app_key=%@&device_id=%@&timestamp=%ld&sdk_version="COUNTLY_VERSION"&begin_session=1&metrics=%@",
					  C(countlyAppKey),
					  CountlyDeviceInfoUDID(),
					  time(NULL),
					  CountlyDeviceInfoMetrics()];
	[C(countlyQueue_) addObject:data];
	[self C(countlyTick)];
}

#define countlyUpdateSessionWithDuration 87
- (void)C(countlyUpdateSessionWithDuration):(int)duration {
	NSString *data = [NSString stringWithFormat:@"app_key=%@&device_id=%@&timestamp=%ld&session_duration=%d",
					  C(countlyAppKey),
					  CountlyDeviceInfoUDID(),
					  time(NULL),
					  duration];
	[C(countlyQueue_) addObject:data];
	[self C(countlyTick)];
}

#define countlyEndSessionWithDuration 88
- (void)C(countlyEndSessionWithDuration):(int)duration {
	NSString *data = [NSString stringWithFormat:@"app_key=%@&device_id=%@&timestamp=%ld&end_session=1&session_duration=%d",
					  C(countlyAppKey),
					  CountlyDeviceInfoUDID(),
					  time(NULL),
					  duration];
	[C(countlyQueue_) addObject:data];
	[self C(countlyTick)];
}

#define countlyRecordEvents 89
- (void)C(countlyRecordEvents):(NSString *)events {
    NSLog(@"record events, %@", events);
	NSString *data = [NSString stringWithFormat:@"app_key=%@&device_id=%@&timestamp=%ld&events=%@", C(countlyAppKey), CountlyDeviceInfoUDID(), time(NULL), events];
	[C(countlyQueue_) addObject:data];
	[self C(countlyTick)];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    C(countlyConnection_) = nil;

    [C(countlyQueue_) removeObjectAtIndex:0];

    [self C(countlyTick)];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)err {
    C(countlyConnection_) = nil;
}

- (void)dealloc {
	[super dealloc];

	if (C(countlyConnection_))
		[C(countlyConnection_) cancel];

	[C(countlyQueue_) release];

	self.C(countlyAppKey) = nil;
	self.C(countlyAppHost) = nil;
}

@end

@implementation C(Countly)

+ (C(Countly) *)C(sharedCountly) {
    static C(Countly) *s_sharedCountly = nil;

	if (s_sharedCountly == nil)
		s_sharedCountly = [[C(Countly) alloc] init];

	return s_sharedCountly;
}

- (id)init {
	if ((self = [super init])) {
		C(countlyIsSuspended) = NO;
		C(unsentSessionLength) = 0;
        C(eventQueue) = [[C(EventQueue) alloc] init];
	}

	return self;
}

- (void)C(countlyStart):(NSString *)appKey C(countlyWithHost):(NSString *)appHost {
	[C(ConnectionQueue) C(sharedQueue)].C(countlyAppKey) = appKey;
	[C(ConnectionQueue) C(sharedQueue)].C(countlyAppHost) = appHost;
	[[C(ConnectionQueue) C(sharedQueue)] C(countlyBeginSession)];
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlyCount):(int)count {
    [C(eventQueue) C(countlyRecordEvent):key C(countlyCount):count];

    [[C(ConnectionQueue) C(sharedQueue)] C(countlyRecordEvents):[C(eventQueue) C(countlyEvents)]];
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlyCount):(int)count C(countlySum):(double)sum {
    [C(eventQueue) C(countlyRecordEvent):key C(countlyCount):count C(countlySum):sum];

    [[C(ConnectionQueue) C(sharedQueue)] C(countlyRecordEvents):[C(eventQueue) C(countlyEvents)]];
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlySegmentation):(NSDictionary *)segmentation C(countlyCount):(int)count {
    [C(eventQueue) C(countlyRecordEvent):key C(countlySegmentation):segmentation C(countlyCount):count];

    [[C(ConnectionQueue) C(sharedQueue)] C(countlyRecordEvents):[C(eventQueue) C(countlyEvents)]];
}

- (void)C(countlyRecordEvent):(NSString *)key C(countlySegmentation):(NSDictionary *)segmentation C(countlyCount):(int)count C(countlySum):(double)sum {
    [C(eventQueue) C(countlyRecordEvent):key C(countlySegmentation):segmentation C(countlyCount):count C(countlySum):sum];

    [[C(ConnectionQueue) C(sharedQueue)] C(countlyRecordEvents):[C(eventQueue) C(countlyEvents)]];
}

- (void)dealloc {
    [C(eventQueue) release];

	[super dealloc];
}

@end

