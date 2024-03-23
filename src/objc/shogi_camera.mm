#include "../shogi_camera.cpp"
#include <opencv2/imgcodecs/ios.h>

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

using namespace std;

@interface CsaAdapterImpl : NSObject <NSStreamDelegate> {
  sci::CsaAdapter *owner;
  NSInputStream *inputStream;
  NSOutputStream *outputStream;
  string username;
  string password;
  string received;
  int ready;
  deque<string> stack;
  string current;
}
@end

@implementation CsaAdapterImpl

- (id)initWithOwner:(sci::CsaAdapter *)owner server:(string)server port:(uint32_t)port username:(string)username password:(string)password {
  if (self = [super init]) {
    self->username = username;
    self->password = password;
    self->ready = 0;
    self->owner = owner;

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

- (void)send:(string)message {
  if (!outputStream) {
    return;
  }
  message += "\x0a";
  [outputStream write:(uint8_t const *)message.c_str() maxLength:message.size()];
}

- (void)onMessage:(string)msg {
  NSLog(@"csa:message:%s", msg.c_str());
  if (owner) {
    owner->onmessage(msg);
  }
}

- (void)close {
  owner = nil;
  if (inputStream) {
    [inputStream setDelegate:nil];
    [inputStream close];
    inputStream = nil;
  }
  if (outputStream) {
    [outputStream setDelegate:nil];
    [outputStream close];
    outputStream = nil;
  }
}

- (void)deinit {
  [self close];
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
    if (strm == inputStream) {
      string buffer(1024, 0);
      while ([inputStream hasBytesAvailable]) {
        NSInteger read = [inputStream read:(uint8_t *)buffer.data() maxLength:buffer.size()];
        if (read > 0) {
          received.append(buffer.data(), read);
        }
      }
      [self flush];
    }
    break;
  }
  case NSStreamEventHasSpaceAvailable: {
    if (ready == 2) {
      [self send:"LOGIN " + username + " " + password];
      ready++;
    }
    break;
  }
  case NSStreamEventErrorOccurred: {
    NSLog(@"NSStreamEventErrorOccurred; error=%@", strm.streamError);
    break;
  }
  case NSStreamEventEndEncountered: {
    [strm close];
    [strm removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self flush];
    if (!received.empty()) {
      [self onMessage:received];
    }
    break;
  }
  default:
    NSLog(@"Unknown event:%lu", event);
    break;
  }
}

- (void)flush {
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
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
  return image;
}

void *Utility::UIImageFromMat(cv::Mat const &m) {
  return (__bridge_retained void *)MatToUIImage(m);
}

struct CsaAdapter::Impl {
  Impl(CsaAdapter *owner, CsaServerParameter parameter) : owner(owner) {
    impl = [[CsaAdapterImpl alloc] initWithOwner:owner
                                          server:parameter.server
                                            port:parameter.port
                                        username:parameter.username
                                        password:parameter.password];
  }

  ~Impl() {
    if (impl) {
      [impl close];
    }
  }

  void send(string const &msg) {
    if (impl) {
      [impl send:msg];
    }
  }

  CsaAdapter *const owner;
  CsaAdapterImpl *impl;
};

void foo();

CsaAdapter::CsaAdapter(CsaServerParameter parameter) : impl(make_unique<Impl>(this, parameter)) {
}

CsaAdapter::~CsaAdapter() {
}

void CsaAdapter::send(std::string const &msg) {
  impl->send(msg);
}

} // namespace sci
