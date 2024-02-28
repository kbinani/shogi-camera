cmake_minimum_required(VERSION 3.28)
enable_language(OBJCXX)
project(shogi-camera LANGUAGES C CXX Swift VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps/sunfish3/src EXCLUDE_FROM_ALL)

add_library(shogi-camera STATIC
  include/shogi_camera/shogi_camera.hpp
  src/game.cpp
  src/img.cpp
  src/move.cpp
  src/piece_book.cpp
  src/position.cpp
  src/random_ai.cpp
  src/session.cpp
  src/shogi_camera.cpp
  src/statistics.cpp
  src/sunfish3_ai.cpp
  src/objc/shogi_camera.mm
  deps/opencv/opencv/build-ios/opencv2.xcframework
  test/doctest_main.cpp
  test/move.test.hpp
  test/sunfish3.test.hpp
)
target_include_directories(shogi-camera PUBLIC
  include
  deps/sunfish3/src
  deps/base64/include
)
target_include_directories(shogi-camera PRIVATE
  deps/doctest
)
set_target_properties(shogi-camera PROPERTIES
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
  XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
)
target_link_libraries(shogi-camera
  sunfish_searcher
  sunfish_core
  sunfish_logger
)

add_executable(shogi-camera-demo-ios MACOSX_BUNDLE
  demo/AppDelegate.swift
  demo/DebugPreview.swift
  demo/OpenCV+Utility.swift
  demo/Reader.swift
  demo/ShogiCameraInput+Utility.swift
  demo/ViewController.swift
  demo/LaunchScreen.storyboard
  demo/Assets.xcassets
  include/shogi_camera/module.modulemap
  res/voice/voice.wav
)
set(shogi_camera_demo_resources
  demo/LaunchScreen.storyboard
  demo/Assets.xcassets
  res/voice/voice.wav
)
set_target_properties(shogi-camera-demo-ios PROPERTIES
  BUNDLE True
  RESOURCE "${shogi_camera_demo_resources}"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.kbinani.shogi-camera-demo-ios"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.github.kbinani.shogi-camera-demo-ios"
  MACOSX_BUNDLE_BUNDLE_NAME shogi-camera-demo-ios
  MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
  MACOSX_BUNDLE_LONG_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
  XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
  XCODE_ATTRIBUTE_INFOPLIST_KEY_UILaunchStoryboardName "LaunchScreen"
  XCODE_ATTRIBUTE_INFOPLIST_KEY_NSCameraUsageDescription "wip"
  XCODE_ATTRIBUTE_GENERATE_INFOPLIST_FILE "YES"
)
target_link_libraries(shogi-camera-demo-ios
  shogi-camera
)
