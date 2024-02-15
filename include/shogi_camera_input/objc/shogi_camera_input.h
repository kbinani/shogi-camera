#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

//#import "shogi_camera_input_demo-Swift.h"

@interface SCIPoint : NSObject
@property(nonatomic) int x;
@property(nonatomic) int y;
- (CGPoint)point;
- (id)initWithX:(int)x y:(int)y;
@end

@interface SCIShape : NSObject
@property(retain, nonatomic) NSArray<SCIPoint *> *points;
- (id)initWithPoints:(NSArray<SCIPoint *> *)points;
@end

@interface SCI : NSObject
//+ (SessionStatus *)FindSquares:(UIImage *)image;
@end
