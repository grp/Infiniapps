#import <Foundation/Foundation.h>

#ifndef CountlyPrefix
#error "Must define CountlyPrefix."
#endif

#ifndef CountlyAppVersion
#error "Must define app version."
#endif

#ifndef CountlyAppIdentifier
#error "App identifier missing.h"
#endif

#ifndef CountlyAppToken
#error "Need an app token."
#endif

#define CountlyQuote_(x) #x
#define CountlyQuote(x) CountlyQuote_(x)

#define CountlyPaste_(x, y) x##y
#define CountlyPaste(x, y) CountlyPaste_(x, y)

#define C(name) CountlyPaste(CountlyPrefix, CountlyPaste(O, name))

#define EventQueue 1
@class C(EventQueue);

#define Countly 0
@interface C(Countly) : NSObject {
#define unsentSessionLength 8
	double C(unsentSessionLength);
#define countlyIsSuspended 11
	BOOL C(countlyIsSuspended);
#define eventQueue 12
    C(EventQueue) *C(eventQueue);
}

#define sharedCountly 13
+ (C(Countly) *)C(sharedCountly);

#define countlyStart 14
#define countlyWithHost 15
- (void)C(countlyStart):(NSString *)appKey C(countlyWithHost):(NSString *)appHost;

#define countlyRecordEvent 16
#define countlyCount 17
#define countlySum 18
#define countlySegmentation 19
- (void)C(countlyRecordEvent):(NSString *)key C(countlyCount):(int)count;
- (void)C(countlyRecordEvent):(NSString *)key C(countlyCount):(int)count C(countlySum):(double)sum;
- (void)C(countlyRecordEvent):(NSString *)key C(countlySegmentation):(NSDictionary *)segmentation C(countlyCount):(int)count;
- (void)C(countlyRecordEvent):(NSString *)key C(countlySegmentation):(NSDictionary *)segmentation C(countlyCount):(int)count C(countlySum):(double)sum;

@end

#define CountlySharedInstance [C(Countly) C(sharedCountly)]
#define CountlyStartWithTokenHost(countly, token, host) [countly C(countlyStart):token C(countlyWithHost):host]
#define CountlyRecordEventCount(event, count) [CountlySharedInstance C(countlyRecordEvent):event C(countlyCount):count]
#define CountlyRecordEventCountSum(event, count, sum) [CountlySharedInstance C(countlyRecordEvent):event C(countlyCount):count C(countlySum):sum]
#define CountlyRecordEventSegmentationCount(event, segmentation, count) [CountlySharedInstance C(countlyRecordEvent):event C(countlySegmentation):segmentation C(countlyCount):count]
#define CountlyRecordEventSegmentationCountSum(event, segmentation, count, sum) [CountlySharedInstance C(countlyRecordEvent):event C(countlySegmentation):segmentation C(countlyCount):count C(countlySum):sum]


