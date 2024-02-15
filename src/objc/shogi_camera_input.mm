#import <MetalKit/MetalKit.h>
#import <AVFoundation/AVFoundation.h>

#include <shogi_camera_input/objc/shogi_camera_input.h>

//#import "shogi_camera_input_demo-Swift.h"

@implementation SCIPoint

- (id)init {
  if (self = [super init]) {
    self.x = 0;
    self.y = 0;
  }
  return self;
}

- (id)initWithX:(int)x y:(int)y {
  if (self = [super init]) {
    self.x = x;
    self.y = y;
  }
  return self;
}

- (CGPoint)point {
  return CGPointMake(self.x, self.y);
}
@end

@implementation SCIShape

- (id)init {
  if (self = [super init]) {
    self.points = [[NSArray alloc] init];
  }
  return self;
}

- (id)initWithPoints:(NSArray<SCIPoint *> *)p {
  if (self = [super init]) {
    if (p) {
      self.points = [p mutableCopy];
    } else {
      self.points = [[NSArray alloc] init];
    }
  }
  return self;
}

@end

#include "../shogi_camera_input.cpp"

@implementation SCI

//+ (SessionStatus *)FindSquares:(UIImage *)input {
//  cv::Mat image;
//  UIImageToMat(input, image, true);
//  if (image.empty()) {
//    return input;
//  }
//
//  Session::Status st = com::github::kbinani::sci::Session::FindSquares(image);
//  for (auto const &square : squares) {
//    NSMutableArray<SCIPoint *> *points = [[NSMutableArray alloc] init];
//    for (auto const &p : square) {
//      [points addObject:[[SCIPoint alloc] initWithX:p.x y:p.y]];
//    }
//
//    SCIShape *shape = [[SCIShape alloc] initWithPoints:points];
//    [ret addObject:shape];
//  }
//  return MatToUIImage(recog);
//}

@end
