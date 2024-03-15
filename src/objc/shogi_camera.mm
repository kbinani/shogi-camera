#include "../shogi_camera.cpp"
#include <opencv2/imgcodecs/ios.h>

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

using namespace std;

@interface CsaAdapterImpl : NSObject <NSStreamDelegate> {
  NSInputStream *inputStream;
  NSOutputStream *outputStream;
  u8string username;
  u8string password;
  string received;
  int ready;
}
@end

@implementation CsaAdapterImpl

- (id)initWithServer:(u8string)server port:(uint32_t)port username:(u8string)username password:(u8string)password {
  if (self = [super init]) {
    self->username = username;
    self->password = password;
    self->ready = 0;

    CFStringRef cfServer = CFStringCreateWithCString(kCFAllocatorDefault, (char const *)server.c_str(), kCFStringEncodingUTF8);
    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;
    CFStreamCreatePairWithSocketToHost(nil, cfServer, port, &readStream, &writeStream);
    inputStream = (__bridge NSInputStream *)readStream;
    outputStream = (__bridge NSOutputStream *)writeStream;

    [inputStream setDelegate:self];
    [outputStream setDelegate:self];

    [inputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [outputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [inputStream open];
    [outputStream open];
  }
  return self;
}

- (void)send:(u8string)message {
  message += u8"\x0a";
  [outputStream write:(uint8_t const *)message.c_str() maxLength:message.size()];
}

- (void)onMessage:(string)msg {
  NSLog(@"csa:message:%s", msg.c_str());
}

- (void)deinit {
  [inputStream close];
  [outputStream close];
}

- (void)stream:(NSStream *)strm handleEvent:(NSStreamEvent)event {
  switch (event) {
  case NSStreamEventOpenCompleted: {
    if (strm == inputStream) {
      ready++;
    }
    if (strm == outputStream) {
      ready++;
    }
    if (ready == 2) {
      NSLog(@"csa:ready");
    }
    break;
  }
  case NSStreamEventHasBytesAvailable: {
    NSLog(@"NSStreamEventHasBytesAvailable");
    if (strm == inputStream) {
      string buffer(1024, 0);
      while ([inputStream hasBytesAvailable]) {
        NSInteger read = [inputStream read:(uint8_t *)buffer.data() maxLength:buffer.size()];
        if (read > 0) {
          received.append(buffer.data(), read);
        }
      }
      size_t offset = 0;
      for (size_t i = 0; i < received.length(); i++) {
        if (received[i] == '\x0a') {
          string line = received.substr(offset, i - offset);
          offset = i + 1;
          [self onMessage:line];
        }
      }
      received = received.substr(offset);
    }
    break;
  }
  case NSStreamEventHasSpaceAvailable: {
    NSLog(@"NSStreamEventHasSpaceAvailable");
    if (ready == 2) {
      [self send:u8"LOGIN " + username + u8" " + password];
      ready++;
    }
    break;
  }
  case NSStreamEventErrorOccurred: {
    NSLog(@"NSStreamEventErrorOccurred; error=%@", strm.streamError);
    break;
  }
  case NSStreamEventEndEncountered: {
    NSLog(@"NSStreamEventEndEncountered");
    [strm close];
    [strm removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    break;
  }
  default:
    NSLog(@"Unknown event:%d", event);
    break;
  }
}
@end

namespace sci {

cv::Mat Utility::MatFromUIImage(void *ptr) {
  cv::Mat image;
  UIImageToMat((__bridge UIImage *)ptr, image, true);
  return image;
}

cv::Mat Utility::MatFromCGImage(void *ptr) {
  cv::Mat image;
  CGImageToMat((CGImageRef)ptr, image);
  return image;
}

void *Utility::UIImageFromMat(cv::Mat const &m) {
  return (__bridge_retained void *)MatToUIImage(m);
}

struct CsaAdapter::Impl {
  Impl(u8string const &server, uint32_t port, u8string const &username, u8string const &password) {
    impl = [[CsaAdapterImpl alloc] initWithServer:server port:port username:username password:password];
  }

  ~Impl() {
    impl = nil;
  }

  optional<Move> next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    return nullopt;
  }

  CsaAdapterImpl *impl;
};

CsaAdapter::CsaAdapter(u8string const &server, uint32_t port, u8string const &username, u8string const &password) : impl(make_unique<Impl>(server, port, username, password)) {
}

CsaAdapter::~CsaAdapter() {
}

optional<Move> CsaAdapter::next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return impl->next(p, moves, hand, handEnemy);
}

} // namespace sci
